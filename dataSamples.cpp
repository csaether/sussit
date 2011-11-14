#include "stdafx.h"
#include "dataSamples.hpp"

void
dataSamples::allocateChannels( int numchans )
{
    if ( numchans > MaxChannels ) {
        throw sExc("too many channels");
    }
    NumChans = numchans;

	// allocate circular buffers for raw data, say 10 seconds worth

	int n = SamplesPerCycle*60*10;
	int pwrof2 = 0;
	do {
		pwrof2 += 1;
	} while (n >>= 1);

    for ( n = 0; n < numchans; n++ ) {
        ChannelData[n].allocateBuffer(pwrof2);
    }
}

void
dataSamples::addLeg( int voltchan, int ampchan )
{
    if ( NumLegs == MaxLegs ) {
        throw sExc("no more legs room");
    }
    if ( voltchan >= NumChans || ampchan >= NumChans ) {
        throw sExc("no such channel");
    }
    LegSign[NumLegs] = 1;  // until shown otherwise
    VoltChan[NumLegs] = voltchan;
    AmpChan[NumLegs++] = ampchan;
}
