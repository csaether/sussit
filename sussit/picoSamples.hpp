#pragma once

class picoSamples : public dataSamples {
public:
	cBuff<int16_t> amins;
	cBuff<int16_t> amaxs;
	cBuff<int16_t> bmins;
	cBuff<int16_t> bmaxs;

	picoSamples() : dataSamples(43486) {}  // 42343/2, 43230, 8640 (diff probe)

	picoSamples*
		isPico() { return this; }
};