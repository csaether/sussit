#include "StdAfx.h"
#include "picoScope.hpp"

void
picoSamples::setup()
{
	// setup base class
	dataSamples::setup();

	// allocate circular buffers for raw data, say 10 seconds worth

	int n = SamplesPerCycle*AvgNSamples*60*10;
	int pwrof2 = 0;
	do {
		pwrof2 += 1;
	} while (n >>= 1);

	amins.allocateBuffer(pwrof2);
	amaxs.allocateBuffer(pwrof2);
	bmins.allocateBuffer(pwrof2);
	bmaxs.allocateBuffer(pwrof2);
}

short
pvalchk( short val ) {
	if ( val == PS3000_LOST_DATA ) {
		throw sExc("bad data");
	}

	if ( val == PS3000_MAX_VALUE ) {
		throw sExc("max value");
	}
	if ( val == PS3000_MIN_VALUE ) {
		throw sExc("min value");
	}

	return val;
}

int
picoSamples::avgSamples( int64_t rsi, int64_t endrawseq )
{
	int val1sum, val2sum, val1, val2;
	int64_t asi;

	for ( asi = avgSampSeq; rsi < endrawseq; asi++ ) {

		// inner loop to average raw samples
		val1sum = 0;
		val2sum = 0;
		int64_t iendi = rsi + AvgNSamples;
		for ( ; rsi < iendi; rsi++ ) {
			val1sum += pvalchk( amins[rsi] );
			val1sum += pvalchk( amaxs[rsi] );

			val2sum += pvalchk( bmins[rsi] );
			val2sum += pvalchk( bmaxs[rsi] );
		}
		val1 = val1sum/(2*AvgNSamples);
		ampsamples.s( val1, asi );
		val2 = val2sum/(2*AvgNSamples);
		voltsamples.s( val2, asi );
	}
	int count = (int)(asi - avgSampSeq);
	avgSampSeq = asi;
	return count;
}