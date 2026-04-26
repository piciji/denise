
#include "drive.h"
#include "../iec.h"
#include "../../system/system.h"
#include "mechanics.cpp"
#include "mechanicsP64.cpp"
#include "mechanicsG64.cpp"
#include "profdos.cpp"
#include "prologic.cpp"
#include "turbotrans.cpp"
#include "serialization.cpp"
#include "../../system/firmware.h"
#include "../../../tools/gcr.h"
#include "../../../tools/memory.h"

// for 300 rpm = 5 rotation / sec = 16.000.000 / 5

namespace LIBC64 {

// valid for 1 MHz operation
// one cpu cycle is 16 reference(drive) cycles.
// we do only progress 6 instead of 8 in the first half-cycle, because of a possible
// external overflow is recognized by CPU within 400 ns.
 
// in the case of a VIA READ, there is more time a change can be read back ~ 875 ns within a cycle.
// 6 ref cycles are progressed in the first half-cycle already, so we need 8 more to get 14 of 16 ref cycles.
    
// for each cycle:

// 6 ref cycles: heck for external overflow
// + 8
// 14 ref cycles: VIA2 read back Changes
// + 2
// 16 ref cycles: complete cycle
// + 6
// repeat this pattern    

// the distance between "overflow" checking and maximum "Read back" time is 8 ref cycles, phase shifted by 2 ref cycles.
// the relative	distance matters, so we can step in 8 ref cycle chunks that are handled in rotateP64 and rotateG64

unsigned Drive::rpm = 30000;
unsigned Drive::wobble = 50;
int Drive::wobblePos = 0;
int Drive::wobbleLimit = 25;
uint32_t Drive::refCyclesPerRevolution = 0;
bool Drive::Mechanics::enabled = false;
uint16_t Drive::Mechanics::acceleration = 0;
uint16_t Drive::Mechanics::deceleration = 0;
uint16_t Drive::Mechanics::stepperSeekTime = 0;
Drive::Type Drive::globalType = Drive::Type::D1541II;

#define SYNC_ROTATE_1541 \
    if (operation & DECODEDDATA_LEVEL) rotateDecoded();    \
    else if (operation & ENCODEDDATA_LEVEL) rotateEncoded();    \
    else rotateFlux();

#define SYNC_ROTATE_1571 \
    if (operation & DECODEDDATA_LEVEL) rotateDecoded<true>();   \
    else if (operation & ENCODEDDATA_LEVEL) rotateEncoded<true>();  \
    else rotateFlux<true>();

#define SYNC_ROTATE_1581 \
if (!mechanics.motorDelay) { \
    if (operation & DECODEDDATA_LEVEL) wd1770.rotateDecoded(motorOn ? refCyclesPerRevolution : 0); \
    else if (operation & ENCODEDDATA_LEVEL) wd1770.rotateEncoded(motorOn ? refCyclesPerRevolution : 0); \
    else wd1770.rotateFlux(motorOn ? refCyclesPerRevolution : 0); \
} else { \
    unsigned _cycles = refCyclesPerRevolution; \
    if(!motorRun(_cycles)) _cycles = 0; \
    if (operation & DECODEDDATA_LEVEL) wd1770.rotateDecoded(_cycles); \
    else if (operation & ENCODEDDATA_LEVEL) wd1770.rotateEncoded(_cycles); \
    else wd1770.rotateFlux(_cycles); \
}

#define SYNC_TAIL   \
    cycleCounter += iecBus.cpuCylcesPerSecond;  \
    if (delayInProgress)    \
        progressDelay();

#define SYNC_1541 \
    cpu.handleSo(); \
    SYNC_ROTATE_1541 \
    via1.process(); \
    via2.process(); \
    SYNC_TAIL

#define SYNC_1571 \
    cpu.handleSo(); \
    SYNC_ROTATE_1571 \
    via1.process(); \
    via2.process(); \
    cia.clock();    \
    if (operation & DRIVE_HAS_EXTRA_CIA)    \
        ciaSpeeder.clock(); \
    SYNC_TAIL

#define SYNC_1581 \
    SYNC_ROTATE_1581 \
    cia.clock(); \
    SYNC_TAIL
    
auto Drive::progressDelay() -> void {
    if (attachDelay) {
        if (--attachDelay == 0) {
            delayInProgress = mechanics.stepperDelay;
        }
    }

    if (mechanics.stepperDelay) {
        if (--mechanics.stepperDelay == 0) {
            changeHalfTrack(nextStep);
            delayInProgress = attachDelay;
        }
    }
}

auto Drive::sync() -> void {
    if (operation & DRIVE_MODE_154x) {
        SYNC_1541
    } else if (operation & DRIVE_MODE_157x) {
        SYNC_1571
    } else { // DRIVE_MODE_158x
        SYNC_1581
    }
}

auto Drive::cpuWrite(uint16_t addr, uint8_t data) -> void {
    if (operation & DRIVE_MODE_154x) {
        SYNC_1541

        if (extendedMemoryMap) {
            if (speeder == 4) { // dolphin v3
                if ((addr & 0xf000) == 0x5000) {
                    pia.write( addr & 3, data );
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
                        pia.write( addr, data );
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
            via1.write(addr, data);

        else if ((addr & 0x9c00) == 0x1c00)
            via2.write(addr, data);

    } else if (operation & DRIVE_MODE_157x) {
        SYNC_1571

        if (extendedMemoryMap) {
            if (speeder == 5) { // dolphin v3
                if ((addr & 0xf000) == 0x5000) {
                    pia.write( addr & 3, data );
                    return;
                }
            } else if (speeder == 13) {
                if ((addr & 0xfff0) == 0x9e20) {
                    ciaSpeeder.write(addr, data);
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
            via1.write(addr, data);

        else if ((addr & 0xfc00) == 0x1c00) {
            byteReady = false;
            via2.write(addr, data);

        } else if ((addr & 0xc000) == 0x4000) {
            cia.write(addr, data);

        } else if ((addr & 0xe000) == 0x2000) {
            wd1770.write(addr, data);
        }
    } else { // DRIVE_MODE_158x
        SYNC_1581

        if ((addr & 0xe000) == 0)
            ram[addr & 0x1fff] = data;

        else if ((addr & 0xf000) == 0x4000) {
            cia.write(addr, data);

        } else if ((addr & 0xf000) == 0x6000) {
            wd1770.write(addr, data);
        }
    }
}

template<bool peek> auto Drive::cpuRead(uint16_t addr) -> uint8_t {
    if (operation & DRIVE_MODE_154x) {
        if constexpr (!peek) {
            SYNC_1541
        }

        if (extendedMemoryMap) {
            if (speeder == 4) {
                if ((addr & 0xf000) == 0x5000) {
                    return peek ? pia.peek( addr & 3 ) : pia.read( addr & 3 );
                }
            } else if (speeder == 6) {
                if (!peek & profDosAutoSpeed)
                    profDosAutoClockControl(addr);

                if((addr & 0xe000) == 0x8000) {
                    return readProfDosEncoderV1(addr);
                }
                if((addr & 0xe000) == 0xe000) {
                    return this->romExpanded[ (addr & 0x1fff) & romExpandedMask];
                }

            } else if (speeder == 7) {
                if (!peek & profDosAutoSpeed)
                    profDosAutoClockControl(addr);

                if((addr & 0xe000) == 0x6000) {
                    return readProfDosEncoder(addr);
                }
                if((addr & 0xe000) == 0xe000) {
                    return this->romExpanded[ (0x2000 | (addr & 0x1fff)) & romExpandedMask ];
                }
            } else if (speeder == 10) {
                if ((addr & 0xfff0) == 0xb800) {
                    addr = (addr >> 2) & 3;

                    if (addr & 2)
                        return peek ? pia.peek( addr ) : pia.read( addr );
                    if ((addr & 1) == 0) {
                        return prologic2Mhz;
                    }
                }

                if (((addr & 0xf000) == 0xa000) || ((addr & 0xf800) == 0xb000)) { // a0 - b7
                    return this->romExpanded[ (addr & 0x1fff) & romExpandedMask ];
                }
                if (((addr & 0xf000) == 0xe000) || ((addr & 0xf800) == 0xf000)) { // e0 - f7
                    return this->romExpanded[ (0x2000 | (addr & 0x1fff)) & romExpandedMask];
                }
                if ((addr & 0xf800) == 0xf800) {
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
                if (((addr & 0xf000) == 0xe000) || ((addr & 0xf800) == 0xf000)) { // e0 - f7
                    out = this->romExpanded[ (0x2000 | (addr & 0x1fff)) & romExpandedMask];
                    return (out & ~0xa0) | ((out & 0x20) << 2) | ((out & 0x80) >> 2);
                }
                if ((addr & 0xf800) == 0xf800) {
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
                    } if ((addr & 0xf800) == 0x7000) {
                        return turboTrans[ 0x40000 | ((turboTransPage << 10) | (addr & 0x3ff))];
                    }
                }

                if (turboTransVisible & 1) {
                    if ((addr & 0xe000) == 0xc000) {
                        return this->romExpanded[ (addr & 0x1fff) & romExpandedMask];
                    } if ((addr & 0xe000) == 0x8000) {
                        return this->romExpanded[ (0x4000 | (addr & 0x1fff)) & romExpandedMask];
                    } if ((addr & 0xe000) == 0xe000) {
                        return this->romExpanded[ (0x6000 | (addr & 0x1fff)) & romExpandedMask];
                    } if ((addr & 0xe000) == 0xa000) {
                        if (expandMemory & (uint8_t) ExpandedMemMode::MA0)
                            return this->ramA0ToBF[addr & 0x1fff];

                        return this->romExpanded[ (0x2000 | (addr & 0x1fff)) & romExpandedMask];
                    }
                } else if ((addr & 0xe000) == 0xa000) {
                    // disable RAM by software, avoid expand mem usage
                    return rom[addr & romMask];
                }
            } else if (speeder == 14) {
                if ((addr & 0xe000) == 0xa000) {
                    return this->romExpanded[ (addr & 0x1fff) & romExpandedMask];
                }
            } else if (speeder == 15) {
                if ((addr & 0xf800) == 0x1000) {
                    return this->romExpanded[ (addr & 0x7ff) & romExpandedMask];
                }
            }

            if ((expandMemory & (uint8_t) ExpandedMemMode::M80) && ((addr & 0xe000) == 0x8000))
                return this->ram80To9F[addr & 0x1fff];
            if ((expandMemory & (uint8_t) ExpandedMemMode::M40) && ((addr & 0xe000) == 0x4000))
                return this->ram40To5F[addr & 0x1fff];
            if ((expandMemory & (uint8_t) ExpandedMemMode::M60) && ((addr & 0xe000) == 0x6000))
                return this->ram60To7F[addr & 0x1fff];
            if ((expandMemory & (uint8_t) ExpandedMemMode::MA0) && ((addr & 0xe000) == 0xA000))
                return this->ramA0ToBF[addr & 0x1fff];
            if ((expandMemory & (uint8_t) ExpandedMemMode::M20) && ((addr & 0xe000) == 0x2000))
                return this->ram20To3F[addr & 0x1fff];
        }

        if (addr & 0x8000)
           return rom[addr & romMask];

        if ((addr & 0x9800) == 0)
            return ram[addr & 0x7ff];

        if ((addr & 0x9c00) == 0x1800)
            return peek ? via1.peek(addr) : via1.read(addr);

        if ((addr & 0x9c00) == 0x1c00)
            return peek ? via2.peek(addr) : via2.read(addr);

        return cpu.dataBus;
    }

    if (operation & DRIVE_MODE_157x) {
        if constexpr (!peek) {
            SYNC_1571
        }
        if (extendedMemoryMap) {
            if (speeder == 5) {
                if ((addr & 0xf000) == 0x5000) {
                    return peek ? pia.peek( addr & 3 ) : pia.read( addr & 3 );
                }
            }
            else if (speeder == 8 || speeder == 9) { // profdos R5, R6
                if ((addr & 0xe000) == 0x6000)
                    return readProfDosEncoder( addr );
            }
            else if (speeder == 13) { // proSpeed 1571

                if (proSpeedControl & (1 | 2 | 0x80)) {

                    if ((addr & 0xfff0) == 0x9e20) {
                        return peek ? ciaSpeeder.peek(addr) : ciaSpeeder.read(addr);
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
            } else if (speeder == 15) {
                if ((addr & 0xf800) == 0x1000) {
                    return this->romExpanded[ (addr & 0x7ff) & romExpandedMask];
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

        if ((addr & 0xf000) == 0)
            return ram[addr & 0x7ff];

        if ((addr & 0xfc00) == 0x1800)
            return peek ? via1.peek(addr) : via1.read(addr);

        if ((addr & 0xfc00) == 0x1c00) {
            // TED line of U6 clears the Byte line in 2 Mhz mode.
            // Line is connected to Chip select of VIA 2. any access of VIA2 clears the line.
            if constexpr (peek) {
                return via2.peek(addr);
            } else {
                byteReady = false;
                return via2.read(addr);
            }
        }
        if ((addr & 0xc000) == 0x4000)
            return peek ? cia.peek(addr) : cia.read(addr);

        if ((addr & 0xe000) == 0x2000)
            return peek ? wd1770.peek(addr) : wd1770.read(addr);

    } else { // DRIVE_MODE_158x
        if constexpr (!peek) {
            SYNC_1581
        }
        if (addr & 0x8000)
            return rom[addr & romMask];

        if ((addr & 0xe000) == 0)
            return ram[addr & 0x1fff];

        if ((addr & 0xf000) == 0x4000)
            return peek ? cia.peek(addr) : cia.read(addr);

        if ((addr & 0xf000) == 0x6000)
            return peek ? wd1770.peek(addr) : wd1770.read(addr);
    }

    return cpu.dataBus;
}

Drive::Drive(uint8_t number, System* system, IecBus& iecBus, Emulator::Interface::Media* media ) :
via1(1),
via2(2),
cia(3),
ciaSpeeder(4),
cpu(system, this),
system(system),
iecBus(iecBus),
structure(system, this) {
     
    this->number = number; 
	this->media = media;
    structure.media = media;
    gcrTrack = nullptr;
    headOffset = 0;

	structure.number = number;
	type = Type::D1541II;
	operation = DECODEDDATA_LEVEL;
    expandMemory = 0;
    speeder = 0;
    profDosAutoSpeed = 0;
    extendedMemoryMap = false;

    wasAttachDetached = false;
    mechanics.stepperDelay = 0;
    mechanics.motorDelay = 0;
    mechanics.refCycles = 0;

    delayInProgress = false;
    motorOn = false;
    hidden = false;
    dskChange = true;
    trackZeroSensor = false;

    frequency = 1000000;
    refCyclesInCpuCycle = 16;
    
    ram = new uint8_t[ 8 * 1024 ]; // 154x, 157x need 2k, 1581 need 8k
    ram20To3F = new uint8_t[ 8 * 1024 ];
    ram40To5F = new uint8_t[ 8 * 1024 ];
    ram60To7F = new uint8_t[ 8 * 1024 ];
    ram80To9F = new uint8_t[ 8 * 1024 ];
    ramA0ToBF = new uint8_t[ 8 * 1024 ];

    rom1541IISize = sizeof( Firmware::drive1541IIRom );
    Emulator::copyMemory<uint8_t>( rom1541II, rom1541IISize, (uint8_t*)Firmware::drive1541IIRom, rom1541IISize );

    rom = rom1541II;
    romMask = rom1541IISize - 1;

    pia.ca2Out = [this, system](bool direction) {
        if (direction)
            return;

        if (speeder == 4 || speeder == 5) {
            system->writeParallelHandshake();
        }
    };

    pia.cb2Out = [this, system](bool direction) {
        if (direction)
            return;

        if (speeder == 10) {
            system->writeParallelHandshake();
        }
    };

    pia.readPort = [this, system]( Emulator::Pia::Port port ) {

        if (port == Emulator::Pia::Port::A) {
            if (speeder == 4 || speeder == 5) {
                if (!system->secondDriveCable.parallelUse)
                    return this->pia.ioa;
            } else
                return this->pia.ioa;
        } else {
            if (speeder == 10) {
                if (!system->secondDriveCable.parallelUse)
                    return this->pia.iob;
            } else
                return this->pia.iob;
        }

        uint8_t out = system->readParallel();

        for (auto drive : system->iecBus.drivesEnabled) {
            if (port == Emulator::Pia::Port::A)
                out &= drive->pia.ioa;
            else
                out &= drive->pia.iob;
        }
        return out;
    };

    pia.peekPort = [this, system]( Emulator::Pia::Port port ) {
        return pia.readPort (port);
    };

    pia.writePort = [this]( Emulator::Pia::Port port, uint8_t data ) {
        // nothing todo here, because CA(B)2 is triggered and port value is latched on pins
    };

    // proSpeed 1571 v2.0 has extra CIA
    ciaSpeeder.writePort = [this, system]( Cia<MOS_8520>::Port port, Cia<MOS_8520>::Lines* lines ) {

        if ( lines->prbChange && (port == Cia<MOS_8520>::PORTB )) {
            system->writeParallelHandshake();
        }
        else if (port == Cia<MOS_8520>::PORTA ) {
            proSpeedControl = lines->ioa;
        }
    };

    ciaSpeeder.readPort = [this, system]( Cia<MOS_8520>::Port port, Cia<MOS_8520>::Lines* lines ) {

        if ( port == Cia<MOS_8520>::PORTB ) {
            if (!system->secondDriveCable.parallelUse )
                return lines->iob;

            uint8_t out = system->readParallelWithHandshake();

            for (auto drive : system->iecBus.drivesEnabled) {
                out &= drive->ciaSpeeder.lines.iob;
            }

            return out;
        }

        return lines->ioa;
    };

    ciaSpeeder.peekPort = [this, system]( Cia<MOS_8520>::Port port, Cia<MOS_8520>::Lines* lines ) {
        if ( port == Cia<MOS_8520>::PORTB ) {
            if (!system->secondDriveCable.parallelUse )
                return lines->iob;

            uint8_t out = system->readParallel();

            for (auto drive : system->iecBus.drivesEnabled) {
                out &= drive->ciaSpeeder.lines.iob;
            }

            return out;
        }

        return lines->ioa;
    };

    cia.serialOut = [this, system](bool spLine, bool cntLine) {
        if (dataDirection) {
              if(!cntLine && system->secondDriveCable.burstUse) {
                dataOut = spLine;
                this->iecBus.updatePort();               
                // do a meaningful kind of fast serial for C64 without burst modification
                system->cia1.setFlag(); // C64 SRQIN shared with cassette
                system->cia1.serialIn(spLine);
            }
        }
    };

    cia.writePort = [this, system]( Cia<MOS_8520>::Port port, Cia<MOS_8520>::Lines* lines ) {

        if ( lines->prbChange && (port == Cia<MOS_8520>::PORTB )) {

            if (operation & DRIVE_MODE_158x) {
                dataDirection = !!(cia.lines.iob & 0x20);
                updateCiaBus();                
                this->iecBus.updatePort();
            } else if ((operation & (DRIVE_HAS_PIA | DRIVE_HAS_EXTRA_CIA) ) == 0)
                system->writeParallelHandshake();
        }

        else if (port == Cia<MOS_8520>::PORTA ) {
            if (operation & DRIVE_MODE_158x) {
                if ((lines->ioa ^ lines->ioaOld) & 4) {
                    motorOn = (lines->ioa & 4) == 0;
                    motorChangeInit();

                    if (system->driveSounds.useFloppy)
                        system->interface->mixDriveSound( this->media, motorOn ? DriveSound::FloppySpinUp : DriveSound::FloppySpinDown, true );

                    if (structure.autoStarted)
                        system->hintObserverMotorChange(this->iecBus.runningDrives());

                    updateDeviceState1581();
                }

                if ((lines->ioa ^ lines->ioaOld) & 0x40) { // act LED change
                    updateDeviceState1581();
                    if (structure.autoStarted)
                        system->hintObserverLEDChange(lines->ioa & 0x40);
                }

                if ((lines->ioa ^ lines->ioaOld) & 1) {
                    side = 1 - (lines->ioa & 1);
                    changeHalfTrack(0);
                }
            }
        }
    };

    cia.readPort = [this, system]( Cia<MOS_8520>::Port port, Cia<MOS_8520>::Lines* lines ) {
        uint8_t out;
        if ( port == Cia<MOS_8520>::PORTB ) {

            if (operation & DRIVE_MODE_158x)
                return (uint8_t)( (((lines->iob & 0x1a) | this->iecBus.readPort()) ^ 0x85) | (writeProtected ? 0 : 0x40) );

            if (!system->secondDriveCable.parallelUse || (operation & (DRIVE_HAS_PIA | DRIVE_HAS_EXTRA_CIA)) )
                return lines->iob;

            out = system->readParallelWithHandshake();

            for (auto drive : system->iecBus.drivesEnabled) {
                out &= drive->cia.lines.iob;
            }

            return out;
        }

        if (operation & DRIVE_MODE_158x) {
            out = this->number << 3;

           if (!motorOn) // RDY: todo: check for minimum delay when motor switching off (like Amiga)
                out |= 2;
            if (!dskChange) // DSK CHANGE: todo: check for minimum delay in case of stepping too fast after inserting disk (like Amiga)
                out |= 0x80;

            out = (lines->pra & lines->ddra) | (out & ~lines->ddra);
            return out;
        }

        return lines->ioa;
    };

    cia.peekPort = [this, system]( Cia<MOS_8520>::Port port, Cia<MOS_8520>::Lines* lines ) {
        uint8_t out;
        if ( port == Cia<MOS_8520>::PORTB ) {

            if (operation & DRIVE_MODE_158x)
                return (uint8_t)( (((lines->iob & 0x1a) | this->iecBus.readPort()) ^ 0x85) | (writeProtected ? 0 : 0x40) );

            if (!system->secondDriveCable.parallelUse || (operation & (DRIVE_HAS_PIA | DRIVE_HAS_EXTRA_CIA)) )
                return lines->iob;

            out = system->readParallel();

            for (auto drive : system->iecBus.drivesEnabled) {
                out &= drive->cia.lines.iob;
            }

            return out;
        }

        if (operation & DRIVE_MODE_158x) {
            out = this->number << 3;

            if (!motorOn) // RDY: todo: check for minimum delay when motor switching off (like Amiga)
                out |= 2;
            if (!dskChange) // DSK CHANGE: todo: check for minimum delay in case of stepping too fast after inserting disk (like Amiga)
                out |= 0x80;

            out = (lines->pra & lines->ddra) | (out & ~lines->ddra);
            return out;
        }

        return lines->ioa;
    };

    wd1770.stepCall = [this](bool direction) {
        if (this->operation & DRIVE_MODE_158x) { // step/dir are not connected for 1571
            if (loaded)
                dskChange = false;

            if (!direction) {
                if (currentHalftrack > 0)
                    currentHalftrack--;

            } else {
                if (currentHalftrack < MAX_TRACKS_1581 )
                    currentHalftrack++;
            }

            if (this->system->driveSounds.useFloppy)
                this->system->interface->mixDriveSound( this->media, DriveSound::FloppyStep, true, currentHalftrack );

            changeHalfTrack(0);

            wd1770.setTrackZero( currentHalftrack == 0 );
        }
    };

    wd1770.toggleWrite = [this]() {
        if (this->operation & DRIVE_MODE_158x)
            updateDeviceState1581();
        else
            updateDeviceState();
    };

    cia.irqCall = [this](bool state) {
        if (state)
            irqIncomming |= 4;
        else
            irqIncomming &= ~4;

        cpu.setIrq( irqIncomming != 0 );
    };

    via1.irqCall = [this](bool state) {
        if (state)
            irqIncomming |= 1;
        else
            irqIncomming &= ~1;

        cpu.setIrq( irqIncomming != 0 );
    };
    
    via2.irqCall = [this](bool state) {
		if (state)
			irqIncomming |= 2;
		else
			irqIncomming &= ~2; 
		
        cpu.setIrq( irqIncomming != 0 );
    };

    //PB 7, CB2: ATN IN
    //PB 6,5: Device address preset switches
    //PB 4:	ATN acknowledge OUT
    //PB 3:	CLOCK OUT
    //PB 2:	CLOCK IN
    //PB 1:	DATA OUT
    //PB 0:	DATA IN
    
    via1.writePort = [this, system]( Via::Port port, Via::Lines* lines ) {
        
        if (port == Via::Port::B) {
            
            if (lines->iob != lines->iobOld) {
            
                updateViaBus();
                                            
                system->iecBus.updatePort();
            }
        } else {

            if (operation & DRIVE_MODE_157x) {
                dataDirection = !!(lines->ioa & 2);

                if ((lines->ioa ^ lines->ioaOld) & 0x20) {
                    updateCycleSpeed(lines->ioa & 0x20, false);
                }

                uint8_t _side = side;
                side = !!(lines->ioa & 4);

                if (!structure.hasSecondSide() || (type == Type::D1570))
                    side = 0;

                if (side != _side) {
                    changeHalfTrack(0);
                }
            }
            // nothing todo here for 1541 parallel cable mode, because CA2 is triggered in VIA core
        }
    };   
    
    via1.readPort = [this, system]( Via::Port port, Via::Lines* lines ) {
        
        if (port == Via::Port::B) {
            // invert the three input bits, add device number  
            return (uint8_t)( ((0x1a | this->iecBus.readPort()) ^ 0x85) | (this->number << 5) );
        }

        // port A
        if (operation & DRIVE_MODE_157x) {
            return (uint8_t) ((((byteReady ? 0 : 0x80) | ((currentHalftrack == 0) ? 0 : 1) | 0x7e) & ~lines->ddra) |
                (lines->pra & lines->ddra));
        }

        if (system->secondDriveCable.parallelUse && ((operation & DRIVE_HAS_PIA) == 0) ) {
            uint8_t out = system->readParallel();

            for (auto drive : system->iecBus.drivesEnabled) {
                out &= drive->via1.lines.ioa;
            }

            return out;

        } else if (type == Type::D1541C) {
            return (uint8_t)(((((trackZeroSensor && (currentHalftrack == 0)) ? 1 : 0) | 0xfe) & ~lines->ddra) | (lines->pra & lines->ddra));
        }

        return lines->ioa;
    };

    via1.peekPort = [system, this]( Via::Port port, Via::Lines* lines ) {
        return via1.readPort( port, lines );
    };
    
    via2.writePort = [this, system]( Via::Port port, Via::Lines* lines ) {
        
        if (port == Via::Port::B) {
            
            if (lines->iob & 4) { // stepper motor works only when drive motor is active

                if (mechanics.stepperDelay) {
                    bool headBang = (currentHalftrack == 0) && ((nextStep == 3) || ( (nextStep == 2) && !coilDir));

                    updateStepper( nextStep );

                    if (system->driveSounds.useFloppy)
                        stepSound( headBang );
                }

                uint8_t _step = ((lines->iob & 3) - (currentHalftrack & 3)) & 3;

                if (_step != 0) {

                    if (!Mechanics::stepperSeekTime || !Mechanics::enabled) {
                        changeHalfTrack(_step);
                    } else {
                        nextStep = _step;
                        mechanics.stepperDelay = Mechanics::stepperSeekTime;
                        if ( use2Mhz() )
                            mechanics.stepperDelay <<= 1;

                        delayInProgress = true;
                    }
                } else if (mechanics.stepperDelay) {
                    mechanics.stepperDelay = 0;
                    changeHalfTrack(0);
                }
            }                            
            
            speedZone = (lines->iob >> 5) & 3;                        

            if ((lines->iob ^ lines->iobOld) & 4) {
                // motor switched between on/off 
                motorOn = (lines->iob & 4) != 0;
                //wd1770.setDiskAccessible(motorOn & loaded);
                motorChangeInit();

                if (system->driveSounds.useFloppy)
                    system->interface->mixDriveSound( this->media, motorOn ? DriveSound::FloppySpinUp : DriveSound::FloppySpinDown );
                
                updateDeviceState();

                if (structure.autoStarted)
                    system->hintObserverMotorChange(this->iecBus.runningDrives());
            }
            
            // LED status change
            if ((lines->iob ^ lines->iobOld) & 8) {
                updateDeviceState();
                if (structure.autoStarted)
                    system->hintObserverLEDChange(lines->iob & 8);
            }
            
        } else {
            // port A
            writeValue = lines->ioa;
        }
    };
        
    via2.readPort = [this]( Via::Port port, Via::Lines* lines ) {

        if (port == Via::Port::B) {

            // only bit 7 and 4 are input bits, all others reads 1 in input mode
            return ( (syncFound() | writeprotectSense() | 0x6f) & ~lines->ddrb)
                | (lines->prb & lines->ddrb); // output mode
        }

        // port A
        return (latchedByte & ~lines->ddra) | ( lines->pra & lines->ddra );
    };

    via2.peekPort = [system, this]( Via::Port port, Via::Lines* lines ) {
        return via2.readPort( port, lines );
    };
    
    via2.ca2Out = [this]( bool direction ) {
        byteReadyOverflow = direction;
        if (!ca1Line && !byteReadyOverflow)
            via2.ca1In( ca1Line = true );
    };

    via2.cb2Out = [this]( bool state ) {
        readMode = state;
        updateDeviceState();
    };

    via1.ca2Out = [this, system]( bool direction ) {
        if (direction || (operation & DRIVE_HAS_PIA) || (operation & DRIVE_MODE_157x))
            return;

        system->writeParallelHandshake();
    };

    structure.write = [this, system](uint8_t* buffer, unsigned length, unsigned offset) {
		
		return system->interface->writeMedia( this->media, buffer, length, offset );
	};
}

Drive::~Drive() {
    
    delete[] ram;
    delete[] ram20To3F;
    delete[] ram40To5F;
    delete[] ram60To7F;
    delete[] ram80To9F;
    delete[] ramA0ToBF;
    if (turboTrans)
        delete[] turboTrans;
}

auto Drive::updateDeviceState(bool forceOff) -> void {
    system->diskSilence.idleFrames = 0;
    uint8_t _led = (!forceOff && (via2.lines.iob & 8)) ? 1 : 0;
    if (_led && (type == Type::D1570 || type == Type::D1541 || type == Type::D1541C))
        _led = 2;

    bool _motorOff = forceOff ? true : !motorOn;
    system->interface->updateDeviceState( media, !readMode || wd1770.writeMode(), 0x8000 | ((side * MAX_TRACKS_1541 * 2) + currentHalftrack + 2), _led, _motorOff );
}

auto Drive::updateDeviceState1581(bool forceOff) -> void {
    system->diskSilence.idleFrames = 0;
    uint8_t _led = (!forceOff && (cia.lines.ioa & 0x40)) ? 1 : 0;
    bool _motorOff = forceOff ? true : !motorOn;
    system->interface->updateDeviceState( media, wd1770.writeMode(), ((currentHalftrack + 1) << 1) | side, _led, _motorOff );
}

// missing BUS communication
auto Drive::updateIdleDeviceState() -> void {
    (operation & DRIVE_MODE_158x) ? updateDeviceState1581(true) : updateDeviceState(true);

    if (structure.autoStarted)
        system->hintObserverMotorChange( false );

    if (system->driveSounds.useFloppy)
        system->interface->mixDriveSound( this->media, DriveSound::FloppySpinDown, operation & DRIVE_MODE_158x );
}

auto Drive::updateViaBus() -> void {
    clockOut = !((via1.lines.iob >> 3) & 1);
    dataOut =  !((via1.lines.iob >> 1) & 1);
    atnOut =  (via1.lines.iob >> 4) & 1;

    if ( iecBus.atnOut == atnOut )
        dataOut = false;
}

auto Drive::updateCiaBus() -> void {
    clockOut = !((cia.lines.iob >> 3) & 1);
    dataOut =  !((cia.lines.iob >> 1) & 1);
    atnOut =  (cia.lines.iob >> 4) & 1;

    if ( !iecBus.atnOut && atnOut )
        dataOut = false;
}

auto Drive::power( ) -> void {

    std::memset(ram, 0, 8 * 1024);
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

    if (turboTrans && (speeder == 12))
        std::memset(turboTrans, 0, 512 * 1024);

    setFirmwareByType();

    via1.reset();
    via2.reset();
    cia.reset();
    ciaSpeeder.reset();
    pia.reset();
    wd1770.reset();

    proSpeedControl = 0xff;
    profDosAutoSpeed = false;
    prologic40TrackMode = false;
    turboTransVisible = 1;
    turboTransPage = 0;
    prologic2Mhz = 0;
    irqIncomming = 0;
    clockOut = dataOut = atnOut = true;
    cycleCounter = 0;
    speedZone = 0;
    byteReadyOverflow = false;
    readMode = true;
    byteReady = true;
    ca1Line = true;
    hidden = false;

    wobblePos = 0;
    wobbleLimit = wobble >> 1;
 
    ue7Counter = uf4Counter = 0;
    randCounter = 0;
    randomizer.initXorShift( 0x1234abcd );
    
    motorOn = (operation & DRIVE_MODE_158x) ? false : true;
    mechanics.stepperDelay = 0;
    mechanics.motorDelay = 0;
    readBuffer = writeBuffer = 0;
    writeValue = 0x55;
    latchedByte = 0x55;
    ue3Counter = 0;
    accum = 0;
    //headOffset = 0;
    currentHalftrack = (type == Type::D1581) ? 40 : (17 * 2);
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
    updateCycleSpeed(type == Type::D1581);
    changeHalfTrack(0);
    driveCycles = (30000ULL * frequency) / rpm;
    refCyclesPerRevolution = (30000ULL * CyclesPerRevolution300Rpm) / rpm;

    extendedMemoryMap = expandMemory || (speeder > 1);
    wd1770.set2Mhz(type == Type::D1581);
    cpu.power();

    if (system->driveSounds.useFloppy) {
        if (loaded && !iecBus.powerOn)
            system->interface->mixDriveSound(media, DriveSound::FloppyInsert, operation & DRIVE_MODE_158x);

        if (motorOn)
            system->interface->mixDriveSound( media, DriveSound::FloppySpinUp, operation & DRIVE_MODE_158x );
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
            mechanics.stepperDelay <<= 1;
            driveCycles = frequency;
        }
        syncPosRead = (int64_t)(-0.875 * (double)iecBus.cpuCylcesPerSecond);
        syncPosWrite = (int64_t)(0.875 * (double)iecBus.cpuCylcesPerSecond);
        wd1770.set2Mhz( true);
    } else {
        //system->interface->log("1 mhz", 1);
        refCyclesInCpuCycle = 16;
        frequency = 1000000;
        if (!init) {
            cycleCounter /= 2;
            attachDelay >>= 1;
            mechanics.stepperDelay >>= 1;
            driveCycles = frequency;
        }
        syncPosRead = (int64_t)(-0.455 * (double)iecBus.cpuCylcesPerSecond);
        syncPosWrite = (int64_t)(0.455 * (double)iecBus.cpuCylcesPerSecond);
        wd1770.set2Mhz( false );
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
    //wd1770.setDiskAccessible(false);
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
            Emulator::copyMemory<uint8_t>( rom1541II, rom1541IISize, data, size );
            break;
        case Interface::FirmwareIdVC1541:
            if (!data) {
                data = (uint8_t*) Firmware::drive1541Rom;
                size = sizeof( Firmware::drive1541Rom );
            }
            Emulator::copyMemory<uint8_t>( rom1541, rom1541Size, data, size );
            break;
        case Interface::FirmwareIdVC1541C:
            if (!data) {
                data = (uint8_t*) Firmware::drive1541CRom;
                size = sizeof( Firmware::drive1541CRom );
            }
            Emulator::copyMemory<uint8_t>( rom1541C, rom1541CSize, data, size );
            break;
        case Interface::FirmwareIdVC1571:
            if (!data) {
                data = (uint8_t*) Firmware::drive1571Rom;
                size = sizeof( Firmware::drive1571Rom );
            }
            Emulator::copyMemory<uint8_t>( rom1571, rom1571Size, data, size );
            break;
        case Interface::FirmwareIdVC1570:
            if (!data) {
                data = (uint8_t*) Firmware::drive1570Rom;
                size = sizeof( Firmware::drive1570Rom );
            }
            Emulator::copyMemory<uint8_t>( rom1570, rom1570Size, data, size );
            break;
        case Interface::FirmwareIdVC1581:
            if (!data) {
                data = (uint8_t*) Firmware::drive1581Rom;
                size = sizeof( Firmware::drive1581Rom );
            }
            Emulator::copyMemory<uint8_t>( rom1581, rom1581Size, data, size );
            break;
        case Interface::FirmwareIdExpanded:
            Emulator::copyMemory<uint8_t>( romExtra, romExtraSize, data, size );
            break;
    }

    if (iecBus.powerOn)
        setFirmwareByType();
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

	int64_t half = iecBus.cpuCylcesPerSecond >> 1;

	if (cycleCounter >= (iecBus.cpuCylcesPerSecond + half)) {
		// expects CPU has missed IRQ recognition
		via1.ca1In( direction, false);
		via1.handleInterrupt();

	} else if (cycleCounter >= half )
		// expects IRQ recognition this cycle
		via1.ca1In( direction, false);

	else
		// expects IRQ recognition next cycle
		via1.ca1In( direction, true);
}

auto Drive::detach() -> void {
    write();
    
    if (loaded) {
        attachDelay = DISC_DELAY;
        if (iecBus.powerOn && system->driveSounds.useFloppy)
            system->interface->mixDriveSound( media, DriveSound::FloppyEject, operation & DRIVE_MODE_158x );
    }

    if (iecBus.powerOn && use2Mhz() )
        attachDelay <<= 1;

    delayInProgress = attachDelay || mechanics.stepperDelay;
    
    structure.detach();
    wasAttachDetached = false;
    
    loaded = false;
    wd1770.setDiskAccessible(false);
    pulseIndex = -1;
    pulseDelta = 1; // to reload quickly
    operation &= ~(ENCODEDDATA_LEVEL | FLUXDATA_LEVEL);
    operation |= DECODEDDATA_LEVEL;
    dskChange = true;
}

auto Drive::attach( uint8_t* data, unsigned size ) -> void {
    detach();
    accum = 0;
    randCounter = 0;
    uf6aFlipFlop = comperatorFlipFlop = false;
    uf4Counter = ue7Counter = 0;
    ue3Counter = 0;

    wobblePos = 0;
    wobbleLimit = wobble >> 1;

    wasAttachDetached = attachDelay != 0;
    attachDelay = DISC_DELAY * 3;

    delayInProgress = attachDelay || mechanics.stepperDelay;

    if ( !structure.attach( data, size ) )
        return;

    bool changed = changeModelByType();
    if (changed)
        system->calcSerializationSize();

    if (iecBus.powerOn && system->driveSounds.useFloppy) {
        system->interface->mixDriveSound( media, DriveSound::FloppyInsert, operation & DRIVE_MODE_158x );
        if (motorOn)
            system->interface->mixDriveSound( this->media, DriveSound::FloppySpinUp, operation & DRIVE_MODE_158x );
    }

    if (iecBus.powerOn && use2Mhz() )
        attachDelay <<= 1;

    if (gcrTrack) {
        headOffset %= gcrTrack->bits;
        pulseIndex = gcrTrack->firstPulse;
        wd1770.setPulseIndex(pulseIndex, pulseDelta);
    }

    loaded = true;
    wd1770.setDiskAccessible(true);

    if (writeProtected && (type == Type::D1570 || type == Type::D1571))
        via1.ca2In( false );

    operation &= ~(DECODEDDATA_LEVEL | ENCODEDDATA_LEVEL | FLUXDATA_LEVEL);

    if (structure.type == DiskStructure::Type::D64 || structure.type == DiskStructure::Type::D71 || structure.type == DiskStructure::Type::D81) {
        operation |= DECODEDDATA_LEVEL;
    } else if (structure.type == DiskStructure::Type::G64 || structure.type == DiskStructure::Type::G71 || structure.type == DiskStructure::Type::G81) {
        operation |= ENCODEDDATA_LEVEL;
    } else if (structure.type == DiskStructure::Type::P64 || structure.type == DiskStructure::Type::P71 || structure.type == DiskStructure::Type::P81) {
        operation |= FLUXDATA_LEVEL;
    } else
        operation |= DECODEDDATA_LEVEL;

    if (iecBus.powerOn && changed)
        power();
}

auto Drive::changeModelByType() -> bool {
    bool _f1541 = type == Type::D1541 || type == Type::D1541C || type == Type::D1541II;
    bool _f1571 = type == Type::D1570 || type == Type::D1571;
    bool _f1581 = type == Type::D1581;

    bool _d1541 = structure.type == DiskStructure::Type::D64 || structure.type == DiskStructure::Type::G64 || structure.type == DiskStructure::Type::P64;
    bool _d1571 = structure.type == DiskStructure::Type::D71 || structure.type == DiskStructure::Type::G71 || structure.type == DiskStructure::Type::P71;
    bool _d1581 = structure.type == DiskStructure::Type::D81 || structure.type == DiskStructure::Type::G81 || structure.type == DiskStructure::Type::P81;

    if (_f1541 && _d1571) {
        iecBus.setDriveType((globalType == Type::D1570) ? globalType : Type::D1571, media);
        return true;
    }
    if (_f1541 && _d1581) {
        iecBus.setDriveType(Type::D1581, media);
        return true;
    }
    //if (_f1571 && _d1541)
      //  return iecBus.setDriveType(Type::D1541II, media), true;
    if (_f1571 && _d1581) {
        iecBus.setDriveType(Type::D1581, media);
        return true;
    }
    if (_f1581 && _d1541) {
        iecBus.setDriveType((globalType == Type::D1541 || globalType == Type::D1541C) ? globalType : Type::D1541II, media);
        return true;
    }
    if (_f1581 && _d1571) {
        iecBus.setDriveType((globalType == Type::D1570) ? globalType : Type::D1571, media);
        return true;
    }

    return false;
}

auto Drive::setWriteProtect(bool state) -> void {
    
    writeProtected = state;
    wd1770.setWriteProtected( state );
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
    
    if (!written && !wd1770.wasWritten())
        return;
    
    written = false;
    wd1770.resetWritten();

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
    rpm = rpmScaled;
}

auto Drive::setWobble(unsigned wobbleScaled) -> void {
    wobble = wobbleScaled;
    wobblePos = 0;
    wobbleLimit = wobble >> 1;
}

auto Drive::setStepperSeekTime( unsigned stepperSeekTimeScaled ) -> void {
    Mechanics::stepperSeekTime = stepperSeekTimeScaled * 100;
}

auto Drive::setType( Type type ) -> void {
    this->type = type;

    updateCycleSpeed(type == Type::D1581);
    wd1770.setType( type == Type::D1581 ? WD1770::T1772 : WD1770::T1770);

    operation &= ~(DRIVE_MODE_154x | DRIVE_MODE_157x | DRIVE_MODE_158x);

    if (type == Type::D1541II || type == Type::D1541 || type == Type::D1541C)
        operation |= DRIVE_MODE_154x;

    else if (type == Type::D1571 || type == Type::D1570)
        operation |= DRIVE_MODE_157x;

    else if (type == Type::D1581)
        operation |= DRIVE_MODE_158x;

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
        case Type::D1581:
            rom = rom1581;
            romMask = rom1581Size - 1;
            break;
    }

    // if (!romExpanded) {
    //     romExpanded = rom;
    //     romExpandedMask = romMask;
    // }

    if (romExtra && romExtraSize) {
        romExpanded = romExtra;
        romExpandedMask = romExtraSize - 1;
    } else {
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

    if (speeder == 12 && !turboTrans)
        turboTrans = new uint8_t[ 512 * 1024 ];

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

auto Drive::editMemory(uint32_t addr, std::vector<uint16_t> values) -> void {
    for (int i = 0; i < values.size(); i++) {
        uint16_t a = (addr + i) & 0xffff;
        uint8_t v = values[i] & 0xff;

        if (a & 0x8000)
            rom[a & romMask] = v;

        cpuWrite(a, v);
    }
}

auto Drive::memoryDump(uint8_t page, uint8_t* dump) -> void {
    page &= 0xf;
    unsigned bank = page << 12;

    for (unsigned addr = bank; addr <= (bank | 0xfff); addr++ )
         *dump++ = cpuRead<true>( addr );
}

template auto Drive::cpuRead<false>(uint16_t addr) -> uint8_t;
template auto Drive::cpuRead<true>(uint16_t addr) -> uint8_t;

}
