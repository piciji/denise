
#pragma once

#include <vector>
#include <string>

namespace M68FAMILY {	
	
struct Memory {
	
	struct Cell {
		uint32_t address;
		uint8_t value;		
	};
	
	std::vector<Cell> cells;
	Cell* faultCell = nullptr;
	
	auto read(uint32_t address) -> uint8_t {
		for( auto& cell : cells ) {
			if (cell.address == address)
				return cell.value;
		}
		return 0;
	}
	
	auto write(uint32_t address, uint8_t value) -> void {
		for( auto& cell : cells ) {
			if (cell.address == address) {				
				cell.value = value;
				return;
			}
		}
		Cell cell{ address, value };
		cells.push_back( cell );
	}
	
	auto readWord(uint32_t address) -> uint16_t {
        return read(address & 0xffffff) << 8 | read( (address + 1) & 0xffffff);
    }

    auto writeWord(uint32_t address, uint16_t value) -> void {
        write(address & 0xffffff, value >> 8);
        write((address + 1) & 0xffffff, value & 0xff);
    }
	
	static auto isEqual(Memory* a, Memory* b, uint32_t startAddress) -> bool {
		if(a) a->faultCell = nullptr;
		if(b) b->faultCell = nullptr;
		
		if (!a && !b) return true;				
		if (!a) return true;
				
		for( auto& cellA : a->cells ) {
			if (cellA.address < startAddress) continue;
			
			bool match = false;
			Cell* matchCell = nullptr;
			
			if(b)
				for( auto& cellB : b->cells ) {
					if (cellB.address < startAddress) continue;

					if (cellA.address == cellB.address) {
						if (cellA.value == cellB.value) {
							match = true;
						}
						matchCell = &cellB;
						break;
					}
				}
			
			if (!match) {
				a->faultCell = &cellA;
				b->faultCell = matchCell;
				return false;
			}
		}		
		return true;
	}		
};

}