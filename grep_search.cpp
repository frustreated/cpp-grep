//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// grep_search.cpp - implementation of grep_search
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "grep_search.h"

grep_search::grep_search()
{
	_searchType		= search_regex;
	_patternCount	= 0;
	_arExact		= NULL;
	_arWild			= NULL;
	_arPhonetic		= NULL;
	_arRegex		= NULL;
	_arFullRegex	= NULL;
}

grep_search::~grep_search()
{
	reset();
}

void grep_search::reset()
{
	delete[] _arExact;
	delete[] _arWild;
	delete[] _arPhonetic;
	delete[] _arRegex;
	delete[] _arFullRegex;

	_arExact		= NULL;
	_arWild			= NULL;
	_arPhonetic		= NULL;
	_arRegex		= NULL;
	_arFullRegex	= NULL;
	_searchType		= search_regex;
	_patternCount	= 0;
}

void grep_search::init( grep_search_type searchType,
						_string_array_* patterns,
						bool caseSensitive,
						bool matchWholeWord,
						bool matchEntireLine )
{
	int i;

	reset();
	
	_searchType = searchType;
	_patternCount = patterns->length();

	switch(_searchType)
	{
	case search_exact:
		_arExact = new _boyer_moore_[_patternCount];
		for(i=0; i<_patternCount; i++)
			_arExact[i].initPattern( patterns->get(i), caseSensitive,
									 matchWholeWord, matchEntireLine );
		break;
	case search_wildcard:
		_arWild = new _wildcard_search_[_patternCount];
		for(i=0; i<_patternCount; i++)
			_arWild[i].initPattern( patterns->get(i), caseSensitive,
									matchWholeWord, matchEntireLine );
		break;
	case search_phonetic:
		_arPhonetic = new _soundex_[_patternCount];
		for(i=0; i<_patternCount; i++)
			_arPhonetic[i].initPattern( patterns->get(i), matchEntireLine );
		break;
	case search_regex:
		_arRegex = new _regex_[_patternCount];
		for(i=0; i<_patternCount; i++)
			_arRegex[i].initPattern( patterns->get(i), false,
									 caseSensitive, matchEntireLine );
		break;
	case search_full_regex:
		_arFullRegex = new _regex_[_patternCount];
		for(i=0; i<_patternCount; i++)
			_arFullRegex[i].initPattern( patterns->get(i), true,
										 caseSensitive, matchEntireLine );
		break;
	}
}

//----------------------------------------------------------------
// Attempt to match the line against any of the supplied patterns.
// Return true if found a match, false if not.
// Params:
// pMatchPatIndex	- the index of the pattern that matched
// pMatchStart		- the starting index of match in the line
// pMatchLength		- the length of the matching substring (chars)
//----------------------------------------------------------------
bool grep_search::match( /* in */ LPCSTR pLine,
						 /* in */  long  nLineLen,
						 /* out */ long* pMatchPatIndex,
						 /* out */ long* pMatchStart,
						 /* out */ long* pMatchLength )
{
	int i;

	switch(_searchType)
	{
	case search_exact:
		for(i=0; i<_patternCount; i++)
		{
			if( _arExact[i].match(pLine, nLineLen, pMatchStart, pMatchLength) )
			{
				if(pMatchPatIndex) *pMatchPatIndex = i;
				return true;
			}
		}
		break;
	case search_wildcard:
		for(i=0; i<_patternCount; i++)
		{
			if( _arWild[i].match(pLine, nLineLen, pMatchStart, pMatchLength) )
			{
				if(pMatchPatIndex) *pMatchPatIndex = i;
				return true;
			}
		}
		break;
	case search_phonetic:
		for(i=0; i<_patternCount; i++)
		{
			if( _arPhonetic[i].match(pLine, nLineLen, pMatchStart, pMatchLength) )
			{
				if(pMatchPatIndex) *pMatchPatIndex = i;
				return true;
			}
		}
		break;
	case search_regex:
		for(i=0; i<_patternCount; i++)
		{
			if( _arRegex[i].match(pLine, nLineLen, pMatchStart, pMatchLength) )
			{
				if(pMatchPatIndex) *pMatchPatIndex = i;
				return true;
			}
		}
		break;
	case search_full_regex:
		for(i=0; i<_patternCount; i++)
		{
			if( _arFullRegex[i].match(pLine, nLineLen, pMatchStart, pMatchLength) )
			{
				if(pMatchPatIndex) *pMatchPatIndex = i;
				return true;
			}
		}
		break;
	
	} // switch(_searchType)
	
	return false;
}
