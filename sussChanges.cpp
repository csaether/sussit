#include "stdafx.h"
#include "sussChanges.hpp"
#include <string.h>
#include <iostream>
#include <sstream>
#include <dirent.h>
#include <fnmatch.h>

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
        lastFileRollover(0),
        cyclesPerTimeStamp(60*15),  // or 15 seconds
        cyclesPerFileRollover(60*60*60*12),  // or every 12 hours
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
sussChanges::setBaseDirectory( const string & dirpath )
{
    baseDirname.assign( dirpath );
    if ( baseDirname[-1] != '/' ) {
        // add trailing slash
        baseDirname += "/";
    }
    // not checking to see whether it actually exists here
}

bool
sussChanges::setBaseFilename()
{
    struct dirent **nlist;
    int num;
    // versionsort specifies some sort of magic that orders by version found in entries
    num = ::scandir(baseDirname.c_str(), &nlist, 0, versionsort);
    if (num < 0) {
        cout << "specified directory not found" << endl;
        return false;
    }

    string basefname;
    int ver = 0;
    while (num--) {
        bool nm;
        // always have an events file, right?
        nm = fnmatch("*vizdata-*-e.csv", nlist[num]->d_name, 0);
        if ( nm == 0 && basefname.empty() ) {  // matches and have not set filename yet
            string fnam(nlist[num]->d_name);  // get into a string
            fnam.erase(fnam.size()-6);  // lose the known ending
            size_t vix = fnam.rfind('-');  // back up past what should be a version string
            if (vix == string::npos) {
                cout << "no vizdata* file: expecting to find a dash" << endl;
                return false;
            }
            // advance past the dash and should be some string left
            if (++vix == fnam.size()) {
                cout << "no vizdata* file: expecting to find a version" << endl;
                return false;
            }
            // get the version characters
            ver = atoi(fnam.substr(vix).c_str());
            if (ver == 0) {
                cout << "no vizdata* file: version should not be zero" << endl;
                return false;
            }
            // erase the existing version characters, and append new version
            fnam.erase(vix);
            ostringstream oss;
            oss << ver + 1;
            fnam.append(oss.str());
            // set the basefname and we will not do this branch again
            basefname = baseDirname + fnam;
        }
        free(nlist[num]);
    }
    free(nlist);
    baseFilename.assign( basefname );
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

    setNumLegs( dsp->NumLegs );

    if ( dsp->rawSamples() ) {

        dsp->startSampling();

        livedata = dsp->liveData();  // true if picosource or adc

        if ( livedata ) try {
            dsp->getSamples(0);
        } catch ( sExc &ex ) {
            cout << "initial getSamples exception " << ex.msg() << endl;
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
    sout << ",V3 starting at " << thetime() << endl;

    while ( !maxcycles || (nCyci < maxcycles) ) try {
        if ( dsp->rawSamples() ) {
            newvals = dsp->getSamples( 77 );
            //      cout << "got back " << newvals << endl;

            doCycles( dsp );

            writeRawOut( dsp );
        } else {
            readCycles( 60, dsp->WattFudgeDivor );
        }

        doChunks();

        // see whether to roll over files, but only when stable
        unsigned cd = 0;  // only looking at diffs, so does not have to be 64 bits
        if ( stableCnt >= chunkRun ) {
            cd = (unsigned)(nCyci - 1 - lastFileRollover);
            if ( cd > cyclesPerFileRollover ) {
                // bump version number on base filename. only fail if new file create fails.
                bool sts = setBaseFilename();
                if ( !sts ) {
                    sout << "failed to set new file name, quitting" << endl;
                    break;
                }
                // calling these w/o arg will close and reopen with current base filename.
                cout << "bumping base files to " << baseFilename << endl;
                setRawOut();
                setConsOut();
                setCycleOut();
                setBurstOut();
                setEventsOut();

                sout << ",SamplesPerCycle: " << SamplesPerCycle << endl;
                sout << ",Oversampling: " << AvgNSamples << endl;
                sout << ",chunkSize: " << chunkSize << endl;
                sout << ",chgDiff: " << chgDiff << endl;
                sout << ",V3 starting at " << thetime() << endl;

                lastFileRollover = nCyci - 1;
                // force a timestamp in the new file
                lastTimeStampCycle = 0;
            }
        }

        // see whether it is time to write a timestamp
        cd = (unsigned)(nCyci - 1 - lastTimeStampCycle);
        if ( cd > cyclesPerTimeStamp ) {
            timestampout( cout );
            timestampout( *eventsOutp );
            lastTimeStampCycle = nCyci -1;
            //writeCycleBurst( dsp, lastTimeStampCycle - 1 );
            //writeCycleBurst( dsp, lastTimeStampCycle );

            // confirm that we should keep going
            if ( livedata ) {
                ifstream controlfile;
                controlfile.open( "runsuss.txt", ios::in );
                if ( !controlfile ) {
                    break;  // exit quietly
                }
                controlfile.close();
            }
        }

        doChanges( dsp );
    } catch ( sExc &x ) {
        sout << ",caught sExc " << x.msg() << endl;
        sout << startRunWatts() << ",\t" << runWatts() << ",\t";
        sout << ",end second,\t" << nCyci/60.0 << endl;
        throw;
    } catch ( exception &x ) {
        sout << ",caught exception " << x.what() << endl;
        sout << startRunWatts() << ",\t" << runWatts() << ",\t";
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
sussChanges::readCycles( int numcycles, int wattfudgedivor[MaxLegs] )
{
    if ( cyclesIn.fail() ) {
        throw sExc( "no more file data" );
    }
    prevCyci = nCyci;
    int16_t val, realreac[2*MaxLegs];
    int count;
    unsigned leg;
    for ( count = 0; count < numcycles; count++, nCyci++ ) {
        cyclesIn.read( (char*)realreac, 4*numLegs );
        if ( cyclesIn.eof() ) {
            break;
        }
        for ( leg = 0; leg < numLegs; leg++ ) {
            val = realreac[2*leg];
            realPowerLeg[leg].s( val, nCyci );
            val = realreac[2*leg+1];
            reactivePowerLeg[leg].s( val, nCyci );
        }
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
    unsigned leg;
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
        // legs number of <real, reactive> int pairs unfudged
        uint64_t i;
        int16_t val, realreac[MaxLegs*2];
        for ( i = prevCyci; i < nCyci; i++ ) {
            for ( leg = 0; leg < numLegs; leg++ ) {
                val = realPowerLeg[leg][i];
                realreac[leg*2] = val;
                val = reactivePowerLeg[leg][i];
                realreac[leg*2+1] = val;
            }
            cycleOutp->write( (char*)realreac, 4*numLegs );
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
    cycval = cycsum/(sampcount*dsp->WattFudgeDivor[leg]);
    cval = (int)cycval - dsp->HumPower[leg];
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
    cycval = cycsum/(sampcount*dsp->WattFudgeDivor[leg]);
    cval = (int)cycval - dsp->HumReactive[leg];
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
    unsigned leg;
    if ( dsp->rawSamples() ) {
        if ( (int)dsp->avgSampSeq < (2*SamplesPerCycle) ) {
            throw sExc("not enough initial samples");
        }

        // find the first zero crossing

        lastCycAri = findCrossing( dsp->voltSamples(0),
                                   SamplesPerCycle,
                                   SamplesPerCycle/2 );

        // find the remaining zero crossings in the first set
        // of samples to establish the actual samples per cycle

        uint64_t ri, nri, eri, cri, lri, lfri;
        int64_t sphc, evensphcsum, oddsphcsum;
        int half, cycnt = 0;
        int64_t ssum[MaxLegs] = {0};
        // end raw sample, will go up to a cycle beyond this
        eri = dsp->avgSampSeq - 2*SamplesPerCycle;

        // advance a half cycle at a time - initially use expected rate
        evensphcsum = oddsphcsum = 0;
        nri = lastCycAri + SamplesPerCycle/2;  // next raw index
        lri = lastCycAri;  // last zero crossing raw index 
        for ( ; nri < eri; ) {
            lfri = lri;  // full cycle
            for ( half = 0; half < 2; half++ ) {
                cri = findCrossing( dsp->voltSamples(0), nri );
                sphc = (signed)cri - (signed)lri;  // observed # of samples
                if ( sphc <= 0 ) {
                    throw sExc("whoa - bad signal");
                }
                evensphcsum += sphc*((half+1) & 1);
                oddsphcsum += sphc*(half & 1);
                nri = cri + sphc;
                lri = cri;
            }

            for ( leg = 0; leg < numLegs; leg++ ) {
                cBuff<int16_t> &ampsamps = dsp->ampSamples(leg);
                for ( ri = lfri; ri < lri; ri++ ) {
                    ssum[leg] += ampsamps[ri];
                }
            }
            cycnt++;  // full cycles
        }
        SamplesPerCycle = (evensphcsum+oddsphcsum)/cycnt;
        cout << "over " << cycnt << " full cycles" << endl;
        cout << "samples per cycle: " << SamplesPerCycle << endl;
        cout << "even cycle count: " << evensphcsum/cycnt << endl;
        cout << "odd cycle count: " << oddsphcsum/cycnt << endl;
        for ( leg = 0; leg < numLegs; leg++ ) {
            cout << "leg " << leg << " amp avg: ";
            cout << ssum[leg]/(SamplesPerCycle*cycnt) << endl;
        }
        doCycles( dsp );
    } else {
        readCycles( 60, dsp->WattFudgeDivor );
    }

    doChunks();
    if ( cHunki == 0 ) {
        throw sExc("firstTime could not do initial chunk");
    }

    // declare the initial value a stable run
    // startVali and start cHunki both zero
    startVali = 0;
    for ( leg = 0; leg < numLegs; leg++ ) {
        startRunWattsLeg[leg] = realPwrChunksLeg[leg][0];
        startRunVARsLeg[leg] = reactiveChunksLeg[leg][0];
        runWattsLeg[leg] = startRunWattsLeg[leg];
        prevRunWattsLeg[leg] = startRunWattsLeg[leg];
        prevRunVARsLeg[leg] = startRunVARsLeg[leg];
    }
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
    prevcHunki = cHunki;  // really the next one to be done
}

void
sussChanges::doChunk( uint64_t cyi )
{
    uint64_t i;
    int64_t rpwrsum, reacsum;
    int rpwrval, reacval;
    unsigned leg;

    for ( leg = 0; leg < numLegs; leg++ ) {
        rpwrsum = 0;
        reacsum = 0;
        for ( i = cyi; i < cyi + chunkSize; i++ ) {
            rpwrval = realPowerLeg[leg][i];
            rpwrsum += rpwrval;
            reacval = reactivePowerLeg[leg][i];
            reacsum += reacval;
        }
        realPwrChunksLeg[leg].s( (int)(rpwrsum/chunkSize), cHunki );
        reactiveChunksLeg[leg].s( (int)(reacsum/chunkSize), cHunki );
    }
    cHunki += 1;
}

void
sussChanges::doChanges( dataSamples *dsp )
// runVal is either a) the current run value, or b) the potential new value
// while we are looking for another run when stableCnt is less than chunkRun
// runVal is updated with the current value regardless, letting it drift slowly
{
    /*
      General story here is we get called here from the main process loop when there
      is a new set of samples available.  Each cycle has been calculated, as well as
      groups of cycles, referred to as chunks.  Finding where a load change has occurred
      first involves looking at difference between the most recent "stable" chunk value
      and newer chunks to be considered.  If the difference exceeds a threshold, then
      we will look into the individual cycles for the exact cycle.  The reasoning was
      to avoid spurious changes because the cycle level changes were jumpy.  Not sure
      if that still makes sense, but that is what is happening now.
      If "stableCnt" is equal to or greater than "chunkRun" we are currently in a stable run.
      When we have seen a change at or beyond the change threshold, stableCnt will be set to
      zero, which means that we are looking for the end of the transition, which is when we
      see the start of a new stable run.  Of course we may not see the start of a new stable
      run within the set of cycles being considered, so stableCnt may be zero already when called.
     */
    int chnkwatts, chnkvars, cval, wattdiff, chnkvalsi;
    uint64_t trycy, chnki = nextVali/chunkSize;
    unsigned leg;
    for( ; chnki < cHunki; chnki++ ) {
        chnkwatts = realPwrChunksTotal(chnki);
        chnkvars = reactiveChunksTotal(chnki);
        chnkvalsi = chnki;

        if ( stableCnt >= chunkRun ) {
            wattdiff = chnkwatts - runWatts();
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
                    wattdiff = cval - runWatts();
                    if ( abs(wattdiff) >= chgDiff ) {
                        cval = realPowerTotal(trycy + 1);
                        wattdiff = cval - runWatts();
                        if ( abs(wattdiff) >= chgDiff ) {
                            break;
                        }
                    }
                    endCyci = trycy;
                }
                // note that we are in a transition
                stableCnt = 0;
                drift = runWatts() - startRunWatts();
                lastDurCycles = (int)(endCyci - startVali);

                // gone to not stable, set to last stable chunk val
                for ( leg = 0; leg < numLegs; leg++ ) {
                    prevRunWattsLeg[leg] = runWattsLeg[leg];
                    prevRunVARsLeg[leg] = runVARsLeg[leg];
                }
            }
        } else {
            if ( stableCnt < chunkRun ) {
                // diff is less than threshold, start new run if in transition

                wattdiff = chnkwatts - prevRunWatts();

                if ( abs(wattdiff) < chgDiff*2 ) {
                    // just a jumpy chunk, so even though endCyci has
                    // been changed, no matter.  leave startVali, startRunWatts alone
                    // and do not write change event records
                    stableCnt = chunkRun;
                    for ( leg = 0; leg < numLegs; leg++ ) {
                        runWattsLeg[leg] = realPwrChunksLeg[leg][chnkvalsi];
                        runVARsLeg[leg] = reactiveChunksLeg[leg][chnkvalsi];
                    }
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

                startVali = (chnki - chunkRun + 1)*chunkSize;  // start of stable run chunk

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

                // set the new starting run watts and vars.
                // Use the first stable chunk values after the transition.
                for ( leg = 0; leg < numLegs; leg++ ) {
                    startRunWattsLeg[leg]
                        = realPwrChunksLeg[leg][ chnki - chunkRun ];
                    startRunVARsLeg[leg]
                        = reactiveChunksLeg[leg][ chnki - chunkRun ];
                }
//              calcChangeArea();
                if ( consOutp ) {
                    startrunout( *consOutp );
                }
                startrunout( cout );

                writeChgOut();
            }
        }
        for ( leg = 0; leg < numLegs; leg++ ) {
            runWattsLeg[leg] = realPwrChunksLeg[leg][chnkvalsi];
            runVARsLeg[leg] = reactiveChunksLeg[leg][chnkvalsi];
        }
    }
    nextVali = chnki*chunkSize;
}

void
sussChanges::setConsOut( bool firsttime )
{
    // check if already open, and close if so.
    if ( consOut.is_open() ) {
        consOut.close();
        consOut.clear();  // not sure not needed, but
        consOutp = 0;
    } else if ( !firsttime ) {
        return;
    }

    if ( baseFilename.empty() ) {
        return;  // will have been null if did not really want this
    }

    string fname(baseFilename);
    fname += "-s.csv";
    consOut.open( fname.c_str() );
    if ( !consOut ) {
        throw sExc( "create consOut failed" );
    }
    consOutp = &consOut;
}
void
sussChanges::setEventsOut( bool firsttime )
{
    // check if already open, close if so.
    if ( eventsOut.is_open() ) {
        eventsOut.close();
        eventsOut.clear();  // see above
        eventsOutp = 0;
    } else if ( !firsttime ) {
        return;
    }

    if ( baseFilename.empty() ) {
//      chgOutp = &cout;
        return;
    }

    string fname(baseFilename);
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
    // blank, <watts, vars>..., driftrate
    unsigned leg;
    out << "E: " << prevRunWatts() << ", ";
    for ( leg = 0; leg < numLegs; leg++ ) {
        out << prevRunWattsLeg[leg] << ", " << prevRunVARsLeg[leg] << ", ";
    }
    out << ((60000*drift)/lastDurCycles)/1000.0 << endl;
}

void
sussChanges::startrunout( ostream &out )
{

    unsigned leg;
    out << "D: " << startRunWatts() - prevRunWatts();
    for ( leg = 0; leg < numLegs; leg++ ) {
        out << ",  " << (startRunWattsLeg[leg] - prevRunWattsLeg[leg]);
        out << ",  " << (startRunVARsLeg[leg] - prevRunVARsLeg[leg]);
    }
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
    unsigned leg;
    // chunks not so jumpy
    out << "T,  " << nCyci - 1;
    for ( leg = 0; leg < numLegs; leg++ ) {
        out << ", " << realPwrChunksLeg[leg][cHunki-1];
        out << ", " << reactiveChunksLeg[leg][cHunki - 1];
    }
    if ( livedata ) {
        out << ", " << thetime();
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
    unsigned leg;

    chout << "E, " << endCyci;
    for ( leg = 0; leg < numLegs; leg++ ) {
        chout << ", " << prevRunWattsLeg[leg] << ", " << prevRunVARsLeg[leg];
    }
    chout << endl;
    chout << "S, " << startVali;
    for ( leg = 0; leg < numLegs; leg++ ) {
        chout << ", " << startRunWattsLeg[leg] << ", " << startRunVARsLeg[leg];
    }
    chout << endl;

    cyci = endCyci;
    if ( cyci > preChgout ) {
        cyci -= preChgout;
    }

    ecyci = startVali + postChgout;
    for ( ; cyci <= ecyci; cyci++ ) {
        chout << "C, " << cyci;
        for ( leg = 0; leg < numLegs; leg++ ) {
            pwrval = realPowerLeg[leg][cyci];
            reacval = reactivePowerLeg[leg][cyci];
            chout << ", " << pwrval << ", " << reacval;
        }
        chout << endl;
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
sussChanges::setRawOut( bool firsttime )
{
    if ( rawOut.is_open() ) {
        rawOut.close();
        rawOut.clear();
        rawOutp = 0;
    } else if ( !firsttime ) {
        return;
    }

    string fname(baseFilename);
    fname += "-raw.dat";
    rawOut.open( fname.c_str(), ios::binary | ios::out  );
    if ( !rawOut ) {
        throw sExc( "create rawOut failed" );
    }
    rawOutp = &rawOut;
}

void
sussChanges::setCycleOut( bool firsttime )
{
    if ( cycleOut.is_open() ) {
        cycleOut.close();
        cycleOut.clear();
        cycleOutp = 0;
    } else if ( !firsttime ) {
        return;
    }

    string fname(baseFilename);
    fname += "-cyc.dat";
    cycleOut.open( fname.c_str(), ios::binary | ios::out  );
    if ( !cycleOut ) {
        throw sExc( "create cycleOut failed" );
    }
    cycleOutp = &cycleOut;
}

void
sussChanges::setBurstOut( bool firsttime )
{
    if ( burstOut.is_open() ) {
        burstOut.close();
        burstOut.clear();
        burstOutp = 0;
    } else if ( !firsttime ) {
        return;
    }

    string fname(baseFilename);
    fname += "-burst.dat";
    burstOut.open( fname.c_str(), ios::binary | ios::out );
    if ( !burstOut ) {
        throw sExc( "create burstOut failed" );
    }
    burstOutp = &burstOut;
}

void
sussChanges::writeRawOut( dataSamples *dsp )
{
    if ( !rawOutp || dsp->isAdc() ) {
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
    short raw[MaxChannels] = { 0x8ab0, 0x8ab0 };
    raw[0] |= dsp->NumChans;
    // write out marker, cycle number
    burstOutp->write( (char*)raw, 4 );
    burstOutp->write( (char*)&cyci, 8 );

    uint64_t ri, endriv;
    ri = cycRi[cyci];
    endriv = cycRi[cyci + 1];  // always have next cycle done

    unsigned chan, numchans = dsp->NumChans;
    for ( ; ri < endriv; ri++ ) {
        for ( chan = 0; chan < numchans; chan++ ) {
            raw[chan] = dsp->ChannelData[chan][ri];
        }
        burstOutp->write( (char*)raw, 2*numchans );
    }
}

int
sussChanges::realPowerTotal( uint64_t ix )
{
    int total = 0;
    for ( unsigned leg = 0; leg < numLegs; leg++ ) {
        total += realPowerLeg[leg][ix];
    }
    return total;
}

int
sussChanges::reactivePowerTotal( uint64_t ix )
{
    int total = 0;
    for ( unsigned leg = 0; leg < numLegs; leg++ ) {
        total += reactivePowerLeg[leg][ix];
    }
    return total;
}

int
sussChanges::realPwrChunksTotal( uint64_t ix )
{
    int total = 0;
    for ( unsigned leg = 0; leg < numLegs; leg++ ) {
        total += realPwrChunksLeg[leg][ix];
    }
    return total;
}

int
sussChanges::reactiveChunksTotal( uint64_t ix )
{
    int total = 0;
    for ( unsigned leg = 0; leg < numLegs; leg++ ) {
        total += reactiveChunksLeg[leg][ix];
    }
    return total;
}

int
sussChanges::startRunWatts()
{
    int val = 0;
    for ( unsigned leg = 0; leg < numLegs; leg++ ) {
        val += startRunWattsLeg[leg];
    }
    return val;
}

int
sussChanges::startRunVARs()
{
    int val = 0;
    for ( unsigned leg = 0; leg < numLegs; leg++ ) {
        val += startRunVARsLeg[leg];
    }
    return val;
}

int
sussChanges::runWatts()
{
    int val = 0;
    for ( unsigned leg = 0; leg < numLegs; leg++ ) {
        val += runWattsLeg[leg];
    }
    return val;
}

int
sussChanges::runVARs()
{
    int val = 0;
    for ( unsigned leg = 0; leg < numLegs; leg++ ) {
        val += runVARsLeg[leg];
    }
    return val;
}

int
sussChanges::prevRunWatts()
{
    int val = 0;
    for ( unsigned leg = 0; leg < numLegs; leg++ ) {
        val += prevRunWattsLeg[leg];
    }
    return val;
}

int
sussChanges::prevRunVARs()
{
    int val = 0;
    for ( unsigned leg = 0; leg < numLegs; leg++ ) {
        val += prevRunVARsLeg[leg];
    }
    return val;
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
