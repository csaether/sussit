#pragma once
#include <fstream>

class fileSource : public dataSamples
{
	ifstream infile;
public:
	fileSource() : dataSamples() {}
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
