#pragma once

class sExc : public exception {
public:
#ifdef WIN32
	sExc( const char *atext) : exception(atext) { }
#else
	sExc( const char *atext) : exception() { }
#endif // WIN32
};
