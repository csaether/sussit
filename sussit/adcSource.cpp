#include "stdafx.h"
#include "adcSource.hpp"
#include <fcntl.h>

adcSource::~adcSource(void)
{
	::close( adcfd );
    if ( stagep ) {
        delete [] stagep;
        stagep = 0;
    }
}

void
adcSource::open( const char *fname )
{
	if ( !fname ) {
        fname = "/dev/adc16";
    }
    adcfd = ::open( fname, O_RDWR );

	if ( adcfd == -1 ) {
		throw sExc( "adc device not opened" );
	}
}

void
adcSource::setup()
{
    this->open();

    stagecnt = 10000*4;  // 10k samples, 4 channels
    stagep = new int16_t[stagecnt];  // FIX - need to calculate this
	dataSamples::setup();
}

void
adcSource::startSampling()
{
    struct timespec tspec;
    tspec.tv_sec = 0;
    tspec.tv_nsec = 1000*1000*35;  // 35 milliseconds
    write( adcfd, "s", 1 );
    nanosleep( &tspec, NULL );
    write( adcfd, "c124", 4 );
    nanosleep( &tspec, NULL );
    write( adcfd, "r", 1 );

    // sets the offset initially
    read( adcfd, stagep, stagecnt*2 );
}

int
adcSource::getSamples( int milliSleep )
{
	uint64_t ri = rawSampSeq;
    struct timespec tspec;
    ssize_t n;

    tspec.tv_sec = 0;
    tspec.tv_nsec = 1000*1000*milliSleep;
    nanosleep( &tspec, NULL );

    n = read( adcfd, stagep, stagecnt*2 );
    if ( n < 0 ) {
        throw sExc("read failed");
    }

    int numchans = 4;
    const int ivolt = 0;
    const int ibias = 1;
    const int iamp = 2;
    int16_t biasval, voltval, ampval;
    int16_t *bp, *endp = stagep + n/2;

    for ( bp = stagep; bp < endp; bp += numchans ) {
        biasval = bp[ibias];
        voltval = bp[ivolt] - biasval;
        ampval = bp[iamp] - biasval;

        voltsamples.s( voltval, ri );
        ampsamples.s( ampval, ri++ );
    }

    n = ri - rawSampSeq;
	rawSampSeq = ri;
	return n;
}
