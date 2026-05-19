
#include "reSid.h"

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

ReSid::ReSid( unsigned nr, System* system, SidManager& sidManager, USBSIDPico& usbSIDPico, Type type ) :
Sid( nr, system, sidManager, usbSIDPico, type ),
sysTimer(system->sysTimer),
filter( this ),
chamberlinFilter(filter) {

    lastBusValue = 0;

    ReSid::setType( type );

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

    sampleRate = 982800.0;
}

auto ReSid::enableFilter( bool state ) -> void {
    useFilter = state;
    filter.setEnable( state );
}

auto ReSid::adjustFilterCurve6581(int value) -> void {
    curve6581 = value;
    filter.adjustFilterBias6581( value );
}

auto ReSid::adjustFilterCurve8580(int value) -> void {
    curve8580 = value;
    filter.adjustFilterBias8580( value );
}

auto ReSid::adjustFilterRange6581(int value) -> void {
    range6581 = value; // unused, but remember when switching between SID engines
}

auto ReSid::setWaveformStrength(uint8_t value) -> void {
    waveStrength = value; // unused, but remember when switching between SID engines
}

auto ReSid::setType( Type type ) -> void {

    this->type = type;

    for( unsigned i = 0; i < 3; i++ ) {
        voice[i].setType( type );
        envelope[i].setType( type );
    }
    filter.setType( type );

    databusDecayTime = type == MOS_8580 ? 0xa2000 : 0x1d00;

    // update digi boost
    // it will be applied for 8580 only
    updateDigiBoost( digiBoost && type == Type::MOS_8580 );

    scaling = type == Type::MOS_8580 ? 5 : 3;
}

auto ReSid::setDigiBoost( bool state ) -> void {

    digiBoost = state;
    filter.digiBoost = state;

    if (type == Type::MOS_6581)
        return;

    updateDigiBoost( state );
}

auto ReSid::updateDigiBoost( bool state ) -> void {
    filter.setVoiceMask( state ? 0xf : 0x7 );
    filter.input( state ? -32768 : 0 );
}

auto ReSid::setSampleRate(double sampleRate) -> void {
    this->sampleRate = sampleRate;
    chamberlinFilter.setSVF(sampleRate);
}

auto ReSid::reset() -> void {

    for( unsigned i = 0; i < 3; i++ ) {

        envelope[i].reset();

        voice[i].reset();
    }
    filter.reset();
    chamberlinFilter.reset(sampleRate);
    externalFilter.reset();
    databusDecay = 0;
}

inline auto ReSid::processEnvVoice() -> void {
    int i;
    for (i = 0; i < 3; i++) {
        envelope[i].clock();
        voice[i].clock();
    }

    for (i = 0; i < 3; i++)
        voice[i].synchronize();

    for (i = 0; i < 3; i++)
        voice[i].setWaveformOutput();

    // bus values decay after a certain amount of time.
    // decay time differs between single bits.
    // single bit decaying is not emulated
    // but approximate time till all bits are decayed
    if (databusDecay && (--databusDecay == 0))
        lastBusValue = 0;
}

auto ReSid::clock() -> void {
    processEnvVoice();

    filter.clock(voice[0].output(), voice[1].output(), voice[2].output());

    externalFilter.clock(filter.output());
}

auto ReSid::clockSilent() -> void {
    processEnvVoice();
}

auto ReSid::clock(int cycles, int sampleCounter, int sampleLimit, bool audioOut) -> int {
    if (audioOut) {
        for (int c = 0; c < cycles; c++) {
            processEnvVoice();

#ifdef DEVELOP_USE_SEPARATE_INPUTS
                filter.clockSeparate(voice[0].output(), voice[1].output(), voice[2].output());

                externalFilter.clock( filter.outputSeparate() );
#else
                filter.clock(voice[0].output(), voice[1].output(), voice[2].output());

                externalFilter.clock( filter.output() );
#endif

                if (++sampleCounter == sampleLimit) {
                    system->audioRefresh( (int16_t)Emulator::sclamp( 16,(externalFilter.output() * scaling) / 2 ) );
                    sampleCounter = 0;
                }
            }

    } else {
        for (int c = 0; c < cycles; c++) {
            processEnvVoice();
        }
    }

    return sampleCounter;
}


}
