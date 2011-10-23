// sussit.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#ifdef WIN32
int _tmain(int argc,
		   _TCHAR* argv[])
#else
int main(int argc,
		 char *argv[])
#endif
// <filebasename> [p | pr | pc | r | c] [minutes] [samplespercycle] [avgN]
/* filebasename - base name, always write event log
   p - picoscope source, only write event log
   pr - picoscope source, write raw sample data
   pc - picoscope source, write cycle data
   pb - cycle burst
   r - use raw sample data source
   c - use cycle data source
   minutes - minute limit for picoscope data, defaults to 30 for scope if not present
*/
{
    sussChanges susser;
    dataSamples *dsp = 0;

    string basename, basefname;
    if ( argc > 1 ) {
        basename = argv[1];
    } else {
		cout << "Usage: <filebasename> [p | pr | pc | r | c] [minutes] [samplespercycle] [avgN]" << endl;
		return 1;
	}

    bool writesamples = false;
    bool writecycles = false;
    bool writebursts = false;
    bool samplesource = false;
    bool picosource = false;
    char ct;
    if ( basename.empty() ) {
        picosource = true;  // everything else false - no output files
    } else if ( argc > 2 ) {
        ct = argv[2][0];
        if ( ct == 'p' ) {
            picosource = true;
            if ( argv[2][1] == 'r' ) {
                writesamples = true;
            } else if ( argv[2][1] == 'c' ) {
                writecycles = true;
            } else if ( argv[2][1] == 'b' ) {
                writebursts = true;
            }
        } else if ( ct == 'r' ) {
            samplesource = true;
            writecycles = true;
        }
    }

    int minutes = 0;
    if ( argc > 3 ) {
        minutes = atoi( argv[3] );
    } else if ( picosource ) {
        minutes = 30;
    }
    if ( argc > 4 ) {
        SamplesPerCycle = atoi( argv[4] );
    }
	if ( argc > 5 ) {
		AvgNSamples = atoi( argv[5] );
		if ( AvgNSamples == 0 ) {
			AvgNSamples = 1;
		}
	}

    int spsec = SamplesPerCycle*60;
    int nsecsper = 1000000000/spsec;
	// needs to be multiple of 50 nanosecs - why the 200, instead of 100?
    nsecsper = (nsecsper/200)*200;  
    spsec = (1000000000/nsecsper);
    SamplesPerCycle = spsec/60;
    cout << SamplesPerCycle << " spc, " << nsecsper/2 << " nanosecs per sample, ";
	cout << AvgNSamples << " oversampling" << endl;

#ifdef WIN32
    fileSource fs;
    picoScope ps;
#else
    adcSource as;
#endif
    if ( !basename.empty() && picosource ) {
        basefname = basename;
    } else {
        basefname = "../";
        basefname += basename;
    }

    if ( picosource ) {
#ifdef WIN32
        dsp = &ps;
#else
        dsp = &as;
#endif
        if ( writesamples ) {
            susser.setRawOut( basefname.c_str() );
        }
    } else try {
            // reading either raw sample or cycle data to process
            if ( basename.empty() ) {
                throw sExc("Need a filename, dude");
            }
#ifdef WIN32
            string fname = "../../scopedata/";
            fname += basename;
            if ( samplesource ) {
                fname += "-raw.dat";
                fs.open( fname.c_str() );
                dsp = &fs;
            } else {
                // open stream for cycle data in
                fname += "-cyc.dat";
                susser.openCyclesIn( fname.c_str() );
            }
#endif // WIN32, no raw data file input on debian
            // null dsp if cycle samples
      } catch ( exception &x ) {
            cout << x.what() << endl;
            return 1;
      }

    try {
        if ( !basefname.empty() ) {
            susser.setConsOut( basefname.c_str() );
        }

        if ( writecycles ) {
            susser.setCycleOut( basefname.c_str() );
        }

        if ( writebursts ) {
            susser.setBurstOut( basefname.c_str() );
        }
        susser.setEventsOut( basefname.c_str() );

        susser.processData( dsp, minutes*60*60 );

    } catch ( exception &x ) {
        cout << x.what() << endl;
    }

    return 0;
}

