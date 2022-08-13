
#include "drive.h"
#include "../iec.h"
#include "mechanics.cpp"
#include "mechanicsP64.cpp"
#include "mechanicsG64.cpp"
#include "profdos.cpp"
#include "prologic.cpp"
#include "turbotrans.cpp"
#include "serialization.cpp"
#include "../../system/firmware.h"
#include "../../expansionPort/fastloader/fastloader.h"
#include "../../../tools/gcr.h"

// for 300 rpm = 5 rotation / sec = 16.000.000 / 5

namespace LIBC64 {

// valid for 1 MHz operation
// one cpu cycle is 16 reference(drive) cycles.
// we do only progress 6 instead of 8 in first half cycle because of a possible
// external overflow is recognized by cpu within 400 ns.
 
// in case of a VIA READ there is more time a change can be read back ~ 875 ns within cycle.
// 6 ref cycles are progressed in first half cycle already, so we need 8 more to get 14 of 16 ref cycles.
    
// for each cycle:

// 6 ref cycles:    check for external overflow
// + 8    
// 14 ref cycles:   VIA2 read back Changes
// + 2    
// 16 ref cycles:   complete cycle
// + 6
// repeat this pattern    

// the distance between "overflow" checking and maximum "Read back" time is 8 ref cycles, phase shifted by 2 ref cycles.
// the relative	distance matters, so we can step in 8 ref cycle chunks which is handled in rotateP64 and rotateG64
    
#define SYNC \
    cpu->handleSo();                                                    \
    if (operation & USERDATA_LEVEL) {                   \
        rotateD64();                                                        \
    } else if (operation & ENCODEDDATA_LEVEL) {            \
        rotateG64();                                                  \
    } else {                                                                \
        rotateP64();                                                  \
    }                                                                       \
    via1->process();                                                        \
    via2->process();                                                        \
    if (operation & DRIVE_MODE_157x) {                                       \
        wd1770->clock();                                                    \
        cia->clock();                                                   \
        if (operation & DRIVE_HAS_EXTRA_CIA)                                    \
            ciaSpeeder->clock();                                                \
    }                                                                           \
    cycleCounter += iecBus->cpuCylcesPerSecond;             \
    if (delayInProgress)              \
        progressDelay();
    
auto Drive::progressDelay() -> void {
    if (attachDelay) {
        if (--attachDelay == 0) {
            delayInProgress = stepperDelay;
        }
    }

    if (stepperDelay) {
        if (--stepperDelay == 0) {
            changeHalfTrack(nextStep);
            delayInProgress = attachDelay;
        }
    }
}

auto Drive::sync() -> void {
    SYNC 
}

auto Drive::cpuWrite(uint16_t addr, uint8_t data) -> void {
    SYNC

    if (operation & DRIVE_MODE_154x) {
        if (extendedMemoryMap) {
            if (speeder == 4) { // dolphin v3
                if ((addr & 0xf000) == 0x5000) {
                    pia->write( addr & 3, data );
                    return;
                }
            } else if (speeder == 6) {  // profdos v1
                if ((addr & 0xf100) == 0xf100) {
                    profDosClockControl(addr);
                    return;
                }
                if (profDosAutoSpeed)
                    profDosAutoClockControl(addr);

            } else if (speeder == 7) {  // profdos R4
                if ((addr & 0x6800) == 0x6800) {
                    profDosClockControl(addr);
                    return;
                }
                if (profDosAutoSpeed)
                    profDosAutoClockControl(addr);

            } else if (speeder == 10) {  // prologic classic

                if ((addr & 0xfff0) == 0xb800) {
                    addr = (addr >> 2) & 3;

                    if (addr & 2)
                        pia->write( addr, data );
                    else
                        prologicControlClassic( addr, data );

                    return;
                }
            } else if (speeder == 11) {  // prologic
                if ((addr & 0xe000) == 0xa000) {
                    prologicControl( addr );
                }
            } else if (speeder == 12) { // turbo trans
                if (turboTransWriteControl(addr, data))
                    return;
            }

            if ((expandMemory & (uint8_t) ExpandedMemMode::M80) && ((addr & 0xe000) == 0x8000)) {
                this->ram80To9F[addr & 0x1fff] = data;
                return;
            } else if ((expandMemory & (uint8_t) ExpandedMemMode::M40) && ((addr & 0xe000) == 0x4000)) {
                this->ram40To5F[addr & 0x1fff] = data;
                return;
            } else if ((expandMemory & (uint8_t) ExpandedMemMode::M60) && ((addr & 0xe000) == 0x6000)) {
                this->ram60To7F[addr & 0x1fff] = data;
                return;
            } else if ((expandMemory & (uint8_t) ExpandedMemMode::MA0) && ((addr & 0xe000) == 0xA000)) {
                this->ramA0ToBF[addr & 0x1fff] = data;
                return;
            } else if ((expandMemory & (uint8_t) ExpandedMemMode::M20) && ((addr & 0xe000) == 0x2000)) {
                this->ram20To3F[addr & 0x1fff] = data;
                return;
            }
        }

        if ((addr & 0x9800) == 0)
            ram[addr & 0x7ff] = data;

        else if ((addr & 0x9c00) == 0x1800)
            via1->write(addr, data);

        else if ((addr & 0x9c00) == 0x1c00)
            via2->write(addr, data);

    } else {
        // 157x
        if (extendedMemoryMap) {
            if (speeder == 5) { // dolphin v3
                if ((addr & 0xf000) == 0x5000) {
                    pia->write( addr & 3, data );
                    return;
                }
            } else if (speeder == 13) {
                if ((addr & 0xfff0) == 0x9e20) {
                    ciaSpeeder->write<MOS_8520>(addr, data);
                    return;
                }
            }

            if ((expandMemory & (uint8_t) ExpandedMemMode::M40) && (((addr & 0xf000) == 0x5000) || ((addr & 0xf800) == 0x4800) )) { // $4800 - $5ffff
                this->ram40To5F[addr & 0x1fff] = data;
                return;
            } else if ((expandMemory & (uint8_t) ExpandedMemMode::M60) && ((addr & 0xe000) == 0x6000)) {
                this->ram60To7F[addr & 0x1fff] = data;
                return;
            } else if ((expandMemory & (uint8_t) ExpandedMemMode::M80) && ((addr & 0xe000) == 0x8000)) {
                this->ram80To9F[addr & 0x1fff] = data;
                return;
            }
        }

        if ((addr & 0xf000) == 0)
            ram[addr & 0x7ff] = data;

        else if ((addr & 0xfc00) == 0x1800)
            via1->write(addr, data);

        else if ((addr & 0xfc00) == 0x1c00) {
            byteReady = false;
            via2->write(addr, data);

        } else if ((addr & 0xc000) == 0x4000) {
            cia->write<MOS_8520>(addr, data);

        } else if ((addr & 0xe000) == 0x2000) {
            wd1770->write(addr, data);
        }
    }
}

auto Drive::cpuRead(uint16_t addr) -> uint8_t {
    SYNC
    if (operation & DRIVE_MODE_154x) {
        if (extendedMemoryMap) {
            if (speeder == 4) {
                if ((addr & 0xf000) == 0x5000) {
                    return pia->read( addr & 3 );
                }
            } else if (speeder == 6) {
                if (profDosAutoSpeed)
                    profDosAutoClockControl(addr);

                if((addr & 0xe000) == 0x8000) {
                    return readProfDosEncoderV1(addr);
                }
                else if((addr & 0xe000) == 0xe000) {
                    return this->romExpanded[ (addr & 0x1fff) & romExpandedMask];
                }

            } else if (speeder == 7) {
                if (profDosAutoSpeed)
                    profDosAutoClockControl(addr);

                if((addr & 0xe000) == 0x6000) {
                    return readProfDosEncoder(addr);
                }
                else if((addr & 0xe000) == 0xe000) {
                    return this->romExpanded[ (0x2000 | (addr & 0x1fff)) & romExpandedMask ];
                }
            } else if (speeder == 10) {
                if ((addr & 0xfff0) == 0xb800) {
                    addr = (addr >> 2) & 3;

                    if (addr & 2)
                        return pia->read( addr );
                    else if ((addr & 1) == 0) {
                        return prologic2Mhz;
                    }
                }

                if (((addr & 0xf000) == 0xa000) || ((addr & 0xf800) == 0xb000)) { // a0 - b7
                    return this->romExpanded[ (addr & 0x1fff) & romExpandedMask ];
                }
                else if (((addr & 0xf000) == 0xe000) || ((addr & 0xf800) == 0xf000)) { // e0 - f7
                    return this->romExpanded[ (0x2000 | (addr & 0x1fff)) & romExpandedMask];
                }
                else if ((addr & 0xf800) == 0xf800) {
                    if (prologic40TrackMode)
                        return this->romExpanded[ (0x1800 + (addr & 0x7ff)) & romExpandedMask];

                    return this->romExpanded[ (0x3800 + (addr & 0x7ff)) & romExpandedMask];
                }
            } else if (speeder == 11) {
                uint8_t out;
                if (((addr & 0xf000) == 0xa000) || ((addr & 0xf800) == 0xb000)) { // a0 - b7
                    out = this->romExpanded[ (addr & 0x1fff) & romExpandedMask ];
                    return (out & ~0xa0) | ((out & 0x20) << 2) | ((out & 0x80) >> 2);
                }
                else if (((addr & 0xf000) == 0xe000) || ((addr & 0xf800) == 0xf000)) { // e0 - f7
                    out = this->romExpanded[ (0x2000 | (addr & 0x1fff)) & romExpandedMask];
                    return (out & ~0xa0) | ((out & 0x20) << 2) | ((out & 0x80) >> 2);
                }
                else if ((addr & 0xf800) == 0xf800) {
                    if (prologic40TrackMode) {
                        out = this->romExpanded[(0x1800 + (addr & 0x7ff)) & romExpandedMask];
                        return (out & ~0xa0) | ((out & 0x20) << 2) | ((out & 0x80) >> 2);
                    }

                    out = this->romExpanded[ (0x3800 + (addr & 0x7ff)) & romExpandedMask];
                    return (out & ~0xa0) | ((out & 0x20) << 2) | ((out & 0x80) >> 2);
                }
            } else if (speeder == 12) { // turbo trans

                if (turboTransVisible & 2) {
                    if ((addr & 0xf800) == 0x6800) {
                        return turboTrans[(turboTransPage << 10) | (addr & 0x3ff)];
                    } else if ((addr & 0xf800) == 0x7000) {
                        return turboTrans[ 0x40000 | ((turboTransPage << 10) | (addr & 0x3ff))];
                    }
                }

                if (turboTransVisible & 1) {
                    if ((addr & 0xe000) == 0xc000) {
                        return this->romExpanded[ (addr & 0x1fff) & romExpandedMask];
                    } else if ((addr & 0xe000) == 0x8000) {
                        return this->romExpanded[ (0x4000 | (addr & 0x1fff)) & romExpandedMask];
                    } else if ((addr & 0xe000) == 0xe000) {
                        return this->romExpanded[ (0x6000 | (addr & 0x1fff)) & romExpandedMask];
                    } else if ((addr & 0xe000) == 0xa000) {
                        if (expandMemory & (uint8_t) ExpandedMemMode::MA0)
                            return this->ramA0ToBF[addr & 0x1fff];

                        return this->romExpanded[ (0x2000 | (addr & 0x1fff)) & romExpandedMask];
                    }
                } else if ((addr & 0xe000) == 0xa000) {
                    // disable RAM by software, avoid expand mem usage
                    return rom[addr & romMask];
                }
            }

            if ((expandMemory & (uint8_t) ExpandedMemMode::M80) && ((addr & 0xe000) == 0x8000))
                return this->ram80To9F[addr & 0x1fff];
            else if ((expandMemory & (uint8_t) ExpandedMemMode::M40) && ((addr & 0xe000) == 0x4000))
                return this->ram40To5F[addr & 0x1fff];
            else if ((expandMemory & (uint8_t) ExpandedMemMode::M60) && ((addr & 0xe000) == 0x6000))
                return this->ram60To7F[addr & 0x1fff];
            else if ((expandMemory & (uint8_t) ExpandedMemMode::MA0) && ((addr & 0xe000) == 0xA000))
                return this->ramA0ToBF[addr & 0x1fff];
            else if ((expandMemory & (uint8_t) ExpandedMemMode::M20) && ((addr & 0xe000) == 0x2000))
                return this->ram20To3F[addr & 0x1fff];
        }

        if (addr & 0x8000)
           return rom[addr & romMask];

        else if ((addr & 0x9800) == 0)
            return ram[addr & 0x7ff];

        else if ((addr & 0x9c00) == 0x1800)
            return via1->read(addr);

        else if ((addr & 0x9c00) == 0x1c00)
            return via2->read(addr);

        return addr >> 8;
    }

    // 157x
    if (extendedMemoryMap) {
        if (speeder == 5) {
            if ((addr & 0xf000) == 0x5000) {
                return pia->read( addr & 3 );
            }
        }
        else if (speeder == 8 || speeder == 9) { // profdos R5, R6
            if ((addr & 0xe000) == 0x6000)
                return readProfDosEncoder( addr );
        }
        else if (speeder == 13) { // proSpeed 1571

            if (proSpeedControl & (1 | 2 | 0x80)) {

                if ((addr & 0xfff0) == 0x9e20) {
                    return ciaSpeeder->read<MOS_8520>(addr);
                }

                if (proSpeedControl & 0x2) {
                    if ((expandMemory & (uint8_t) ExpandedMemMode::M80) && ((addr & 0xe000) == 0x8000)) {
                        return this->ram80To9F[addr & 0x1fff];
                    }

                    if ((addr & 0xe000) == 0xc000) {
                        if ((proSpeedControl & 0x80) == 0) {
                            // copy programs
                            return this->romExpanded[(0x8000 | (addr & 0x1fff)) & romExpandedMask];
                        }
                    }

                    if ((addr & 0xf800) == 0xf800) {
                        // 35/40 track mode
                        if ((proSpeedControl & 0x1) == 0x0) {
                            return this->romExpanded[(0x8000 + 0x3800 + (addr & 0x7ff)) & romExpandedMask];
                        }

                        return this->romExpanded[(0x8000 + 0x7800 + (addr & 0x7ff)) & romExpandedMask];
                    }
                }

                if (addr & 0x8000) {
                    return this->romExpanded[(((proSpeedControl & 2) ? 0x8000 : 0x0) | (addr & 0x7fff)) & romExpandedMask];
                }
            }
        }

        if ((expandMemory & (uint8_t) ExpandedMemMode::M40) && (((addr & 0xf000) == 0x5000) || ((addr & 0xf800) == 0x4800) )) { // $4800 - $5ffff
            return this->ram40To5F[addr & 0x1fff];
        } else if ((expandMemory & (uint8_t) ExpandedMemMode::M60) && ((addr & 0xe000) == 0x6000)) {
            return this->ram60To7F[addr & 0x1fff];
        }
    }

    if (addr & 0x8000)
        return rom[addr & romMask];

    else if ((addr & 0xf000) == 0)
        return ram[addr & 0x7ff];

    else if ((addr & 0xfc00) == 0x1800)
        return via1->read(addr);

    else if ((addr & 0xfc00) == 0x1c00) {
        // TED line of U6 clears the Byte line in 2 Mhz mode.
        // Line is connected to Chip select of VIA 2. any access of VIA2 clears the line.
        byteReady = false;
        return via2->read(addr);

    } else if ((addr & 0xc000) == 0x4000) {
        return cia->read<MOS_8520>(addr);
    }
    else if ((addr & 0xe000) == 0x2000) {
        return wd1770->read(addr);
    }

    return addr >> 8;
}

Drive::Drive(uint8_t number, Emulator::Interface::Media* mediaConnected ) : structure(this) {
     
    this->number = number; 
	this->mediaConnected = mediaConnected;

	dummyTrack = new DiskStructure::MTrack;
    dummyTrack->pulses.push_back({0,0,1,1});
    dummyTrack->pulses.push_back({1000,0,0,0});
    dummyTrack->firstPulse = 0;
    gcrTrack = dummyTrack;

	structure.number = number;
	type = Type::D1541II;
	operation = 0;
    expandMemory = 0;
    speeder = 0;
    profDosAutoSpeed = 0;
    extendedMemoryMap = false;

	emulateDxxMoreAccurate = false;
    media = nullptr;
    wasAttachDetached = false;
    stepperDelay = 0;
    delayInProgress = !!attachDelay;
    motorOn = false;
    hidden = false;

    frequency = 1000000;
    refCyclesInCpuCycle = 16;
    
    ram = new uint8_t[ 2 * 1024 ];
    ram20To3F = new uint8_t[ 8 * 1024 ];
    ram40To5F = new uint8_t[ 8 * 1024 ];
    ram60To7F = new uint8_t[ 8 * 1024 ];
    ram80To9F = new uint8_t[ 8 * 1024 ];
    ramA0ToBF = new uint8_t[ 8 * 1024 ];
    turboTrans = new uint8_t[ 512 * 1024 ];

    rom1541II = (uint8_t*)Firmware::drive1541IIRom;
    rom1541 = (uint8_t*)Firmware::drive1541Rom;
    rom1541C = (uint8_t*)Firmware::drive1541CRom;
    rom1571 = (uint8_t*)Firmware::drive1571Rom;
    rom1570 = (uint8_t*)Firmware::drive1570Rom;
    rom1541IISize = sizeof( Firmware::drive1541IIRom );
    rom1541Size = sizeof( Firmware::drive1541Rom );
    rom1541CSize = sizeof( Firmware::drive1541CRom );
    rom1571Size = sizeof( Firmware::drive1571Rom );
    rom1570Size = sizeof( Firmware::drive1570Rom );

    rom = rom1541II;
    romMask = rom1541IISize - 1;
    
    via1 = new Via( 1 );
    via2 = new Via( 2 );
    cia = new Cia( 3 );
    ciaSpeeder = new Cia( 4 );
    cpu = new M6502(this);
    pia = new Emulator::Pia;
    wd1770 = new WD1770;

    wd1770->setTrack(dummyTrack, true);

    pia->ca2Out = [this](bool direction) {
        if (direction)
            return;

        if (speeder == 4 || speeder == 5) {
            system->writeParallelHandshake();
        }
    };

    pia->cb2Out = [this](bool direction) {
        if (direction)
            return;

        if (speeder == 10) {
            system->writeParallelHandshake();
        }
    };

    pia->readPort = [this]( Emulator::Pia::Port port ) {

        if (port == Emulator::Pia::Port::A) {
            if (speeder == 4 || speeder == 5) {
                if (!system->secondDriveCable.parallelUse)
                    return this->pia->ioa;
            } else
                return this->pia->ioa;
        } else {
            if (speeder == 10) {
                if (!system->secondDriveCable.parallelUse)
                    return this->pia->iob;
            } else
                return this->pia->iob;
        }

        uint8_t out = system->readParallel();

        for (auto drive : iecBus->drivesEnabled) {
            if (port == Emulator::Pia::Port::A)
                out &= drive->pia->ioa;
            else
                out &= drive->pia->iob;
        }
        return out;
    };

    pia->writePort = [this]( Emulator::Pia::Port port, uint8_t data ) {
        // nothing todo here, because CA(B)2 is triggered and port value is latched on pins
    };

    // proSpeed 1571 v2.0 has extra CIA
    ciaSpeeder->writePort = [this]( Cia::Port port, Cia::Lines* lines ) {

        if ( lines->prbChange && (port == Cia::PORTB )) {
            system->writeParallelHandshake();
        }
        else if (port == Cia::PORTA ) {
            proSpeedControl = lines->ioa;
        }
    };

    ciaSpeeder->readPort = [this]( Cia::Port port, Cia::Lines* lines ) {

        if ( port == Cia::PORTB ) {
            if (!system->secondDriveCable.parallelUse )
                return lines->iob;

            uint8_t out = system->readParallelWithHandshake();

            for (auto drive : iecBus->drivesEnabled) {
                out &= drive->ciaSpeeder->lines.iob;
            }

            return out;
        }

        return lines->ioa;
    };

    cia->serialOut = [this](bool bit) {
        if (dataDirection) {
            if (system->secondDriveCable.burstUse) {
                cia1->serialIn(bit);
            }
        }
    };

    cia->writePort = [this]( Cia::Port port, Cia::Lines* lines ) {

        if ( lines->prbChange && (port == Cia::PORTB )) {

            if ((operation & (DRIVE_HAS_PIA | DRIVE_HAS_EXTRA_CIA) ) == 0)
                system->writeParallelHandshake();
        }

        else if (port == Cia::PORTA ) {

        }
    };

    cia->readPort = [this]( Cia::Port port, Cia::Lines* lines ) {

        if ( port == Cia::PORTB ) {

            if (!system->secondDriveCable.parallelUse || (operation & (DRIVE_HAS_PIA | DRIVE_HAS_EXTRA_CIA)) )
                return lines->iob;

            uint8_t out = system->readParallelWithHandshake();

            for (auto drive : iecBus->drivesEnabled) {
                out &= drive->cia->lines.iob;
            }

            return out;
        }
        return lines->ioa;
    };

    cia->irqCall = [this](bool state) {
        if (state)
            irqIncomming |= 4;
        else
            irqIncomming &= ~4;

        cpu->setIrq( irqIncomming != 0 );
    };

    via1->irqCall = [this](bool state) {                
        if (state)
            irqIncomming |= 1;
        else
            irqIncomming &= ~1;

        cpu->setIrq( irqIncomming != 0 );
    };
    
    via2->irqCall = [this](bool state) {
		if (state)
			irqIncomming |= 2;
		else
			irqIncomming &= ~2; 
		
        cpu->setIrq( irqIncomming != 0 );
    };

    //PB 7, CB2: ATN IN
    //PB 6,5: Device address preset switches
    //PB 4:	ATN acknowledge OUT
    //PB 3:	CLOCK OUT
    //PB 2:	CLOCK IN
    //PB 1:	DATA OUT
    //PB 0:	DATA IN
    
    via1->writePort = [this]( Via::Port port, Via::Lines* lines ) {        
        
        if (port == Via::Port::B) {
            
            if (lines->iob != lines->iobOld) {
            
                updateBus();
                                            
                iecBus->updatePort();
            }
        } else {

            if (type == Type::D1570 || type == Type::D1571) {
                dataDirection = !!(lines->ioa & 2);

                if ((lines->ioa ^ lines->ioaOld) & 0x20) {
                    updateCycleSpeed(lines->ioa & 0x20, false);
                }

                uint8_t _side = side;
                side = !!(lines->ioa & 4);

                if (!structure.hasSecondSide())
                    side = 0;

                if (side != _side) {
                    changeHalfTrack(0);
                }
            }
            // nothing todo here for 1541 parallel cable mode, because CA2 is triggered in VIA core
        }
    };   
    
    via1->readPort = [this]( Via::Port port, Via::Lines* lines ) {
        
        if (port == Via::Port::B) {
            // invert the three input bits, add device number  
            return (uint8_t)( ((0x1a | iecBus->readVia()) ^ 0x85) | (this->number << 5) ); 
        }

        // port A
        if (operation & DRIVE_MODE_157x) {
            return (uint8_t) ((((byteReady ? 0 : 0x80) | ((currentHalftrack == 0) ? 0 : 1) | 0x7e) & ~lines->ddra) |
                (lines->pra & lines->ddra));
        }

        if (system->secondDriveCable.parallelUse && ((operation & DRIVE_HAS_PIA) == 0) ) {
            uint8_t out = system->readParallel();

            for (auto drive : iecBus->drivesEnabled) {
                out &= drive->via1->lines.ioa;
            }

            return out;

        } else if (type == Type::D1541C) {
            return (uint8_t) ( ( ( ((currentHalftrack == 0) ? 1 : 0) | 0xfe ) & ~lines->ddra) | ( lines->pra & lines->ddra ) );
        }

        return lines->ioa;
    };
    
    via2->writePort = [this]( Via::Port port, Via::Lines* lines ) {        
        
        if (port == Via::Port::B) {
            
            if (lines->iob & 4) { // stepper motor works only when drive motor is active

                if (stepperDelay) {
                    bool headBang = (currentHalftrack == 0) && ((nextStep == 3) || ( (nextStep == 2) && !coilDir));

                    updateStepper( nextStep );

                    if (system->driveSounds.useFloppy)
                        stepSound( headBang );
                }

                uint8_t _step = ((lines->iob & 3) - (currentHalftrack & 3)) & 3;

                if (_step != 0) {

                    if (!stepperSeekTime) {
                        changeHalfTrack(_step);
                    } else {
                        nextStep = _step;
                        stepperDelay = stepperSeekTime;
                        if ( use2Mhz() )
                            stepperDelay <<= 1;

                        delayInProgress = true;
                    }
                } else if (stepperDelay) {
                    stepperDelay = 0;
                    changeHalfTrack(0);
                }
            }                            
            
            speedZone = (lines->iob >> 5) & 3;                        

            if ((lines->iob ^ lines->iobOld) & 4) {
                // motor switched between on/off 
                motorOn = (lines->iob & 4) != 0;
                wd1770->setDiskAccessible(motorOn & loaded);
                if (!motorOn)
                    motorOffInit();

                if (system->driveSounds.useFloppy) {
                    if (motorOn)
                        system->interface->mixDriveSound( this->mediaConnected, DriveSound::FloppySpinUp );
                    else
                        system->interface->mixDriveSound( this->mediaConnected, DriveSound::FloppySpinDown );
                }
                
                updateDeviceState();

                bool _loadingState = false;
                for( auto drive : iecBus->drivesEnabled ) {
                    if (drive->motorOn) {
                        _loadingState = true;
                        break;
                    }
                }

                if (structure.autoStarted)
                    system->motorChange( _loadingState );
            }
            
            // LED status change
            if ((lines->iob ^ lines->iobOld) & 8)
                updateDeviceState();
            
        } else {
            // port A
            writeValue = lines->ioa;
        }
    };
        
    via2->readPort = [this]( Via::Port port, Via::Lines* lines ) {

        if (port == Via::Port::B) {

            // only bit 7 and 4 are input bits, all others reads 1 in input mode
            return ( (syncFound() | writeprotectSense() | 0x6f) & ~lines->ddrb)
                | (lines->prb & lines->ddrb); // output mode
        }

        // port A
        return (latchedByte & ~lines->ddra) | ( lines->pra & lines->ddra );
    };
    
    via2->ca2Out = [this]( bool direction ) {
        byteReadyOverflow = direction;
        if (!ca1Line && !byteReadyOverflow)
            via2->ca1In( ca1Line = true );
    };

    via2->cb2Out = [this]( bool state ) {
        readMode = state;
        updateDeviceState();
    };

    via1->ca2Out = [this]( bool direction ) {
        if (direction || (operation & DRIVE_HAS_PIA) || (operation & DRIVE_MODE_157x))
            return;

        system->writeParallelHandshake();
    };

    structure.write = [this](uint8_t* buffer, unsigned length, unsigned offset) {
		
		return system->interface->writeMedia( getMedia(), buffer, length, offset );
	};
    
    for(unsigned i = 0; i < motorOff.CHUNKS; i++)
        motorOff.chunkSize.push_back( 0 );
}

Drive::~Drive() {
    
    delete[] ram;
    delete[] ram20To3F;
    delete[] ram40To5F;
    delete[] ram60To7F;
    delete[] ram80To9F;
    delete[] ramA0ToBF;
    delete[] turboTrans;
    delete dummyTrack;
}

auto Drive::updateDeviceState() -> void {
    system->diskSilence.idleFrames = 0;
    system->interface->updateDeviceState( getMediaConnected(), !readMode, (side * MAX_TRACKS_1541 * 2) + currentHalftrack + 2, via2->lines.iob & 8, !motorOn );
}

// missing BUS communication
auto Drive::updateIdleDeviceState() -> void {
    
    system->interface->updateDeviceState( getMediaConnected(), !readMode, (side * MAX_TRACKS_1541 * 2) + currentHalftrack + 2, false, true );

    if (structure.autoStarted)
        system->motorChange( false );
}

auto Drive::updateBus() -> void {
    
    clockOut = !((via1->lines.iob >> 3) & 1);    
    dataOut =  !((via1->lines.iob >> 1) & 1);
    atnOut =  (via1->lines.iob >> 4) & 1;            

    if ( iecBus->atnOut == atnOut )
        dataOut = 0;
}

auto Drive::power( ) -> void {

    std::memset(ram, 0, 2 * 1024);
    if (expandMemory & (uint8_t)ExpandedMemMode::M20)
        std::memset(ram20To3F, 0, 8 * 1024);
    if (expandMemory & (uint8_t)ExpandedMemMode::M40)
        std::memset(ram40To5F, 0, 8 * 1024);
    if (expandMemory & (uint8_t)ExpandedMemMode::M60)
        std::memset(ram60To7F, 0, 8 * 1024);
    if (expandMemory & (uint8_t)ExpandedMemMode::M80)
        std::memset(ram80To9F, 0, 8 * 1024);
    if (expandMemory & (uint8_t)ExpandedMemMode::MA0)
        std::memset(ramA0ToBF, 0, 8 * 1024);

    if (speeder == 12)
        std::memset(turboTrans, 0, 512 * 1024);

    setFirmwareByType();

    via1->reset();
    via2->reset();
    cia->reset();
    ciaSpeeder->reset();
    pia->reset();
    wd1770->reset();

    proSpeedControl = 0xff;
    profDosAutoSpeed = false;
    prologic40TrackMode = false;
    turboTransVisible = 1;
    turboTransPage = 0;
    prologic2Mhz = 0;
    irqIncomming = 0;
    clockOut = dataOut = atnOut = 1;  
    cycleCounter = 0;
    speedZone = 0;
    byteReadyOverflow = false;
    readMode = true;
    byteReady = true;
    ca1Line = true;
    hidden = false;
    cpu->power();    
 
    ue7Counter = uf4Counter = 0;
    randCounter = 0;
    randomizer.initXorShift( 0x1234abcd );
    
    motorOn = true;
    motorOff.slowDown = false;
    readBuffer = writeBuffer = 0;
    writeValue = 0x55;
    latchedByte = 0x55;
    ue3Counter = 0;
    accum = 0;
    headOffset = 0;
    currentHalftrack = 17 * 2;
    coilDir = 0;
    structure.autoStarted = false;
    structure.serializationSize = 0;
    pulseIndex = -1;
    pulseDelta = 1;
    comperatorFlipFlop = false;
    uf6aFlipFlop = false;
    pulseDuration = 0;
    side = 0;
    dataDirection = true;
    syncPos = 0;
    nibble = 0;
    updateCycleSpeed(false);
    changeHalfTrack(0);
    randomizeRpm();
    extendedMemoryMap = expandMemory || (speeder > 1);
    wd1770->setRateInMhz( 1, 16 );

    if (system->driveSounds.useFloppy) {
        if (loaded && !iecBus->powerOn)
            system->interface->mixDriveSound(mediaConnected, DriveSound::FloppyInsert);

        system->interface->mixDriveSound( mediaConnected, DriveSound::FloppySpinUp );
    }
}

auto Drive::updateCycleSpeed(bool mhz2x, bool init) -> void {
    if (mhz2x) {
        //system->interface->log("2 mhz", 1);
        refCyclesInCpuCycle = 8;
        frequency = 2000000;
        if (!init) {
            cycleCounter *= 2;
            attachDelay <<= 1;
            stepperDelay <<= 1;
            driveCycles = frequency;
        }
        syncPosRead = (int64_t)(-0.875 * (double)iecBus->cpuCylcesPerSecond);
        syncPosWrite = (int64_t)(0.875 * (double)iecBus->cpuCylcesPerSecond);
        wd1770->setRateInMhz( 2, 16 );
    } else {
        //system->interface->log("1 mhz", 1);
        refCyclesInCpuCycle = 16;
        frequency = 1000000;
        if (!init) {
            cycleCounter /= 2;
            attachDelay >>= 1;
            stepperDelay >>= 1;
            driveCycles = frequency;
        }
        syncPosRead = (int64_t)(-0.455 * (double)iecBus->cpuCylcesPerSecond);
        syncPosWrite = (int64_t)(0.455 * (double)iecBus->cpuCylcesPerSecond);
        wd1770->setRateInMhz( 1, 16 );
    }

    setSyncPos( syncPos );
}

auto Drive::setSyncPos(int direction) -> void {
    if (direction < 0)
        syncPos = syncPosRead;
    else if (direction > 0)
        syncPos = syncPosWrite;
    else
        syncPos = 0;
}

auto Drive::powerOff( ) -> void {
    write();  
    motorOn = false;
    wd1770->setDiskAccessible(false);
}

auto Drive::setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void {

    if ( (size == 0) || ((size & (size - 1)) != 0) )
        data = nullptr;

    switch (typeId) {
        default:
        case Interface::FirmwareIdVC1541II:
            if (!data) {
                data = (uint8_t*) Firmware::drive1541IIRom;
                size = sizeof( Firmware::drive1541IIRom );
            }

            rom1541II = data;
            rom1541IISize = size;
            break;
        case Interface::FirmwareIdVC1541:
            if (!data) {
                data = (uint8_t*) Firmware::drive1541Rom;
                size = sizeof( Firmware::drive1541Rom );
            }
            rom1541 = data;
            rom1541Size = size;
            break;
        case Interface::FirmwareIdVC1541C:
            if (!data) {
                data = (uint8_t*) Firmware::drive1541CRom;
                size = sizeof( Firmware::drive1541CRom );
            }
            rom1541C = data;
            rom1541CSize = size;
            break;
        case Interface::FirmwareIdVC1571:
            if (!data) {
                data = (uint8_t*) Firmware::drive1571Rom;
                size = sizeof( Firmware::drive1571Rom );
            }
            rom1571 = data;
            rom1571Size = size;
            break;
        case Interface::FirmwareIdVC1570:
            if (!data) {
                data = (uint8_t*) Firmware::drive1570Rom;
                size = sizeof( Firmware::drive1570Rom );
            }
            rom1570 = data;
            rom1570Size = size;
            break;
        case Interface::FirmwareIdExpanded:
            romExpanded = data;
            romExpandedMask = size ? (size - 1) : 0;
            break;
    }
}

auto Drive::setViaTransition( bool direction ) -> void {
	
	// we need to check how much the drive is ahead of the c64.
    // if the drive is more than two cycles ahead we need to manually register
    // IRQ in CPU, because the drive cpu run a few cycles without knowing from interrupt.
    // NOTE: the drive CPU is interrupted before IRQ sample cycle.
    // so it can only pass opcode edge when not fully synced. means not the sample cycle is missable
    // but the recognition cycle.
    // for performance and code complexity reasons i have decided the drive CPU can only be interrupted
	// before read/write access and before an irq sample cycle,
	// but not during address generation or interrupt service routine. (because there is no VIA access)
	
	// we check by half cycles, hence CPU IRQ line must be stable during second half cycle for recognition

	int64_t half = iecBus->cpuCylcesPerSecond >> 1;

	if (cycleCounter >= (iecBus->cpuCylcesPerSecond + half)) {
		// expects CPU has missed IRQ recognition
		via1->ca1In( direction, false);
		via1->handleInterrupt();

	} else if (cycleCounter >= half )
		// expects IRQ recognition this cycle
		via1->ca1In( direction, false);

	else
		// expects IRQ recognition next cycle
		via1->ca1In( direction, true);
}

auto Drive::detach() -> void {
    write();
    
    if (loaded) {
        attachDelay = DISC_DELAY;
        if (iecBus->powerOn && system->driveSounds.useFloppy)
            system->interface->mixDriveSound( mediaConnected, DriveSound::FloppyEject );
    }

    if (iecBus->powerOn && use2Mhz() )
        attachDelay <<= 1;

    delayInProgress = attachDelay || stepperDelay;
    
    structure.detach();
    motorOff.slowDown = false;
    wasAttachDetached = false;
    
    loaded = false;
    wd1770->setDiskAccessible(false);
    pulseIndex = -1;
    pulseDelta = 1; // to reload quickly
}

auto Drive::attach( Emulator::Interface::Media* media, uint8_t* data, unsigned size, bool loadGracefully ) -> void {
    this->media = media;
    detach();
    accum = 0;
    randCounter = 0;
    uf6aFlipFlop = comperatorFlipFlop = false;
    uf4Counter = ue7Counter = 0;
    ue3Counter = 0;
    
	structure.media = media;

    wasAttachDetached = attachDelay != 0;
    attachDelay = DISC_DELAY * 3;

    if (iecBus->powerOn && use2Mhz() )
        attachDelay <<= 1;

    delayInProgress = attachDelay || stepperDelay;

    if (iecBus->powerOn && system->driveSounds.useFloppy)
        system->interface->mixDriveSound( mediaConnected, DriveSound::FloppyInsert, wasAttachDetached );

    if ( !structure.attach( data, size, loadGracefully ) )
        return;

    postAttach();
}

auto Drive::postAttach() -> void {
    headOffset = 0;
    pulseIndex = gcrTrack->firstPulse;
    wd1770->setPulseIndex(pulseIndex, pulseDelta);

    loaded = true;
    wd1770->setDiskAccessible(motorOn);

    if (writeProtected && (type == Type::D1570 || type == Type::D1571))
        via1->ca2In( false );

    operation &= ~(USERDATA_LEVEL | ENCODEDDATA_LEVEL | FLUXDATA_LEVEL);
    wd1770->setMode( WD1770::Mode::None );

    if (structure.type == DiskStructure::Type::D64 || structure.type == DiskStructure::Type::D71) {
        if (emulateDxxMoreAccurate)
            operation |= ENCODEDDATA_LEVEL;
        else
            operation |= USERDATA_LEVEL;

        // no MFM support
    } else if (structure.type == DiskStructure::Type::G64 || structure.type == DiskStructure::Type::G71) {
        operation |= ENCODEDDATA_LEVEL;
        wd1770->setMode( WD1770::Mode::USERDATA ); // MFM is included as user data
    } else if (structure.type == DiskStructure::Type::P64 || structure.type == DiskStructure::Type::P71) {
        operation |= FLUXDATA_LEVEL;
        wd1770->setMode( WD1770::Mode::FLUX );
    }
}

auto Drive::setWriteProtect(bool state) -> void {
    
    writeProtected = state;
    wd1770->setWriteProtected( state );
}

auto Drive::writeprotectSense() -> uint8_t {

    if (attachDelay) {
        if (wasAttachDetached) {
            unsigned compareDelay = DISC_DELAY;
            if (use2Mhz())
                compareDelay <<= 1;

            if ( (attachDelay > compareDelay) && (attachDelay < (compareDelay << 1)))
                return 0x10;
        }
        return 0;
    }
    
    if (!loaded)
        return 0x10;
    
    return writeProtected ? 0 : 0x10;
}

auto Drive::write() -> void {
    
    if (!written && !wd1770->wasWritten())
        return;
    
    written = false;
    wd1770->resetWritten();

    if (structure.serializationSize) {
        system->serializationSize -= structure.serializationSize;
        structure.serializationSize = 0;
    }
    
    if (!loaded)
        return;

    if (!system->interface->questionToWrite(media))
        return;
    
    structure.storeWrittenTracks();
}

auto Drive::setSpeed(unsigned rpmScaled) -> void {
    this->rpm = rpmScaled;
}

auto Drive::setWobble(unsigned wobbleScaled) -> void {
    this->wobble = wobbleScaled;
}

auto Drive::setStepperSeekTime( unsigned stepperSeekTimeScaled ) -> void {
    this->stepperSeekTime = stepperSeekTimeScaled * 100;
}

auto Drive::setType( Type type ) -> void {
    this->type = type;

    updateCycleSpeed(false);

    operation &= ~(DRIVE_MODE_154x | DRIVE_MODE_157x);

    if (type == Type::D1541II || type == Type::D1541 || type == Type::D1541C)
        operation |= DRIVE_MODE_154x;

    else if (type == Type::D1571 || type == Type::D1570)
        operation |= DRIVE_MODE_157x;

    setFirmwareByType();
}

auto Drive::setFirmwareByType( ) -> void {
    switch (type) {
        default:
        case Type::D1541II:
            rom = rom1541II;
            romMask = rom1541IISize - 1;
            break;
        case Type::D1541:
            rom = rom1541;
            romMask = rom1541Size - 1;
            break;
        case Type::D1541C:
            rom = rom1541C;
            romMask = rom1541CSize - 1;
            break;
        case Type::D1571:
            rom = rom1571;
            romMask = rom1571Size - 1;
            break;
        case Type::D1570:
            rom = rom1570;
            romMask = rom1570Size - 1;
            break;
    }

    if (!romExpanded) {
        romExpanded = rom;
        romExpandedMask = romMask;
    }
}

auto Drive::setExpandedMemory( ExpandedMemMode& expandedMemMode, bool state ) -> void {

    if (state) {
        expandMemory |= (uint8_t)expandedMemMode;
    } else {
        expandMemory &= ~((uint8_t)expandedMemMode);
    }

    extendedMemoryMap = expandMemory || (speeder > 1);
}

auto Drive::setSpeeder(uint8_t speeder) -> void {

    this->speeder = speeder;

    extendedMemoryMap = expandMemory || (speeder > 1);

    operation &= ~(DRIVE_HAS_PIA | DRIVE_HAS_EXTRA_CIA);

    if (speeder == 4 || speeder == 5 || speeder == 10)
        operation |= DRIVE_HAS_PIA;

    if (speeder == 13)
        operation |= DRIVE_HAS_EXTRA_CIA;
}

auto Drive::hide() -> void {
    hidden = true;
}

}
