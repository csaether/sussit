#include "stdafx.h"
#include "sussChanges.hpp"

int SamplesPerCycle = 128;
int AvgNSamples = 4;

sussChanges::sussChanges(void) : lastcycri(0),
		ncyci(0),
		prevcyci(0),
		chunki(0),
		prevchunki(0),
		endCyci(0),
		chunkSize(7),  // was 12 forever..
		chunkRun(2),
		chgDiff(25),
		preChgout(12),
		postChgout(12),  // will only have 1 to 2 chunks ahead
		lastTimeStampCycle(0),
		cyclesPerTimeStamp(60*15),
		livedata(false),
		consOutp(0),
		eventsOutp(0),
		rawOutp(0),
		cycleOutp(0),
		burstOutp(0),
		nextrawouti(0)
{
	reactivePower.allocateBuffer( 10 );  // 2^10 = 1024
	realPower.allocateBuffer( 10 );  // 2^10 = 1024
	cycRi.allocateBuffer( 10 );  // 2^10 = 1024
	realPwrChunks.allocateBuffer( 4 );  // 2^4 = 16
	reactiveChunks.allocateBuffer( 4 );  // 2^4 = 16
}

sussChanges::~sussChanges(void)
{
}

void
sussChanges::processData( dataSamples *dsp, uint64_t maxcycles )
{
	ostream &sout = *consOutp;
	// keep track of raw offsets in object state
	// have raw data to work out partials
	// keep recent cycles and multi-cycle averages in state

	// raw data is not phase adjusted (previously it was)
	unsigned newvals = 0;

	sout << ",SamplesPerCycle: " << SamplesPerCycle << endl;
	sout << ",Oversampling: " << AvgNSamples << endl;
	sout << ",chunkSize: " << chunkSize << endl;
	sout <<	",chgDiff: " << chgDiff << endl;
	sout << ",wattFudgeDivor: " << dsp->wattFudgeDivor << endl;
	sout << ",V2 starting at " << thetime() << endl;

	if ( dsp ) {  // have picosource flag now also
		dsp->setup();

		dsp->startSampling();

		livedata = dsp->liveData();  // true if picosource

		// get at least an initial chunk run's worth of data
		// plus a full cycle so initial crossing can be set
		do {
			newvals += dsp->getSamples( 100 );
		} while ( newvals < (SamplesPerCycle*(chunkSize*chunkRun+3)) );

		writeRawOut( dsp );
	}

	firstTime( dsp );
	timestampout( cout );  // initial timestamp
	timestampout( *eventsOutp );

	while ( !maxcycles || (ncyci < maxcycles) ) try {
		if ( dsp ) {
			newvals = dsp->getSamples( 77 );
			//		cout << "got back " << newvals << endl;

			doCycles( dsp );

			writeRawOut( dsp );
		} else {
			readCycles( 60, 21743 );  // FIX: - retrieve from file (and store first)
		}

		doChunks();

		unsigned cd = (unsigned)(ncyci - 1 - lastTimeStampCycle);
		if ( cd > cyclesPerTimeStamp ) {
			timestampout( cout );
			timestampout( *eventsOutp );
			lastTimeStampCycle = ncyci -1;
			//writeCycleBurst( dsp, lastTimeStampCycle - 1 );
			//writeCycleBurst( dsp, lastTimeStampCycle );

			ifstream controlfile;
			controlfile.open( "runsuss.txt", ios::in );
			if ( !controlfile ) {
				throw sExc( "runsuss file not found" );
			}
			controlfile.close();
		}

		doChanges( dsp );
	} catch ( exception &x ) {
		sout << ",caught exception " << x.what() << endl;
		sout << startRunVal << ",\t" << runVal << ",\t";
		sout << ",end second,\t" << ncyci/60.0 << endl;
		throw;
	}
}

void
sussChanges::openCyclesIn( const char *fname )
{
	cyclesIn.open( fname, ios::binary | ios::in );
	if ( !cyclesIn ) {
		throw sExc( "could not open cyclesIn" );
	}
}

void
sussChanges::readCycles( int numcycles, int wattfudgedivor )
{
	if ( cyclesIn.fail() ) {
		throw sExc( "no more file data" );
	}
	prevcyci = ncyci;
	int realreac[2];
	int count;
	for ( count = 0; count < numcycles; count++, ncyci++ ) {
		cyclesIn.read( (char*)realreac, 8 );
		if ( cyclesIn.eof() ) {
			break;
		}
		realPower.s( realreac[0]/wattfudgedivor, ncyci );  // re-fudge
		reactivePower.s( realreac[1], ncyci );
	}
}

void
sussChanges::doCycles( dataSamples *dsp )
{
	uint64_t nextcycri;
	prevcyci = ncyci;  // so we know how many are new

	// lastcycri is always on a cycle boundary

	uint64_t crossi1, endri;
	int64_t delta = 0;
	endri = dsp->avgSampSeq - SamplesPerCycle;  // leave an extra cycle
	nextcycri = lastcycri + SamplesPerCycle;
	for ( ; nextcycri < endri; 
		   lastcycri = nextcycri,
		   nextcycri += SamplesPerCycle ) { //+ (1 & tog++)

		crossi1 = findCrossing( dsp->voltsamples,
			nextcycri ); //  + 10, 20 ? // + SamplesPerCycle/4 );

		// adjust nextcycri before doing this cycle
#ifdef hmmmcode
	the idea here was to not make tiny adjustments with large
	sample rates, but now we are using all of the samples, not just
	the nominal samples per second, so set this each time.  the crossing
	point for the voltage signal really should not be very jumpy, if it is
	try to smooth it in the find crossing code first.
		delta += (signed)crossi1 - (signed)nextcycri;
		if ( delta > dsp->deltadjust ) {  // 2*
			nextcycri += dsp->deltadjust;
//			cout << delta << " plus " << dsp->deltadjust << endl;
			delta = 0;
		} else if ( delta < -dsp->deltadjust ) {  // 2*
			nextcycri -= dsp->deltadjust;
//			cout << delta << " minus " << dsp->deltadjust << endl;
			delta = 0;
		} else {
//			cout << " none" << endl;
		}
#else
		nextcycri = crossi1;
#endif // hmmmcode

		doCycle( dsp, nextcycri );
	}
	// write out cycle real and reactive power data if requested
	if ( cycleOutp ) {
		uint64_t i;
		int realreac[2];
		for ( i = prevcyci; i < ncyci; i++ ) {
			realreac[0] = realPower[i]*dsp->wattFudgeDivor;  // for possible re-fudging
			realreac[1] = reactivePower[i]*dsp->wattFudgeDivor;
			cycleOutp->write( (char*)realreac, 8 );
		}
	}
}

void
sussChanges::doCycle( dataSamples *dsp,
					 const uint64_t nextcycri )
{
	int64_t cycsum = 0;
	uint64_t ari, ri, sampcount = nextcycri - lastcycri;

	for ( ri = lastcycri; ri < nextcycri; ri++ ) {
		// this is where checking for any per sample noise would go
		cycsum += dsp->ampsamples[ri]*dsp->voltsamples[ri];
	}
	cycsum /= sampcount;
	int cval = (int)(cycsum/dsp->wattFudgeDivor);  // big fudge
	realPower.s( cval, ncyci );
#ifdef oldcode
	uint64_t crossi = findCrossing( dsp->ampsamples,
		lastcycri );
	int delta = (signed)crossi - (signed)lastcycri;
#endif  // oldcode
	// a positive delta is lagging in time (further ahead in the sample array)
	// calculate reactive power by using current samples shifted a quarter of
	// a cycle delayed (ahead in array)
	cycsum = 0;
	for ( ri = lastcycri, ari = lastcycri + (sampcount/4);
			ri < nextcycri; ri++, ari++ ) {
		// this is where checking for any per sample noise would go
		cycsum += dsp->ampsamples[ari]*dsp->voltsamples[ri];
	}
	cycsum /= sampcount;
	cval = (int)(cycsum/dsp->wattFudgeDivor);  // big fudge
	reactivePower.s( cval, ncyci );

	cycRi.s( lastcycri, ncyci++ );
}

uint64_t
sussChanges::findCrossing(cBuff<short> &samples,
						  uint64_t fromi,
						  int testrange )
{
	// the search is centered around fromi, looking back samplespercycle,
	// plus or minus half the testrange
	uint64_t toi, ri, crossi;
	int here, back1;
	unsigned spc, val, low = 0xffffffff;
	spc = SamplesPerCycle;
	if ( !testrange ) {
		testrange = spc/2 - 1;
	}
	toi = fromi - testrange/2;
	ri = fromi + testrange/2;
	crossi = ri;
	for ( ; ri > toi; ri-- ) {
		here = samples[ri];
		here += samples[ri - 1];
		here += samples[ri + 1];

		back1 = samples[ri - spc];
		back1 += samples[ri - spc - 1];
		back1 += samples[ri - spc + 1];

		val = abs(here) + abs(back1);
		if ( val < low ) {
			low = val;
			crossi = ri;
		}
	}
	return crossi;
}

void
sussChanges::firstTime( dataSamples *dsp )
{
	// don't need two full cycles to get going, but honestly..

	if ( dsp ) {
		if ( dsp->avgSampSeq < (2*SamplesPerCycle) ) {
			throw sExc("not enough initial samples");
		}

		// the B channel is the voltage signal, and we are looking for the zero
		// crossing initally to set lastcycri.

		lastcycri = findCrossing( dsp->voltsamples,
			(3*SamplesPerCycle)/2 );	

		doCycles( dsp );
	} else {
		readCycles( 60, 21743 );
	}

	doChunks();
	if ( chunki == 0 ) {
		throw sExc("firstTime could not do initial chunk");
	}

	// declare the initial value a stable run
	// startVali and start chunki both zero
	startVali = 0;
	startRunVal = realPwrChunks[0];
	runVal = startRunVal;
	prevRunVal = startRunVal;
// 	lastChunkVal = startRunVal;
	stable = true;
	nextVali = chunkSize;
}

void
sussChanges::doChunks()
{
	uint64_t cyi;
	for ( cyi = prevchunki*chunkSize; (cyi + chunkSize) <= ncyci; cyi += chunkSize ) {
		doChunk( cyi );
	}
	prevchunki = chunki;  // really the next one to be done
}

void
sussChanges::doChunk( uint64_t cyi )
{
	uint64_t i;
	int64_t rpwrsum = 0;
	int64_t reacsum = 0;
	int rpwrval, reacval;
	for ( i = cyi; i < cyi + chunkSize; i++ ) {
		rpwrval = realPower[i];
		rpwrsum += rpwrval;
		reacval = reactivePower[i];
		reacsum += reacval;
	}
	realPwrChunks.s( (int)(rpwrsum/chunkSize), chunki );
	reactiveChunks.s( (int)(reacsum/chunkSize), chunki++ );
}

void
sussChanges::doChanges( dataSamples *dsp )
// runVal is either a) the current run value, or b) the potential new value
// while we are looking for another run when stable is false
// runVal is updated with the current value regardless, letting it drift slowly
{
	int chval, cval, diff;
	uint64_t trycy, chi = nextVali/chunkSize;
	for( ; chi < chunki; chi++ ) {
		chval = realPwrChunks[chi];

		if ( stable ) {
			diff = chval - runVal;
		} else {
			diff = chval - realPwrChunks[chi - 1];
		}

		if ( abs(diff) >= chgDiff ) {
			if ( stable ) {
				// was stable, but now have exceeded diff
				// the last stable chunk was the previous one
				// and the cycles may have started moving inside it
				endCyci = chi*chunkSize - chunkSize/2;  // start looking here
				if ( endCyci <= startVali ) {
					cout << "whoa - endCyci less than startVali: " << endCyci << startVali << endl;
					endCyci = startVali + 1;
				}

				// go forward by cycle to find
				// specific cycle where change starts
				for ( trycy = endCyci + 1; trycy < (chi+1)*chunkSize; trycy += 1 ) {
					cval = realPower[trycy];
					diff = cval - runVal;
					if ( abs(diff) >= chgDiff ) {
						cval = realPower[trycy + 1];
						diff = cval - runVal;
						if ( abs(diff) >= chgDiff ) {
							break;
						}
					}
					endCyci = trycy;
				}

				stable = false;
				drift = runVal - startRunVal;
				lastDurCycles = (int)(endCyci - startVali);

				prevRunVal = runVal;  // gone to not stable, set to last stable chunk val
			}
		} else {
			// diff is less than threshold, start new run
			if ( !stable ) {

				diff = chval - prevRunVal;

				if ( abs(diff) < chgDiff*3 ) {  // BIG CHANGE - only report bigger changes!!!
					// just a jumpy chunk, so even though endCyci has
					// been changed, no matter.  leave startVali, startRunVal alone
					// and do not write change event records
					stable = true;
					runVal = chval;
					continue;
				}

				if ( (chi*chunkSize - endCyci) < chunkSize ) {
					// leave unstable, might just be a blip
					continue;  // does not update runVal
				}

				stable = true;
				if ( consOutp ) {
					endrunout( *consOutp );  // uses prevRunVal
				}
				endrunout( cout );

				startVali = chi*chunkSize - 1;  // previous chunk

				if ( startVali <= endCyci ) {
					throw sExc( "what the - startVali less than endCyci??" );
				}
				// back up and identify where it really starts

				for ( trycy = startVali; trycy > endCyci; trycy-- ) {
					cval = realPower[trycy];
					diff = cval - chval;
					if ( abs(diff) >= chgDiff ) {
						// see if this is just a jumpy cycle
						cval = realPower[trycy - 1];
						diff = cval - chval;
						if ( abs(diff) >= chgDiff ) {
							break;
						}
					}
					startVali = trycy;
				}

				// writing a chunk size set of samples before, during,
				// and after the transition
				for ( trycy = endCyci - chunkSize;
					trycy < startVali + chunkSize;
					trycy++ ) {
						writeCycleBurst( dsp, trycy );
				}

				// startRunVal is only being used for this change area calculation and drift output
				// both of which are not necessary to figure out here.  pretty sure.
				startRunVal = chval;
				calcChangeArea();
				if ( consOutp ) {
					startrunout( *consOutp );
				}
				startrunout( cout );

				writeChgOut();
			}
		}
		runVal = chval;
	}
	nextVali = chi*chunkSize;
}

void
sussChanges::setConsOut( const char *fnamebase )
{
	if ( !fnamebase ) {
		return;
	}

	string fname(fnamebase);
	fname += "-s.csv";
	consOut.open( fname.c_str() );
	if ( !consOut ) {
		throw sExc( "create consOut failed" );
	}
	consOutp = &consOut;
}
void
sussChanges::setEventsOut( const char *fnamebase )
{
	if ( !fnamebase ) {
//		chgOutp = &cout;
		return;
	}

	string fname(fnamebase);
	fname += "-e.csv";
	eventsOut.open( fname.c_str() );
	if ( !eventsOut ) {
		throw sExc( "create eventsOut failed" );
	}
	eventsOutp = &eventsOut;
}
void
sussChanges::endrunout( ostream &out )
{
	// blank, value, driftrate
	out << ",     ";
	out << prevRunVal << ",   " << ((60000*drift)/lastDurCycles)/1000.0;
	out << endl;
}

void
sussChanges::startrunout( ostream &out )
{
	// time, change, real power value, reactive power change, changeArea
	out << startRunVal - prevRunVal;
	out << ",  " << startRunVal;
	out << ",  " << reactivePower[startVali+2] - reactivePower[endCyci-2];
	// changeArea - rampfactor?
	if ( livedata ) {
		out << ",  " << thetime() << endl;
	} else {
		out << ",  " << startVali << ",  ";
		unsigned hours, mins, secs;
		hours = (unsigned)(startVali/(60*60*60));
		mins = (unsigned)(startVali/(60*60)) - hours*(60*60*60);
		secs = (unsigned)(startVali/60) - hours*(60*60*60) - mins*(60*60);
		out << hours << ":" << mins << ":" << secs << endl;
	}
}

void
sussChanges::timestampout( ostream &out )
{
	out << "T,  " << ncyci - 1 << ",  " << realPwrChunks[chunki-1];  // chunks not so jumpy
	out << ",  " << reactiveChunks[chunki-1];
	if ( livedata ) {
		out << ",  " << thetime();
	}
	out << endl;
}

void
sussChanges::writeChgOut()
{
	if ( !eventsOutp ) {
		return;
	}
	ostream &chout = *eventsOutp;

	int pwrval, reacval;
	uint64_t cyci, ecyci;
	pwrval = 0; reacval = 0;
	for ( cyci = endCyci - 4; cyci < endCyci; cyci++ ) {
		pwrval += realPower[cyci];
		reacval += reactivePower[cyci];
	}
	pwrval /= 4;
	reacval /= 4;
	chout << "E, " << endCyci << ", " << pwrval << ", " << reacval << endl;
	pwrval = 0; reacval = 0;
	for ( cyci = startVali + 1; cyci < startVali + 5; cyci++ ) {
		pwrval += realPower[cyci];
		reacval += reactivePower[cyci];
	}
	pwrval /= 4;
	reacval /= 4;
	chout << "S, " << startVali << ", " << pwrval << ", " << reacval << endl;
#ifdef oldcode
	pval = (reactivePower[endCyci-3] + reactivePower[endCyci-2] + reactivePower[endCyci-1])/3;
	chout << "E, " << endCyci << ", " << prevRunVal << ", " << pval << endl;
	pval = (reactivePower[startVali+3] + reactivePower[startVali+2] + reactivePower[startVali+1])/3;
	chout << "S, " << startVali << ", " << startRunVal << ", " << pval << endl;
#endif // oldcode
	cyci = endCyci;
	if ( cyci > preChgout ) {
		cyci -= preChgout;
	}

	ecyci = startVali + postChgout;
	for ( ; cyci <= ecyci; cyci++ ) {
		pwrval = realPower[cyci];
		reacval = reactivePower[cyci];
		chout << "C, " << cyci << ", " << pwrval << ", " << reacval << endl;
	}
}

void
sussChanges::calcChangeArea()
{
	uint64_t cyci;
	changeArea = 0;
	for ( cyci = endCyci + 1; cyci <= startVali; cyci++ ) {
		changeArea += (realPower[cyci - 1] + realPower[cyci] - 2*startRunVal)/2;
	}
}

void
sussChanges::setRawOut( const char *fnamebase )
{
	string fname(fnamebase);
	fname += "-raw.dat";
	rawOut.open( fname.c_str(), ios::binary | ios::out );
	if ( !rawOut ) {
		throw sExc( "create rawOut failed" );
	}
	rawOutp = &rawOut;
}

void
sussChanges::setCycleOut( const char *fnamebase )
{
	string fname(fnamebase);
	fname += "-cyc.dat";
	cycleOut.open( fname.c_str(), ios::binary | ios::out );
	if ( !cycleOut ) {
		throw sExc( "create cycleOut failed" );
	}
	cycleOutp = &cycleOut;
}

void
sussChanges::setBurstOut( const char *fnamebase )
{
	string fname(fnamebase);
	fname += "-burst.dat";
	burstOut.open( fname.c_str(), ios::binary | ios::out );
	if ( !cycleOut ) {
		throw sExc( "create burstOut failed" );
	}
	burstOutp = &burstOut;
}

void
sussChanges::writeRawOut( dataSamples *dsp )
{
	if ( !rawOutp ) {
		return;
	}
	picoSamples *psp = dsp->isPico();
	if ( !psp ) {
		return;
	}
	short raw[4];
	uint64_t ri;
	for ( ri = nextrawouti; ri < psp->rawSampSeq; ri++ ) {
		raw[0] = psp->amaxs[ri];
		raw[1] = psp->amins[ri];
		raw[2] = psp->bmaxs[ri];
		raw[3] = psp->bmins[ri];
		rawOutp->write( (char*)raw, 8 );
	}
	nextrawouti = ri;
}

void
sussChanges::writeCycleBurst( dataSamples *dsp, uint64_t cyci )
	                          
{
	if ( !burstOutp ) {
		return;
	}
	short raw[2] = { 0x1abc, 0x1abc };
	// write out marker, cycle number, count
	burstOutp->write( (char*)raw, 4 );
	burstOutp->write( (char*)&cyci, 8 );

	uint64_t ri, endriv;
	ri = cycRi[cyci];
	endriv = cycRi[cyci + 1];  // always have next cycle done

	for ( ; ri < endriv; ri++ ) {
		// write the averaged value twice for compatibility now that
		// raw samples might be oversampled.
		raw[0] = dsp->ampsamples[ri];
		raw[1] = dsp->voltsamples[ri];
		burstOutp->write( (char*)raw, 4 );
	}
}

#ifdef oldversion
void
sussChanges::writeCycleBurst( dataSamples *dsp, uint64_t cyci )
{
	if ( !burstOutp ) {
		return;
	}
	//picoSamples *psp = dsp->isPico();
	//if ( !psp ) {
	//	return;
	//}
	short raw[4] = { 0x7abc, 0x7abc, 0x7abc, 0x7abc };
	// write out marker, cycle number
	burstOutp->write( (char*)raw, 8 );
	burstOutp->write( (char*)&cyci, 8 );

	uint64_t ri, endriv;
	ri = cycRi[cyci];
	endriv = ri + SamplesPerCycle;

	for ( ; ri < endriv; ri++ ) {
		// write the averaged value twice for compatibility now that
		// raw samples might be oversampled.
		raw[0] = dsp->ampsamples[ri];
		raw[1] = dsp->ampsamples[ri];
		raw[2] = dsp->voltsamples[ri];
		raw[3] = dsp->voltsamples[ri];
		burstOutp->write( (char*)raw, 8 );
	}
}
#endif // oldversion