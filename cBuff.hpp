#pragma once

template<class T> 
class cBuff {
	int asize;
	uint64_t imask;
	T *data;
public:
	cBuff() : asize(0), imask(0), data(0) {}
	~cBuff();

	void allocateBuffer( const int pwr2cnt );

	// store a value at sequence index
	void s( const T val, const uint64_t seqi );

	// retrieve the value at sequence index
	T operator[](const uint64_t seqi);
};
