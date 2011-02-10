#include "stdafx.h"

template<class T> void
cBuff<T>::allocateBuffer(int pwr2count)
{
	asize = 1 << pwr2count;
	data = new T[asize];
	imask = asize - 1;
}

template<class T>
cBuff<T>::~cBuff()
{
	if ( data ) {
		delete []data;
		data = 0;
	}
}


template<class T> void
cBuff<T>::s( const T val, const uint64_t seqi )
{
	int i = (int)(seqi & imask);
	data[i] = val;
}

template<class T> T
cBuff<T>::operator[](const uint64_t seqi)
{
	int i = (int)(seqi & imask);
	return data[i];
}

// instantiate needed types

template class cBuff<short>;
template class cBuff<int>;
template class cBuff<uint64_t>;
