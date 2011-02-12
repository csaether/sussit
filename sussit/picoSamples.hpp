#pragma once

class picoSamples : public dataSamples {
public:
	cBuff<int16_t> amins;
	cBuff<int16_t> amaxs;
	cBuff<int16_t> bmins;
	cBuff<int16_t> bmaxs;

	picoSamples() : dataSamples() {}

	picoSamples*
		isPico() { return this; }
};