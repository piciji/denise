
#include "events.h"

namespace Emulator {

struct Flash {
	
    enum Type { TypeNormal, TypeB, Type010, Type032BA01Swap, Type064 } type;
    
    Flash(Type type) {
        
        this->type = type;
        this->manufacturerId = 1;
        this->deviceId = 0xa4;
        this->deviceIdAddr = 1;
        this->size = 524288;
        this->sectorSize = 65536;
        this->sectorBytes = 1;
        this->unlock1addr = 0x5555;
        this->unlock2addr = 0x2aaa;
        this->unlock1mask = 0x7fff;
        this->unlock2mask = 0x7fff;
        this->statusToggleBits = 0x40;
        this->eraseSectorTimeoutCycles = 80;
        this->eraseSectorCycles = 1000000;
        this->eraseChipCycles = 14000000;
        
        switch(type) {
            case TypeNormal:
                this->eraseSectorCycles = 2000000;
                break;
                
            case TypeB:
                this->unlock1addr = 0x555;
                this->unlock2addr = 0x2aa;
                this->unlock1mask = 0x7ff;
                this->unlock2mask = 0x7ff;
                this->eraseSectorTimeoutCycles = 50;
                this->eraseChipCycles = 8000000;
                break;
                
            case Type010:
                this->sectorSize = 16384;
                this->deviceId = 0x20;
                this->size = 131072;
                this->sectorSize = 16384;
                this->eraseChipCycles = 1000000;
                break;
                
            case Type032BA01Swap:
                this->deviceId = 0x41;
                this->size = 4194304;
                this->sectorBytes = 8;
                this->unlock1addr = 0x556;
                this->unlock2addr = 0x2a9;
                this->unlock1mask = 0x7ff;
                this->unlock2mask = 0x7ff;
                this->statusToggleBits = 0x44;
                this->eraseSectorTimeoutCycles = 50;
                this->eraseChipCycles = 64000000;
                break;
                
            case Type064:
                this->deviceId = 0x7e;
                this->deviceIdAddr = 2;
                this->size = 8388608;
                this->sectorBytes = 16;
                this->unlock1addr = 0xaaa;
                this->unlock2addr = 0x555;
                this->unlock1mask = 0xfff;
                this->unlock2mask = 0xfff;
                this->eraseSectorTimeoutCycles = 50;
                this->eraseSectorCycles = 500000;
                this->eraseChipCycles = 64000000;
                break;
        }

        uint32_t mask = sectorSize - 1;

        while( true ) {
            if (!(mask & 1))
                break;

            mask >>= 1;
            this->sectorShift++;
        }
    }
	
	enum class State {  Read, unlock1, unlock2, AutoSelect, Program, ProgramError, EraseUnlock1, EraseUnlock2,
                        EraseSelect, ChipErase, SectorEraseTimeout, SectorEraseSuspend, SectorErase } state, baseState;
	
	using Callback = std::function<void ()>;
	Callback erase;
	
	Events* events;
	uint8_t* data = nullptr;
	uint8_t byteToProgram;
	uint8_t eraseMask[16];
	bool dirty = false;
	uint8_t clock = 0;

	uint32_t size;
	uint32_t sectorSize;
    uint32_t sectorShift;
    uint8_t sectorBytes;
    uint8_t manufacturerId;
    uint8_t deviceId;
    uint8_t deviceIdAddr;
    uint32_t unlock1addr;
    uint32_t unlock2addr;
    uint32_t unlock1mask;
    uint32_t unlock2mask;
    uint8_t statusToggleBits;
    unsigned eraseSectorTimeoutCycles;
    unsigned eraseSectorCycles;
    unsigned eraseChipCycles;
	
	auto setData( uint8_t* data ) -> void {
		
		this->data = data;
	}
	
	auto setEvents( Events* events ) -> void {
		
		this->events = events;
		
		erase = [this]() {

			switch (state) {
				case State::SectorEraseTimeout:
					events->add( &erase, eraseSectorCycles, Emulator::Events::UpdateExisting );
					state = State::SectorErase;
					break;
                    
				case State::SectorErase:
					for ( uint8_t sector = 0; sector < (this->sectorBytes << 3); sector++ ) {
						uint8_t pos = sector >> 3;
						uint8_t mask = 1 << (sector & 7);
						
						if (eraseMask[pos] & mask) {
                            eraseMask[pos] &= ~mask;
                            
                            std::memset( &data[ sector * sectorSize ], 0xff, sectorSize);
                            dirty = true;                            							
							break;
						}
					}

                    for ( uint8_t i = 0; i < this->sectorBytes; i++ ) {
                        if ( eraseMask[i] ) {
                            // there are more sectors to erase
                            events->add( &erase, eraseSectorCycles, Emulator::Events::UpdateExisting );
                            return;
                        }
                    }
                    // all marked sectors were erased
                    state = baseState;							
					break;

				case State::ChipErase:
                    std::memset( data, 0xff, size );
                    dirty = true;
					state = baseState;		
					break;

				default:					
					break;
			}
                        
		};
		
		events->registerCallback( { {&erase, 1} } );   
	}
	
	auto reset() -> void {        
        state = State::Read;
        baseState = State::Read;
        byteToProgram = 0;
        clearEraseMask();
        dirty = false;
        clock = 0;
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
		dirty = true;

		return newData == value;
	}

	auto addSectorForErase(uint32_t addr) -> void {

		uint8_t sector = addr >> sectorShift;

		eraseMask[(sector >> 3) & 0xf] |= 1 << (sector & 7);
	}
    
    auto clearEraseMask() -> void {

        for (unsigned i = 0; i < this->sectorBytes; i++)
            eraseMask[i] = 0;
    }     
	
	auto sync() -> void {
		clock++;
	}
	
    auto read( uint32_t addr ) -> uint8_t {

        switch (state) {
            case State::AutoSelect: {
                uint8_t _addr = addr & 0xff;
                
                if (type == Type032BA01Swap) {                                                
                    if (_addr == 0) addr = 0;
                    else if (_addr == 1) addr = 2;
                    else if (_addr == 2) addr = 1;
                    else if (_addr == 3) addr = 3;
                }
                
                _addr = addr & 0xff;

                if (_addr == 0)
                    return manufacturerId;
                if (_addr == deviceIdAddr)
                    return deviceId;
                if (_addr == 2)
                    return 0;
                
                return data[addr];                
            }

            case State:::ProgramError:
                return ((byteToProgram ^ 0x80) & 0x80) | ((clock & 2) << 5) | (1 << 5);         

            case State::SectorEraseSuspend:
            case State::ChipErase:
            case State::SectorErase:
            case State::SectorEraseTimeout: {                
                uint8_t value = byteToProgram;
                byteToProgram ^= statusToggleBits;

                return (state != State::SectorEraseTimeout) ? (value | 8) : value;
            }
        }
        
        return data[addr];
    }
    
	auto write(uint32_t addr, uint8_t value) -> void {
	
		switch(state) {
			case State::Read:
				if (unlock1(addr) && (value == 0xaa))
					state = State::unlock1;
				
				break;
				
			case State::unlock1:
				if (unlock2(addr) && (value == 0x55))
					state = State::unlock2;
				else
					state = baseState;					
					
				break;
				
			case State::unlock2:
                if (!unlock1(addr)) {
                    state = baseState;
                    break;
                }
                
                if (value == 0x90) {
                    state = State::AutoSelect;
                    baseState = State::AutoSelect;
                } else if (value == 0xf0) {
                    state = State::Read;
                    baseState = State::Read;
                } else if (value == 0xa0) {
                    state = State::Program;
                } else if (value == 0x80) {
                    state = State::EraseUnlock1;
                } else
                    state = baseState;
                break;
				
			case State::Program:
				if (programByte(addr, value)) {
					state = baseState;
				} else {
					state = State::ProgramError;
				}
				break;
				
			case State::EraseUnlock1:
				if (unlock1(addr) && (value == 0xaa))
					state = State::EraseUnlock2;
				else
					state = baseState;
				
				break;
				
			case State::EraseUnlock2:
				if (unlock2(addr) && (value == 0x55))
					state = State::EraseSelect;
				else
					state = baseState;
				
				break;
				
			case State::EraseSelect:
				if (unlock1(addr) && (byte == 0x10)) {
					state = State::ChipErase;
					byteToProgram = 0;
					events->add( &erase, eraseChipCycles, Emulator::Events::UpdateExisting );
				} else if (byte == 0x30) {
					addSectorForErase(addr);
					byteToProgram = 0;
					state = SectorEraseTimeout;
                    events->add( &erase, eraseSectorTimeoutCycles, Emulator::Events::UpdateExisting );
				} else {
					state = baseState;
				}
				break;
                
            case State::SectorEraseTimeout:
                if (byte == 0x30) {
                    addSectorForErase(addr);
                } else {
                    state = baseState;
                    clearEraseMask();
                    events->remove( &erase );
                }
                break;

            case State::SectorErase:
                if (byte == 0xb0) {
                    state = SectorEraseSuspend;
                    events->remove( &erase );
                }
                break;

            case State::SectorEraseSuspend:
                if (byte == 0x30) {
                    state = State::SectorErase;
                    events->add( &erase, eraseSectorCycles, Emulator::Events::UpdateExisting );
                }
                break;

            case State::ProgramError:
            case State::AutoSelect:
                if (unlock1(addr) && (byte == 0xaa)) {
                    state = State::unlock1;
                    
                } else if (byte == 0xf0) {
                    state = State::Read;
                    baseState = State::Read;
                }
                break;

            case State::ChipErase:
            default:
                break;
		}
	}
};

}
