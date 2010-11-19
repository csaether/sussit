#pragma once
extern unsigned SamplesPerCycle;
class dataSamples {
public:
	unsigned __int64 rawSeq;
//	unsigned samplesPerCycle;
	unsigned phaseOff;  // mistaken idea when using bogus voltage signal
	int deltadjust;

	cBuff<short> amins;
	cBuff<short> amaxs;
	cBuff<short> bmins;
	cBuff<short> bmaxs;
	dataSamples() : rawSeq(0),
//		samplesPerCycle(4096),  // 271
		phaseOff(0*SamplesPerCycle/271),  // mistaken idea - zero with correct voltage signal
		deltadjust(SamplesPerCycle/271 > 0 ? SamplesPerCycle/271 : 1)
	{}

	virtual void
		setup() = 0;
	virtual void
		startSampling() = 0;
	virtual int
		getSamples( int milliSleep ) = 0;
	virtual bool
		liveData() { return false; }
};