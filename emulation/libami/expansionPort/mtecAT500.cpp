
#include "mtecAT500.h"
#include "../agnus/agnus.h"
#include "../system/firmware.h"
#include "../../tools/buffer.h"

namespace LIBAMI {

MtecAT500::~MtecAT500() {
    if (rom)
        delete[] rom;
}

auto MtecAT500::reset() -> void {     
    if (!rom) {
        romSize = sizeof(Firmware::mtecAT500);
        rom = new uint8_t[romSize << 1];
        std::memcpy(rom, Firmware::mtecAT500, romSize);
        std::memset(rom + romSize, 0xff, romSize); // upper 8k are unused
    }
}

auto MtecAT500::readAutoConf(uint32_t addr) -> uint8_t {
    addr &= 0xffff;

    if (addr & 1)
        return 0xff;

    if (!(addr & 0x8000))
        return rom[addr >> 1];

    return 0xff;
}

auto MtecAT500::read(uint32_t addr) -> uint8_t {
    addr &= 0xffff;    
    
    if (!(addr & 0x8000)) {
        if (addr & 1)
            return 0xff;

        return rom[addr >> 1];
    }

    return (uint8_t)hardDrive.readReg((addr >> 8) & 7);
}

auto MtecAT500::peekW(uint32_t addr) -> uint16_t {
    addr &= 0xffff;

    if ((addr & 0x8000) == 0)
        return 0xffff;

    uint8_t reg = (addr >> 8) & 7;

    if (reg == (uint8_t)HardDrive::Register::Data)
        return hardDrive.peekReg(reg);

    return hardDrive.peekReg(reg) << 8;
}

auto MtecAT500::readW(uint32_t addr) -> uint16_t {
    addr &= 0xffff;

    if ((addr & 0x8000) == 0)
        return 0xffff;
    
    uint8_t reg = (addr >> 8) & 7;

    if (reg == (uint8_t)HardDrive::Register::Data)
        return hardDrive.readReg(reg);
    
    return hardDrive.readReg(reg) << 8;
}

auto MtecAT500::write(uint32_t addr, uint8_t data) -> void {
    addr &= 0xffff;

    if ((addr & 0x8000) == 0)
        return;

    hardDrive.writeReg((addr >> 8) & 7, (uint16_t)data);
}

auto MtecAT500::writeW(uint32_t addr, uint16_t data) -> void {
    addr &= 0xffff;

    if ((addr & 0x8000) == 0)
        return;

    uint8_t reg = (addr >> 8) & 7;

    if (reg == (uint8_t)HardDrive::Register::Data)
        hardDrive.writeReg(reg, data);
    else
        hardDrive.writeReg(reg, data >> 8);
}

}
