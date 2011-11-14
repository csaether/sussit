#pragma once
extern int SamplesPerCycle;
extern int AvgNSamples;
class picoSamples;

const int MaxChannels = 8;
const int MaxLegs = 8;

class dataSamples {
public:
	uint64_t rawSampSeq;  // where we are in actual samples
	uint64_t avgSampSeq;  // and in the averaged ones, above

    int NumChans;
	cBuff<int16_t> ChannelData[MaxChannels];
    
    int NumLegs;
    int VoltChan[MaxLegs];
    int AmpChan[MaxLegs];
    int LegSign[MaxLegs];

	unsigned wattFudgeDivor;

	dataSamples( unsigned wattfudge)
        : rawSampSeq(0),
          avgSampSeq(0),
          NumChans(0),
          NumLegs(0),
          wattFudgeDivor(wattfudge)
	{}

	void
		allocateChannels( int numchans );
    void
        addLeg( int voltchan, int ampchan );
    cBuff<int16_t> &
        voltSamples(int leg) { return ChannelData[ VoltChan[leg] ]; }
    cBuff<int16_t> &
        ampSamples(int leg) { return ChannelData[ AmpChan[leg] ]; }
	virtual void
        setup() = 0;  // derived class setup
	virtual void
		startSampling() = 0;
	virtual int
		getSamples( int milliSleep ) = 0;
	virtual bool
		liveData() { return false; }
	virtual picoSamples *
		isPico() { return 0; }
};
