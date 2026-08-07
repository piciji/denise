
#include "expert.h"

namespace LIBC64 {

auto Expert::assign( Cart* cart ) -> void {
    // don't rebuild
}

auto Expert::create( Interface::CartridgeId cartridgeId, unsigned _size ) -> Cart* {
    // don't rebuild
    return this;
}

auto Expert::setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void {
    if ( (this->rom == nullptr) && (rom == nullptr) )
        return;

    build( Interface::CartridgeIdExpert, rom, romSize );

    this->media = media;
    prepare();
}

auto Expert::readChips() -> bool {
    bool found = Cart::readChips();

    imageValid = found && !binFormat && chips.size() == 1
        && chips[0].size == RamSize && chips[0].addr == 0x8000
        && chips[0].offset <= size && RamSize == size - chips[0].offset;

    /* Suppress Cart::assumeChips() for malformed CRT data. */
    return found;
}

auto Expert::assumeChips() -> void {
    imageValid = false;
    chips.clear();
}

auto Expert::prepare() -> void {
    if (imageValid)
        std::memcpy(expertRam, chips[0].ptr, RamSize);

    /* CHIP data is an initializer, never a ROM backing store. */
    cRomL = nullptr;
    cRomH = nullptr;
}

auto Expert::setSwitchMode(SwitchMode mode) -> void {
    switchMode = mode;

    if (switchMode != SwitchMode::On) {
        /* A switch change must also cancel a pending button press/NMI. */
        resetFreeze();
        if (nmiCall)
            nmiCall(false);
    }

    switch (switchMode) {
        case SwitchMode::Off:
            disableRam();
            ioRegisterEnabled = false;
            break;
        case SwitchMode::Prg:
            ramEnabled = true;
            ramWriteable = true;
            ioRegisterEnabled = false;
            break;
        case SwitchMode::On:
            /* Moving to ON alone does not set the latch; RESET/NMI does. */
            disableRam();
            ioRegisterEnabled = false;
            break;
    }
}

auto Expert::setJumper(unsigned jumperId, bool state) -> void {
    if (jumperId == 0)
        switchOn = state;
    else if (jumperId == 1)
        switchPrg = state;
    else
        return;

    /* PRG has priority for the otherwise redundant both-selected state. */
    setSwitchMode(switchPrg ? SwitchMode::Prg
        : (switchOn ? SwitchMode::On : SwitchMode::Off));
}

auto Expert::getJumper(unsigned jumperId) -> bool {
    if (jumperId == 0)
        return switchOn;
    if (jumperId == 1)
        return switchPrg;
    return false;
}

auto Expert::reset(bool softReset) -> void {
    cRomL = nullptr;
    cRomH = nullptr;
    exRom = true;       // /EXROM is not connected
    game = true;        // /GAME is overlaid per decoded address below
    nmiObserved = false;
    resetFreeze();

    switch (switchMode) {
        case SwitchMode::Off:
            disableRam();
            ioRegisterEnabled = false;
            break;
        case SwitchMode::Prg:
            ramEnabled = true;
            ramWriteable = true;
            ioRegisterEnabled = false;
            break;
        case SwitchMode::On:
            if (imageValid)
                enableOnMode();
            else {
                disableRam();
                ioRegisterEnabled = false;
            }
            break;
    }

    /* Deliberately do not copy the CRT initializer on reset. */
}

auto Expert::clock() -> void {
    /* didFreeze() may enable the RAM for the current NMI-vector access. */
    FreezeButton::clock();

    bool requestedGame = true;

    if (imageValid && ramEnabled) {
        uint16_t addr = system->cpu.addressBus();
        bool romLSelected = addr >= 0x8000 && addr <= 0x9fff;
        bool romHSelected = switchMode == SwitchMode::On && addr >= 0xe000;

        requestedGame = !(romLSelected || romHSelected);
    }

    if (game != requestedGame) {
        game = requestedGame;
        system->changeExpansionPortMemoryMode(exRom, game, true);
    }
}

auto Expert::didFreeze() -> void {
    if (imageValid && switchMode == SwitchMode::On) {
        enableOnMode();
    }

    /* Release the cartridge-generated NMI so another freeze can occur. */
    nmiCall(false);
}

auto Expert::observeNmi(bool state) -> void {
    if (state && !nmiObserved && imageValid && switchMode == SwitchMode::On) {
        enableOnMode();
    }
    nmiObserved = state;
}

auto Expert::serialize(Emulator::Serializer& s) -> void {
    FreezeButton::serializeStep2(s);

    s.integer((uint8_t&)switchMode);
    s.integer(ramEnabled);
    s.integer(ramWriteable);
    s.integer(ioRegisterEnabled);
    s.integer(imageValid);
    s.integer(nmiObserved);
    s.integer(switchOn);
    s.integer(switchPrg);
    s.buffer(expertRam, RamSize);

    if (s.mode() == Emulator::Serializer::Mode::Load) {
        exRom = true;
        game = true;
    }
}

auto Expert::enableOnMode() -> void {
    ramEnabled = true;
    ramWriteable = true;
    ioRegisterEnabled = true;
}

auto Expert::disableRam() -> void {
    ramEnabled = false;
    ramWriteable = false;
}

auto Expert::io1Access() -> void {
    if (switchMode != SwitchMode::On || !ioRegisterEnabled)
        return;

    /*
     * U3C/U3D are a cross-coupled NAND latch.  /IO1 low forces U3 pin 8
     * high, which forces the enable output at U3 pin 11 low.  It clears;
     * unlike VICE's compatibility model, it cannot toggle back on.
     */
    disableRam();
}

}
