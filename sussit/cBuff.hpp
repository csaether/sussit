#pragma once

template<class T> 
class cBuff {
	int asize;
	unsigned __int64 imask;
	T *data;
public:
	cBuff() : asize(0), imask(0), data(0) {}
	~cBuff();

	void allocateBuffer( const int pwr2cnt );

	// store a value at sequence index
	void s( const T val, const unsigned __int64 seqi );

	// retrieve the value at sequence index
	T operator[](const unsigned __int64 seqi);
};
