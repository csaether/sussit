// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#ifdef WIN32
#include "targetver.h"
#endif // WIN32
#include <stdio.h>
#include <stdint.h>
#ifdef WIN32
#include <tchar.h>
#endif // MSWIN
#include <iostream>
#include <string>
#include <stdlib.h>
#include <fstream>

using namespace std;

#include "cBuff.hpp"
#include "sExc.hpp"
#include "dataSamples.hpp"
#include "fileSource.hpp"

#ifdef WIN32
#include "windows.h"

#include "ps3000.h"
#include "PScope.h"
#include "picoSamples.hpp"
#include "picoScope.hpp"

#endif // WIN32

#ifndef WIN32
/* #include "adcSamples.hpp" */
#include "adcSource.hpp"
#endif  // not WIN32
#include "sussChanges.hpp"

string thetime();

extern ostream *Stabo;
extern ostream *Chgo;
extern ostream *Rawo;
