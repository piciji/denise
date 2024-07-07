
//  This code is a modification of the resid engine in VICE
//  You can get a copy of the original here: https://sourceforge.net/projects/vice-emu/

//  ---------------------------------------------------------------------------
//  This file is part of VICE, the Versatile Commodore Emulator.
//  This file is part of reSID, a MOS6581 SID emulator engine.
//  Copyright (C) 2010  Dag Lem <resid@nimrod.no>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  ---------------------------------------------------------------------------

#include "sid.h"

namespace LIBC64 {
    
uint16_t Sid::Voice::waveTable[ 2 ][ 8 ][ 1 << 12 ] = {
    { {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0} },
    { {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0} }
};

Emulator::DAC<uint16_t> Sid::Voice::dac6581( 12, 2.20, false );
Emulator::DAC<uint16_t> Sid::Voice::dac8580( 12, 2.00, true );
    
Sid::Voice::Voice( ) {
    
    static bool waveTableCreated = false;
    
    if (!waveTableCreated) {
        
        accumulator = 0;

        for (unsigned i = 0; i < (1 << 12); i++) {
            uint32_t msb = accumulator & 0x800000 ? ~0 : 0;

            waveTable[0][0][i] = waveTable[1][0][i] = 0xfff;
            waveTable[0][1][i] = waveTable[1][1][i] = (( accumulator ^ msb ) >> 11) & 0xffe;
            waveTable[0][2][i] = waveTable[1][2][i] = (accumulator >> 12) & 0xfff;
            waveTable[0][4][i] = waveTable[1][4][i] = 0xfff;

            accumulator += 0x1000;
        }
        syncSource = this;
		
		generateWaveTable();
        waveTableCreated = true; //one time for all instances
    }
    
    accumulator = 0x555555;
	waveTemp = 0x555;		    
}

auto Sid::Voice::generateWaveTable() -> void {        
    
    #include "tables.cpp"
	std::function<void ( std::vector<uint16_t>&, uint16_t* )> generate
    
    = []( std::vector<uint16_t>& source, uint16_t* target ) {

		for( unsigned i = 0; i < source.size(); i+=2 )
			target[ source[i] ] = source[i+1] << 4;
    };
	
    generate( wave6581_ST, waveTable[0][3] );
    generate( wave6581_PT, waveTable[0][5] );    
    generate( wave6581_PS, waveTable[0][6] );  
    generate( wave6581_PST, waveTable[0][7] );  
    
    generate( wave8580_ST, waveTable[1][3] );
    generate( wave8580_PT, waveTable[1][5] );    
    generate( wave8580_PS, waveTable[1][6] );  
    generate( wave8580_PST, waveTable[1][7] );  
}

auto Sid::Voice::setFrequencyLo( uint8_t value ) -> void {
    
    freq = (freq & 0xff00) | value;
}

auto Sid::Voice::setFrequencyHi( uint8_t value ) -> void {
    
    freq = (value << 8) | (freq & 0x00ff);
}

auto Sid::Voice::setPwLo( uint8_t value ) -> void {
    
    pw = (pw & 0xf00) | value;
    
    pulseOutput = (accumulator >> 12) >= pw ? 0xfff : 0x000;
}

auto Sid::Voice::setPwHi( uint8_t value ) -> void {
    
    pw = ((value << 8) & 0xf00) | (pw & 0x0ff);
    
    pulseOutput = (accumulator >> 12) >= pw ? 0xfff : 0x000;
}

auto Sid::Voice::setType( Type type, bool useDigitalFilter ) -> void {
    
    this->type = type;
    
    wave = waveTable[type][waveform & 0x7];
    
    waveZero = type == Type::MOS_6581 ? 0x380 : ( useDigitalFilter ? 0x9e0 : 0x9e0 );
	
	dac = type == Type::MOS_6581 ? &dac6581 : &dac8580;
}

auto Sid::Voice::setControl( uint8_t value ) -> void {
    
    bool testPrev = test;
    test = !!(value & 0x8);
    uint8_t waveformPrev = waveform;        
    
    waveform = (value >> 4) & 0x0f;
	sync = !!(value & 2);
	ringMsbMask = ((~value >> 5) & (value >> 2) & 0x1) << 23; //sawtooth = 0, ring mod = 1
	noNoise = waveform & 0x8 ? 0x000 : 0xfff;
	noNoiseOrNoiseOutput = noNoise | noiseOutput;
	noPulse = waveform & 0x4 ? 0x000 : 0xfff;
    
    wave = waveTable[type][waveform & 0x7];
    
    if ( !testPrev && test ) {
        accumulator = 0;
		shiftPipeline = 0;
        shiftReset = type == Type::MOS_6581 ? 35000 : 2519864;
    	pulseOutput = 0xfff;
        
    } else if ( testPrev && !test ) {        

        if ( doPreWriteback(waveformPrev) )
            writeShiftRegister();
        
        bool bit0 = (~shiftRegister >> 17) & 0x1;
        shiftRegister = ((shiftRegister << 1) | bit0) & 0x7fffff;

        setNoiseOutput();
    }
    
    if ( waveform )
		setWaveformOutput();
    else if (waveformPrev)
        aging = type == Type::MOS_6581 ? 182000 : 4400000;
}

inline auto Sid::Voice::clock() -> void {    
    
	if(unlikely(test)) {
        if (unlikely(shiftReset) && unlikely(!--shiftReset)) {
        	shiftRegister |= 1;
        	shiftRegister |= shiftRegister << 1;

            setNoiseOutput();

        	if (shiftRegister != 0x7fffff)
        		shiftReset = (type == Type::MOS_6581) ? 1000 : 315000;
        }
        
        pulseOutput = 0xfff;
		
    } else {		
		uint32_t accumulatorNext = (accumulator + freq) & 0xffffff;
		uint32_t risingBits = ~accumulator & accumulatorNext;
		accumulator = accumulatorNext;

		msbRising = !!(risingBits & 0x800000);

		if (unlikely(risingBits & 0x080000)) {// bit 19
			shiftPipeline = 2;
            
		} else if (unlikely(shiftPipeline) && !--shiftPipeline) {
            bool bit0 = ((shiftRegister >> 22) ^ (shiftRegister >> 17)) & 0x1;
            shiftRegister = ((shiftRegister << 1) | bit0) & 0x7fffff;

            setNoiseOutput();
        }        
	} 	
}

inline auto Sid::Voice::setWaveformOutput() -> void {
	
	if (likely( waveform )) {
		int ix = (accumulator ^ (~syncSource->accumulator & ringMsbMask)) >> 12;

		waveformOutput = wave[ix & 0xfff] & ( noPulse | pulseOutput ) & noNoiseOrNoiseOutput;

		if (unlikely((waveform & 0xc) == 0xc)) {
			waveformOutput = (type == Type::MOS_6581) ? noisePulse6581(waveformOutput) : noisePulse8580(waveformOutput);
		}

		if ((waveform & 3) && (type == Type::MOS_8580)) {
			osc3 = waveTemp & ( noPulse | pulseOutput ) & noNoiseOrNoiseOutput;
			waveTemp = wave[ix];
		} else
			osc3 = waveformOutput;

		if ((waveform & 0x2) && unlikely(waveform & 0xd) && (type == Type::MOS_6581)) {
			// In the 6581 the top bit of the accumulator may be driven low by combined waveforms
			// when the sawtooth is selected
			accumulator &= (waveformOutput << 12) | 0x7fffff;
		}

		if (unlikely(waveform > 0x8) && likely(!test) && likely(shiftPipeline != 1) )
			// Combined waveforms write to the shift register.
			writeShiftRegister();
        
	} else {        
        if (likely(aging) && unlikely(!--aging)) {
			waveformOutput &= waveformOutput >> 1;
			osc3 = waveformOutput;
			if (waveformOutput != 0)
				aging = type == Type::MOS_6581 ? 1500 : 50000;
		}           
    }		
	
	pulseOutput = -((accumulator >> 12) >= pw) & 0xfff;
}

inline auto Sid::Voice::setSyncSource( Voice* source ) -> void {
	syncSource = source;
	source->syncDest = this;
}

inline auto Sid::Voice::synchronize() -> void {
	if ( unlikely(msbRising) && syncDest->sync && !(sync && syncSource->msbRising))
		syncDest->accumulator = 0;
}

inline auto Sid::Voice::setNoiseOutput() -> void {
	noiseOutput =
    ((shiftRegister & 0x100000) >> 9) |
    ((shiftRegister & 0x040000) >> 8) |
    ((shiftRegister & 0x004000) >> 5) |
    ((shiftRegister & 0x000800) >> 3) |
    ((shiftRegister & 0x000200) >> 2) |
    ((shiftRegister & 0x000020) << 1) |
    ((shiftRegister & 0x000004) << 3) |
    ((shiftRegister & 0x000001) << 4);

	noNoiseOrNoiseOutput = noNoise | noiseOutput;
}

inline auto Sid::Voice::writeShiftRegister() -> void {

	shiftRegister &=
    ~((1<<20)|(1<<18)|(1<<14)|(1<<11)|(1<<9)|(1<<5)|(1<<2)|(1<<0)) |
    ((waveformOutput & 0x800) << 9) |  // Bit 11 -> bit 20
    ((waveformOutput & 0x400) << 8) |  // Bit 10 -> bit 18
    ((waveformOutput & 0x200) << 5) |  // Bit  9 -> bit 14
    ((waveformOutput & 0x100) << 3) |  // Bit  8 -> bit 11
    ((waveformOutput & 0x080) << 2) |  // Bit  7 -> bit  9
    ((waveformOutput & 0x040) >> 1) |  // Bit  6 -> bit  5
    ((waveformOutput & 0x020) >> 3) |  // Bit  5 -> bit  2
    ((waveformOutput & 0x010) >> 4);   // Bit  4 -> bit  0

	noiseOutput &= waveformOutput;
	noNoiseOrNoiseOutput = noNoise | noiseOutput;
}

inline auto Sid::Voice::doPreWriteback( uint8_t waveformPrev ) -> bool {
    // no writeback without combined waveforms
    if ( waveformPrev <= 0x8 )
        return false;
    // This need more investigation
    if (waveform == 8)
        return false;
    
    if (waveformPrev == 0xc)
        return false;
    
    // What's happening here?
    if (type == Type::MOS_6581 &&
            ((((waveformPrev & 0x3) == 0x1) && ((waveform & 0x3) == 0x2))
            || (((waveformPrev & 0x3) == 0x2) && ((waveform & 0x3) == 0x1))))
        return false;

    return true;
}

inline auto Sid::Voice::noisePulse6581(uint16_t noise) -> uint16_t {
	return (noise < 0xf00) ? 0x000 : noise & (noise << 1) & (noise << 2);
}

inline auto Sid::Voice::noisePulse8580(uint16_t noise) -> uint16_t {
	return (noise < 0xfc0) ? noise & (noise << 1) : 0xfc0;
}

auto Sid::Voice::reset() -> void {
    
    freq = 0;
    pw = 0;
    pulseOutput = 0xfff;
    noNoise = 0xfff;
    noPulse = 0xfff;
    wave = waveTable[type][0];
    waveform = 0;
    waveformOutput = 0;
    osc3 = 0;
    test = 0;
    msbRising = false;
    sync = 0;
    ringMsbMask = 0;
    shiftRegister = 0x7ffffe;
    aging = 0;
    shiftReset = 0;
    shiftPipeline = 0;
    setNoiseOutput();
}

inline auto Sid::Voice::output() -> int {
    
    return (dac->get( waveformOutput ) - waveZero) * envelope->output();
}

}
