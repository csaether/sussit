#pragma once
extern int SamplesPerCycle;
extern int AvgNSamples;
class picoSamples;

const unsigned MaxChannels = 8;
const unsigned MaxLegs = 8;

class dataSamples {
public:
	uint64_t rawSampSeq;  // where we are in actual samples
	uint64_t avgSampSeq;  // and in the averaged ones, above

    unsigned NumChans;
	cBuff<int16_t> ChannelData[MaxChannels];
    
    unsigned NumLegs;
    int VoltChan[MaxLegs];
    int AmpChan[MaxLegs];

	int WattFudgeDivor[MaxLegs];
    int HumPower[MaxLegs];
    int HumReactive[MaxLegs];

	dataSamples( );

	void
		allocateChannelBuffers();
    unsigned
        checkLeg( unsigned leg );
    unsigned
        checkChan( unsigned chan );
    cBuff<int16_t> &
        voltSamples(int leg) { return ChannelData[ VoltChan[leg] ]; }
    cBuff<int16_t> &
        ampSamples(int leg) { return ChannelData[ AmpChan[leg] ]; }
	virtual void
        setup( const char *inifname = "cfgsuss.ini" ) = 0;
	virtual void
		startSampling() = 0;
	virtual int
		getSamples( int milliSleep ) = 0;
	virtual bool
		liveData() { return false; }
	// virtual picoSamples *
	// 	isPico() { return 0; }
};
