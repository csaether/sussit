#include "StdAfx.h"
#include "picoScope.hpp"

cBuff<short> *Aminsp = 0;
cBuff<short> *Amaxsp = 0;
cBuff<short> *Bminsp = 0;
cBuff<short> *Bmaxsp = 0;

unsigned __int64 *RawSeqp = 0;

picoScope::~picoScope(void)
{
	ps3000_stop( scopeh );
	ps3000_close_unit( scopeh );
}

void
picoScope::setup()
{
	scopeh = ps3000_open_unit();
	if (scopeh == 0) {
		throw sExc( "ps3000_open_unit no such scope" );
	} else if ( scopeh < 0 ) {
		throw sExc( "ps3000_open_unit failed" );
	}
	int sts;
	sts = ps3000_set_channel(
		scopeh,
		PS3000_CHANNEL_A,
		true,  // enabled true
		0,  // AC coupling
		rangeA );
	if ( !sts ) {
		throw sExc( "set channel A failed" );
	}
	sts = ps3000_set_channel(
		scopeh,
		PS3000_CHANNEL_B,
		true,  // enabled true
		0,  // AC coupling
		rangeB );
	if ( !sts ) {
		throw sExc( "set channel B failed" );
	}

	// allocate circular buffers for raw data, say 10 seconds worth

	int n = SamplesPerCycle*60*10;
	int pwrof2 = 0;
	do {
		pwrof2 += 1;
	} while (n >>= 1);

	amins.allocateBuffer(pwrof2);
	Aminsp = &amins;
	amaxs.allocateBuffer(pwrof2);
	Amaxsp = &amaxs;
	bmins.allocateBuffer(pwrof2);
	Bminsp = &bmins;
	bmaxs.allocateBuffer(pwrof2);
	Bmaxsp = &bmaxs;

	RawSeqp = &rawSeq;
}

void
picoScope::startSampling()
{
	int spsec = SamplesPerCycle*60;
	int nsecsper = 1000000000/spsec;
	nsecsper = (nsecsper/200)*200;  // needs to be multiple of 50 nanosecs
	spsec = (1000000000/nsecsper);
	SamplesPerCycle = spsec/60;
	cout << SamplesPerCycle << " spc, " << nsecsper/2 << " nanosecs per sample" << endl;
	int obc = 400000;  // spsec*3 > 10240 ? spsec*3 : 10240;
	int sts = ps3000_run_streaming_ns( scopeh,
		nsecsper/2, PS3000_NS,  // div 2 with aggr at 2
		80000,  // 2048, 64000 // what is the best value for this?
		0,  // do not stop
		2,  // aggregation, 2 uses both min and max buffs
		obc );  // overview buffers count
	if ( !sts ) {
		throw sExc( "run_streaming_ns call failed" );
	}
}

void __stdcall StreamingDataCB (
								   short **overviewBuffers,
								   short overflow,
								   unsigned long triggeredAt,
								   short triggered,
								   short auto_stop,
								   unsigned long nValues)
{
	if ( !nValues ) {
		return;
	}
	if ( nValues > 200000 ) {
		cout << "got back " << nValues << " nValues" << endl;
	}

	short *amax, *amin, *bmax, *bmin;
	amax = overviewBuffers[0];
	amin = overviewBuffers[1];
	bmax = overviewBuffers[2];
	bmin = overviewBuffers[3];
	short val = 0;

	unsigned vi = 0;
	unsigned __int64 ri = *RawSeqp;
	for ( ; vi < nValues; vi++ ) {
		val = amin[vi];
		Aminsp->s( val, ri );
		val = amax[vi];
		Amaxsp->s( val, ri );
		val = bmin[vi];
		Bminsp->s( val, ri );
		val = bmax[vi];
		Bmaxsp->s( val, ri++ );
	}
	*RawSeqp = ri;
}

int
picoScope::getSamples( int milliSleep )
{
	__int64 sampi = rawSeq;
	short over1 = 0;
	int sts;

	if ( milliSleep != 0 ) {
		::Sleep(milliSleep);
	}

	sts = ps3000_get_streaming_last_values( scopeh,
		GetOverviewBuffersMaxMin( StreamingDataCB ) );
	if ( !sts ) {
		throw sExc( "get_streaming_last_values call failed" );
	}

	sts = ps3000_overview_buffer_status( scopeh, &over1 );
	if ( !sts ) {
		throw sExc( "over buff status 1 failure" );
	}

	return (int)(rawSeq - sampi);
}
