
#include "sid.h"

#include "../../tools/macros.h"
#include "../system/system.h"
#include "register.cpp"
#include "envelope.cpp"
#include "voice.cpp"
#include "filter/main.cpp"
#include "filter/external.cpp"
#include "filter/resid24.cpp"
#include "serialization.cpp"
#include "../../tools/clamp.h"
#include "../../tools/systimer.h"

namespace LIBC64 {

std::vector<std::string> Sid::adrOptions = {"Default", "D400", "D420", "D440", "D460", "D480", "D4A0", "D4C0", "D4E0", "D500", "D520", "D540", "D560", "D580",
                                            "D5A0", "D5C0", "D5E0", "D600", "D620", "D640", "D660", "D680", "D6A0", "D6C0", "D6E0", "D700", "D720", "D740", "D760", "D780",
                                            "D7A0", "D7C0", "D7E0", "DE00", "DE20", "DE40", "DE60", "DE80", "DEA0", "DEC0", "DEE0", "DF00", "DF20", "DF40", "DF60", "DF80", "DFA0", "DFC0", "DFE0"};


Sid::Sid( unsigned nr, System* system, SidManager& sidManager, Type type ) :
nr(nr),
system(system),
usbSIDPico(sidManager.usbSIDPico),
sidManager(sidManager),
sysTimer(system->sysTimer),
filter( this ),
chamberlinFilter(filter) {

    lastBusValue = 0;
    separateFilterInputs = false;
	
    setType( type );
    
    filterType = FilterType::Resid;
	
	Envelope::dac6581.generate();	
	Envelope::dac8580.generate();

	Voice::dac6581.generate();	
	Voice::dac8580.generate();
	
	voice[0].setSyncSource( &voice[2] );
	voice[1].setSyncSource( &voice[0] );
	voice[2].setSyncSource( &voice[1] );
    
	for( unsigned i = 0; i < 3; i++ )     
        voice[i].envelope = &envelope[i];    
	
    ioMask = 0xD420;
    ioPos = 1;
}

auto Sid::useLeftChannel(bool state) -> void {
    leftChannel = state;
}

auto Sid::useRightChannel(bool state) -> void {
    rightChannel = state;
}

auto Sid::setIoMask( uint8_t pos ) -> void {

    if (pos >= adrOptions.size())
        return;

    ioPos = pos;

    if (pos == 0) {
        ioMask = 0;
        return;
    }

    auto str = adrOptions[pos];

    ioMask = std::stoul(str, nullptr, 16);
}

auto Sid::setFilterType( FilterType filterType ) -> void {
    
    this->filterType = filterType;

    for( unsigned i = 0; i < 3; i++ )
        voice[i].setType( this->type, this->filterType == FilterType::Chamberlin );
    
    //volumeCorrection();
}

auto Sid::volumeCorrection( bool state ) -> void {
    correction = 1.0;
    
    if (!state)
        return;
    
    switch (filterType) {
        case Sid::FilterType::Resid: {
            
            if (type == Type::MOS_8580)
                correction = 2.0;
            
        } break;
        case Sid::FilterType::Chamberlin: {
            correction = 0.7;
            
        } break;
        default:
            break;
    }
}

auto Sid::setType( Type type ) -> void {

    this->type = type;
    
    for( unsigned i = 0; i < 3; i++ ) {
        voice[i].setType( type, this->filterType == FilterType::Chamberlin );
        envelope[i].setType( type );
    }	
    filter.setType( type );

    databusDecayTime = type == MOS_8580 ? 0xa2000 : 0x1d00;

    // update digi boost
    // it will be applied for 8580 only    
    updateDigiBoost( filter.digiBoost && type == Type::MOS_8580 );
}

auto Sid::setDigiBoost( bool state ) -> void {
    
    filter.digiBoost = state;        

    if (type == Type::MOS_6581)
        return;
    
    updateDigiBoost( state );        
}

auto Sid::updateDigiBoost( bool state ) -> void {
    filter.setVoiceMask( state ? 0xf : 0x7 );
    filter.input( state ? -32768 : 0 );
}

auto Sid::setSeparateFilterInputs(bool state) -> void {
    separateFilterInputs = state;
    filter.setType( type );
}

auto Sid::reset() -> void {
    
    for( unsigned i = 0; i < 3; i++ ) {                                
        
        envelope[i].reset();
        
        voice[i].reset();
    }
    filter.reset();
    chamberlinFilter.reset();
    externalFilter.reset();
    databusDecay = 0;
}

template<int options> auto Sid::clock() -> void {
    constexpr bool audioOut = options & 1;
    constexpr bool useExtFilter = options & 2;
    constexpr bool useChamberlain = options & 4;
    constexpr bool needResult = options & 8;
    constexpr bool useResid24 = options & 16;

    int i;

    for (i = 0; i < 3; i++) {
        envelope[i].clock();
        voice[i].clock();
    }

    for (i = 0; i < 3; i++)
        voice[i].synchronize();

    for (i = 0; i < 3; i++)
        voice[i].setWaveformOutput();

    if constexpr (audioOut) {
        if constexpr (useExtFilter) {
            if constexpr (useChamberlain) {
                double _sample = chamberlinFilter.clock((double) voice[0].output() / 255.0,
                                                        (double) voice[1].output() / 255.0,
                                                        (double) voice[2].output() / 255.0);

                externalFilter.clock(Emulator::sclamp(16, _sample));

            } else if constexpr (useResid24) {
                filter.clock24(voice[0].output(), voice[1].output(), voice[2].output());

                externalFilter.clock(filter.output24());
            } else {
                filter.clock(voice[0].output(), voice[1].output(), voice[2].output());

                externalFilter.clock(filter.output());
            }

            if constexpr (needResult)
                curSample = (double) externalFilter.output() * correction;

        } else {
            if constexpr (useChamberlain) {
                curSample = chamberlinFilter.clock((double) voice[0].output() / 255.0,
                                                   (double) voice[1].output() / 255.0,
                                                   (double) voice[2].output() / 255.0);
            } else if constexpr (useResid24) {
                filter.clock24(voice[0].output(), voice[1].output(), voice[2].output());

            } else {
                filter.clock(voice[0].output(), voice[1].output(), voice[2].output());
            }

            if constexpr (needResult) {
                if constexpr (!(useChamberlain)) {
                    if constexpr(useResid24)
                        curSample = filter.output24();
                    else {
                        curSample = filter.output();
                    }

                }
                curSample *= correction;
            }
        }
    }

    if (databusDecay && (--databusDecay == 0))
        lastBusValue = 0;
}

template<int options> auto Sid::clock(int cycles, int sampleCounter, int sampleLimit) -> int {
    constexpr bool audioOut = options & 1;
    constexpr bool useExtFilter = options & 2;
    constexpr bool useChamberlain = options & 4;
    constexpr bool useResid24 = options & 16;
    constexpr bool useSeparateInputs = options & 32;

    int i, c;
    double curSample;

    for (c = 0; c < cycles; c++) {

        for (i = 0; i < 3; i++) {
            envelope[i].clock();
            voice[i].clock();
        }

        for (i = 0; i < 3; i++)
            voice[i].synchronize();

        for (i = 0; i < 3; i++)
            voice[i].setWaveformOutput();

        if constexpr (audioOut) {
            if constexpr (useExtFilter) {

                if constexpr (useChamberlain) {

                    curSample = chamberlinFilter.clock((double) voice[0].output() / 255.0,
                                                            (double) voice[1].output() / 255.0,
                                                            (double) voice[2].output() / 255.0);

                    externalFilter.clock(Emulator::sclamp(16, curSample));

                } else if constexpr (useResid24) {
                    filter.clock24(voice[0].output(), voice[1].output(), voice[2].output());

                    externalFilter.clock(filter.output24());

                } else {
                    if constexpr (useSeparateInputs) {
                        filter.clockSeparate(voice[0].output(), voice[1].output(), voice[2].output());

                        externalFilter.clock( filter.outputSeparate() );
                    } else {
                        filter.clock(voice[0].output(), voice[1].output(), voice[2].output());

                        externalFilter.clock( filter.output() );
                    }
                }

                if (++sampleCounter == sampleLimit) {
                    system->audioRefresh( Emulator::sclamp( 16, (float) externalFilter.output() * correction ) );
                    sampleCounter = 0;
                }

            } else {
                if constexpr (useChamberlain) {
                    curSample = chamberlinFilter.clock((double) voice[0].output() / 255.0,
                                                       (double) voice[1].output() / 255.0,
                                                       (double) voice[2].output() / 255.0);
                } else if constexpr (useResid24) {
                    filter.clock24(voice[0].output(), voice[1].output(), voice[2].output());

                } else if constexpr (useSeparateInputs) {
                    filter.clockSeparate(voice[0].output(), voice[1].output(), voice[2].output());

                } else {
                    filter.clock(voice[0].output(), voice[1].output(), voice[2].output());
                }

                if (++sampleCounter == sampleLimit) {
                    if constexpr (!(useChamberlain)) {
                        if constexpr(useResid24)
                            curSample = filter.output24();
                        else if constexpr (useSeparateInputs)
                            curSample = filter.outputSeparate();
                        else
                            curSample = filter.output();
                    }

                    system->audioRefresh( Emulator::sclamp( 16, curSample * correction ) );
                    sampleCounter = 0;
                }
            }
        }

        // bus values decay after a certain amount of time.
        // decay time differs between single bits.
        // single bit decaying is not emulated
        // but approximate time till all bits are decayed
        if (databusDecay && (--databusDecay == 0))
            lastBusValue = 0;
    }

    return sampleCounter;
}

template auto Sid::clock<0>() -> void;
template auto Sid::clock<1>() -> void;
template auto Sid::clock<2>() -> void;
template auto Sid::clock<3>() -> void;
template auto Sid::clock<4>() -> void;
template auto Sid::clock<5>() -> void;
template auto Sid::clock<6>() -> void;
template auto Sid::clock<7>() -> void;
template auto Sid::clock<8>() -> void;
template auto Sid::clock<9>() -> void;
template auto Sid::clock<10>() -> void;
template auto Sid::clock<11>() -> void;
template auto Sid::clock<12>() -> void;
template auto Sid::clock<13>() -> void;
template auto Sid::clock<14>() -> void;
template auto Sid::clock<15>() -> void;
template auto Sid::clock<16>() -> void;
template auto Sid::clock<17>() -> void;
template auto Sid::clock<18>() -> void;
template auto Sid::clock<19>() -> void;

template auto Sid::clock<24>() -> void;
template auto Sid::clock<25>() -> void;
template auto Sid::clock<26>() -> void;
template auto Sid::clock<27>() -> void;

template auto Sid::clock<0>(int cycles, int sampleCounter, int sampleLimit) -> int;
template auto Sid::clock<1>(int cycles, int sampleCounter, int sampleLimit) -> int;
template auto Sid::clock<2>(int cycles, int sampleCounter, int sampleLimit) -> int;
template auto Sid::clock<3>(int cycles, int sampleCounter, int sampleLimit) -> int;
template auto Sid::clock<4>(int cycles, int sampleCounter, int sampleLimit) -> int;
template auto Sid::clock<5>(int cycles, int sampleCounter, int sampleLimit) -> int;
template auto Sid::clock<6>(int cycles, int sampleCounter, int sampleLimit) -> int;
template auto Sid::clock<7>(int cycles, int sampleCounter, int sampleLimit) -> int;

template auto Sid::clock<16>(int cycles, int sampleCounter, int sampleLimit) -> int;
template auto Sid::clock<17>(int cycles, int sampleCounter, int sampleLimit) -> int;
template auto Sid::clock<18>(int cycles, int sampleCounter, int sampleLimit) -> int;
template auto Sid::clock<19>(int cycles, int sampleCounter, int sampleLimit) -> int;

// seperate inputs
template auto Sid::clock<32>(int cycles, int sampleCounter, int sampleLimit) -> int;
template auto Sid::clock<33>(int cycles, int sampleCounter, int sampleLimit) -> int;
template auto Sid::clock<34>(int cycles, int sampleCounter, int sampleLimit) -> int;
template auto Sid::clock<35>(int cycles, int sampleCounter, int sampleLimit) -> int;

}
