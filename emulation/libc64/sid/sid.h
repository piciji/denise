
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

#pragma once

#include <functional>
#include <cmath>
#include <string>
#include <cstdint>

#include "../../tools/dac.h"
#include "../../tools/splines.h"
#include "../../tools/clamp.h"
#include "../../tools/serializer.h"

namespace Emulator {
    struct SystemTimer;
}

namespace LIBC64 {

struct System;
struct SidManager;
struct USBSIDPico;

typedef double doublePoint[2];

struct Sid {
    enum Type { MOS_6581 = 0, MOS_8580 = 1 } type;
    enum FilterType { Resid = 0, ResidVice24 = 1, Chamberlin = 2 } filterType;
    Sid( unsigned nr, System* system, SidManager& sidManager, Type type);

    static std::vector<std::string> adrOptions;

    auto setType( Type type ) -> void;    
    auto setDigiBoost( bool state ) -> void;
    auto setFilterType( FilterType filterType ) -> void;
    auto updateDigiBoost( bool state ) -> void;
    auto readIO( uint8_t addr ) -> uint8_t;
    auto writeIO( uint8_t addr, uint8_t value ) -> void;
	auto writeIOFilter( uint8_t addr, uint8_t value ) -> void;
    auto reset() -> void;
    template<int options> auto clock(int cycles, int sampleCounter, int sampleLimit) -> int;
    template<int options> auto clock() -> void;
    auto serialize(Emulator::Serializer& s, bool light = false) -> void;
    auto setIoMask(uint8_t pos) -> void;
    auto useLeftChannel(bool state) -> void;
    auto useRightChannel(bool state) -> void;
    auto volumeCorrection(bool state) -> void;
	auto setSeparateFilterInputs(bool state) -> void;
	auto hasSeparateFilterInputs() -> bool { return separateFilterInputs; }

    unsigned nr;
    System* system;
    SidManager& sidManager;
    Emulator::SystemTimer& sysTimer;
    USBSIDPico& usbSIDPico;

    bool leftChannel = true;
    bool rightChannel = true;
    uint16_t ioMask;
    uint8_t ioPos;
    float correction = 1.0;

    double curSample;
    uint8_t lastBusValue;
    unsigned databusDecay;
    unsigned databusDecayTime;
    struct Envelope;
		
	int v1;
	int v2;
	int v3;

	bool separateFilterInputs = false;
    
    struct Voice {
        
        Voice( );        
        Type type;
        Envelope* envelope;
        uint32_t accumulator;
        uint16_t freq;
        uint16_t pw;
        uint16_t pulseOutput;
        uint16_t* wave;
		uint16_t waveTemp;
        uint8_t waveform;
		uint16_t waveformOutput;
		uint16_t osc3;
		bool test;
        bool msbRising;
		bool sync;
		uint32_t shiftRegister;
		uint16_t noNoise;
		uint16_t noiseOutput;
		uint16_t noNoiseOrNoiseOutput;
		uint16_t noPulse;
        int waveZero;

		Voice* syncSource;
		Voice* syncDest;
		uint32_t ringMsbMask;
        
        uint32_t aging;
        uint32_t shiftReset;
        uint8_t shiftPipeline;
		
        static auto generateWaveTable() -> void;
        static uint16_t waveTable[ 2 ][ 8 ][ 1 << 12 ];
		static Emulator::DAC<uint16_t> dac6581;
        static Emulator::DAC<uint16_t> dac8580;
		Emulator::DAC<uint16_t>* dac;
        
        auto setFrequencyLo( uint8_t freqLo ) -> void;
        auto setFrequencyHi( uint8_t freqHi ) -> void;
        auto setPwLo( uint8_t value ) -> void;
        auto setPwHi( uint8_t value ) -> void;
        auto setControl( uint8_t value ) -> void;
        auto setType( Type type, bool useDigitalFilter ) -> void;        
        auto clock() -> void;
		
		auto setNoiseOutput() -> void;
		auto writeShiftRegister() -> void;
        auto doPreWriteback( uint8_t waveformPrev ) -> bool;
		inline auto setWaveformOutput() -> void;
		auto setSyncSource( Voice* source ) -> void;
		inline auto synchronize() -> void;
        auto reset() -> void;		
        auto output() -> int;

    	inline auto noisePulse6581(uint16_t noise) -> uint16_t;
    	inline auto noisePulse8580(uint16_t noise) -> uint16_t;
        
    } voice[ 3 ];
    
    struct Envelope {

        enum State { S_ATTACK = 0, S_DECAY = 1, S_RELEASE = 2 } state;
        
        static uint16_t ratePeriodLookup[16];
        static Emulator::DAC<uint8_t> dac6581;
        static Emulator::DAC<uint8_t> dac8580;
		Emulator::DAC<uint8_t>* dac;
        Type type;
        
        auto callEnvelope() -> void;
        auto callExponentialCounter() -> void;
        
        uint8_t counter;
		uint8_t env3;
        bool lockEnvCounter;
        bool gateBefore;
        bool resetRateCounter;
        
        uint16_t ratePeriod;
        uint16_t rateCounter;
				
        uint8_t exponentialPeriod;
		uint8_t exponentialCounter;        
        
        uint8_t attack;
        uint8_t decay;
        uint8_t sustain;
        uint8_t release; 
        
        uint32_t delay;
		
        auto control( bool gate ) -> void;
        auto setAttackDecay( uint8_t value ) -> void;
        auto setSustainRelease( uint8_t value ) -> void;
        auto sustainComparator() -> uint8_t;
        auto updateExponentialPeriod() -> void;
		inline auto clock() -> void;
        
        auto reset() -> void;
        
        auto output() -> uint8_t;
        auto setType( Type type ) -> void;
        
    } envelope[ 3 ];
			
	struct Filter {

		Filter(Sid* sid);
		
        bool digiBoost = false;
        
        // Sid control
        Type type;  // model
        Sid* sid;
        bool enabled; // disable filter
        uint8_t voiceMask; // disable voices
        int bias6581;
        int bias8580;
        // Regsiter
		uint16_t fc; // cutoff frequency
		uint8_t res; // Resonanz
		uint8_t filt; // Filter or Mixer
		uint8_t mode; // disable Filter result mixing
		uint8_t vol; // volume
		
        // internal
        int _1024_div_Q;
        uint8_t sum;
        uint8_t mix;

		int ve;
		int v3;
		int v2;
		int v1;
		
		int Vhp; // highpass
		int Vbp; // bandpass
		int Vlp; // lowpass
		int Vbp_x, Vbp_vc;
		int Vlp_x, Vlp_vc;
        int Vddt_Vw_2, Vw_bias;
        int VbpRes;
        int w0;
        
        int kVgt; // 8580 only
        int n_dac; // 8580 only
        static int n_snake; // 6581 only
				
		static doublePoint opamp6581[];
		static doublePoint opamp8580[];
		
		struct Parameter {
			doublePoint* opampVoltage;	// voltage transfer function
			int opampVoltageSize;	// size of transfer samples
			double voiceVoltageRange;
			double voiceDCVoltage;
			double C;
			double Vdd;
			double Vth; // Threshold voltage
			double Ut; // Thermal voltage: Ut = k*T/q = 8.61734315e-5*T ~ 26mV
			double k; // Gate coupling coefficient: K = Cox/(Cox+Cdep) ~ 0.7
			double uCox; // u*Cox
			double WL_vcr; // W/L for VCR
			double WL_snake; // W/L for "snake"
			// DAC parameters.
			double dac_zero;
			double dac_scale;
			double dac_2R_div_R;
			bool dac_term;
		};
				
		static Parameter parameter[2];
		
		struct Opamp {
			unsigned short vx;
			short dvx;
		};		
		
		struct Calculated {
			int voiceScaleS14;
            int voiceScaleS14Old;
			int voiceDC;
            int voiceDCOld;
			int kVddt;
			double kVddtWithoutVmin;
			double vmin;
			int ak;
			int bk;
            double tmp_n_param;                                 
			double vo_N16;

			unsigned short gain[16][1 << 16];
			unsigned short summer[ (6 + 5 + 4 + 3 + 2) << 16 ];
			unsigned short mixer[ ( 7 + 6 + 5 + 4 + 3 + 2 + 1) << (16 + 1) ];
			unsigned short resonance[16][1 << 16];
            
            unsigned short f0_dac[1 << 11];
            unsigned short opamp_rev[1 << 16];

			Opamp opamp[1 << 16];
		};
		
		static Calculated calculated[2];

        // 6581 only
        static unsigned short vcr_kVg[1 << 16];
        static unsigned short vcr_n_Ids_term[1 << 16];
				
		static auto build(int m) -> void;
		static auto solveOpamp(Opamp* opamp, double n, int vi, int& x, Calculated& ca) -> int;
        auto solveIntegrate8580(int vi, int& vx, int& vc, Calculated& ca) -> int;
        auto solveIntegrate6581(int vi, int& vx, int& vc, Calculated& ca) -> int;
		
		auto writeFcLow( uint8_t data ) -> void;
		auto writeFcHi( uint8_t data ) -> void;
        auto updateFrequency() -> void;
		auto setEnable( bool state ) -> void;
		auto writeResFilt( uint8_t data ) -> void;
		auto writeModeVol( uint8_t data ) -> void;
		auto updateSumMix() -> void;
		auto setType( Type type ) -> void;
		auto clock(int voice1, int voice2, int voice3) -> void;
		auto output() -> short;
        auto setVoiceMask( uint8_t mask ) -> void;
		auto adjustFilterBias6581(int value) -> void;
        auto adjustFilterBias8580(int value) -> void;
        auto updateQ() -> void;
        auto input(short sample) -> void;
        auto reset() -> void;

		auto clock24(int voice1, int voice2, int voice3) -> void;
		auto output24() -> short;

		int* veP;
        int* v3P;
        int* v2P;
        int* v1P;
        int* VhpP;
        int* VbpP;
        int* VlpP;
        int* VbpResP;

		int nrXFilter;
		int nrXMixer;

		struct SeparateInput {
			int* vi;
			double n; // Verhältnis: R input / R output
		};

        std::vector<SeparateInput> separateMix;
        std::vector<SeparateInput> separateFlt;
        
		auto clockSeparate(int voice1, int voice2, int voice3) -> void;
		auto prepareSeparate() -> void;
		auto outputSeparate() -> short;
		auto solveOpampSeparate(Opamp* opamp, double a, double c, int& x, Calculated& ca) -> int;
	} filter;
    
    struct ChamberlinFilter {
        Filter& filter;
        
        static unsigned resolution;
        static double* sinTable;        
        
        double svfQ = 0;
        double svfF = 0;

        double lp = 0.0;
        double hp = 0.0;
        double bp = 0.0;
        double np = 0.0;
        double sampleRate;
                
        auto setSVF() -> void;
        
        auto process(double sample) -> void;
        
        auto clock(double voice1, double voice2, double voice3) -> double;
        
        static auto init() -> void;
        
        auto getSin(double a) -> double;
        
        auto updateFrequency(double sampleRate) -> void;
        
        auto reset() -> void;
        
        ChamberlinFilter(Filter& filter);
        
        ~ChamberlinFilter();
        
    } chamberlinFilter;
    
    struct ExternalFilter {
        ExternalFilter();
        
        int Vlp;
        int Vhp;

        int w0lp_1_s7;
        int w0hp_1_s17;
        
        auto clock( short Vi ) -> void;
        auto reset() -> void;
        auto output() -> int;
        
    } externalFilter;
};

}
