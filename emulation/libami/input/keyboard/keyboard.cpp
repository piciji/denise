
#include "keyboard.h"
#include "../../../tools/serializer.h"
#include "../../agnus/agnus.h"
#include "../../../cia/new/cia.h"

// keyboard computer is a standalone system with CPU (6502 like), RAM, ROM and timer logic. Currently, there is no Low Level Emulation (LLE) for this.
// this LLE is planned later, but always optional, as this costs a lot of additional performance without any real added value for the user.
namespace LIBAMI {

const uint8_t Keyboard::keymap[] = {
        0x20, 0x35, 0x33, 0x22, 0x12, 0x23, 0x24, 0x25, 0x17, 0x26, 0x27, 0x28, 0x37, 0x36, 0x18, 0x19, 0x10, 0x13, 0x21, 0x14, 0x16, 0x34, 0x11, 0x32, 0x15, 0x31,
        0x0a, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
        0x0f, 0x1d, 0x1e, 0x1f, 0x2d, 0x2e, 0x2f, 0x3d, 0x3e, 0x3f,
        0x5a, 0x5b, 0x5c, 0x5d, 0x4a, 0x5e, 0x43, 0x3c,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
        0x4c, 0x4d, 0x4f, 0x4e,
        0x40, 0x41, 0x42, 0x44, 0x45, 0x46, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x5f,
        0x1a, 0x1b, 0x29, 0x38, 0x39, 0x3a, 0x0d, 0x2a, 0x2b, 0x30, 0x00, 0x0b, 0x0c };

Keyboard::Keyboard(Emulator::Interface* interface, Agnus& agnus, Cia<MOS_8520>& cia) : agnus(agnus), cia(cia) {
    this->interface = interface;

    queue.resize(10);

    callback = [&](uint8_t job, uint16_t data) {

        switch (job) {
            case KBD_Selftest:
                resync();
                break;

            case KBD_Transfer:
                if (shiftPos == 0) // finish
                    // set SP line high (logical 0)
                    // there is no emulation of SP line without context of CNT, because it's sampled only when CNT is rising
                    // wait for handshake
                    agnus.updateEvent<Agnus::EVENT_KBD>(KBD_Wait_For_Timeout, agnus.msecToDMACycles(143));
                else
                    // put next bit onto SP line
                    agnus.updateEvent<Agnus::EVENT_KBD>(KBD_Transfer1, agnus.usecToDMACycles(20));
                break;

            case KBD_Transfer1:
                cia.setCNTAndSP(false);
                agnus.updateEvent<Agnus::EVENT_KBD>(KBD_Transfer2, agnus.usecToDMACycles(20));
                break;

            case KBD_Transfer2:
                // CIA process state of SP line, when CNT is rising and serial direction is input
                cia.setCNTAndSP(true, shiftOut & 0x80);
                shiftOut <<= 1;
                shiftPos--;
                agnus.updateEvent<Agnus::EVENT_KBD>(KBD_Transfer, agnus.usecToDMACycles(20));
                break;

            case KBD_Wait_For_Timeout:
                if (state != KBD_Selftest) {
                    // todo: check behaviour for double fault, assume re transmit of 0xf9 and faulted key stroke
                    if (state != KBD_Lost_Sync_Init && state != KBD_Lost_Sync_Transmit)
                        memState = state; // remember state to resume later on

                    state = KBD_Lost_Sync_Init;
                }
                resync();
                break;

            case KBD_Hardreset:
                if ( keyState[0x63] && keyState[0x66] && keyState[0x67] );
                    // at least one key needs to be released
                else {
                    agnus.pullResetLine(false);
                    reset();
                }
                break;
        }
    };

    agnus.addEvent<Agnus::EVENT_KBD>( &callback );
}

auto Keyboard::handshake(bool spLine) -> void {
    if (hardReset)
        return;
    // when CIA switches serial direction to output, SP line goes low and Keyboard recognizes this.
    // because SP will be set high after each transmission (byte or single resync bits)
    if (!spLine)
        handshakeClock = agnus.clock;
    else { // CIA switches back to input.
        if (agnus.fallBackCycles(handshakeClock) > agnus.usecToDMACycles(1)) { // recognized
            if (agnus.getActiveEvent<Agnus::EVENT_KBD>() == KBD_Wait_For_Timeout) {
                if (overflow) {
                    overflow = false;
                    sendCode( 0xfa );
                } else if (state == KBD_Lost_Sync_Init) {
                    state = KBD_Lost_Sync_Transmit;
                    sendCode( 0xf9 );
                } else if (state == KBD_Lost_Sync_Transmit) {
                    state = memState;
                    sendCode( curCode );
                } else if (state == KBD_Selftest) {
                    state = KBD_Initiate;
                    sendCode( 0xfd );
                } else if (state == KBD_Initiate) {
                    if (!queue.empty())
                        sendCode( queue.read() );
                    else {
                        state = KBD_Terminate;
                        sendCode( 0xfe );
                    }
                } else { // terminate or send
                    if (state == KBD_Terminate) {
                        interface->informCapsLock( false ); // LED control
                        state = KBD_Send;
                    }

                    if (!queue.empty())
                        sendCode( queue.read() );
                    else
                        agnus.setEventInactive<Agnus::EVENT_KBD>();
                }
            }
        }
    }
}

auto Keyboard::sendCode(uint8_t code) -> void {
    if (code != 0xf9)
        curCode = code;

    shiftPos = 8;
    // press/release Bit is sent last
    shiftOut = ~((code << 1) | (code >> 7)) & 0xff;
    agnus.updateEvent<Agnus::EVENT_KBD>(KBD_Transfer, agnus.usecToDMACycles(20));
}

auto Keyboard::resync() -> void {
    shiftPos = 1;
    shiftOut = 0;  // SP line to 0 (logical 1)
    agnus.updateEvent<Agnus::EVENT_KBD>(KBD_Transfer, agnus.usecToDMACycles(20));
}

auto Keyboard::reset() -> void {
    state = KBD_Selftest;
    overflow = false;
    capsLock = false;
    hardReset = false;
    shiftPos = 1;
    shiftOut = 0;
    std::memset(keyState, 0, sizeof(keyState));

    interface->informCapsLock( true );
    queue.reset();
    // bypass keyboard selftest
    agnus.updateEvent<Agnus::EVENT_KBD>(KBD_Selftest, agnus.msecToDMACycles(1000));
}

auto Keyboard::setDevice( Emulator::Interface::Device* device ) -> void {
    if (!device->isKeyboard())
        return;

    this->device = device;
}

auto Keyboard::sendKeyChange(bool pressed, Emulator::Interface::Device::Input* input) -> void {
    uint8_t stroke = keymap[input->id];

    if (pressed) {
        if (keyState[stroke])
            return;

        keyState[stroke] = 1;
    } else {
        if (!keyState[stroke])
            return;

        keyState[stroke] = 0;
        if (stroke == 0x62) // Caps Lock release
            return;

        stroke |= 0x80;
    }

    if ( keyState[0x63] && keyState[0x66] && keyState[0x67] ) { // independant from normal processing with overflow check
        if (!hardReset) {
            hardReset = true;
            agnus.updateEvent<Agnus::EVENT_KBD>(KBD_Hardreset, agnus.msecToDMACycles(500));
            agnus.pullResetLine();
        }
    } else if (hardReset && (agnus.getActiveEvent<Agnus::EVENT_KBD>() != KBD_Hardreset)) {// 500 ms minimum
        agnus.pullResetLine(false);
        reset();
    }

    if (hardReset)
        return;

    if (queue.full()) {
        overflow = true;
        return;
    }

    // even Caps Lock wont toggle LED, when queue is full ... need to check with my nose if Caps Locks toggles when Reset keys pressed same time.
    if (stroke == 0x62) {
        if (state != KBD_Send)
            return;

        capsLock ^= 1;
        interface->informCapsLock( capsLock );

        if (!capsLock)
            stroke |= 0x80;
    }

    if (agnus.getActiveEvent<Agnus::EVENT_KBD>())
        queue.write(stroke);
    else
        sendCode(stroke);
}

auto Keyboard::serialize( Emulator::Serializer& s ) -> void {
    s.integer(handshakeClock);
    s.integer(shiftOut);
    s.integer(shiftPos);
    s.integer(curCode);
    s.integer(overflow);
    s.integer(capsLock);
    s.integer(hardReset);
    s.integer( (uint8_t&)state);
    s.integer( (uint8_t&)memState);
    queue.serialize(s);
    // don't serialize keystate
}

}
