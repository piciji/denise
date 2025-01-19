
#include "traps.h"
#include "../system/system.h"
#include "../disk/iec.h"
#include "../tape/tape.h"
#include "../expansionPort/superCpu/superCpu.h"

#define FILE_CLOSED           0
#define FILE_AWAITING_NAME    1
#define FILE_OPEN             2

#define TRAP_OPCODE         0x02
#define SERIAL_LISTEN       0x20
#define SERIAL_TALK         0x40
#define SERIAL_SECONDARY    0x60
#define SERIAL_CLOSE        0xE0
#define SERIAL_OPEN         0xF0

namespace LIBC64 {

Traps::Traps(System* system) :
system(system),
cpu(system->cpu),
iecBus(system->iecBus),
tape(system->tape) {

}

auto Traps::add(Trap trap) -> void {
    trapList.push_back( trap );
}

auto Traps::installSerial() -> void {
    superCpu = dynamic_cast<SuperCpu*>(system->expansionPort) ? system->superCpu : nullptr;
    installed = trapList.size() > 0;
    installDelayed = false;

    for(auto& trap : trapList) {
        if(trap.name.find("Serial") == std::string::npos)
            continue;

        if (!install(trap)) {
            installed = false;
            break;
        }
    }
}

auto Traps::installTape() -> void {
    installed = trapList.size() > 0;
    installDelayed = false;

    for(auto& trap : trapList) {
        if(trap.name.find("Tape") == std::string::npos)
            continue;

        if (!install(trap)) {
            installed = false;
            break;
        }
    }
}

auto Traps::install(Trap& t) -> bool {
    for (uint8_t i = 0; i < 3; i++) {
        if (readKernal(t.address + i) != t.check[i]) {
            return false;
        }
    }

    storeKernal(t.address, TRAP_OPCODE);

    return true;
}

auto Traps::uninstall() -> void {
    for(auto& trap : trapList)
        uninstall(trap);

    installed = false;
}

auto Traps::uninstall(Trap& t) -> void {
    if (readKernal(t.address) != TRAP_OPCODE) {
        return;
    }

    storeKernal(t.address, t.check[0]);
    installDelayed = false;
}

auto Traps::handler() -> bool {
    if (!installed)
        return false;

    auto pc = superCpu ? superCpu->getPC() : cpu.pc;

    for(auto& trap : trapList) {
        if ((trap.address + 1) == pc) {
            auto resumeAddr = trap.resumeAddress;
            trap.job();
            if (superCpu)
                superCpu->setPC(resumeAddr);
            else
                cpu.pc = resumeAddr;
            return true;
        }
    }

    return false;
}

auto Traps::reset(bool sendFinishEvent) -> void {
    unsigned int i, j;
    Serial* p;
    static BaseDevice* emptyDevice = new BaseDevice;
    auto connectedDrives = iecBus.drivesEnabled.size();
    this->sendFinishEvent = sendFinishEvent;    

    SerialPtr = 0;
    for (i = 0; i < 16; i++) {
        p = &serialdevices[i];
        p->inuse = false;
        p->device = emptyDevice;

        if (i >= 8 && i <= 11) {
            unsigned drivePos = i - 8;
            if (drivePos < connectedDrives) {
                p->inuse = true;
                p->device = (BaseDevice*)iecBus.drivesEnabled[drivePos]->structure.virtualDrive;
                p->device->reset();
            }
        }

        for (j = 0; j < 16; j++) {
            p->nextbyte[j] = 0;
            p->isopen[j] = FILE_CLOSED;
        }
    }
}

auto Traps::storeKernal(uint16_t addr, uint8_t value) -> void {
    switch (addr & 0xf000) {
        case 0xe000:
        case 0xf000: {
            if (superCpu)
                superCpu->sram[0x1e000 | (addr & 0x1fff)] = value;
            else
                system->kernalRom[addr & 0x1fff] = value;
        } break;
    }
}

auto Traps::readKernal(uint16_t addr) -> uint8_t {
    switch (addr & 0xf000) {
        case 0xe000:
        case 0xf000: {
            if (superCpu)
                return superCpu->sram[0x1e000 | (addr & 0x1fff)];

            return system->kernalRom[addr & 0x1fff];
        }
    }
    return 0;
}

auto Traps::attention() -> void {
    uint8_t b;
    Serial* p;
    //system->interface->log("attention");
    if (superCpu)
        b = superCpu->sram[0x95];
    else
        b = system->memoryCpu.read( 0x95 ); // BSOUR

    if (b == (SERIAL_LISTEN + 0x1f)) {
        unlisten(device, secondary);
    } else if (b == (SERIAL_TALK + 0x1f)) {
        untalk(device, secondary);
    } else {
        switch (b & 0xf0) {
            case SERIAL_LISTEN:
            case SERIAL_TALK:
                device = b;
                secondary = 0;
                break;
            case SERIAL_SECONDARY:
                listentalkSecondary(b);
                break;
            case SERIAL_CLOSE:
                secondary = b;
                close(device, secondary);
                break;
            case SERIAL_OPEN:
                secondary = b;
                open(device, secondary);
                break;
        }
    }

    p = &serialdevices[device & 0x0f];
    if (!(p->inuse)) {
        setSt(0x80);
    }

    if (superCpu) {
        superCpu->setRegP_I(false);
        superCpu->setRegP_C(false);
    } else {
        cpu.regP &= ~1; // carry off
        cpu.regP &= ~4; // int off
    }
}

auto Traps::send() -> void {
    uint8_t data;
    //system->interface->log("send");
    if (secondary == 0) {
        listentalkSecondary(SERIAL_SECONDARY + 0);
    }

    if (superCpu)
        data = superCpu->sram[0x95];
    else
        data = system->memoryCpu.read( 0x95 ); // BSOUR

    write(device, secondary, data);

    if (superCpu) {
        superCpu->setRegP_I(false);
        superCpu->setRegP_C(false);
    } else {
        cpu.regP &= ~1; // carry off
        cpu.regP &= ~4; // int off
    }
}

auto Traps::receive() -> void {
    uint8_t data;
    bool hasFinished;
    //system->interface->log("receive");
    if (secondary == 0) {
        listentalkSecondary(SERIAL_SECONDARY + 0);
    }
    data = read(device, secondary);

    if (superCpu) {
        superCpu->sram[0xa4] = data;
        hasFinished = superCpu->sram[0x90] & 0x40;
    } else {
        system->memoryCpu.write(0xa4, data);
        hasFinished = system->memoryCpu.read(0x90) & 0x40;
    }

    if (hasFinished) {
        //system->interface->log("eof");
        uninstall();
        Serial *p = &serialdevices[device & 0x0f];
        p->device->finish(sendFinishEvent);
    }

    if (superCpu) {
        superCpu->setRegA(data);
        superCpu->setRegP_N(data & 0x80 ? true : false );
        superCpu->setRegP_Z(data == 0);
        superCpu->setRegP_I(false);
        superCpu->setRegP_C(false);
    } else {
        cpu.regA = data;
        cpu.flagN = data;
        cpu.flagZ = data;
        cpu.regP &= ~1; // carry off
        cpu.regP &= ~4; // int off
    }
}

auto Traps::ready() -> void {
    //system->interface->log("ready");
    if (superCpu) {
        superCpu->setRegA(1);
        superCpu->setRegP_N(false );
        superCpu->setRegP_Z(false);
        superCpu->setRegP_I(false);
    } else {
        cpu.regA = 1;
        cpu.flagN = 0;
        cpu.flagZ = 0;
        cpu.regP &= ~4; // int off
    }
}

auto Traps::testForComplexTapeLoader() -> bool {

    TapeStructure::FileEntry* fileEntry = tape.structure.getCurFile();

    if (!fileEntry || fileEntry->turoTape)
        return false;

    // system->interface->log( fileEntry->startAddr, 1,1 );
    // system->interface->log( fileEntry->endAddr,1,1 );
    if (fileEntry->type == 3) {
        if (fileEntry->startAddr == 0x2bc && fileEntry->endAddr == 0x304) // ciphoid
            return true;

        if (fileEntry->startAddr == 0x300 && fileEntry->endAddr == 0x30C) // cricket crazy
            return true;

        if (fileEntry->startAddr == 0x2a7 && fileEntry->endAddr == 0x305) // mutant monty
            return true;

        if (fileEntry->startAddr == 0x2a7 && fileEntry->endAddr == 0x308) // orbitron
            return true;

        if( (fileEntry->startAddr == 0x29f && fileEntry->endAddr == 0x33b) // cyberload
			|| (fileEntry->startAddr == 0x29f && fileEntry->endAddr == 0x338) // sanxion
        ) {
            for (unsigned i = 0; i <= 0xffff; i++)
                system->ram[i] = 255 ^ (((i / 64) & 1) ? 0xff : 0x00);
        }
        else if( (fileEntry->startAddr == 0x29f && fileEntry->endAddr == 0x30c) // APB
        ) {
            for (unsigned i = 0; i <= 0xffff; i++)
                system->ram[i] = ((i / 64) & 1) ? 0xff : 0x00;
        }

        if ((fileEntry->startAddr <= 0x314) && (fileEntry->endAddr >= 0x314) ) {
            //system->interface->log("x2");
            fileEntry = tape.structure.readCurFile();
            if (!fileEntry)
                return true;

            auto pos = 0x314 - fileEntry->startAddr;

            if (fileEntry->endAddr >= 0x315) {
                //system->interface->log(fileEntry->buffer[pos],1,1);
                //system->interface->log(fileEntry->buffer[pos+1],1,1);
                if (fileEntry->buffer[pos] == 0 && fileEntry->buffer[pos + 1] == 0) { // Flintstones

                    return true;
                }
                // 4 game pack no 1, 4 Zzzap Sizzlers
                if (fileEntry->buffer[pos] == 0x2c && fileEntry->buffer[pos + 1] == 0xf9) {
                    //system->interface->log("x3");

                    //system->interface->log( fileEntry->startAddr, 1,1 );
                    //system->interface->log( fileEntry->endAddr,1,1 );

                    // disallow
                    // 0x2a7 - 0x10a7
                    // 0x300 - 0x370
                    // 0x300 - 0x334

                    // allow
                    if ( (fileEntry->startAddr == 0x29f && fileEntry->endAddr == 0x33b) // cyberload
                         || (fileEntry->startAddr == 0x29f && fileEntry->endAddr == 0x338) // Sanxion (cyberload)
                         || (fileEntry->startAddr == 0x29f && fileEntry->endAddr == 0x3c0)
                         || (fileEntry->startAddr == 0x2b0 && fileEntry->endAddr == 0x334)
                         || (fileEntry->startAddr == 0x2a7 && fileEntry->endAddr == 0x3ff) // Frak 64
                         || (fileEntry->startAddr == 0x2a7 && fileEntry->endAddr == 0x34f) // Goonies
                         || (fileEntry->startAddr == 0x29f && fileEntry->endAddr == 0x41c) // Pacland
                            ) {

                        return false;
                    }
                    return true;
                }
            } else { // 0x314 only
				//system->interface->log("0x314 only");
                //system->interface->log(fileEntry->buffer[pos]);
                if ((fileEntry->buffer[pos] == 0x43) // Arcade classic
                || (fileEntry->buffer[pos] == 0xc2) // emlyn hughes
                ) {
                    return true;
                }

            }
        }
    }
    return false;
}

auto Traps::tapeFindHeader() -> void {
    uint16_t addr = system->memoryCpu.read( 0xb2 ) | (system->memoryCpu.read( 0xb3 ) << 8);

    uint8_t* buf = system->ram + addr;

    TapeStructure::FileEntry* fileEntry = tape.structure.getCurFile();

    if (fileEntry) {
        tape.setPosition( fileEntry->dataOffset, true );
        // system->interface->log(fileEntry->startAddr,1,1);
        // system->interface->log(fileEntry->endAddr,1,1);

        buf[0] = fileEntry->type;
        buf[1] = fileEntry->startAddr & 0xff;
        buf[2] = fileEntry->startAddr >> 8;
        buf[3] = fileEntry->endAddr & 0xff;
        buf[4] = fileEntry->endAddr >> 8;

        std::memcpy( buf + 5, fileEntry->header + 5, std::min<unsigned>(fileEntry->headerSize, 192) - 5 );
    } else
        buf[0] = 5; // end of tape marker

    system->memoryCpu.write(0x90, 0);
    system->memoryCpu.write(0x93, 0);

    system->memoryCpu.write(0x29f, 0);
    system->memoryCpu.write(0x2a0, 0);

    cpu.regP &= ~1; // reset carry
    cpu.flagZ = 0;
}

auto Traps::tapeReceive() -> void {
    uint16_t start, end;
    TapeStructure::FileEntry* fileEntry;
    uint8_t st = 0x40;

    if (cpu.regX == 0x0e) {
        fileEntry = tape.structure.readCurFile();

        if (fileEntry) {
            if (fileEntry->type == 1) {
                // some Novaload versions use relocatable PRG's
                start = system->memoryCpu.read(0xc3) | (system->memoryCpu.read(0xc4) << 8);
                end = system->memoryCpu.read( 0xac ) | (system->memoryCpu.read( 0xad ) << 8);
            } else { // typical
                start = system->memoryCpu.read(0xc1) | (system->memoryCpu.read(0xc2) << 8);
                end = system->memoryCpu.read( 0xae ) | (system->memoryCpu.read( 0xaf ) << 8);
            }

            // Kernel does this and mastertronic loader expect it
            system->memoryCpu.write(0xac, fileEntry->endAddr & 0xff);
            system->memoryCpu.write(0xad, fileEntry->endAddr >> 8);

            // system->interface->log("file ok",1);
            // system->interface->log(fileEntry->size,0, 1);

            auto _size = fileEntry->size;
            // small 3 byte programs expect 2 bytes only
            if ( (fileEntry->startAddr == 0x29f && fileEntry->endAddr == 0x2a1) // Skull & Crossbone
                || (fileEntry->startAddr == 0x302 && fileEntry->endAddr == 0x304) // boggit the borred - septical II
				|| (fileEntry->startAddr == 0x2a7 && fileEntry->endAddr == 0x304) // crackpots, keystone capers
            )
                _size -= 1; // why ?

            // game "Ah diddum" has one byte more as in header specified.
            std::memcpy( system->ram + start, fileEntry->buffer, _size );

            tape.setPosition( fileEntry->endOffset, false );

        } else {
            st = 0x10;
        }
    }

    setSt(st);

    cpu.regP &= ~1; // carry off
    cpu.regP &= ~4; // int off

    system->memoryCpu.write(0xc0, 0x01 ); // stop motor
	
	// Hunchback expects this in input buffer
	system->ram[0x32a] = 62;

    uninstall();
}

auto Traps::listentalkSecondary(uint8_t b) -> void {
    secondary = b;
    switch (device & 0xf0) {
        case SERIAL_LISTEN:
            listentalk(device, secondary);
            break;
        case SERIAL_TALK:
            listentalk(device, secondary);
            break;
    }
}

auto Traps::unlisten(unsigned int device, uint8_t secondary) -> void {
    uint8_t st;
    Serial* p = &serialdevices[device & 0x0f];

    if ((secondary & 0xf0) == 0xf0
        || (secondary & 0x0f) == 0x0f) {
        st = serialcommand(device, secondary);
        setSt( st );
    } else {
        if ((device & 0x0f) >= 8) {
            p->device->listen(secondary & 0x0f);
        }
    }
}

auto Traps::listentalk(unsigned int device, uint8_t secondary) -> void {
    uint8_t st;
    Serial* p = &serialdevices[device & 0x0f];

    st = serialcommand(device, secondary);
    setSt(st);

    if ((device & 0x0f) >= 8) {
        p->device->listen(secondary & 0x0f);
    }
}

auto Traps::close(unsigned int device, uint8_t secondary) -> void {
    uint8_t st = serialcommand(device, secondary);
    setSt(st);
}

auto Traps::open(unsigned int device, uint8_t secondary) -> void {
    Serial* p = &serialdevices[device & 0x0f];

    p->isopen[secondary & 0x0f] = FILE_AWAITING_NAME;
}

auto Traps::write(unsigned int device, uint8_t secondary, uint8_t data) -> void {
    uint8_t st;
    Serial* p = &serialdevices[device & 0x0f];

    if (p->inuse) {
        if (p->isopen[secondary & 0x0f] == FILE_AWAITING_NAME) {

            if (SerialPtr < 255) {
                SerialBuffer[SerialPtr++] = data;
            }
        } else {
            st = p->device->put(data, (int)(secondary & 0x0f));
            setSt(st);
        }
    } else {
        setSt(0x83);
    }
}

auto Traps::read(unsigned int device, uint8_t secondary) -> uint8_t {
    int st = 0, secadr = secondary & 0x0f;
    uint8_t data;
    Serial* p = &serialdevices[device & 0x0f];

    st = p->device->get(&(p->nextbyte[secadr]), secadr);

    data = p->nextbyte[secadr];
    setSt((uint8_t)st);

    return data;
}

auto Traps::serialcommand(unsigned int device, uint8_t secondary) -> uint8_t {

    int channel;
    int i;
    uint8_t st = 0;
    Serial* p = &serialdevices[device & 0x0f];

    channel = secondary & 0x0f;

    switch (secondary & 0xf0) {
        case 0x20:
        case 0x30: // Listen, no call to driver
            //system->interface->log("listen");
            break;
        case 0x40:
        case 0x50: // Talk, no call to driver
            //system->interface->log("talk");
            break;
        case 0x60: // Open channel
            if (p->isopen[channel] == FILE_AWAITING_NAME) {
                p->isopen[channel] = FILE_OPEN;
                st = (uint8_t)p->device->open( nullptr, 0, channel);

                for (i = 0; i < SerialPtr; i++) {
                    p->device->put(((uint8_t)(SerialBuffer[i])), channel);
                }
                SerialPtr = 0;
            }

            p->device->flush(channel);
            break;

        case 0xE0: // Close File
            //system->interface->log("close");
            p->isopen[channel] = FILE_CLOSED;
            st = (uint8_t)p->device->close( channel );
            break;

        case 0xF0: // Open File
            if (p->isopen[channel] != FILE_CLOSED) {

                if (SerialPtr != 0 || channel == 0x0f) {
                    p->device->close( channel);
                    p->isopen[channel] = FILE_OPEN;
                    SerialBuffer[SerialPtr] = 0;
                    st = (uint8_t)p->device->open(SerialBuffer, SerialPtr, channel);
                    SerialPtr = 0;
                    if (st) {
                        p->isopen[channel] = FILE_CLOSED;
                        p->device->close(channel);

                    }
                }

                st = st & (~2);
            }
            p->device->flush(channel);
            break;

        default:
            break;
    }

    return st;
}

auto Traps::setSt(uint8_t st) -> void {
    if (superCpu)
        superCpu->sram[0x90] = superCpu->sram[0x90] | st;
    else
        system->memoryCpu.write( 0x90, system->memoryCpu.read( 0x90 ) | st );
}

}
