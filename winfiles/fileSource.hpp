#pragma once
#include <fstream>

class fileSource : public picoSamples {
	ifstream infile;
public:
	fileSource() : picoSamples() {}
	~fileSource(void);

	void
		open( const char *fname );
	void
		setup();
	void
		startSampling() {}
	int
		getSamples( int milliSleep );
};
