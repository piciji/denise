
// has full support for eapi calls in EasyFlash³
// todo: Word mode, mx29lv640et model (Top Boot Sector), protect sectors

#include "mx29lv640eb.h"

namespace Emulator {

    MX29LV640EB::MX29LV640EB() {
        this->manufacturerId = 0xc2;
        this->deviceId = 0xcb;
        this->size = 8 * 1024 * 1024;
        this->unlock1Addr = 0xaaa;
        this->unlock2Addr = 0x555;
        this->unlock1Mask = ~0;
        this->unlock2Mask = ~0;
        this->eraseSectorTimeoutCycles = 50;
        this->eraseSectorCycles = 500000;
        this->eraseChipCycles = 45000000;

        reset();
    }

    auto MX29LV640EB::setData(uint8_t *data) -> void {

        this->data = data;
    }

    auto MX29LV640EB::setEvents(SystemTimer *events) -> void {

        this->events = events;

        erase = [this]() {

            switch (state) {
                case State::SectorEraseTimeout:
                    this->events->add(&erase, eraseSectorCycles, Emulator::SystemTimer::UpdateExisting);
                    state = State::SectorErase;
                    break;

                case State::SectorErase: {
                    bool lookForOtherSectorsToErase = false;

                    for (uint8_t sector = 0; sector < MX29LV_SECTORS; sector++) {

                        if (!eraseMask[sector])
                            continue;

                        if (lookForOtherSectorsToErase) {
                            this->events->add(&erase, eraseSectorCycles, Emulator::SystemTimer::UpdateExisting);
                            return;
                        }

                        eraseMask[sector] = false;

                        unsigned sectorSize = 64 * 1024;
                        unsigned offset = sectorSize;

                        if (sector < 8) {
                            sectorSize = 8 * 1024;
                            offset = sector * sectorSize;
                        } else {
                            offset += (sector - 8) * sectorSize;
                        }

                        std::memset(&data[offset], 0xff, sectorSize);

                        written();

                        lookForOtherSectorsToErase = true;
                    }

                    // all marked sectors were erased
                    endCommand();
                } break;

                case State::ChipErase:
                    std::memset(data, 0xff, size);
                    written();
                    endCommand();
                    break;

                default:
                    break;
            }

        };

        events->registerCallback({{&erase, 1}});
    }

    auto MX29LV640EB::reset() -> void {
        state = State::Read;
        autoSelect = false;
        byteToProgram = 0;
        clearEraseMask();
    }

    auto MX29LV640EB::unlock1(uint32_t addr) -> bool {

        return (addr & unlock1Mask) == unlock1Addr;
    }

    auto MX29LV640EB::unlock2(uint32_t addr) -> bool {

        return (addr & unlock2Mask) == unlock2Addr;
    }

    auto MX29LV640EB::programByte(uint32_t addr, uint8_t value) -> bool {
        uint8_t newData = data[addr] & value;

        byteToProgram = value;
        data[addr] = newData;
        written();

        return newData == value;
    }

    inline auto MX29LV640EB::getSectorByAddr(uint32_t addr) -> uint8_t {

        uint8_t sectorNr = addr >> (3 + 13);

        if (sectorNr == 0) {
            sectorNr += (addr >> 13) & 7;
        } else
            sectorNr += 7;

        return sectorNr;
    }

    auto MX29LV640EB::addSectorForErase(uint32_t addr) -> void {

        // sanity check is not needed if memory addr doesn't exceed 8 MB
        eraseMask[ getSectorByAddr(addr) ] = true;
    }

    auto MX29LV640EB::clearEraseMask() -> void {

        std::memset( &eraseMask[0], 0, MX29LV_SECTORS );
    }

    auto MX29LV640EB::read(uint32_t addr) -> uint8_t {

        switch (state) {
            case State::AutoSelect: {
                uint8_t _addr = addr & 0xff;

                if (_addr == 0)
                    return manufacturerId;
                if (_addr == 2)
                    return deviceId;
                if (_addr == 6)
                    return 0x08; // unlocked
                // todo get protection state

                return data[addr];
            }

            case State::ProgramError: {
                uint8_t value = byteToProgram;
                byteToProgram ^= 0x40;

                return ((byteToProgram ^ 0x80) & 0x80) | ( value & 0x40 ) | (1 << 5);
            }
            case State::SectorEraseSuspend: {
                uint8_t requestedSector = getSectorByAddr(addr);

                for (uint8_t sector = 0; sector < MX29LV_SECTORS; sector++) {

                    if (!eraseMask[sector])
                        continue;

                    if (requestedSector == sector) {
                        uint8_t value = byteToProgram;
                        byteToProgram ^= 0x04;

                        return (value | 0x80) & ~0x20;
                    }

                    if (sector > requestedSector)
                        break;
                }

                return data[addr];

            } break;
            case State::ChipErase:
            case State::SectorErase:
            case State::SectorEraseTimeout: {
                uint8_t value = byteToProgram;
                byteToProgram ^= 0x44;

                return (state != State::SectorEraseTimeout) ? (value | 8) : value;
            }
            default:
                break;
        }

        return data[addr];
    }

    auto MX29LV640EB::write(uint32_t addr, uint8_t value) -> void {

        switch (state) {
            case State::Read:
                if (unlock1(addr) && (value == 0xaa))
                    state = State::unlock1;

                break;

            case State::unlock1:
                if (unlock2(addr) && (value == 0x55))
                    state = State::unlock2;
                else
                    endCommand();

                break;

            case State::unlock2:
                if (!unlock1(addr)) {
                    endCommand();
                    break;
                }

                if (value == 0x90) {
                    state = State::AutoSelect;
                    autoSelect = true;
                } else if (value == 0xf0) {
                    state = State::Read;
                    autoSelect = false;
                } else if (value == 0xa0) {
                    state = State::Program;
                } else if (value == 0x80) {
                    state = State::EraseUnlock1;
                } else
                    endCommand();
                break;

            case State::Program:
                if (programByte(addr, value)) {
                    endCommand();
                } else {
                    state = State::ProgramError;
                }
                break;

            case State::EraseUnlock1:
                if (unlock1(addr) && (value == 0xaa))
                    state = State::EraseUnlock2;
                else
                    endCommand();

                break;

            case State::EraseUnlock2:
                if (unlock2(addr) && (value == 0x55))
                    state = State::EraseSelect;
                else
                    endCommand();

                break;

            case State::EraseSelect:
                if (unlock1(addr) && (value == 0x10)) {
                    state = State::ChipErase;
                    byteToProgram = 0;
                    events->add(&erase, eraseChipCycles, Emulator::SystemTimer::UpdateExisting);
                } else if (value == 0x30) {
                    addSectorForErase(addr);
                    byteToProgram = 0;
                    state = State::SectorEraseTimeout;
                    events->add(&erase, eraseSectorTimeoutCycles, Emulator::SystemTimer::UpdateExisting);
                } else {
                    endCommand();
                }
                break;

            case State::SectorEraseTimeout:
                if (value == 0x30) {
                    addSectorForErase(addr);
                    events->add(&erase, eraseSectorTimeoutCycles, Emulator::SystemTimer::UpdateExisting); // recount
                } else {
                    endCommand();
                    clearEraseMask();
                    events->remove(&erase);
                }
                break;

            case State::SectorErase:
                if (value == 0xb0) {
                    state = State::SectorEraseSuspend;
                    events->remove(&erase);
                }
                break;

            case State::SectorEraseSuspend:
                if (value == 0x30) {
                    state = State::SectorErase;
                    events->add(&erase, eraseSectorCycles, Emulator::SystemTimer::UpdateExisting);
                }
                break;

            case State::ProgramError:
            case State::AutoSelect:
                if (unlock1(addr) && (value == 0xaa)) {
                    state = State::unlock1;

                } else if (value == 0xf0) { // Reset mode
                    state = State::Read;
                    autoSelect = false;
                }
                break;

            case State::ChipErase:
            default:
                break;
        }
    }

    auto MX29LV640EB::endCommand() -> void {
        state = autoSelect ? State::AutoSelect : State::Read;
    }

    auto MX29LV640EB::serialize(Emulator::Serializer &s) -> void {
        s.integer(byteToProgram);
        s.integer((uint8_t&) state);
        s.integer(autoSelect);
        s.array(eraseMask);
    }

}
