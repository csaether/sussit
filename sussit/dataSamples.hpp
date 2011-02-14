#pragma once
extern unsigned SamplesPerCycle;
class picoSamples;
class dataSamples {
public:
	cBuff<int16_t> voltsamples;
	cBuff<int16_t> ampsamples;
	unsigned wattFudgeDivor;

	uint64_t rawSeq;
	int deltadjust;

	dataSamples(unsigned wattfudge) : wattFudgeDivor(wattfudge), rawSeq(0),
		deltadjust(SamplesPerCycle/271 > 0 ? SamplesPerCycle/271 : 1)
	{}

	virtual void
		setup();
	virtual void
		startSampling() = 0;
	virtual int
		getSamples( int milliSleep ) = 0;
	virtual bool
		liveData() { return false; }
	virtual picoSamples *
		isPico() { return 0; }
};
