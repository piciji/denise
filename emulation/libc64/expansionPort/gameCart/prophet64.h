#pragma once

namespace LIBC64 {

struct Prophet64 : GameCart {
    static constexpr uint8_t BankMask = 0x1f;
    static constexpr uint8_t DisableBit = 0x20;

    uint8_t reg = 0;

    Prophet64( System* system ) : GameCart( system, true, false ) {

    }

    auto writeIo2( uint16_t addr, uint8_t value ) -> void override {
        reg = value;
        updateMemoryMap();
    }

    auto peekIo2( uint16_t addr ) -> uint8_t override {
        return reg;
    }

    auto reset( bool softReset = false ) -> void override {
        reg = 0;
        updateMemoryMap();
    }

    auto serializeSwitchedIn(Emulator::Serializer& s) -> void override {
        Cart::serialize( s );
        s.integer( reg );
    }

private:
    auto updateMemoryMap() -> void {
        cRomL = getChip( reg & BankMask );
        cRomH = nullptr;

        if (reg & DisableBit) {
            // Cartridge off
            system->changeExpansionPortMemoryMode( exRom = true, game = true );
        } else {
            // 8K GAME cartridge, ROML at $8000-$9FFF
            system->changeExpansionPortMemoryMode( exRom = false, game = true );
        }
    }
};

}
