//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// grep.cpp - The main source of NT grep
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "grep.h"
#include "grep_options.h"
#include "grep_search.h"

//----------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------
void DoGrepOnFile(_win32_file_& file);
void GrepUsage(bool bVerbose);

//----------------------------------------------------------------
// Globals
//----------------------------------------------------------------

// Global grep options
grep_options g_options;
// The searcher
grep_search  g_searcher;

// File and line counts
ulong  g_uAllFileCount		= 0;
ulong  g_uMatchedFileCount	= 0;
ulong  g_uAllLineCount		= 0;
ulong  g_uMatchedLineCount	= 0;

// String comparison function; set in parseOptions depending on case-sensitivity
PSTRCMP	pfncmp;
// String searching function; set in parseOptions depending on case-sensitivity
PSTRSTR	pfnstr;

// stdout
_win32_file_	g_stdout(_win32_file_::ft_stdout);


//----------------------------------------------------------------
//							main()
//----------------------------------------------------------------
int main(int argc, char* argv[])
{
	_file_finder_ ff;
	_win32_file_ w32f;
	TCHAR curfile[MAX_PATH*2];
	bool bGoodFileSpec;		// is the current filespec good?
	int i;

	if( argc == 1 )
		return GrepUsage(false), RTN_ERROR;
	else if( streq(argv[1], _T("/?") ) || streq( argv[1], _T("-?") ) )
		return GrepUsage(true), RTN_ERROR;
	
	if( !g_options.parseOptions(argc, argv) )
		return GrepUsage(false), RTN_ERROR;

	if( g_options.fileSpecCount() == 0 )
	{
		// no file specs - use stdin
		w32f.reset( _win32_file_::ft_stdin );
		DoGrepOnFile(w32f);
	}
	else
	{
		// go through file specifications, open each file and search it
		for(i=0; i<g_options.fileSpecCount(); i++)
		{
			// Init the file finder with specification
			ff.initPattern( g_options.getFileSpec(i), g_options.bSearchSubDirs );
			if( ff.fileCount() == -1 )  // file count unknown, but likely more than one
				g_options.bOneFile = false;
			bGoodFileSpec = false;
			while( ff.getNextFile(curfile) )
			{
				bGoodFileSpec = true;
				w32f.reset(curfile);
				if( !w32f.open(access_read, share_read | share_write, open_existing) )
				{
					if( !g_options.bSuppressBadFiles && !g_options.bQuiet )
						g_stdout.writeFormatted( "grep: Cannot open file \'%s\'\r\n", curfile );
					continue;
				}
				DoGrepOnFile(w32f);
			}
			if( !bGoodFileSpec && !g_options.bSuppressBadFiles && !g_options.bQuiet )
				g_stdout.writeFormatted( "grep: Can\'t find file(s) \'%s\'\r\n", g_options.getFileSpec(i) );
		}
	}

	if( g_options.bShowSummary && !g_options.bQuiet )
		g_stdout.writeFormatted( "\r\nSearched %lu line(s) in %lu file(s)."
								 "\r\nMatched %lu line(s) in %lu file(s)\r\n",
								 g_uAllLineCount, g_uAllFileCount,
								 g_uMatchedLineCount, g_uMatchedFileCount );
	
	return (g_uMatchedFileCount? RTN_MATCH : RTN_NOMATCH);
}


//----------------------------------------------------------------
// The main grep logic. Called for each file.
// Checks the file against all the available patterns.
// Returns on the first match w/o checking the remaining patterns.
//----------------------------------------------------------------
void DoGrepOnFile(_win32_file_& file)
{
	char  curLine[8192];	// 8K line length limit
	long  nLineLen;
	ulong nCurLine;
	ulong nMatchedLines;
	long  nMatchingPat;		// index of the pattern in the pattern list that matched
	long  nMatchStart;		// the beginning of the match in the line
	long  nMatchLength;		// the length of the matching substring in the line (chars)
	bool  bMatched;
	char* pc;

	
	////////////////////////////////////////////////////
	// Process the file:
	// read each line and pass it to the searcher object
	// which will match against the specified patterns
	nCurLine		=  0;
	nMatchedLines	=  0;
	nMatchingPat	= -1;
	nMatchStart		= -1;
	nMatchLength	= -1;
	while( (nLineLen = file.readLine(curLine, sizeof(curLine))) != -1 )
	{
		nCurLine++;
		bMatched = g_searcher.match( curLine,
									 nLineLen,
									 &nMatchingPat,
									 &nMatchStart,
									 &nMatchLength );

		if( (bMatched && !g_options.bShowNoMatch) || (!bMatched && g_options.bShowNoMatch) )
		{
			nMatchedLines++;
			// output according to the options
			if(g_options.bQuiet)
			{
				file.close();
				exit(RTN_MATCH);  // exit on first match
			}
			else if(g_options.bFileNameOnly)
			{
				if(nMatchedLines == 1)
				{
					g_stdout.writeLine( file.getFileName() );
					if(!g_options.bShowSummary)
						return;
				}
			}
			else if(g_options.bJustCount)
				;
			else if(g_options.bOneFile || g_options.bNoFileAppend)
			{
				if( g_options.bLineNumber &&
					(file.getFileType() != _win32_file_::ft_stdin) )
					g_stdout.writeFormatted( "%lu: ", nCurLine );
				pc = curLine;
				// while( (pc+=printf("%1s", pc)) < &curLine[nLineLen] )
				while(pc < &curLine[nLineLen])
				{
					// g_stdout.write( isprint(*pc)? pc : "?", 1 );
					g_stdout.write( ((*pc<32 && *pc!=9) || *pc>128) ?
									&NON_DISPLAYABLE_CHAR : pc, 1 );
					pc++;
				}
				g_stdout.writeString( "\r\n" );
			}
			else
			{
				if( file.getFileType() != _win32_file_::ft_stdin )
					g_stdout.writeFormatted( "%s: ", file.getFileName() );
				if( g_options.bLineNumber &&
					(file.getFileType() != _win32_file_::ft_stdin) )
					g_stdout.writeFormatted( "%lu: ", nCurLine );
				pc = curLine;
				// while( (pc+=printf("%1s", pc)) < &curLine[nLineLen] )
				while(pc < &curLine[nLineLen])
				{
					g_stdout.write( ((*pc<32 && *pc!=9) || *pc>128) ?
									&NON_DISPLAYABLE_CHAR : pc, 1 );
					pc++;
				}
				g_stdout.writeString( "\r\n" );
			}
		}
		
	}

	if(g_options.bJustCount)
	{
		if( !(g_options.bOneFile || g_options.bNoFileAppend) &&
			(file.getFileType() != _win32_file_::ft_stdin) )
			g_stdout.writeFormatted( "%s: ", file.getFileName() );
		g_stdout.writeFormatted( "%lu\r\n", nMatchedLines );
	}

	// finish up
	g_uAllFileCount++;
	g_uAllLineCount += nCurLine;
	if(nMatchedLines)
	{
		g_uMatchedFileCount++;
		g_uMatchedLineCount += nMatchedLines;
	}
}


//----------------------------------------------------------------
// GrepUsage() - Displays usage syntax. What a surprise!
//----------------------------------------------------------------
void GrepUsage(bool bVerbose)
{
	g_stdout.writeString ( "\r\nNT Grep 0.0.1 by Vasya, Masha, Dasha & Serge Inc.\r\n" );

	g_stdout.writeString
		(
			"\r\nUsage:\r\n"
			"  grep [ -F | -W | -P | -E ] [ -c | -l | -q ] [ -nhsviwxRm ]\r\n"
			"       pattern [ file... ]\r\n"
			"  grep [ -F | -W | -P | -E ] [ -c | -l | -q ] [ -nhsviwxRm ]\r\n"
			"       -e pattern... [ -f pattern_file ]... [ file... ]\r\n"
			"  grep [ -F | -W | -P | -E ] [ -c | -l | -q ] [ -nhsviwxRm ]\r\n"
			"       [ -e pattern ]... -f pattern_file... [ file... ]\r\n\r\n"
		);
	
	if(!bVerbose)	// terse
		g_stdout.writeString( "Type \'grep -? | more\' for detailed explanation of options\r\n" );
	else			// verbose
		g_stdout.writeString
		(
			"Where:\n"
			"  -F\tMatch using fixed strings. Treat each speci-\n"
				"\tfied  pattern  as an exact string instead of\n"
				"\ta regular expression. If an  input line con-\n"
				"\ttains any of  the  patterns as  a contiguous\n"
				"\tsequence of bytes, the line will be matched.\n"
				"\tA null (empty) string  matches every line.\n\n"

			"  -W\tMatch using a simple wildcard pattern. Valid\n"
				"\twildcards  include  * (matches  zero or more\n"
				"\tcharacters),  and ? (exactly one character).\n"
				"\tAll other characters  are matched literally.\n"
				"\tThe -w and -x options are not valid with the\n"
				"\twildcard search.  This option is NT only.\n\n"

			"  -P\tMatch  using  phonetic  search.  If an input\n"
				"\tline contains any  words that sound like any\n"
				"\tof the specified patterns,  the line will be\n"
				"\tmatched.  Uses a modified  Soundex algorithm\n"
				"\tto perform matching. This option is NT only.\n\n"

			"  -E\tMatch using full regular expressions.  Treat\n"
				"\teach  pattern  specified  as  a full regular\n"
				"\texpression.   If  any  entire  full  regular\n"
				"\texpression pattern  matches  an  input line,\n"
				"\tthe line will be matched.  A null/empty full\n"
				"\tregular expression matches every line.\n\n"

				"\tEach pattern will be interpreted  as  a full\n"
				"\tregular expression, which includes the basic\n"
				"\tregular expression syntax, except for \\( and\n"
				"\t\\), and including:\n\n" // except \{m,n\} also?

				"\t 1. A full regular expression  followed by +\n"
				"\t    that matches  one or more occurrences of\n"
				"\t    the full regular expression.\n\n"

				"\t 2. A full regular expression  followed by ?\n"
				"\t    that matches 0 or 1  occurrences  of the\n"
				"\t    full regular expression.\n\n"

				"\t 3. Full regular expressions separated  by |\n"
				"\t    or by a NEWLINE  that match strings that\n"
				"\t    are matched by any of the expressions.\n\n"

				"\t 4. A full  regular expression that  may  be\n"
				"\t    enclosed in parentheses () for grouping.\n\n"

				"\tThe order of precedence of  operators is [],\n"
				"\tthen  *?+,  then  concatenation,  then | and\n"
				"\tNEWLINE.\n\n"
			
			"  -c\tPrint only a count of the lines that contain\n"
				"\tthe pattern(s) for each file.\n\n"

			"  -l\tPrint the names of files with matching lines\n"
				"\tonce,  separated  by  a  NEWLINE  character.\n"
				"\tDoes not repeat the names of files  when the\n"
				"\tpattern is found more than once.\n\n"
			
			"  -q\tQuiet. Do not write anything to the standard\n"
				"\toutput,  regardless  of matching lines. Exit\n"
				"\twith 0 code as soon as first match is found.\n\n"
			
			"  -n\tPrecede each line by its line number  in the\n"
				"\tfile (first line is 1).\n\n"

			"  -h\tPrevents the name of the file containing the\n"
				"\tmatching  line  from  being appended to that\n"
				"\tline. Used when searching multiple files.\n\n"
			
			"  -s\tSuppress error messages about nonexistent or\n"
				"\tunreadable files.\n\n"

			"  -v\tRevert match:  select all lines which do not\n"
				"\tcontain any of the pattern(s).\n\n"

			"  -i\tIgnore case distinctions during comparisons.\n"
				"\tDoes not apply to phonetic search.\n\n"
			
			"  -w\tSearch for the expression as  a  word  as if\n"
				"\tsurrounded by  \\<  and  \\>.\n\n" // Automatic for phonetic searches

			"  -x\tConsider only input lines that use all char-\n"
				"\tacters  in the line to match an entire fixed\n"
				"\tstring or regular expression to  be matching\n"
				"\tlines.\n\n" // Does not apply to phonetic and simple wildcard searches

			"  -R\tSearch subdirectories for the specified file\n"
				"\tor file pattern.  This option is NT only.\n\n"

			"  -m\tDisplay the summary of files searched, total\n"
				"\tnumber of lines,  files matched,  and number\n"
				"\tof lines  matched  at the end of the search.\n"
				"\tThis option is NT only.\n\n"

			"  -e pattern\n"
				"\tSpecify one or more patterns to be used dur-\n"
				"\ting the search for input.  Each pattern must\n"
				"\tbe preceded  by  -e.  A  null pattern can be\n"
				"\tspecified by two adjacent -e options. Unless\n"
				"\t-E, -F, -W, or -P  option is also specified,\n"
				"\teach pattern is  treated as a  basic regular\n"
				"\texpression.  Multiple -e and -f  options are\n"
				"\taccepted  by  grep.  All  of  the  specified\n"
				"\tpatterns are used  when  matching lines, but\n"
				"\tthe order of evaluation is unspecified.\n\n"
			
			"  -f pattern_file\n"
				"\tRead one or more  patterns from the  file(s)\n"
				"\tnamed by pattern_file (which itself can be a\n"
				"\twildcard  file  specification).  Patterns in\n"
				"\tpattern_file are  terminated  by  a  NEWLINE\n"
				"\tcharacter.  A  null pattern can be specified\n"
				"\tby an empty line in pattern_file. Unless the\n"
				"\t-E, -F, -W, or -P  option is also specified,\n"
				"\teach  pattern  will be  treated as  a  basic\n"
				"\tregular expression.\n\n"
			
			"  pattern\n"
				"\tThe string/regular expression to search for.\n\n"

			"  file...\n"
				"\tFilename or wildcard file pattern to search.\n"
				"\tAll arguments  following  the  last  pattern\n"
				"\tare considered to be file specifications. If\n"
				"\tthis argument is not supplied, use stdin.\n\n"

			"Notes:\n"
			"  1. All options are case sensitive.\n"
			"  2. Options  -c, -l, and -q are mutually exclusive,\n"
			"     as are  -F, -W, -P, and -E.  If several of them\n"
			"     are specified, the last option will be used.\n"
			"  3. If any pattern starts with -  or contains shell\n"
			"     characters, like \\, |, >, etc.,  enclose entire\n"
			"     pattern in  double  quotes (\"\").  If  a  double\n"
			"     quote is part of the pattern, precede it with a\n"
			"     backslash (\\). Double quotes that don\'t enclose\n"
			"     a pattern and  are not escaped with a backslash\n"
			"     are not passed by the shell and will be ignored.\n"

/*
			"Where:\r\n"
			"\t-c\tPrint only a count of the lines that contain\r\n"
			  "\t\tthe pattern(s).\r\n\r\n"

			"\t-l\tPrint the names of files with matching lines\r\n"
			  "\t\tonce,  separated  by  a  NEWLINE  character.\r\n"
			  "\t\tDoes not repeat the names of files  when the\r\n"
			  "\t\tpattern is found more than once.\r\n\r\n"
			
			"\t-q\tQuiet. Do not write anything to the standard\r\n"
			  "\t\toutput,  regardless  of matching lines. Exit\r\n"
			  "\t\twith 0 code as soon as first match is found.\r\n\r\n"
			
			"\t-F\tMatch using fixed strings. Treat each speci-\r\n"
			  "\t\tfied  pattern  as an exact string instead of\r\n"
			  "\t\ta regular expression. If an  input line con-\r\n"
			  "\t\ttains any of  the  patterns as  a contiguous\r\n"
			  "\t\tsequence of bytes, the line will be matched.\r\n"
			  "\t\tA null (empty) string  matches every line.\r\n\r\n"

			"\t-W\tMatch using a simple wildcard pattern. Valid\r\n"
			  "\t\twildcards  include  * (matches  zero or more\r\n"
			  "\t\tcharacters),  and ? (exactly one character).\r\n"
			  "\t\tAll other characters  are matched literally.\r\n"
			  "\t\tThe -w and -x options are not valid with the\r\n"
			  "\t\twildcard search.  This option is NT only.\r\n\r\n"

			"\t-P\tMatch  using  phonetic  search.  If an input\r\n"
			  "\t\tline contains any  words that sound like any\r\n"
			  "\t\tof the specified patterns,  the line will be\r\n"
			  "\t\tmatched.  Uses a modified  Soundex algorithm\r\n"
			  "\t\tto perform matching. This option is NT only.\r\n\r\n"

			"\t-E\tMatch using full regular expressions.  Treat\r\n"
			  "\t\teach  pattern  specified  as  a full regular\r\n"
			  "\t\texpression.   If  any  entire  full  regular\r\n"
			  "\t\texpression pattern  matches  an  input line,\r\n"
			  "\t\tthe line will be matched.  A null/empty full\r\n"
			  "\t\tregular expression matches every line.\r\n\r\n"

			  "\t\tEach pattern will be interpreted  as  a full\r\n"
			  "\t\tregular expression, which includes the basic\r\n"
			  "\t\tregular expression syntax, except for \\( and\r\n"
			  "\t\t\\), and including:\r\n\r\n" // except \{m,n\} also?

			  "\t\t 1.\r\n"
			  "\t\t   A full regular expression  followed  by +\r\n"
			  "\t\t   that  matches  one or more occurrences of\r\n"
			  "\t\t   the full regular expression.\r\n\r\n"

			  "\t\t 2.\r\n"
			  "\t\t   A full regular expression  followed  by ?\r\n"
			  "\t\t   that matches  0  or  1 occurrences of the\r\n"
			  "\t\t   full regular expression.\r\n\r\n"

			  "\t\t 3.\r\n"
			  "\t\t   Full  regular expressions separated  by |\r\n"
			  "\t\t   or by a  NEWLINE  that match strings that\r\n"
			  "\t\t   are matched by any of the expressions.\r\n\r\n"

			  "\t\t 4.\r\n"
			  "\t\t   A full  regular expression  that  may  be\r\n"
			  "\t\t   enclosed in parentheses () for grouping.\r\n\r\n"

			  "\t\tThe order of precedence of  operators is [],\r\n"
			  "\t\tthen  *?+,  then  concatenation,  then | and\r\n"
			  "\t\tNEWLINE.\r\n\r\n"
			
			"\t-h\tPrevents the name of the file containing the\r\n"
			  "\t\tmatching  line  from  being appended to that\r\n"
			  "\t\tline. Used when searching multiple files.\r\n\r\n"
			
			"\t-i\tIgnore upper/lower  case  distinction during\r\n"
			  "\t\tcomparisons.\r\n\r\n" // Does not apply to phonetic search.
			
			"\t-n\tPrecede each line by its line number  in the\r\n"
			  "\t\tfile (first line is 1).\r\n\r\n"

			"\t-s\tSuppress error messages about nonexistent or\r\n"
			  "\t\tunreadable files.\r\n\r\n"

			"\t-v\tPrint all lines except those  containing the\r\n"
			  "\t\tpattern.\r\n\r\n"

			"\t-w\tSearch for the expression as  a  word  as if\r\n"
			  "\t\tsurrounded by  \\<  and  \\>.\r\n\r\n" // Automatic for phonetic searches

			"\t-x\tConsider only input lines that use all char-\r\n"
			  "\t\tacters  in the line to match an entire fixed\r\n"
			  "\t\tstring or regular expression to  be matching\r\n"
			  "\t\tlines.\r\n\r\n" // Does not apply to phonetic and simple wildcard searches

			"\t-R\tSearch subdirectories for the specified file\r\n"
			  "\t\tor file pattern.  This  option is  NT  only.\r\n\r\n"

			"\t-m\tDisplay the summary of files searched, total\r\n"
			  "\t\tnumber of lines,  files matched,  and number\r\n"
			  "\t\tof lines  matched  at the end of the search.\r\n"
			  "\t\tThis option is NT only.\r\n\r\n"

			"\t-e pattern\r\n"
			  "\t\tSpecify one or more patterns to be used dur-\r\n"
			  "\t\ting the search for input.  Each pattern must\r\n"
			  "\t\tbe preceded  by  -e.  A  null pattern can be\r\n"
			  "\t\tspecified by two adjacent -e options. Unless\r\n"
			  "\t\tthe  -E, -F, or -P option is also specified,\r\n"
			  "\t\teach pattern is  treated as a  basic regular\r\n"
			  "\t\texpression.  Multiple -e and -f  options are\r\n"
			  "\t\taccepted  by  grep.  All  of  the  specified\r\n"
			  "\t\tpatterns are used  when  matching lines, but\r\n"
			  "\t\tthe order of evaluation is unspecified.\r\n\r\n"
			
			"\t-f pattern_file\r\n"
			  "\t\tRead one/more  patterns from the  file named\r\n"
			  "\t\tby  the path name  pattern_file. Patterns in\r\n"
			  "\t\tpattern_file are  terminated  by  a  NEWLINE\r\n"
			  "\t\tcharacter.  A  null pattern can be specified\r\n"
			  "\t\tby an empty line in pattern_file. Unless the\r\n"
			  "\t\t-E, -F, or -P option is also specified, each\r\n"
			  "\t\tpattern will be treated as  a  basic regular\r\n"
			  "\t\texpression.\r\n\r\n"
			
			"\tpattern\tThe string/regular expression to search for.\r\n\r\n"

			"\tfile...\tFilename or wildcard file pattern to search.\r\n"
			  "\t\tAll arguments  following  the  last  pattern\r\n"
			  "\t\tare considered to be file specifications. If\r\n"
			  "\t\tthis argument is not supplied, use stdin.\r\n\r\n"

			"Notes:\r\n"
			"1. All options are case sensitive.\r\n"
			"2. Options  -c, -l, and -q  are mutually exclusive,  as are -F,\r\n"
			"   -W, -P, and -E.  If several of them are specified,  only the\r\n"
			"   last encountered option will be used.\r\n"
			"3. If pattern starts with - or contains shell characters,  like\r\n"
			"   \\, |, >, etc., enclose entire pattern in double quotes (\"\").\r\n"
			"   If a double quote is part of the pattern, precede it with \\.\r\n"
			"   Double quotes that are not escaped with \\ and do not enclose\r\n"
			"   a pattern are not passed  by the shell  and will be ignored.\r\n"
			"4. UNIX option -b is not implemented. So what?\r\n"
*/
		);

/*
"
  -E, --extended-regexp     PATTERN is an extended regular expression
  -F, --fixed-regexp        PATTERN is a fixed string separated by newlines
  -G, --basic-regexp        PATTERN is a basic regular expression
  -e, --regexp=PATTERN      use PATTERN as a regular expression
  -f, --file=FILE           obtain PATTERN from FILE
  -i, --ignore-case         ignore case distinctions
  -w, --word-regexp         force PATTERN to match only whole words
  -x, --line-regexp         force PATTERN to match only whole lines

Miscellaneous:
  -s, --no-messages         suppress error messages
  -v, --revert-match        select non-matching lines
  -V, --version             print version information and exit
      --help                display this help and exit

Output control:
  -b, --byte-offset         print the byte offset with output lines
  -n, --line-number         print line number with output lines
  -H, --with-filename       print the filename for each match
  -h, --no-filename         suppress the prefixing filename on output
  -q, --quiet, --silent     suppress all normal output
  -L, --files-without-match only print FILE names containing no match
  -l, --files-with-matches  only print FILE names containing matches
  -c, --count               only print a count of matching lines per FILE

Context control:
  -B, --before-context=NUM  print NUM lines of leading context
  -A, --after-context=NUM   print NUM lines of trailing context
  -NUM                      same as both -B NUM and -A NUM
  -C, --context             same as -2
  -U, --binary              do not strip CR characters at EOL (MSDOS)
  -u, --unix-byte-offsets   report offsets as if CRs were not there (MSDOS)

If no -[GEF], then `egrep' assumes -E, `fgrep' -F, else -G.
With no FILE, or when FILE is -, read standard input. If less than
two FILEs given, assume -h. Exit with 0 if matches, with 1 if none.
Exit with 2 if syntax errors or system errors.
"
*/
}

