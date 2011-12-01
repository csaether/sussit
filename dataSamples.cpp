#include "stdafx.h"
#include "dataSamples.hpp"
#include "ini.h"

dataSamples::dataSamples( )
        : rawSampSeq(0),
          avgSampSeq(0),
          NumChans(0),
          NumLegs(0)
{
    unsigned i;
    for ( i = 0; i < MaxLegs; i++ ) {
        VoltChan[i] = 0;
        AmpChan[i] = 0;
        WattFudgeDivor[i] = 0;
        HumPower[i] = 0;
        HumReactive[i] = 0;
    }
}

void
dataSamples::allocateChannelBuffers()
{
    if ( NumChans == 0 ) {
        throw sExc("no channels");
    }

	// allocate circular buffers for raw data, say 10 seconds worth

	unsigned n = SamplesPerCycle*60*10;
	int pwrof2 = 0;
	do {
		pwrof2 += 1;
	} while (n >>= 1);

    for ( n = 0; n < NumChans; n++ ) {
        ChannelData[n].allocateBuffer(pwrof2);
    }
}

unsigned
dataSamples::checkLeg( unsigned leg )
{
    if ( leg >= MaxLegs ) {
        throw sExc("no more legs room");
    }
    if ( leg >= NumLegs ) {
        NumLegs = leg + 1;
    }
    return leg;
}

unsigned
dataSamples::checkChan( unsigned chan )
{
    if ( chan >= MaxChannels ) {
        throw sExc("no more chans room");
    }
    if ( chan >= NumChans ) {
        NumChans = chan + 1;
    }
    return chan;
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

  [humPower]
  0:15  ; leg:value - subtract to get to zero

  [humReactive]
  0:14  ; leg;value  - subtract to get to zero

 */
{
    dataSamples *dsp = (dataSamples*)thisp;
    adcSource *asp = dsp->isAdc();
    string sect( section ), name;
    unsigned leg, chan;

    if ( sect == "adcSetup" && asp ) {
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
    } else if ( sect == "adcZeroVal" && asp ) {
        chan = asp->checkChan( atoi( cname ) );
        asp->zeroVal[chan] = atoi( value );
    } else if ( sect == "legVoltChan" ) {
        leg = dsp->checkLeg( atoi( cname ) );
        dsp->VoltChan[leg] = dsp->checkChan( atoi( value ) );
    } else if ( sect == "legAmpChan" ) {
        leg = dsp->checkLeg( atoi( cname ) );
        dsp->AmpChan[leg] = dsp->checkChan( atoi( value ) );
    } else if ( sect == "legWattFudgeDivor" ) {
        leg = dsp->checkLeg( atoi( cname ) );
        dsp->WattFudgeDivor[leg] = atoi( value );
    } else if ( sect == "legHumPower" ) {
        leg = dsp->checkLeg( atoi( cname ) );
        dsp->HumPower[leg] = atoi( value );
    } else if ( sect == "legHumReactive" ) {
        leg = dsp->checkLeg( atoi( cname ) );
        dsp->HumReactive[leg] = atoi( value );
    }

    return 1;  // success to ini_parse
}

void
dataSamples::setup( const char *inifname )
{
    int sts;
    sts = ini_parse( inifname, cfg_value_callback, this );
    if ( sts ) {
        cout << "ini_parse failed: " << sts << endl;
    }

    if ( rawSamples() ) {
        allocateChannelBuffers();
    }
}
