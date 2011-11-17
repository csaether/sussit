#include "stdafx.h"
#include "adcSource.hpp"
#include <fcntl.h>
#include "inih/ini.h"

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

static int
cfg_value_callback( void *thisp, const char *section,
                    const char *cname, const char *value )
/*
  [adcSetup]
  channels = c012
  speed = hz6800000
  dupes = d8
  ignore = i0

  [adcZeroVal]
  0:8184  ; channel:value
  1:8180

  [legVoltChan]
  0:1  ; leg:voltchan
  1:1

  [legAmpChan]
  0:0  ; leg:ampchan
  1:2

  [legWattFudgeDivor]
  0:1134  ; leg:value
  1:1130

 */
{
    adcSource *asp = (adcSource*)thisp;
    string sect( section ), name;
    unsigned leg, chan;

    if ( sect == "adcSetup" ) {
        name = cname;
        if ( name == "channels" ) {
            asp->setchannels = value;
        } else if ( name == "speed" ) {
            asp->setspeed = value;
        } else if ( name == "dupes" ) {
            asp->setdupes = value;
        } else if ( name == "ignore" ) {
            asp->setignore = value;
        }
    } else if ( sect == "adcZeroVal" ) {
        chan = asp->checkChan( atoi( cname ) );
        asp->zeroVal[chan] = atoi( value );
    } else if ( sect == "legVoltChan" ) {
        leg = asp->checkLeg( atoi( cname ) );
        asp->VoltChan[leg] = asp->checkChan( atoi( value ) );
    } else if ( sect == "legAmpChan" ) {
        leg = asp->checkLeg( atoi( cname ) );
        asp->AmpChan[leg] = asp->checkChan( atoi( value ) );
    } else if ( sect == "legWattFudgeDivor" ) {
        leg = asp->checkLeg( atoi( cname ) );
        asp->WattFudgeDivor[leg] = atoi( value );
    }

    return 1;  // success to ini_parse
}

void
adcSource::setup( const char *cfgfile )
{
    int sts;
    sts = ini_parse( cfgfile, cfg_value_callback, this );
    if ( sts ) {
        cout << "ini_parse failed: " << sts << endl;
    }

    this->open();

    stagecnt = 10000*4;  // 10k samples, 4 channels
    stagep = new int16_t[stagecnt];  // FIX - need to calculate this
    allocateChannelBuffers();
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
    write( adcfd, "hz6800000", 10 );
    nanosleep( &tspec, NULL );
    write( adcfd, "i0", 2 );
    nanosleep( &tspec, NULL );
    write( adcfd, "d8", 2 );

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

    unsigned i;
    //    const int zeroval[] = {8184, 8181, 8182};
    int16_t val;
    int16_t *bp, *endp = stagep + n/2;

    for ( bp = stagep; bp < endp; bp += NumChans, ri++ ) {
        for ( i = 0; i < NumChans; i++ ) {
            val = bp[i] - zeroVal[i];
            ChannelData[i].s( val, ri );
        }
    }

    n = ri - rawSampSeq;
    rawSampSeq = ri;
    avgSampSeq = ri;
    return n;
}
