
#pragma once

#include "systimer.h"

namespace Emulator {

struct Flash040 {
	
    enum Type { TypeNormal, TypeB, Type010 } type;
    
    Flash040(Type type) {
        
        this->type = type;
        this->deviceId = 0xa4;
        this->size = 524288;
        this->sectorSize = 65536;
        this->unlock1Addr = 0x5555;
        this->unlock2Addr = 0x2aaa;
        this->unlock1Mask = 0x7fff;
        this->unlock2Mask = 0x7fff;
        this->eraseSectorTimeoutCycles = 80;
        this->eraseSectorCycles = 1000000;
        this->eraseChipCycles = 8000000;
        
        switch(type) {
            case TypeNormal:
                break;
                
            case TypeB:
                this->unlock1Addr = 0x555;
                this->unlock2Addr = 0x2aa;
                this->unlock1Mask = 0x7ff;
                this->unlock2Mask = 0x7ff;
                this->eraseSectorTimeoutCycles = 50;
                break;
                
            case Type010:
                this->deviceId = 0x20;
                this->size = 131072;
                this->sectorSize = 16384;
                this->eraseChipCycles = 1000000;
                this->eraseSectorTimeoutCycles = 50;
                break;
        }

        this->sectorShift = 0;
        uint32_t mask = sectorSize - 1;

        while( true ) {
            if (!(mask & 1))
                break;

            mask >>= 1;
            this->sectorShift++;
        }
        
        dirty = false;
        reset();
    }
	
	enum class State {  Read, Unlock1, Unlock2,  EraseUnlock1, EraseUnlock2, AutoSelect, Program, ProgramError,
                        EraseSelect, ChipErase, SectorEraseTimeout, SectorEraseSuspend, SectorErase } state;
	
	using Callback = std::function<void ()>;
	Callback erase;
    Callback written;
	
	SystemTimer* events;
	uint8_t* data = nullptr;
	uint8_t byteToProgram;
	uint8_t eraseMask;
	bool dirty = false;

	uint32_t size;
	uint32_t sectorSize;
    uint32_t sectorShift;
    uint8_t deviceId;
    uint32_t unlock1Addr;
    uint32_t unlock2Addr;
    uint32_t unlock1Mask;
    uint32_t unlock2Mask;
    unsigned eraseSectorTimeoutCycles;
    unsigned eraseSectorCycles;
    unsigned eraseChipCycles;
    bool autoSelect;
	
	auto setData( uint8_t* data ) -> void {
		
		this->data = data;
	}
	
	auto setEvents( SystemTimer* events ) -> void {
		
		this->events = events;

		erase = [this]() {

			switch (state) {
				case State::SectorEraseTimeout:
					this->events->add( &erase, eraseSectorCycles, Emulator::SystemTimer::UpdateExisting );
					state = State::SectorErase;
					break;
                    
				case State::SectorErase: {
                    int sector = eraseSector(true);

                    if (sector != -1) {
                        std::memset(&data[sector * sectorSize], 0xff, sectorSize);
                        if (!dirty)
                            written();
                        dirty = true;
                    }

                    if (eraseMask) {
                        // there are more sectors to erase
                        this->events->add(&erase, eraseSectorCycles, Emulator::SystemTimer::UpdateExisting);
                        return;
                    }

                    // all marked sectors were erased
                    endCommand();
                } break;

				case State::ChipErase:
                    std::memset( data, 0xff, size );
                    if (!dirty)
                        written();
                    dirty = true;
                    endCommand();
					break;

				default:					
					break;
			}
                        
		};

        events->registerCallback( { {&erase, 1} } );
	}
	
	auto reset() -> void {        
        state = State::Read;
        autoSelect = false;
        byteToProgram = 0;
        eraseMask = 0;
        //dirty = false;
    }
    
	auto unlock1(uint32_t addr) -> bool {
		
		return (addr & unlock1Mask) == unlock1Addr;
	}

	auto unlock2(uint32_t addr) -> bool {
		
		return (addr & unlock2Mask) == unlock2Addr;
	}

	auto programByte(uint32_t addr, uint8_t value) -> bool {
		uint8_t newData = data[addr] & value;

		byteToProgram = value;
		data[addr] = newData;
        if (!dirty)
            written();
		dirty = true;

		return newData == value;
	}

	auto addSectorForErase(uint32_t addr) -> void {

		uint8_t sector = addr >> sectorShift;

		eraseMask |= 1 << (sector & 7);
	}
	
    auto read( uint32_t addr ) -> uint8_t {

        switch (state) {
            case State::AutoSelect: {
                uint8_t _addr = addr & 0xff;

                if (_addr == 0)
                    return 1; // manufacturer Id
                if (_addr == 1)
                    return deviceId;
                if (_addr == 2)
                    return 0; // unprotected
                
                return data[addr];                
            }

            // sector time exceeded means bad sector (hardware error, doesn't make sense in emulating this)
            case State::ProgramError: { // manual calls it: program time exceeded (usage error)
                uint8_t value = byteToProgram;
                byteToProgram ^= 0x40;
                return ((byteToProgram ^ 0x80) & 0x80) | (value & 0x40) | (1 << 5);
            }

            // todo: get status while programming (a few micro, too short to have practical use either)
            case State::SectorEraseSuspend: {
                uint8_t sector = addr >> sectorShift;
                int _sector = eraseSector();

                if ((int)sector == _sector) {
                    if (type == Type::TypeB) {
                        uint8_t value = byteToProgram;
                        byteToProgram ^= 4;

                        return 0x80 | (value & 4);
                    }
                    return 0x80 | 8;
                }
                break;
            }
            case State::ChipErase:
            case State::SectorErase:
            case State::SectorEraseTimeout: {                
                uint8_t value = byteToProgram;
                byteToProgram ^= 0x40;
                if (type == Type::TypeB) {
                    byteToProgram ^= 4;
                }

                return (state != State::SectorEraseTimeout) ? (value | 8) : value;
            }
            default:
                break;
        }
        
        return data[addr];
    }
    
	auto write(uint32_t addr, uint8_t value) -> void {
	
		switch(state) {
			case State::Read:
				if (unlock1(addr) && (value == 0xaa))
					state = State::Unlock1;
				
				break;
				
			case State::Unlock1:
				if (unlock2(addr) && (value == 0x55))
					state = State::Unlock2;
				else
                    endCommand();
					
				break;
				
			case State::Unlock2:
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
					events->add( &erase, eraseChipCycles, Emulator::SystemTimer::UpdateExisting );
				} else if (value == 0x30) {
					addSectorForErase(addr);
					byteToProgram = 0;
					state = State::SectorEraseTimeout;
                    events->add( &erase, eraseSectorTimeoutCycles, Emulator::SystemTimer::UpdateExisting );
				} else {
                    endCommand();
				}
				break;
                
            case State::SectorEraseTimeout:
                if (value == 0x30) {
                    addSectorForErase(addr);
                    events->add( &erase, eraseSectorTimeoutCycles, Emulator::SystemTimer::UpdateExisting );
                } else {
                    endCommand();
                    eraseMask = 0;
                    events->remove( &erase );
                }
                break;

            case State::SectorErase:
                if (value == 0xb0) {
                    state = State::SectorEraseSuspend;
                    events->remove( &erase );
                }
                break;

            case State::SectorEraseSuspend:
                if (value == 0x30) {
                    state = State::SectorErase;
                    events->add( &erase, eraseSectorCycles, Emulator::SystemTimer::UpdateExisting );
                }
                break;

            case State::ProgramError:
            case State::AutoSelect:
                if (unlock1(addr) && (value == 0xaa)) {
                    state = State::Unlock1;
                    
                } else if (value == 0xf0) {
                    state = State::Read;
                    autoSelect = false;
                }
                break;

            case State::ChipErase:
            default:
                break;
		}
	}

    auto endCommand() -> void {
        state = autoSelect ? State::AutoSelect : State::Read;
    }

    auto eraseSector(bool clearSector = false) -> int {
        for ( uint8_t sector = 0; sector < 8; sector++ ) {
            uint8_t mask = 1 << sector;
            if (eraseMask & mask) {
                if (clearSector) {
                    eraseMask &= ~mask;
                }
                return sector;
            }
        }
        return -1;
    }

    auto serialize(Emulator::Serializer& s) -> void {
        s.integer(dirty);
        s.integer(byteToProgram);
        s.integer((uint8_t&)state);
        s.integer(autoSelect);
        s.integer(eraseMask);
    }
};

}
