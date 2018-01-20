//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// grep_options.cpp - implementation of grep_options
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "grep_options.h"

grep_options::grep_options()
{
	bOneFile = true;
	bJustCount = false;
	bFileNameOnly = false;
	bQuiet = false;
	bNoFileAppend = false;
	bNoCase = false;
	bLineNumber = false;
	bSuppressBadFiles = false;
	bShowNoMatch = false;
	bTreatAsWord = false;
	bMatchEntireLine = false;
	bSearchSubDirs = false;
	bShowSummary = false;
	_searchType = search_regex;
}

// destructor
grep_options::~grep_options()
{
	_patterns.clear();
	_fileSpecs.clear();
}

//----------------------------------------------------------------
// Parse the command line. Return false on invalid option.
//----------------------------------------------------------------
bool grep_options::parseOptions(int argc, char* argv[])
{
	int i, j;
	_string_array_ pat_files;
	bool bGot_e_Or_f = false;

	if(argc<2)
		return false;
	
	for(i=1; i<argc; i++)
	{
		if(argv[i][0] == '-')  // it's a switch
		{
			if(argv[i][1] == 'e')
			{
				// for -e switches, add the string specified to the _patterns array
				if( (i == argc-1) && (lstrlen(argv[i]) == 2) )
				{
					g_stdout.writeString("grep: Incomplete option: -e has to be followed by a pattern specification\r\n");
					return false;
				}
				else
				{
					if(lstrlen(argv[i]) > 2)
						_patterns.append(argv[i] + 2); // arg immediately follows -e without a space
					else if( (argv[i+1][0] == '-') && (argv[i+1][1] == 'e') )
						_patterns.append(""); // two consecutive -e's - null pattern
					else
						_patterns.append(argv[++i]);
				}
				bGot_e_Or_f = true;
				continue;
			}
			else if(argv[i][1] == 'f')
			{
				// for -f switches, add the filename specified to pattern files array
				if( (i == argc-1) && (lstrlen(argv[i]) == 2) )
				{
					g_stdout.writeString("grep: Incomplete option: -f has to be followed by a file specification\r\n");
					return false;
				}
				else
				{
					if(lstrlen(argv[i]) > 2)
						pat_files.append(argv[i] + 2); // arg immediately follows -f without a space
					else
						pat_files.append(argv[++i]);
				}
				bGot_e_Or_f = true;
				continue;
			}

			// parse the contiguous switches
			for(j=1; argv[i][j]; j++)
			{
				switch(argv[i][j])
				{
				case '-':
					break;
				case 'c':
					bJustCount = true;
					bFileNameOnly = false;
					bQuiet = false;
					break;
				case 'l':
					bFileNameOnly = true;
					bJustCount = false;
					bQuiet = false;
					break;
				case 'q':
					bQuiet = true;
					bJustCount = false;
					bFileNameOnly = false;
					break;
				case 'h':
					bNoFileAppend = true;
					break;
				case 'i':
					bNoCase = true;
					break;
				case 'n':
					bLineNumber = true;
					break;
				case 's':
					bSuppressBadFiles = true;
					break;
				case 'v':
					bShowNoMatch = true;
					break;
				case 'w':
					bTreatAsWord = true;
					break;
				case 'x':
					bMatchEntireLine = true;
					break;
				case 'R':
					bSearchSubDirs = true;
					bOneFile = false;
					break;
				case 'm':
					bShowSummary = true;
					break;
				//case 'E':
				//	_searchType = search_full_regex;
				//	break;
				case 'F':
					_searchType = search_exact;
					break;
				case 'P':
					_searchType = search_phonetic;
					break;
				case 'W':
					_searchType = search_wildcard;
					break;
				default:
					// invalid option
					g_stdout.writeFormatted("grep: Invalid option: %c\r\n", argv[i][j]);
					return false;
				}
			}
		}
		else
		{
			// The first argument not starting with - is the pattern,
			// all operands after it are filenames, except...
			
			// ...if there was an -e or -f switch, we don't expect a pattern at the end
			if(!bGot_e_Or_f)
				// get the pattern
				_patterns.append(argv[i++]);

			// get the files, if any
			for(; i<argc; i++)
				_addFileSpec(argv[i]);
		}
	}

	// Point to the proper comparison & searching functions
	// depending on case sensitivity
	pfncmp = (bNoCase ? memicmp : memcmp);
	pfnstr = (bNoCase ? stristr : strstr);

	_buildPatternList(&pat_files);
	return _validate();
}

int grep_options::patternCount()
{
	return _patterns.length();
}

LPCTSTR grep_options::getPattern(int index)
{
	return _patterns.get(index);
}

int grep_options::fileSpecCount()
{
	return _fileSpecs.length();
}

LPCTSTR grep_options::getFileSpec(int index)
{
	return _fileSpecs.get(index);
}

void grep_options::_buildPatternList(_string_array_* pPatFiles)
{
	// add each line in pPatFiles files as a pattern to this->_patterns
	_file_finder_ ff;
	_win32_file_ w32f;
	TCHAR curfile[MAX_PATH];
	bool bGoodSpec;		// is the current pattern filespec good?
	LPSTR line = NULL;
	int i;
	
	for(i=0; i<pPatFiles->length(); i++)
	{
		// Init the file finder with specification
		ff.initPattern(pPatFiles->get(i), false);
		bGoodSpec = false;
		while(ff.getNextFile(curfile))
		{
			bGoodSpec = true;
			w32f.reset(curfile);
			if(!w32f.open(access_read))
			{
				g_stdout.writeFormatted("grep: Can\'t open pattern file \'%s\'\r\n", curfile);
				continue;
			}
			while(w32f.readLine(line) != -1)
			{
				_patterns.append(line);
				free(line);
			}
		}
		if(!bGoodSpec)
			g_stdout.writeFormatted("grep: Can\'t find pattern file(s) \'%s\'\r\n", pPatFiles->get(i));
	}
}

bool grep_options::_validate()
{
	int i, j;

	// check the patterns for validity
	if(search_phonetic == _searchType)
	{
		// validate the patterns to conform to the soundex rules
		for(i=0; i<_patterns.length(); i++)
		{
			for(j=0; j<lstrlen(_patterns[i]); j++)
			{
				if(!isalpha(_patterns[i][j]))
				{
					if(!bQuiet)
						g_stdout.writeFormatted (
							"grep: Pattern \"%s\" does not conform to the specified type of search:\r\n"
							"For Phonetic search (-P), the pattern must contain a\r\n"
							"single word consisting of all alphabetic characters.\r\n\r\n", _patterns[i] );
					_patterns.removeAt(i--);
					break; // next pattern
				}
			}
		}
	}
	else if(search_regex == _searchType)
	{
		// validate the patterns to conform to basic regex rules
		for(i=0; i<_patterns.length(); i++)
		{
		}
	}
	else if(search_full_regex == _searchType)
	{
		// validate the patterns to conform to extended regex rules
		for(i=0; i<_patterns.length(); i++)
		{
		}
	}
	
	// exact search does not require any pattern checks

	if(_patterns.length()==0)
	{
		if(!bQuiet)
			g_stdout.writeString("grep: No valid patterns were specified\r\n");
		return false;
	}
	
	// seed the search object with the patterns, search type, and options
	g_searcher.init(_searchType, &_patterns, !bNoCase, bTreatAsWord, bMatchEntireLine);
	return true;
}

void grep_options::_addFileSpec(LPCTSTR filespec)
{
	_fileSpecs.append(filespec);
	// if we have more than one pattern, or the file spec is a dir or contains a wildcard,
	// we most likely have more than one file
	if( _fileSpecs.length()>1 ||
		filespec[lstrlen(filespec)-1]==_T('\\') || filespec[lstrlen(filespec)-1]==_T('/') ||
	    (_tcschr(filespec, _T('*')) || _tcschr(filespec, _T('?'))) )
		bOneFile = false;
}

