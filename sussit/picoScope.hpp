#pragma once
#ifdef WIN32
class picoScope : public dataSamples {
	int scopeh;
	enPS3000Range rangeA;
	enPS3000Range rangeB;

public:
	picoScope() : dataSamples(), scopeh(0),
		rangeA(PS3000_500MV), rangeB(PS3000_20V)
	{};
	~picoScope();

//	void
//		setSamplesPerCycle( int spc ) { samplesPerCycle = spc; }
	void
		setRangeA( enPS3000Range r ) { rangeA = r; }
	void
		setRangeB( enPS3000Range r ) { rangeB = r; }

	void
		setup();
	void
		startSampling();
	int
		getSamples( int milliSleep );
	bool
		liveData() { return true; }
};
#endif // WIN32
