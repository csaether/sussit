========================================================================
    CONSOLE APPLICATION : sussit Project Overview
========================================================================

AppWizard has created this sussit application for you.

This file contains a summary of what you will find in each of the files that
make up your sussit application.


sussit.vcproj
    This is the main project file for VC++ projects generated using an Application Wizard.
    It contains information about the version of Visual C++ that generated the file, and
    information about the platforms, configurations, and project features selected with the
    Application Wizard.

sussit.cpp
    This is the main application source file.

/////////////////////////////////////////////////////////////////////////////
Other standard files:

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named sussit.pch and a precompiled types file named StdAfx.obj.

/////////////////////////////////////////////////////////////////////////////
Other notes:

AppWizard uses "TODO:" comments to indicate parts of the source code you
should add to or customize.

/////////////////////////////////////////////////////////////////////////////

dev notes:

Use a stream socket intially to put events out to for rails
app.  Don't care if it blocks when procesing captured data,
and when live will be slow enough rate that blocking no prob.
