#pragma once

class sExc {
    string Msg;
public:
	sExc( const char *atext) : Msg(atext) { }

    std::string
    msg() { return Msg; }
};
