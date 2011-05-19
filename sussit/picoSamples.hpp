#pragma once

class picoSamples : public dataSamples {
public:
	cBuff<int16_t> amins;
	cBuff<int16_t> amaxs;
	cBuff<int16_t> bmins;
	cBuff<int16_t> bmaxs;

	picoSamples() : dataSamples(10873) {}  // 42343/2, 43230, 8640 (diff probe)

	int
		avgSamples( int64_t rsi, int64_t endseq );
	void
		setup();
	picoSamples*
		isPico() { return this; }
};