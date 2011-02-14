#pragma once

class sussChanges
{
	uint64_t lastcycri;  // doCycles - raw sample index
	uint64_t ncyci;      // doCycle - next full cycle
	uint64_t prevcyci;   // doCycles - start of unprocessed run
	uint64_t chunki;     // doChunk - last processed chunk index
	uint64_t prevchunki; // doChunks
	uint64_t startVali;  // cycle index at start of run
	uint64_t nextVali;   // next cycle index for doChanges
	uint64_t endCyci;    // doChanges when stable -> false
	cBuff<int> phaseOffs;
	cBuff<int> cycleVals;
	cBuff<uint64_t> cycRi;
	cBuff<int> cyChunks;
	cBuff<int> lagChunks;
    unsigned chunkSize;
	unsigned chunkRun;
	int chgDiff;
	unsigned preChgout;
	unsigned postChgout;
	int startRunVal;  // value at start of run
	int runVal;  // latest stable run value
	int prevRunVal; // previous stable run value
	int drift;
	int lastDurCycles;
	int changeArea;
	uint64_t lastTimeStampCycle;
	unsigned cyclesPerTimeStamp;
	bool stable;
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
		writeCycleBurst( dataSamples *dsp, uint64_t cyci );
	uint64_t
		findCrossing( cBuff<short> &samples,
		              uint64_t from,
		              int testrange = 0 );
	void
		endrunout( ostream &out );
	void
		startrunout( ostream &out );
	void
		calcChangeArea();
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
