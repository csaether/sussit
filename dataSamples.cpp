#include "stdafx.h"
#include "dataSamples.hpp"

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
