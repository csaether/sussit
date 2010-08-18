class pExc : public exception {
	string text;
public:
	pExc( const char *atext) { text.assign( atext ); }

	const char *  // do not understand this declaration...
		what() const throw() { return text.c_str(); }
};
class PScope {
	int scopeh;
	enPS3000Range rangeA;
	enPS3000Range rangeB;

public:
	PScope() : scopeh(0), rangeA(PS3000_500MV), rangeB(PS3000_2V) {};
	~PScope();

	void
		setSamplesPerCycle( int spc );
	void
		setRangeA( enPS3000Range r ) { rangeA = r; }
	void
		setRangeB( enPS3000Range r ) { rangeB = r; }

	void
		setup();
	void
		outfile( string fname );
	void
		filesamples( int millisecs );
	void
		dosums( int chgdiff, int ccount );
};