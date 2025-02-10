
#pragma once

namespace LIBC64 {

struct SuperSnapshotV5 : Freezer {

    SuperSnapshotV5(System* system) : Freezer(system, false, true) {
        unbeatable = 0xff;
        ram = new uint8_t[ 32 * 1024 ];
    }

    ~SuperSnapshotV5() {
        delete[] ram;
    }

    uint8_t* ram;
    bool enable;
    uint8_t bank;

    auto assumeChips( ) -> void {
        Cart::assumeChips( {16384} );
    }

    auto writeIo1( uint16_t addr, uint8_t value ) -> void {
        if (!enable)
            return;

        bank = ((value >> 2) & 1) | (((value >> 4) & 1) << 1) | (((value >> 5) & 1) << 2);
        enable = ((value >> 3) & 1) == 0;

        // 128k support
        cRomL = &chips[bank % chips.size()];
        cRomH = &chips[bank % chips.size()];

        game = value & 1;
        exRom = ((value >> 1) & 1) ^ 1;
        system->changeExpansionPortMemoryMode( exRom, game );

        if (game) {
            nmiCall(false);
            irqCall(false);
        }
    }

    auto readIo1( uint16_t addr ) -> uint8_t {
        if (!enable)
            return 0;

        return Cart::readRomL(0x1e00 | (addr & 0xff));
    }

    auto readRomL(uint16_t addr) -> uint8_t {
        if (exRom)
            return ram[((bank & 3) << 13) | (addr & 0x1fff) ];

        if (enable)
            return Cart::readRomL(addr & 0x1fff);

        return ExpansionPort::readRomL(addr);
    }

    auto writeRomL( uint16_t addr, uint8_t data ) -> void {
        if (exRom)
            ram[((bank & 3) << 13) | (addr & 0x1fff) ] = data;

        ExpansionPort::writeRomL( addr, data );
    }

    auto writeUltimaxRomL( uint16_t addr, uint8_t data ) -> void {
        if (exRom)
            ram[((bank & 3) << 13) | (addr & 0x1fff) ] = data;

        ExpansionPort::writeUltimaxRomL( addr, data );
    }

    auto didFreeze() -> void {
        enable = true;
    }

    auto reset(bool softReset = false) -> void {
        cRomH = cRomL = getChip(0);
        enable = true;
        bank = 0;
        game = false;
        exRom = true;
        std::memset(ram, 0, 32 * 1024);
    }

    auto serializeStep2(Emulator::Serializer& s) -> void {

        FreezeButton::serializeStep2( s );

        s.integer( enable );
        s.integer( bank );
        s.array( ram, 32 * 1024 );
    }
};

}
