#pragma once

class sussChanges
{
	uint64_t lastcycari;  // doCycles - sample index
	uint64_t ncyci;      // doCycle - next full cycle
	uint64_t prevcyci;   // doCycles - start of unprocessed run
	uint64_t chunki;     // doChunk - last processed chunk index
	uint64_t prevchunki; // doChunks
	uint64_t startVali;  // cycle index at start of current run
	uint64_t nextVali;   // next cycle index for doChanges
	uint64_t endCyci;    // doChanges when stable -> false
	cBuff<int> realPower;
	cBuff<int> reactivePower;
	cBuff<uint64_t> cycRi;
	cBuff<int> realPwrChunks;
	cBuff<int> reactiveChunks;
    unsigned chunkSize;
	unsigned chunkRun;
	int chgDiff;
	unsigned preChgout;
	unsigned postChgout;
	int startRunWatts;  // real power at start of run
	int startRunVARs;  // reactive power at start
	int runWatts;  // latest stable run real power
	int runVARs;  // latest stable reactive power
	int prevRunWatts; // previous stable run real power
	int prevRunVARs;  // previous reactive power
	int drift;
	int lastDurCycles;
//	int changeArea;
	uint64_t lastTimeStampCycle;
	unsigned cyclesPerTimeStamp;
	unsigned stableCnt;
	bool livedata;
	ofstream consOut;
	ostream *consOutp;
	ofstream eventsOut;
	ostream *eventsOutp;
	ofstream rawOut;
	ostream *rawOutp;
	ofstream cycleOut;
	ostream *cycleOutp;
	ofstream burstOut;
	ostream *burstOutp;
	uint64_t nextrawouti;  // writeRawOut
	ifstream cyclesIn;

	// methods
	void
		doCycles( dataSamples *dsp );
	void
		doCycle( dataSamples *dsp, const uint64_t nextcycri );
	void
		firstTime( dataSamples *dsp );
	void
		doChunks();
	void
		doChunk( uint64_t cyi );
	void
		doChanges( dataSamples *dsp );
	void
		writeChgOut();
	void
		writeRawOut( dataSamples *dsp );
	void
		writeCycleBurst( dataSamples *dsp, 
						 uint64_t cyci );
	uint64_t
		findCrossing( cBuff<short> &samples,
		              uint64_t from,
		              int testrange = 0 );
	void
		endrunout( ostream &out );
	void
		startrunout( ostream &out );
//	void
//		calcChangeArea();
	void
		readCycles( int numcycles, int wattfudgedivor );
	void
		timestampout( ostream &out );
public:
	sussChanges(void);
	~sussChanges(void);
	// accessors
	void
		setChgDiff( int chg ) { chgDiff = chg; }

	// methods
	void
		setConsOut( const char *fnamebase );
	void
		setEventsOut( const char *fnamebase );
	void
		setRawOut( const char *fnamebase );
	void
		setCycleOut( const char *fnamebase );
	void
		setBurstOut( const char *fnamebase );
	void
		processData( dataSamples *dsp,uint64_t maxcycles = 0 );
	void
		openCyclesIn( const char *fname );
};
