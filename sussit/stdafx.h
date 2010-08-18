// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <string>

using namespace std;

#include "windows.h"

#include "ps3000.h"
#include "PScope.h"

#include "cBuff.hpp"
#include "sExc.hpp"
#include "dataSamples.hpp"
#include "picoScope.hpp"
#include "fileSource.hpp"
#include "sussChanges.hpp"
#include "eventpub.hpp"

string thetime();

extern ostream *Stabo;
extern ostream *Chgo;
extern ostream *Rawo;
