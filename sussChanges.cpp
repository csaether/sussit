#include "stdafx.h"
#include "sussChanges.hpp"

int SamplesPerCycle = 260;
int AvgNSamples = 1;

sussChanges::sussChanges(void) : lastCycAri(0),
        nCyci(0),
        prevCyci(0),
        cHunki(0),
        prevcHunki(0),
        endCyci(0),
        numLegs(0),
        chunkSize(8),  // was 12 forever..
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
    cycRi.allocateBuffer( 10 );  // 2^10 = 1024
}

sussChanges::~sussChanges(void)
{
}

void
sussChanges::setNumLegs( int legs )
{
    int i;
    for ( i = 0; i < legs; i++ ) {
        realPowerLeg[i].allocateBuffer( 10 );  // 2^10 = 1024
        reactivePowerLeg[i].allocateBuffer( 10 );  // 2^10 = 1024
        realPwrChunksLeg[i].allocateBuffer( 6 );  // 2^6 = 64
        reactiveChunksLeg[i].allocateBuffer( 6 );  // 2^6 = 64
    }
    numLegs = legs;
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

    if ( dsp ) {  // have picosource flag now also
        dsp->setup();

        setNumLegs( dsp->NumLegs );

        dsp->startSampling();

        livedata = dsp->liveData();  // true if picosource

        if ( livedata ) try {
            dsp->getSamples(0);
        } catch ( sExc &ex ) {
            cout << "initial getSamples exception " << ex.what() << endl;
        }

        dsp->rawSampSeq = 0;  // throw away initial set

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

    sout << ",SamplesPerCycle: " << SamplesPerCycle << endl;
    sout << ",Oversampling: " << AvgNSamples << endl;
    sout << ",chunkSize: " << chunkSize << endl;
    sout << ",chgDiff: " << chgDiff << endl;
    sout << ",wattFudgeDivor: " << (dsp ? dsp->wattFudgeDivor : 10873) << endl;
    sout << ",V2 starting at " << thetime() << endl;

    while ( !maxcycles || (nCyci < maxcycles) ) try {
        if ( dsp ) {
            newvals = dsp->getSamples( 77 );
            //      cout << "got back " << newvals << endl;

            doCycles( dsp );

            writeRawOut( dsp );
        } else {
            readCycles( 60, 10873 );
        }

        doChunks();

        unsigned cd = (unsigned)(nCyci - 1 - lastTimeStampCycle);
        if ( cd > cyclesPerTimeStamp ) {
            timestampout( cout );
            timestampout( *eventsOutp );
            lastTimeStampCycle = nCyci -1;
            //writeCycleBurst( dsp, lastTimeStampCycle - 1 );
            //writeCycleBurst( dsp, lastTimeStampCycle );

            if ( dsp ) {
                ifstream controlfile;
                controlfile.open( "runsuss.txt", ios::in );
                if ( !controlfile ) {
                    throw sExc( "runsuss file not found" );
                }
                controlfile.close();
            }
        }

        doChanges( dsp );
    } catch ( exception &x ) {
        sout << ",caught exception " << x.what() << endl;
        sout << startRunWatts << ",\t" << runWatts << ",\t";
        sout << ",end second,\t" << nCyci/60.0 << endl;
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
    prevCyci = nCyci;
    int realreac[2];
    int count;
    for ( count = 0; count < numcycles; count++, nCyci++ ) {
        cyclesIn.read( (char*)realreac, 8 );
        if ( cyclesIn.eof() ) {
            break;
        }
        realPowerLeg[0].s( realreac[0]/wattfudgedivor, nCyci );  // re-fudge
        reactivePowerLeg[0].s( realreac[1]/wattfudgedivor, nCyci );
    }
}

void
sussChanges::doCycles( dataSamples *dsp )
{
    uint64_t nextcycri;
    prevCyci = nCyci;  // so we know how many are new

    // lastCycAri is always on a cycle boundary

    uint64_t crossi1, endri;
    // for now, assuming a single voltage channel, so use leg zero
    cBuff<int16_t> & voltsamps = dsp->voltSamples( 0 );

    // leaving some samples ahead of where we will stop because the
    // reactive power calculation is now reaching into the next cycle.
    endri = dsp->avgSampSeq - 2*SamplesPerCycle;  // leave two extra cycles
    nextcycri = lastCycAri + SamplesPerCycle;
    int leg;
    for ( ; nextcycri < endri; 
           lastCycAri = nextcycri,
           nextcycri += SamplesPerCycle ) {

        crossi1 = findCrossing( voltsamps, nextcycri, SamplesPerCycle/8 );

        // adjust nextcycri before doing this cycle
        nextcycri = crossi1;

        for ( leg = 0; leg < dsp->NumLegs; leg++ ) {
            doCycle( dsp, nextcycri, leg );
        }
        cycRi.s( lastCycAri, nCyci++ );
    }
    // write out cycle real and reactive power data if requested
    if ( cycleOutp ) {
        uint64_t i;
        int realreac[2];
        for ( i = prevCyci; i < nCyci; i++ ) {
            realreac[0] = realPowerLeg[0][i]*dsp->wattFudgeDivor;  // for re-fudging
            realreac[1] = reactivePowerLeg[0][i]*dsp->wattFudgeDivor;
            cycleOutp->write( (char*)realreac, 8 );
        }
    }
}

void
sussChanges::doCycle( dataSamples *dsp,
                      const uint64_t nextcycri,
                      int leg )
{
    int64_t cycval, cycsum = 0;
    uint64_t ampri, ri;
    int cval, ival, sampcount = (int)(nextcycri - lastCycAri);
    cBuff<int16_t> & voltsamps = dsp->voltSamples( leg );
    cBuff<int16_t> & ampsamps = dsp->ampSamples( leg );

    for ( ri = lastCycAri; ri < nextcycri; ri++ ) {
        // this is where checking for any per sample noise would go
        ival = ampsamps[ri]*voltsamps[ri];
        cycsum += ival;
    }
    cycval = cycsum/(sampcount*dsp->wattFudgeDivor);
    cval = (int)cycval;
    realPowerLeg[leg].s( cval, nCyci );

    // a positive delta is lagging in time (further ahead in the sample array)
    // calculate reactive power by using current samples shifted a quarter of
    // a cycle delayed (ahead in array)
    cycsum = 0;
    for ( ri = lastCycAri, ampri = lastCycAri + (sampcount/4);
            ri < nextcycri; ri++, ampri++ ) {
        // this is where checking for any per sample noise would go
        cycsum += ampsamps[ampri]*voltsamps[ri];
    }
    cycval = cycsum/(sampcount*dsp->wattFudgeDivor);
    cval = (int)cycval;
    reactivePowerLeg[leg].s( cval, nCyci );
}

uint64_t
sussChanges::findCrossing(cBuff<short> &samples,
                          uint64_t fromi,
                          int testrange )
{
    // the search is centered around fromi, looking back samplespercycle,
    // plus or minus half the testrange
    // looking for the absolute minimum of a set of samples centered
    // around the test sample
    const int delta = 2;
    uint64_t toi, ri, crossi;
    int here;
    unsigned val, low;
    if ( !testrange ) {
        testrange = SamplesPerCycle/4 - 1;
    }
    if ( testrange < 3*delta ) {
        testrange = 3*delta;
    }

    ri = fromi - testrange/2;
    crossi = ri;
    toi = ri + delta;
    here = 0;
    for ( ri -= delta; ri <= toi; ri++ ) {
        here += samples[ri];
    }
    low = abs(here);

    toi = fromi + testrange/2;
    for ( ri = fromi - testrange/2 + 1; ri < toi; ri++ ) {
        here -= samples[ri - delta - 1];  // lose previous low cycnum
        here += samples[ri + delta];  // add new high cycnum

        val = abs(here);
        if ( val < low ) {
            low = val;
            crossi = ri;
        }
    }
    return crossi;
}

#ifdef oldcode
uint64_t
sussChanges::xfindCrossing(cBuff<short> &samples,
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
        testrange = spc/4 - 1;
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
#endif

void
sussChanges::firstTime( dataSamples *dsp )
{
    if ( dsp ) {
        if ( (int)dsp->avgSampSeq < (2*SamplesPerCycle) ) {
            throw sExc("not enough initial samples");
        }

        // find the first zero crossing

        lastCycAri = findCrossing( dsp->voltSamples(0),
                                   SamplesPerCycle,
                                   SamplesPerCycle/2 );

        // find the remaining zero crossings in the first set
        // of samples to establish the actual samples per cycle

        uint64_t nri, eri, cri, lri;
        int64_t sphc, sphcsum = 0;
        int cycnt = 0;
        // end raw sample to look at
        eri = dsp->avgSampSeq - SamplesPerCycle;

        // advance a half cycle at a time - initially use expected rate
        nri = lastCycAri + SamplesPerCycle/2;  // next raw index
        lri = lastCycAri;  // last zero crossing raw index 
        for ( ; nri < eri; ) {
            cri = findCrossing( dsp->voltSamples(0), nri );
            sphc = (signed)cri - (signed)lri;  // observed # of samples
            sphcsum += sphc;
            cycnt++;
            nri = cri + sphc;
            lri = cri;
        }
        SamplesPerCycle = (2*sphcsum)/cycnt;

        doCycles( dsp );
    } else {
        readCycles( 60, 10873 );
    }

    doChunks();
    if ( cHunki == 0 ) {
        throw sExc("firstTime could not do initial chunk");
    }

    // declare the initial value a stable run
    // startVali and start cHunki both zero
    startVali = 0;
    startRunWatts = realPwrChunksTotal(0);
    startRunVARs = reactiveChunksTotal(0);
    runWatts = startRunWatts;
    prevRunWatts = startRunWatts;
    prevRunVARs = startRunVARs;
    stableCnt = chunkRun;
    nextVali = chunkSize;
}

void
sussChanges::doChunks()
{
    uint64_t cyi;
    // cyi + chunkSize is one more than will be used, so the end test
    // is less than or equal
    for ( cyi = prevcHunki*chunkSize; (cyi + chunkSize) <= nCyci; cyi += chunkSize ) {
        doChunk( cyi );
    }
    prevcHunki = ++cHunki;  // really the next one to be done
}

void
sussChanges::doChunk( uint64_t cyi )
{
    uint64_t i;
    int64_t rpwrsum = 0;
    int64_t reacsum = 0;
    int leg, rpwrval, reacval;

    for ( leg = 0; leg < numLegs; leg++ ) {
        for ( i = cyi; i < cyi + chunkSize; i++ ) {
            rpwrval = realPowerLeg[leg][i];
            rpwrsum += rpwrval;
            reacval = reactivePowerLeg[leg][i];
            reacsum += reacval;
        }
        realPwrChunksLeg[leg].s( (int)(rpwrsum/chunkSize), cHunki );
        reactiveChunksLeg[leg].s( (int)(reacsum/chunkSize), cHunki );
    }
}

void
sussChanges::doChanges( dataSamples *dsp )
// runVal is either a) the current run value, or b) the potential new value
// while we are looking for another run when stableCnt is less than chunkRun
// runVal is updated with the current value regardless, letting it drift slowly
{
    int chnkwatts, chnkvars, cval, wattdiff;
    uint64_t trycy, chnki = nextVali/chunkSize;
    for( ; chnki < cHunki; chnki++ ) {
        chnkwatts = realPwrChunksTotal(chnki);
        chnkvars = reactiveChunksTotal(chnki);

        if ( stableCnt >= chunkRun ) {
            wattdiff = chnkwatts - runWatts;
        } else {
            wattdiff = chnkwatts - realPwrChunksTotal(chnki - 1);
        }

        if ( abs(wattdiff) >= chgDiff ) {
            if ( stableCnt >= chunkRun ) {
                // was stable, but now have exceeded diff, so
                // state goes to unstable until consecutive chunks
                // stop changing again.
                // the last stable chunk was the previous one (chi - 1)
                // and the cycles may have started moving inside it
                endCyci = chnki*chunkSize - chunkSize/2;  // halfway into previous chunk
                if ( endCyci <= startVali ) {
                    cout << "whoa - endCyci less than startVali: " << endCyci << startVali << endl;
                    endCyci = startVali + 1;
                }

                // go forward by cycle to find
                // specific cycle where change starts
                for ( trycy = endCyci + 1; trycy < (chnki+1)*chunkSize; trycy += 1 ) {
                    cval = realPowerTotal(trycy);
                    wattdiff = cval - runWatts;
                    if ( abs(wattdiff) >= chgDiff ) {
                        cval = realPowerTotal(trycy + 1);
                        wattdiff = cval - runWatts;
                        if ( abs(wattdiff) >= chgDiff ) {
                            break;
                        }
                    }
                    endCyci = trycy;
                }

                stableCnt = 0;
                drift = runWatts - startRunWatts;
                lastDurCycles = (int)(endCyci - startVali);

                // gone to not stable, set to last stable chunk val
                prevRunWatts = runWatts;
                prevRunVARs = runVARs;
            }
        } else {
            if ( stableCnt < chunkRun ) {
                // diff is less than threshold, start new run if in transition

                wattdiff = chnkwatts - prevRunWatts;

                if ( abs(wattdiff) < chgDiff*2 ) {
                    // just a jumpy chunk, so even though endCyci has
                    // been changed, no matter.  leave startVali, startRunWatts alone
                    // and do not write change event records
                    stableCnt = chunkRun;
                    runWatts = chnkwatts;
                    runVARs = chnkvars;
                    continue;
                }

                // this makes sure there are at least chunkRun chunks between transitions
                if ( ++stableCnt < chunkRun ) {  // was (chi*chunkSize - endCyci) < chunkSize
                    // leave unstable, might just be a blip
                    continue;  // does not update runVal
                }

//              stable = true;  
                if ( consOutp ) {
                    endrunout( *consOutp );  // uses prevRunVal
                }
                endrunout( cout );

                startVali = (chnki - chunkRun + 1)*chunkSize - 1;  // start of stable run chunk

                if ( startVali <= endCyci ) {
                    throw sExc( "what the - startVali less than endCyci??" );
                }
                // back up and identify where it really starts

                for ( trycy = startVali; trycy > endCyci; trycy-- ) {
                    cval = realPowerTotal(trycy);
                    wattdiff = cval - chnkwatts;
                    if ( abs(wattdiff) >= chgDiff ) {
                        // see if this is just a jumpy cycle
                        cval = realPowerTotal(trycy - 1);
                        wattdiff = cval - chnkwatts;
                        if ( abs(wattdiff) >= chgDiff ) {
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

                // set the new starting run watts and vars.  Use the first stable chunk values after the transition.
                startRunWatts = realPwrChunksTotal( chnki - chunkRun );
                startRunVARs = reactiveChunksTotal( chnki - chunkRun );
//              calcChangeArea();
                if ( consOutp ) {
                    startrunout( *consOutp );
                }
                startrunout( cout );

                writeChgOut();
            }
        }
        runWatts = chnkwatts;
        runVARs = chnkvars;
    }
    nextVali = chnki*chunkSize;
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
//      chgOutp = &cout;
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
    out << prevRunWatts << ",   " << ((60000*drift)/lastDurCycles)/1000.0;
    out << endl;
}

void
sussChanges::startrunout( ostream &out )
{
    // real power change, real power value, reactive power change, changeArea
    out << startRunWatts - prevRunWatts;
    out << ",  " << startRunWatts;
    out << ",  " << startRunVARs - prevRunVARs;
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
    // FIX - output legs
    out << "T,  " << nCyci - 1 << ",  " << realPwrChunksTotal(cHunki-1);  // chunks not so jumpy
    out << ",  " << reactiveChunksTotal(cHunki-1);
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

    uint64_t cyci, ecyci;
    int pwrval, reacval;
#ifdef noasold
    pwrval = 0; reacval = 0;
    for ( cyci = endCyci - 4; cyci < endCyci; cyci++ ) {
        pwrval += realPower[cyci];
        reacval += reactivePower[cyci];
    }
    pwrval /= 4;
    reacval /= 4;
#endif  // noasold
    chout << "E, " << endCyci << ", " << prevRunWatts << ", " << prevRunVARs << endl;
#ifdef notasold
    pwrval = 0; reacval = 0;
    for ( cyci = startVali + 1; cyci < startVali + 5; cyci++ ) {
        pwrval += realPower[cyci];
        reacval += reactivePower[cyci];
    }
    pwrval /= 4;
    reacval /= 4;
#endif
    chout << "S, " << startVali << ", " << startRunWatts << ", " << startRunVARs << endl;
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
        pwrval = realPowerTotal(cyci);
        reacval = reactivePowerTotal(cyci);
        chout << "C, " << cyci << ", " << pwrval << ", " << reacval << endl;
    }
}

#ifdef deadcode
void
sussChanges::calcChangeArea()
{
    uint64_t cyci;
    changeArea = 0;
    for ( cyci = endCyci + 1; cyci <= startVali; cyci++ ) {
        changeArea += (realPower[cyci - 1] + realPower[cyci] - 2*startRunWatts)/2;
    }
}
#endif // deadcode

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
#ifdef WIN32
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
#endif
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

    cBuff<int16_t> & ampsamps = dsp->ampSamples(0);  // FIX
    cBuff<int16_t> & voltsamps = dsp->voltSamples(0); // FIX
    for ( ; ri < endriv; ri++ ) {
        // write the averaged value twice for compatibility now that
        // raw samples might be oversampled.
        raw[0] = ampsamps[ri];
        raw[1] = voltsamps[ri];
        burstOutp->write( (char*)raw, 4 );
    }
}

int
sussChanges::realPowerTotal( uint64_t ix )
{
    int leg, total = 0;
    for ( leg = 0; leg < numLegs; leg++ ) {
        total += realPowerLeg[leg][ix];
    }
    return total;
}

int
sussChanges::reactivePowerTotal( uint64_t ix )
{
    int leg, total = 0;
    for ( leg = 0; leg < numLegs; leg++ ) {
        total += reactivePowerLeg[leg][ix];
    }
    return total;
}

int
sussChanges::realPwrChunksTotal( uint64_t ix )
{
    int leg, total = 0;
    for ( leg = 0; leg < numLegs; leg++ ) {
        total += realPwrChunksLeg[leg][ix];
    }
    return total;
}

int
sussChanges::reactiveChunksTotal( uint64_t ix )
{
    int leg, total = 0;
    for ( leg = 0; leg < numLegs; leg++ ) {
        total += reactiveChunksLeg[leg][ix];
    }
    return total;
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
    //  return;
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
