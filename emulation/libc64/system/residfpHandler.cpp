
#include "residfpHandler.h"
#include "../../../deps/libresidfp/src/SID.h"
#include "../system/system.h"

namespace LIBC64 {

ResidfpHandler::ResidfpHandler(unsigned nr, System* system, SidManager& sidManager, USBSIDPico& usbSIDPico, Type type) :
Sid( nr, system, sidManager, usbSIDPico, type ),

sysTimer(system->sysTimer),
residfp(*(new reSIDfp::SID) ) {
    sampleRate = 982800.0; // for 50 FPS
    ioMask = 0xD420;
    ioPos = 1;
    residfp.setSamplingParameters( sampleRate, reSIDfp::SamplingMethod::NONE, sampleRate );
    buffer = new short[SidManager::UpdateCycles << 1];
    stateBuffer = new uint8_t[sizeof(reSIDfp::State)];
}

ResidfpHandler::~ResidfpHandler() {
    delete &residfp;
    delete[] buffer;
    delete[] stateBuffer;
}

auto ResidfpHandler::reset() -> void {
    residfp.reset();
}

auto ResidfpHandler::setType( Type type ) -> void {
    this->type = type;

    if (type == Type::MOS_6581)
        residfp.setChipModel( reSIDfp::ChipModel::MOS6581 );
    else
        residfp.setChipModel( reSIDfp::ChipModel::CSG8580 );
}

auto ResidfpHandler::setDigiBoost( bool state ) -> void {
    digiBoost = state;
    residfp.input( state ? -32768 : 0 );
}

auto ResidfpHandler::readIO( uint8_t addr ) -> uint8_t {
    addr &= 0x1f;

    switch( addr ) {
        case 0x19:
        case 0x1a:
            if (!sysTimer.has( &sidManager.callPotUpdate )) {
                sidManager.potX = sidManager.getPotX();
                sidManager.potY = sidManager.getPotY();
                sysTimer.add( &sidManager.callPotUpdate, 512, Emulator::SystemTimer::Action::WhenNotExistsOnly );
            }
            return addr == 0x19 ? sidManager.potX : sidManager.potY;

        default:
            break;
    }

    return residfp.read(addr);
}

auto ResidfpHandler::peekIO( uint8_t addr ) -> uint8_t {
    return 0xff;
}

auto ResidfpHandler::writeIO( uint8_t addr, uint8_t value ) -> void {
    addr &= 0x1f;
    //if (system->debugSID())
    rememberParams(addr, value);

    if (usbSIDPico.enabled && system->displayFrame())
        usbSIDPico.store(addr, value, nr);

    residfp.write( addr, value );
}

auto ResidfpHandler::clock(int cycles, int sampleCounter, int sampleLimit, bool audioOut) -> int {
    if (audioOut) {
        int samples = residfp.clock( cycles, buffer );

        if (samples != cycles)
            fprintf( stderr, "residfp cycles <> samples " );

        for (int c = 0; c < cycles; c++) {

            if (++sampleCounter == sampleLimit) {
                system->audioRefresh( buffer[c] );
                sampleCounter = 0;
            }
        }
    } else
        residfp.clockDigital( cycles );

    return sampleCounter;
}

auto ResidfpHandler::clock() -> void {
    residfp.clock( 1, buffer );
}

auto ResidfpHandler::clockSilent() -> void {
    residfp.clockDigital( 1 );
}

auto ResidfpHandler::getSample() -> float {
    return buffer[0];
}

auto ResidfpHandler::enableFilter( bool state ) -> void {
    useFilter = state;
    residfp.enableFilter( state );
}

auto ResidfpHandler::adjustFilterCurve6581(int value) -> void {
    curve6581 = value;
    residfp.setFilter6581Curve( (double)value / 10000.0 );
}

auto ResidfpHandler::adjustFilterCurve8580(int value) -> void {
    curve8580 = value;
    residfp.setFilter8580Curve( (double)value / 10000.0 );
}

auto ResidfpHandler::adjustFilterRange6581(int value) -> void {
    range6581 = value;
    residfp.setFilter6581Range( (double)value / 10000.0 );
}

auto ResidfpHandler::setWaveformStrength(uint8_t value) -> void {
    waveStrength = value;
    residfp.setCombinedWaveforms( static_cast<reSIDfp::CombinedWaveforms>(waveStrength) );
}

auto ResidfpHandler::setSampleRate(double sampleRate) -> void {
    this->sampleRate = sampleRate;
    residfp.setSamplingParameters( sampleRate, reSIDfp::SamplingMethod::NONE, sampleRate );
}

auto ResidfpHandler::serialize(Emulator::Serializer& s, bool light) -> void {
    s.integer( leftChannel );
    s.integer( rightChannel );
    s.integer( ioMask );
    s.integer( ioPos );

    s.integer( curve6581 );
    s.integer( curve8580 );
    s.integer( range6581 );
    s.integer( waveStrength );
    s.integer( digiBoost );
    s.integer( useFilter );

    s.integer( (uint8_t&) type );

    int reserve = reSIDfp::State::size(residfp);
    uint8_t* ptr = s.bufferPtr( reserve );

    if (s.mode() == Emulator::Serializer::Mode::Save)
        reSIDfp::State::saveState(residfp, reinterpret_cast<char*>(ptr), reserve);

    if(s.mode() == Emulator::Serializer::Mode::Load)
        reSIDfp::State::restoreState(residfp, reinterpret_cast<char*>(ptr), reserve);
}

auto ResidfpHandler::clone(Sid* src, bool keepProps) -> void {
    if (!keepProps) {
        leftChannel = src->leftChannel;
        rightChannel = src->rightChannel;
        ioPos = src->ioPos;
        ioMask = src->ioMask;
    }

    // these props are same for all SID's
    curve6581 = src->curve6581;
    curve8580 = src->curve8580;
    range6581 = src->range6581;
    waveStrength = src->waveStrength;
    digiBoost = src->digiBoost;
    useFilter = src->useFilter;
    sampleRate = src->sampleRate;

    setType( type );
    enableFilter( useFilter );
    adjustFilterCurve6581( curve6581 );
    adjustFilterCurve8580( curve8580 );
    setSampleRate( sampleRate );
    setDigiBoost( digiBoost );
    reset();
}

auto ResidfpHandler::updateSnapshot(DebuggerSnapshot& snap) -> void {
    auto& s = snap.sids[nr];
    s = snapshot;
    s.active = true;
}

auto ResidfpHandler::rememberParams(uint8_t addr, uint8_t value) -> void {
    switch( addr ) {
        case 0x0: snapshot.voices[0].frequency = (snapshot.voices[0].frequency & 0xff00) | value; break;
        case 0x1: snapshot.voices[0].frequency = (value << 8) | (snapshot.voices[0].frequency & 0x00ff); break;
        case 0x2: snapshot.voices[0].pulseWidth = (snapshot.voices[0].pulseWidth & 0xf00) | value; break;
        case 0x3: snapshot.voices[0].pulseWidth = ((value << 8) & 0xf00) | (snapshot.voices[0].pulseWidth & 0x0ff); break;
        case 0x4: snapshot.voices[0].wave = (value >> 4) & 0xf; snapshot.voices[0].control = value; break;
        case 0x5: snapshot.voices[0].attack = (value >> 4) & 0xf; snapshot.voices[0].decay = value & 0xf; break;
        case 0x6: snapshot.voices[0].sustain = (value >> 4) & 0xf; snapshot.voices[0].release = value & 0xf; break;

        case 0x7: snapshot.voices[1].frequency = (snapshot.voices[1].frequency & 0xff00) | value; break;
        case 0x8: snapshot.voices[1].frequency = (value << 8) | (snapshot.voices[1].frequency & 0x00ff); break;
        case 0x9: snapshot.voices[1].pulseWidth = (snapshot.voices[1].pulseWidth & 0xf00) | value; break;
        case 0xa: snapshot.voices[1].pulseWidth = ((value << 8) & 0xf00) | (snapshot.voices[1].pulseWidth & 0x0ff); break;
        case 0xb: snapshot.voices[1].wave = (value >> 4) & 0xf; snapshot.voices[1].control = value; break;
        case 0xc: snapshot.voices[1].attack = (value >> 4) & 0xf; snapshot.voices[1].decay = value & 0xf; break;
        case 0xd: snapshot.voices[1].sustain = (value >> 4) & 0xf; snapshot.voices[1].release = value & 0xf; break;

        case 0xe: snapshot.voices[2].frequency = (snapshot.voices[2].frequency & 0xff00) | value; break;
        case 0xf: snapshot.voices[2].frequency = (value << 8) | (snapshot.voices[2].frequency & 0x00ff); break;
        case 0x10: snapshot.voices[2].pulseWidth = (snapshot.voices[2].pulseWidth & 0xf00) | value; break;
        case 0x11: snapshot.voices[2].pulseWidth = ((value << 8) & 0xf00) | (snapshot.voices[2].pulseWidth & 0x0ff); break;
        case 0x12: snapshot.voices[2].wave = (value >> 4) & 0xf; snapshot.voices[2].control = value; break;
        case 0x13: snapshot.voices[2].attack = (value >> 4) & 0xf; snapshot.voices[2].decay = value & 0xf; break;
        case 0x14: snapshot.voices[2].sustain = (value >> 4) & 0xf; snapshot.voices[2].release = value & 0xf; break;

        case 0x15: snapshot.filter.cutOff = (snapshot.filter.cutOff & 0x7f8) | (value & 7); break;
        case 0x16: snapshot.filter.cutOff = ((value << 3) & 0x7f8) | (snapshot.filter.cutOff & 7); break;
        case 0x17: snapshot.filter.resonance = ( value >> 4 ) & 0xf;  snapshot.filter.voices = value & 0xf; break;
        case 0x18: snapshot.filter.mode = (value >> 4) & 0xf; snapshot.volume = value & 0xf; break;
        default:
            break;
    }
}

}
