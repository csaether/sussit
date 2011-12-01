#pragma once
#include <fstream>

class adcSource : public dataSamples {
	int adcfd;
    int16_t *stagep;
    int stagecnt;

public:
    int zeroVal[MaxChannels];
    string setchannels;
    string setspeed;
    string setdupes;
    string setignore;

	adcSource()
        : dataSamples(), stagep(0), stagecnt(0) {}
	~adcSource(void);

	void
	open( const char *fname = 0 );

	void
	setup( const char *inifname );
	void
	startSampling();
	int
	getSamples( int milliSleep );
    bool
    liveData() { return true; }
    bool
    rawSamples() { return true; }
};
