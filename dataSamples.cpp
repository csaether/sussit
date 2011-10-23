#include "stdafx.h"
#include "dataSamples.hpp"

void
dataSamples::setup()
{
	// allocate circular buffers for raw data, say 10 seconds worth

	int n = SamplesPerCycle*60*10;
	int pwrof2 = 0;
	do {
		pwrof2 += 1;
	} while (n >>= 1);

	voltsamples.allocateBuffer(pwrof2);
	ampsamples.allocateBuffer(pwrof2);
}
