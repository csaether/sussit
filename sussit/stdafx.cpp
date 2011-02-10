// stdafx.cpp : source file that includes just the standard includes
// sussit.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

ostream *Stabo = 0;
ostream *Chgo = 0;
ostream *Rawo = 0;

#include <ctime>

string
thetime()
{
	time_t t = ::time(0);
	char cb[28];
#ifdef WIN32
	ctime_s( cb, sizeof(cb), &t );
#else
	ctime_r( &t, cb );
#endif
	string c( cb );
	string a( c, 0, c.length() - 1 );
	return a;
}
