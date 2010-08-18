#include "StdAfx.h"
#include "fileSource.hpp"

fileSource::~fileSource(void)
{
	infile.close();
}

void
fileSource::open( const char *fname )
{
	infile.open( fname, ios::binary | ios::in );
	if (!infile) {
		throw sExc( "file not found" );
	}
}

void
fileSource::setup()
{
	// could store and read samplespercycle in file, but not for now
	// 2^18 is 256K
	amins.allocateBuffer(18);
	amaxs.allocateBuffer(18);
	bmins.allocateBuffer(18);
	bmaxs.allocateBuffer(18);
}

int
fileSource::getSamples( int milliSleep )
{
	if ( infile.fail() ) {
		throw sExc( "no more file data" );
	}
	short raw[4];
	unsigned __int64 max, ri = rawSeq;
	max = (SamplesPerCycle*60*milliSleep)/1000 + ri;
	for ( ; ri < max; ri++ ) {
		infile.read( (char*)raw, 8 );
		if ( infile.eof() ) {
			break;
		}
		amaxs.s( raw[0], ri );
		amins.s( raw[1], ri );
		bmaxs.s( raw[2], ri );
		bmins.s( raw[3], ri );
	}
	int n = (int)(ri - rawSeq);
	if ( n == 0 ) {
		throw sExc( "no more file data" );
	}
	rawSeq = ri;
	return n;
}
