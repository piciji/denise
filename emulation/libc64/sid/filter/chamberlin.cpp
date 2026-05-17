
/**
 * this code is a modification of chamberlin filter engine in Hoxs64
 */

#include "../reSid.h"

#ifndef M_PI 
#define M_PI    3.14159265358979323846f 
#endif

namespace LIBC64 {

double* ReSid::ChamberlinFilter::sinTable = nullptr;
unsigned ReSid::ChamberlinFilter::resolution = 131072L;
    
ReSid::ChamberlinFilter::ChamberlinFilter(ReSid::Filter& filter) : filter(filter) {

    static bool initialized = false;

    if (!initialized) {
        init();
        initialized = true;
    }
}

auto ReSid::ChamberlinFilter::reset(double sampleRate) -> void {

    lp = hp = bp = np = 0.0;
    
    setSVF(sampleRate);
}
    
auto ReSid::ChamberlinFilter::clock(double voice1, double voice2, double voice3) -> double {

    double prefilter;
    double mixer;
    double voice3Mixer = (filter.mode & 0x80) ? 0.0 : voice3;    
    
    if (filter.enabled) {
        switch (filter.filt & 7) {
            case 0:
                prefilter = 0;
                mixer = voice1 + voice2 + voice3Mixer;
                break;
            case 1:
                prefilter = voice1;
                mixer = voice2 + voice3Mixer;
                break;
            case 2:
                prefilter = voice2;
                mixer = voice1 + voice3Mixer;
                break;
            case 3:
                prefilter = voice1 + voice2;
                mixer = voice3Mixer;
                break;
            case 4:
                prefilter = voice3;
                mixer = voice1 + voice2;
                break;
            case 5:
                prefilter = voice1 + voice3;
                mixer = voice2;
                break;
            case 6:
                prefilter = voice2 + voice3;
                mixer = voice1;
                break;
            case 7:
                prefilter = voice1 + voice2 + voice3;
                mixer = 0;
                break;
            default:
                prefilter = 0.0;
                mixer = 0.0;
                break;
        }

        process( prefilter );    

        if (filter.mode & 0x10) // low pass
            mixer -= lp;    

        if (filter.mode & 0x20) // band pass
            mixer -= bp;    

        if (filter.mode & 0x40) // high pass
            mixer -= hp;    
        
    } else {
        
        mixer = voice1 + voice2 + voice3Mixer;
    }
    
    if (filter.digiBoost) {
        mixer += (double) (1730 * 3);

        mixer = 2.0 * ((mixer * (double)filter.vol) / (15.0));
    } else {
    
        mixer = 2.6 * ((double) (mixer * (double)filter.vol) / (15.0));
    }

    return mixer;
}
    
inline auto ReSid::ChamberlinFilter::process(double sample) -> void {
    static double min = 1.0e-300;

	np  = sample - svfQ * bp;
	if (std::fabs(np) < min)
		np = 0;	

	lp = lp + svfF * bp;
	if (std::fabs(lp) < min)	
		lp = 0;	

	hp = np - lp;
	if (std::fabs(hp) < min)	
		hp = 0;	

	bp = svfF * hp + bp;
	if (std::fabs(bp) < min)
		bp = 0;	

	//if (!std::isfinite(lp) | !std::isfinite(bp) | !std::isfinite(hp) | !std::isfinite(np))	
	//	lp = bp = hp = np = 0;	
}    
    
auto ReSid::ChamberlinFilter::setSVF( double sampleRate ) -> void {
    double a,b,f;
    
    double cutoff = (double)filter.fc * (5.8) + 30.0;

	svfF = 2 * getSin(M_PI * cutoff / sampleRate);
	f = 2 * getSin(M_PI * (cutoff+100) / sampleRate);
	a = 1 - (1 - .35) * (filter.res / 15.0);
    
	if (f >= M_PI/3) {
		b = 2.0/f - f*0.5;
		if (a > b)	
			a = b;		

		if (f >= 1) {
			b = 1 / f;
			if (a > b)			
				a = b;			
		}
		
		b = 2 - f;
		if (a > b)		
			a = b;		
		
		b = (- f + std::sqrt(f * f + 8)) / 2;
		if (a > b)		
			a = b;		
	}

	svfQ = a;
}

auto ReSid::ChamberlinFilter::init() -> void {

    double a;
    
    sinTable = new double[ resolution + 1 ];
    
    for (unsigned i = 0; i <= resolution; i++) {
        a = 2.0 * M_PI * (double) i / (double) resolution;
        sinTable[i] = std::sin( a );
    }
}

auto ReSid::ChamberlinFilter::getSin(double a) -> double {
    long i;

	if (std::fabs(a) >= (2.0 * M_PI)) 
		a = std::fmod(a, 2.0 * M_PI);	

	if (fabs(a)<0.0024)	
		return a;	

	if (a >= 0) {
		i = (long)((double)resolution * a / (2.0 * M_PI));
		return sinTable[i];
	}
    		
    i = (long)((double)resolution * -a / (2.0 * M_PI));
    return -sinTable[i];	
}

}
