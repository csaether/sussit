#pragma once
extern int SamplesPerCycle;
extern int AvgNSamples;
class picoSamples;
class dataSamples {
public:
	cBuff<int16_t> voltsamples;
	cBuff<int16_t> ampsamples;
	unsigned wattFudgeDivor;

	uint64_t rawSampSeq;
	uint64_t avgSampSeq;
	int deltadjust;

	dataSamples(unsigned wattfudge) : wattFudgeDivor(wattfudge), rawSampSeq(0),
		avgSampSeq(0),
		// TODO: - where did 271 come from?
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
