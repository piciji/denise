
#include "chessCard.h"
#include "../../interface.h"

namespace LIBC64 {

FinalChessCard::FinalChessCard(System* system) : Cart(system, false, false) {
    setId( Interface::ExpansionIdFinalChessCard );
    ram = new uint8_t[8 * 1024];
    MHz = 5'000'000;
    jumpers = 0;
    writeProtectRAM = false;
    mediaWrite = nullptr;
}

FinalChessCard::~FinalChessCard() {
    delete[] ram;
}

auto FinalChessCard::setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void {
    if (media->secondary) {
        int _referencedId;

        if (this->media == nullptr) {
            _referencedId = media->id;
        } else
            _referencedId = this->media->id + 2;

        if (media->id == _referencedId) {
            if (!rom || (romSize != (32 * 1024)))
                romFCC = nullptr;
            else
                romFCC = rom;

        } else if (media->id == (_referencedId + 2) ) {
            if (rom) {
                if (romSize >= (8 * 1024))
                    romSize = 8 * 1024;
                else
                    std::memset(ram, 0x0, 8 * 1024);

                memcpy(ram, rom, romSize);
                mediaWrite = media;
            } else {
                if (mediaWrite) { // unset
                    if (!mediaWrite->guid || writeProtectRAM) {
                        mediaWrite = nullptr;
                        return;
                    }

                    if (!system->interface->questionToWrite(mediaWrite)) {
                        mediaWrite = nullptr;
                        return;
                    }

                    system->interface->truncateMedia( mediaWrite );
                    system->interface->writeMedia(mediaWrite, ram, 8 * 1024, 0);
                    mediaWrite = nullptr;
                }
            }
        }
    } else
        Cart::setRom(media, rom, romSize);
}

auto FinalChessCard::reset(bool softReset) -> void {
    cycles = 0;
    latch = 0;
    latch2 = 0;
    cRomL = getChip(0);
    cRomH = getChip(0);
    game = false;
    exRom = false;
    writeProtectIO = false;
    frequency = vicII->frequency();
    if (!mediaWrite)
        std::memset(ram, 0, 8 * 1024);
    power();
}

inline auto FinalChessCard::readByte(uint16_t addr) -> uint8_t {
    cycles += frequency;

    if (addr & 0x8000)
        return romFCC ? romFCC[addr & 0x7fff] : 0xff;

    if ((addr & 0xe000) == 0)
        return ram[addr & 0x1fff];

    if ((addr & 0x7f00) == 0x7f00) {
        setNmiLineLow(false);
        return latch2;
    }

    return 0;
}

inline auto FinalChessCard::writeByte(uint16_t addr, uint8_t value) -> void {
    cycles += frequency;

    if ((addr & 0xe000) == 0)
        ram[addr & 0x1fff] = value;

    else if ((addr & 0x7f00) == 0x7f00) {
        latch = value;
        nmiCall(true);
    }
}

inline auto FinalChessCard::idleCycle() -> void {
    cycles += frequency;
}

auto FinalChessCard::clock() -> void {
    while(cycles < MHz) {
         process();
    }

    cycles -= MHz;
}

auto FinalChessCard::writeIo1( uint16_t addr, uint8_t data ) -> void {
    if (writeProtectIO)
        return;

    cRomL = getChip(data & 1);
    cRomH = getChip(data & 1);

    exRom = !!(data & 2);
    game = !!(data & 2);
    writeProtectIO = data & 0x80;

    system->changeExpansionPortMemoryMode( exRom, game );
}

auto FinalChessCard::writeIo2( uint16_t addr, uint8_t data ) -> void {
    latch2 = data;
    setNmiLineLow(true);
}

auto FinalChessCard::readIo2( uint16_t addr ) -> uint8_t {
    nmiCall(false);
    return latch;
}

auto FinalChessCard::assumeChips( ) -> void {
    Cart::assumeChips( {16384} );
}

auto FinalChessCard::create( Interface::CartridgeId cartridgeId, unsigned _size ) -> Cart* {
    // don't rebuild
    return this;
}

auto FinalChessCard::setJumper(unsigned jumperId, bool state) -> void {
    if (state)
        jumpers |= (1 << jumperId);
    else
        jumpers &= ~(1 << jumperId);

    MHz = 5'000'000;
    for(int i = 0; i < 8; i++) {
        if (jumpers & (1 << i)) {
            switch(i) {
                default: break;
                case 0: MHz += 10'000'000; break;
                case 1: MHz += 20'000'000; break;
                case 2: MHz += 30'000'000; break;
                case 3: MHz += 50'000'000; break;
            }
        }
    }
}

auto FinalChessCard::getJumper(unsigned jumperId) -> bool {
    return (jumpers & (1 << jumperId)) ? true : false;
}

auto FinalChessCard::setWriteProtect( bool state ) -> void {
    writeProtectRAM = state;
}

auto FinalChessCard::isWriteProtected() -> bool {
    return writeProtectRAM;
}

auto FinalChessCard::serializeStep2(Emulator::Serializer& s) -> void {
    s.integer( cycles );
    s.integer( latch );
    s.integer( latch2 );
    s.integer( writeProtectIO );
    s.integer( MHz );
    s.integer( jumpers );
    s.integer( frequency );
    s.array(ram, 8 * 1024);

    // W65C02 context
    s.integer(pc);
    s.integer(a);
    s.integer(x);
    s.integer(y);
    s.integer(this->s);
    uint8_t _p = this->p;
    s.integer(_p);
    this->p = _p;
    s.integer(control);
    s.integer(lines);

    Cart::serializeStep2(s);
}

auto FinalChessCard::createImage(unsigned& imageSize) -> uint8_t* {
    imageSize = 8 * 1024;
    uint8_t* buffer = new uint8_t[ imageSize ];
    std::memset(buffer, 0x0, imageSize);
    return buffer;
}

}
