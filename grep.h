//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// grep.h  - common includes, types, and declarations.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifndef _grep_h_inc_
#define _grep_h_inc_

#include <stdlib.h>
#include <conio.h>

#include <_strfuncs_.h>
#include <_string_array_.h>
#include <_file_finder_.h>
#include <_win32_file_.h>
#include <_boyer_moore_.h>
#include <_wildcard_search_.h>
#include <_soundex_.h>
#include <_regex_.h>

using namespace soige;  // my util classes namespace

//----------------------------------------------------------------
// Types and #defines
//----------------------------------------------------------------

// Return values
#define RTN_MATCH		0
#define RTN_NOMATCH		1
#define RTN_ERROR		2

// Replace non-displayable chars with this one
const char NON_DISPLAYABLE_CHAR = '?';

typedef unsigned long ulong;

// Typedef for string comparison func (depends on case)
typedef int   (*PSTRCMP)(const void*, const void*, size_t);
// Typedef for string searching func (depends on case)
typedef char* (*PSTRSTR)(const char*, const char*);

// quick string testing
inline int streq(LPCTSTR s1, LPCTSTR s2) {return (!lstrcmp(s1, s2));}

// Search type
enum grep_search_type
{
	search_exact,		//  -F: exact string search - boyer-moore
	search_phonetic,	//  -P: phonetic search - soundex
	search_regex,		// def: basic regular expression search - regex
	search_full_regex,	//  -E: full (extended) regular expression search - full regex
	search_wildcard		//  -W: simple wildcard (* and ?) search - wildcard
};


//----------------------------------------------------------------
// Globals
//----------------------------------------------------------------
// Global search options; for all types of searches except RE
class  grep_options;
extern grep_options g_options;
class  grep_search;
extern grep_search  g_searcher;

// File and line counts
extern ulong g_uAllFileCount;
extern ulong g_uMatchedFileCount;
extern ulong g_uAllLineCount;
extern ulong g_uMatchedLineCount;

// String comparison function; set in g_options.parseOptions depending on case-sensitivity
extern PSTRCMP pfncmp;
// String searching function; set in g_options.parseOptions depending on case-sensitivity
extern PSTRSTR pfnstr;

// stdout
extern _win32_file_  g_stdout;

#endif	// _grep_h_inc_

