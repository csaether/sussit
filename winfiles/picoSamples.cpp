#include "StdAfx.h"
#include "picoScope.hpp"

void
picoSamples::setup()
{
	// setup base class
	dsetup( 2 );

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

inline short
pvalchk( short val ) {
	if ( val == PS3000_LOST_DATA ) {
		throw sExc("bad data");
	}
#if 0
	if ( val == PS3000_MAX_VALUE ) {
		throw sExc("max value");
	}
	if ( val == PS3000_MIN_VALUE ) {
		throw sExc("min value");
	}
#endif
	return val;
}

inline int
skipchk( int skipcount )
{
	if ( ++skipcount > AvgNSamples ) {
		throw sExc("too many skips");
	}
}

int
picoSamples::avgSamples( int64_t rsi, int64_t endrawseq )
{
	int val1sum, val2sum, val1, val2, val1skips, val2skips;
	int64_t asi;
	short val;

	for ( asi = avgSampSeq; rsi < endrawseq; asi++ ) {

		// inner loop to average raw samples
		val1sum = 0;
		val2sum = 0;
		int64_t iendi = rsi + AvgNSamples;
		val1skips = 0;
		val2skips = 0;
		for ( ; rsi < iendi; rsi++ ) {
			val = pvalchk( amins[rsi] );
			if ( val == PS3000_MIN_VALUE ) {
				val1skips = skipchk( val1skips );
				cout << "skipping amin min" << endl;
			}
			val1sum += val;
			val = pvalchk( amaxs[rsi] );
			if ( val == PS3000_MAX_VALUE ) {
				val1skips = skipchk( val1skips );
				cout << "skipping amax max" << endl;
			}
			val1sum += val;

			val = pvalchk( bmins[rsi] );
			if ( val == PS3000_MIN_VALUE ) {
				val2skips = skipchk( val2skips );
				cout << "skipping bmin min" << endl;
			}
			val2sum += val;
			val = pvalchk( bmaxs[rsi] );
			if ( val == PS3000_MAX_VALUE ) {
				val2skips = skipchk( val2skips );
				cout << "skipping bmax max" << endl;
			}
			val2sum += val;
		}
		val1 = val1sum/((2*AvgNSamples) - val1skips);
		ampsamples.s( val1, asi );
		val2 = val2sum/((2*AvgNSamples) - val2skips);
		voltsamples.s( val2, asi );
	}
	int count = (int)(asi - avgSampSeq);
	avgSampSeq = asi;
	return count;
}
