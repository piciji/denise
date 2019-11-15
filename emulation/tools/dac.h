
#pragma once

namespace Emulator {
    
/**
 * R2R resistor ladder network DAC converter
 * 
 * basic rules: R = U / I
 * 
 * series connection of resistors:
 *	R = R1 + R2
 *  U = U1 + U2
 *  I = I1 = I2
 * 
 * parallel connection of resistors
 *  R = R1 * R2 / (R1 + R2)  Note: formula is valid for two resitors in parallel only
 *  U = U1 = U2
 *  I = I1 + I2
 * 
 *         Vout                        VGND
 *          |                           |
 *          |                          2R termination ( missing in some circuits )
 *          |                           |
 *          --R---R---R---R---R---R---R--
 *          |   |   |   |   |   |   |   |
 *         2R  2R  2R  2R  2R  2R  2R  2R
 *          |   |   |   |   |   |   |   |
 *          7   6   5   4   3   2   1   0
 *         /|G /|G /|G /|G /|G /|G /|G /| Ground
 *        | - | - | - | - | - | - | - | -
 *  Vin-------------------------------- 
 */

template<typename T = uint16_t>    
struct DAC {
    
    bool terminated;            // vground terminated or not
    unsigned bits;              // resolution of the DAC
    double ratio_R2R = 2.0;     // ratio between R and 2R
    
    T* result = nullptr;
    
    DAC( unsigned bits, double ratio_R2R = 2.0, bool terminated = true ) {
                
        this->bits = bits;
        this->ratio_R2R = ratio_R2R;
        this->terminated = terminated;  		
    }
        
    ~DAC() {
		free();
    }
	// call free afterwards, if you change values
	auto free() -> void {
		if( result )
			delete[] result;
			
		result = nullptr;
	}
	
	auto get() -> T* {
		return result;
	}
	
	inline auto get( unsigned pos ) -> T { // no sanity checking
		return result[ pos ];
	}
    
    auto generate( ) -> T* {
        
        return generate( (T) ( (1 << bits) - 1 ), true );
    }
    // scaler could be the incomming voltage value of the circuit
    auto generate( T scaler, bool roundUp = false ) -> T* {

		free();
		result = new T[ 1 << bits ];
		
        const double Vi = 1.0; // normalized input voltage for variable scaling 
        const double R = 2.0; // init value doesn't matter because we use a ratio between R and 2R
        const double R2 = ratio_R2R * R; // means 2R (not a valid variable descriptor in C++)

        for (unsigned i = 0; i < (1 << bits); i++) {

            double Vo = 0.0; // output voltage
            double Ro = R2; // overall resistance
            double I = 0.0; // current

            for (unsigned bit = 0; bit < bits; bit++) {

                bool active = (i & (1 << bit)) > 0;

                // check if circuit has a termination resistor
                // calculates the final resistor value for parallel 2R and resistance so far
                // the overall resistance in the circuit is independant from the seted bits
                Ro = (bit == 0 && !terminated) ? R2 : R2 * Ro / (R2 + Ro);

                Vo = Ro * I;

                if (active)
                    Vo += Vi * Ro / R2; //new incomming voltage	will be added to voltage so far	

                Ro += R;
                I = Vo / Ro;
            }

            result[i] = (T) ( scaler * Vo + (roundUp ? 0.5 : 0) ); //scale up 
        }

        return result;
    }
    
};

}
