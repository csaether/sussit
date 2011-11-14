#pragma once
#include <fstream>

class adcSource : public adcSamples {
	int adcfd;
    int16_t *stagep;
    int stagecnt;
public:
	adcSource()
        : adcSamples(), stagep(0), stagecnt(0) {}
	~adcSource(void);

	void
	open( const char *fname = 0 );
	void
	setup();
	void
	startSampling();
	int
	getSamples( int milliSleep );
    bool
    liveData() { return true; }
	adcSource*
	isAdc() { return this; }
};
