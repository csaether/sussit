#pragma once

class sussChanges
{
	unsigned __int64 lastcycri;  // doCycles - raw sample index
	unsigned __int64 ncyci;      // doCycle - next full cycle
	unsigned __int64 prevcyci;   // doCycles - start of unprocessed run
	unsigned __int64 chunki;     // doChunk - last processed chunk index
	unsigned __int64 prevchunki; // doChunks
	unsigned __int64 startVali;  // cycle index at start of run
	unsigned __int64 nextVali;   // next cycle index for doChanges
	unsigned __int64 endCyci;    // doChanges when stable -> false
	cBuff<int> phaseOffs;
	cBuff<int> cycleVals;
	cBuff<unsigned __int64> cycRi;
	cBuff<int> cyChunks;
	cBuff<int> lagChunks;
	int wattFudgeDivor;
	int chunkSize;
	int chunkRun;
	int chgDiff;
	int preChgout;
	int postChgout;
	int startRunVal;  // value at start of run
	int runVal;  // latest stable run value
	int prevRunVal; // previous stable run value
	int drift;
	int lastDurCycles;
	int changeArea;
	unsigned __int64 lastTimeStampCycle;
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
	unsigned __int64 nextrawouti;  // writeRawOut
	ifstream cyclesIn;

	// methods
	void
		doCycles( dataSamples *dsp );
	void
		doCycle( dataSamples *dsp, const unsigned __int64 nextcycri );
	void
		firstTime( dataSamples *dsp );
	void
		doChunks();
	void
		doChunk( unsigned __int64 cyi );
	void
		doChanges( dataSamples *dsp );
	void
		writeChgOut();
	void
		writeRawOut( dataSamples *dsp );
	void
		writeCycleBurst( dataSamples *dsp, unsigned __int64 cyci );
	unsigned __int64
		findCrossing( dataSamples *dsp, 
		cBuff<short> &mins, 
		cBuff<short> &maxs,
		unsigned __int64 from,
		int testrange = 0 );
	void
		endrunout( ostream &out );
	void
		startrunout( ostream &out );
	void
		calcChangeArea();
	void
		readCycles( int numcycles );
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
		processData( dataSamples *dsp, int maxcycles = 0 );
	void
		openCyclesIn( const char *fname );
};
