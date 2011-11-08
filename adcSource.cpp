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
        fname = "/dev/adc1";  // let the driver do averaging
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
    write( adcfd, "c012", 4 );  // three channels
    nanosleep( &tspec, NULL );
    write( adcfd, "hz6400000", 10 );
    nanosleep( &tspec, NULL );
    write( adcfd, "i0", 2 );
    nanosleep( &tspec, NULL );
    write( adcfd, "d7", 2 );
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

    int numchans = 3;
    const int ivolt = 0;
    const int ct1 = 1;
    //    const int ct2 = 2;
    const int zeroval[] = {8184, 8181, 8182};
    int16_t voltval, ampval1;
    int16_t *bp, *endp = stagep + n/2;

    for ( bp = stagep; bp < endp; bp += numchans ) {
        voltval = bp[ivolt] - zeroval[ivolt];
        ampval1 = bp[ct1] - zeroval[ct1];

        voltsamples.s( voltval, ri );
        ampsamples.s( ampval1, ri++ );
    }

    n = ri - rawSampSeq;
    rawSampSeq = ri;
    avgSampSeq = ri;
    return n;
}
