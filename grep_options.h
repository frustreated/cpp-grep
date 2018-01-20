//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// grep_options.h - command line options class
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifndef _grep_options_inc_
#define _grep_options_inc_

#include "grep.h"
#include "grep_search.h"

// command line options
class grep_options
{
public:
	grep_options();
	~grep_options();

	// operations
	bool parseOptions(int argc, char* argv[]);
	int  patternCount();
	LPCTSTR getPattern(int index);
	int  fileSpecCount();
	LPCTSTR getFileSpec(int index);

public:
	// Flag for quick checking whether there is just one file to be searched or many;
	// this is not a guarantee, but rather an educated guess
	bool bOneFile;

	// grep flags
	bool bJustCount;		// -c
	bool bFileNameOnly;		// -l
	bool bQuiet;			// -q
	bool bNoFileAppend;		// -h
	bool bNoCase;			// -i
	bool bLineNumber;		// -n
	bool bSuppressBadFiles;	// -s
	bool bShowNoMatch;		// -v
	bool bTreatAsWord;		// -w
	bool bMatchEntireLine;	// -x
	bool bSearchSubDirs;	// -R
	bool bShowSummary;		// -m

private:
	grep_search_type _searchType;  // default, -F, -W, -P, -E
	// Regular expression(s) / patterns to be searched for
	_string_array_   _patterns;
	// File specifications to be searched
	_string_array_   _fileSpecs;

private:
	// helpers
	void _buildPatternList(_string_array_* pPatFiles);
	bool _validate();
	void _addFileSpec(LPCTSTR filespec);
};

#endif	// _grep_options_inc_

