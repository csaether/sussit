#include "stdafx.h"
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
	//amins.allocateBuffer(18);
	//amaxs.allocateBuffer(18);
	//bmins.allocateBuffer(18);
	//bmaxs.allocateBuffer(18);
	picoSamples::setup();
}

int
fileSource::getSamples( int milliSleep )
{
	if ( infile.fail() ) {
		throw sExc( "no more file data" );
	}
	short raw[4];
	uint64_t maxi, rawi = rawSampSeq;
	maxi = (SamplesPerCycle*AvgNSamples*60*milliSleep)/1000 + rawi;
	for ( ; rawi < maxi; rawi++ ) {
		infile.read( (char*)raw, 8 );
		if ( infile.eof() ) {
			break;
		}
		amins.s( raw[0], rawi );
		amaxs.s( raw[1], rawi );
		bmins.s( raw[2], rawi );
		bmaxs.s( raw[3], rawi );
	}
	int n = (int)(rawi - rawSampSeq);
	if ( n == 0 ) {
		throw sExc( "no more file data" );
	}
	n = avgSamples( rawSampSeq, rawi );
	rawSampSeq = rawi;

	return n;
}
