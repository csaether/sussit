// sussit.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#ifdef WIN32
int _tmain(int argc,
           _TCHAR* argv[])
#else
int main(int argc,
         char *argv[])
#endif
// <directory> [p | pr | pc | r | c] [minutes] [samplespercycle] [avgN]
/* directory - will add "vizdata-<version>-", always write event log
   p - picoscope or live source, only write event log
   pr - picoscope or live source, + write raw sample data
   pc - picoscope or live source, + write cycle data and sample bursts
   pb - + cycle burst
   r - use raw sample data source
   c - use cycle data source
   minutes - minute limit for picoscope data, defaults to forever if not present
*/
{
    sussChanges susser;
    dataSamples *dsp = 0;  // bug if not set somewhere later

    string argname, basefname;
    if ( argc > 1 ) {
        argname = argv[1];
    } else {
        cout << "Usage: <directory> [p | pr | pc | r | c] [minutes] [samplespercycle] [avgN]" << endl;
        return 1;
    }

    bool writesamples = false;
    bool writecycles = false;
    bool writebursts = false;
    bool samplesource = false;
    bool picosource = false;
    char ct;

    if ( argname.empty() ) {
        picosource = true;  // everything else false - no output files
    } else if ( argc > 2 ) {
        ct = argv[2][0];
        if ( ct == 'p' ) {
            picosource = true;  // this just means adc on electrum
            if ( argv[2][1] == 'r' ) {
                writesamples = true;
            } else if ( argv[2][1] == 'c' ) {
                writecycles = true;
                writebursts = true;
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

    // this is for picoscope and actual sample rate will be calculated
    // by looking at zero crossings on the first set of samples.  If
    // this initial guess is way off that will probably not work right.
    int spsec = SamplesPerCycle*60;
    int nsecsper = 1000000000/spsec;
    // needs to be multiple of 50 nanosecs - why the 200, instead of 100?
    nsecsper = (nsecsper/200)*200;  
    spsec = (1000000000/nsecsper);
    SamplesPerCycle = spsec/60;
    cout << SamplesPerCycle << " spc, " << nsecsper/2 << " nanosecs per sample, ";
    cout << AvgNSamples << " oversampling" << endl;

    fileSource fs;
    dataSamples ds;
#ifdef WIN32
    picoScope ps;
#else
    adcSource as;
#endif
    if ( !argname.empty() && picosource ) {
        // Looks like we have a valid directory, at least
        susser.setBaseDirectory( argname );
        if ( !susser.setBaseFilename() ) {
            return 1;
        }
        basefname = susser.getBaseFilename();
    } else {
        // kind of forgot what this branch is about..
        basefname = "../";
        basefname += argname;
    }

    if ( picosource ) {
#ifdef WIN32
        dsp = &ps;
#else
        dsp = &as;
#endif
        if ( writesamples ) {
            susser.setRawOut( true );
        }
    } else {
        try {
            // reading either raw sample or cycle data to process
            if ( argname.empty() ) {
                throw sExc("Need a filename, dude");
            }

            string fname(basefname);
            if ( samplesource ) {
                fname += "-raw.dat";
                fs.open( fname.c_str() );
                dsp = &fs;
            } else {
                // open stream for cycle data in
                fname += "-cyc.dat";
                susser.openCyclesIn( fname.c_str() );
                dsp = &ds;
            }

        } catch ( sExc &x ) {
            cout << x.msg() << endl;
            return 1;
        } catch ( exception &x ) {
            cout << x.what() << endl;
            return 1;
        }
    }

    try {
        // catch errors before forking
        dsp->setup();

        if ( !basefname.empty() ) {
            susser.setConsOut( true );
        }

        if ( writecycles ) {
            susser.setCycleOut( true );
        }

        if ( writebursts ) {
            susser.setBurstOut( true );
        }
        susser.setEventsOut( true );

    } catch ( sExc &x ) {
        cout << x.msg() << endl;
        return 2;
    } catch ( exception &x ) {
        cout << x.what() << endl;
        return 2;
    }

#ifdef forkit
    pid_t fpid;
    int exitsts;

    while ( true ) {
        fpid = fork();
        if ( fpid == 0 ) {
            // the child
            break;
        }
        if ( fpid < 0 ) {
            cout << strerror(errno) << endl;
        }

        fpid = wait( &exitsts );
        if ( fpid < 0 ) {
            cout << strerror(errno) << endl;
            return 3;
        }
        if ( WIFEXITED(exitsts) && WEXITSTATUS(exitsts) == 0 ) {
            cout << "normal exit" << endl;
            return 0;
        }
        // cannot refork until fixes to keep cyclenum advancing
        // probably need to open file in child and parse to get last one
        // probably need to open output files in child in order
        // to fork more than once - is it adc or regular files 
        // making trouble?

        return 4;
    }
#endif
    // the child, if forked - call the main method that will do the work
    // until something goes wrong or told to stop

    try {

        susser.processData( dsp, minutes*60*60 );
        
    } catch ( sExc &x ) {
        cout << x.msg() << endl;
        return 1;
    } catch ( exception &x ) {
        cout << x.what() << endl;
        return 2;
    }

    return 0;
}
