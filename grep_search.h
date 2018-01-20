//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// grep_search.h - class with the array of search objects
// corresponding to each search pattern.
// The type of the objects depends on the search type.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifndef _grep_search_inc_
#define _grep_search_inc_

#include "grep.h"
#include "grep_options.h"

// The classes used in the five supported search types:
// _boyer_moore_	 - exact searches
// _wildcard_search_ - simple wildcard (* and ?) searches
// _soundex_		 - soundex (phonetic) searches
// _regex_			 - basic regular expression searches
// _full_regex_		 - full (extended) regular expression searches


// Command line options that apply to exact search:
// -i: options.bNoCase
// -x: options.bMatchEntireLine
// -w: options.bTreatAsWord

// Command line options that apply to soundex search:
// -x: options.bMatchEntireLine

// Command line options that apply to basic regex search:
//

// Command line options that apply to full regex search:
//

class grep_search
{
public:
	grep_search();
	~grep_search();

	void reset();
	void init ( grep_search_type searchType, _string_array_* patterns,
				bool caseSensitive, bool matchWholeWord, bool matchEntireLine );
	// matches the line against any of the specified patterns
	bool match( LPCSTR pLine, long nLineLen, long* pMatchPatIndex,
				long* pMatchStart, long* pMatchLength );

private:
	grep_search_type	_searchType;
	int					_patternCount;

	// Search objects arrays
	// which one of them is used depends on the search type
	_boyer_moore_*		_arExact;
	_wildcard_search_*	_arWild;
	_soundex_*			_arPhonetic;
	_regex_*			_arRegex;
	_regex_*			_arFullRegex;
};

#endif	// _grep_search_inc_

