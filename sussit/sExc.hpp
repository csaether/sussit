#pragma once

class sExc : public exception {
public:
	sExc( const char *atext) : exception(atext) { }
};
