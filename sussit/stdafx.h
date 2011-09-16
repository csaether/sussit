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

using namespace std;

#ifdef WIN32
#include "windows.h"

#include "ps3000.h"
#include "PScope.h"
#endif // WIN32

#include "cBuff.hpp"
#include "sExc.hpp"
#include "dataSamples.hpp"
#include "picoSamples.hpp"
#include "picoScope.hpp"
#include "fileSource.hpp"
#include "sussChanges.hpp"
#include "eventpub.hpp"

string thetime();

extern ostream *Stabo;
extern ostream *Chgo;
extern ostream *Rawo;
