
#include "traps.h"
#include "../system/system.h"
#include "../disk/iec.h"

#define ISOPEN_CLOSED           0
#define ISOPEN_AWAITING_NAME    1
#define ISOPEN_OPEN             2

#define TRAP_OPCODE         0x02
#define SERIAL_LISTEN       0x20
#define SERIAL_TALK         0x40
#define SERIAL_SECONDARY    0x60
#define SERIAL_CLOSE        0xE0
#define SERIAL_OPEN         0xF0

namespace LIBC64 {
    
Traps* traps = nullptr;

auto Traps::add(Trap trap) -> void {
    trapList.push_back( trap );
}

auto Traps::install() -> void {
    installed = false;

    for(auto& trap : trapList)
        installed |= install(trap);
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
}

auto Traps::handler() -> bool {
    if (!installed)
        return false;
system->interface->log("handler");
    auto pc = cpu->pc;

    for(auto& trap : trapList) {
        if ((trap.address + 1) == pc) {
            system->interface->log("match");
            auto resumeAddr = trap.resumeAddress;
            trap.job();
            cpu->pc = resumeAddr;
            return true;
        }
    }

    return false;
}

auto Traps::reset() -> void {
    unsigned int i, j;
    Serial* p;
    static BaseDevice* emptyDevice = new BaseDevice;
    auto connectedDrives = iecBus->drivesEnabled.size();

    SerialPtr = 0;
    for (i = 0; i < 16; i++) {
        p = &serialdevices[i];
        p->inuse = false;
        p->device = emptyDevice;

        if (i >= 8 && i <= 11) {
            unsigned drivePos = i - 8;
            if (drivePos < connectedDrives) {
                p->inuse = true;
                p->device = (BaseDevice*)iecBus->drivesEnabled[drivePos]->structure1541.virtualDrive;
                p->device->reset();
            }
        }

        if (p->inuse) {
            for (j = 0; j < 16; j++) {
                p->nextbyte[j] = 0;
                //  if (p->isopen[j] != ISOPEN_CLOSED) {
                p->isopen[j] = ISOPEN_CLOSED;
                //    p->device->close(j);
                //  }
            }
        }
    }
}

auto Traps::storeKernal(uint16_t addr, uint8_t value) -> void {
    switch (addr & 0xf000) {
        case 0xe000:
        case 0xf000:
            system->kernalRom[addr & 0x1fff] = value;
            break;
    }
}

auto Traps::readKernal(uint16_t addr) -> uint8_t {
    switch (addr & 0xf000) {
        case 0xe000:
        case 0xf000:
            return system->kernalRom[addr & 0x1fff];
    }
    return 0;
}


auto Traps::attention() -> void {
    uint8_t b;
    Serial* p;

    b = system->memoryCpu.read( 0x95 ); // BSOUR

    /* do a flush if unlisten for close and command channel */
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
                send_listen_talk_secondary(b);
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

    cpu->regP &= ~1; // carry off
    cpu->regP &= ~4; // int off

   // if (attention_callback_func) {
     //   attention_callback_func();
   // }
}

auto Traps::send() -> void {
    uint8_t data;

    /*
     * If no secondary address was sent, it means that no LISTEN was
     * sent either. Do both now with SA = 0.
     */
    if (secondary == 0) {
        send_listen_talk_secondary(SERIAL_SECONDARY + 0);
    }

    data = system->memoryCpu.read( 0x95 ); // BSOUR

    write(device, secondary, data);

    cpu->regP &= ~1; // carry off
    cpu->regP &= ~4; // int off
}

auto Traps::receive() -> void {
    uint8_t data;
    /*
     * If no secondary address was sent, it means that no TALK was
     * sent either. Do both now with SA = 0.
     */
    if (secondary == 0) {
        send_listen_talk_secondary(SERIAL_SECONDARY + 0);
    }
    data = read(device, secondary);

    system->memoryCpu.write(0xa4, data);

    /* If at EOF, call specified callback function.  */
    if ((system->memoryCpu.read( 0x90 ) & 0x40) ) {
        system->interface->log("eof");
        uninstall();
        Serial* p = &serialdevices[device & 0x0f];
        p->device->finish();
    }

    /* Set registers like the Kernal routine does.  */
    cpu->regA = data;
    cpu->flagN = data;
    cpu->flagZ = data;
    cpu->regP &= ~1; // carry off
    cpu->regP &= ~4; // int off
}

auto Traps::ready() -> void {
    cpu->regA = 1;
    cpu->flagN = 0;
    cpu->flagZ = 0;
    cpu->regP &= ~4; // int off
}

auto Traps::send_listen_talk_secondary(uint8_t b) -> void {
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
        /* send unlisten to emulated devices for flushing of
           REL file write buffer. */
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

    /* send listen/talk to emulated devices for flushing of
       REL file write buffer. */
    if ((device & 0x0f) >= 8) {
        /* single drive only */
        p->device->listen(secondary & 0x0f);
    }

}

auto Traps::close(unsigned int device, uint8_t secondary) -> void {
    uint8_t st = serialcommand(device, secondary);
    setSt(st);
}

auto Traps::open(unsigned int device, uint8_t secondary) -> void {
    Serial* p = &serialdevices[device & 0x0f];

    p->isopen[secondary & 0x0f] = ISOPEN_AWAITING_NAME;
}

auto Traps::write(unsigned int device, uint8_t secondary, uint8_t data) -> void {
    uint8_t st;
    Serial* p = &serialdevices[device & 0x0f];

    if (p->inuse) {
        if (p->isopen[secondary & 0x0f] == ISOPEN_AWAITING_NAME) {
            /* Store name here */
            if (SerialPtr < 255) {
                SerialBuffer[SerialPtr++] = data;
            }
        } else {
            /* Send to device */
            st = p->device->put(data, (int)(secondary & 0x0f));
            setSt(st);
        }
    } else {                    /* Not present */
        setSt(0x83);
    }
}

auto Traps::read(unsigned int device, uint8_t secondary) -> uint8_t {
    int st = 0, secadr = secondary & 0x0f;
    uint8_t data;
    Serial* p = &serialdevices[device & 0x0f];

    st = p->device->get(&(p->nextbyte[secadr]), secadr);

    /* Move byte from buffer to output.  */
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
            break;
        case 0x40:
        case 0x50: // Talk, no call to driver
            break;
        case 0x60: // Open channel
            if (p->isopen[channel] == ISOPEN_AWAITING_NAME) {
                p->isopen[channel] = ISOPEN_OPEN;
                st = (uint8_t)p->device->open( nullptr, 0, channel);

                for (i = 0; i < SerialPtr; i++) {
                    p->device->put(((uint8_t)(SerialBuffer[i])), channel);
                }
                SerialPtr = 0;
            }

            p->device->flush(channel);
            break;

        case 0xE0: // Close File
            p->isopen[channel] = ISOPEN_CLOSED;
            st = (uint8_t)p->device->close( channel );
            break;

        case 0xF0: // Open File
            if (p->isopen[channel] != ISOPEN_CLOSED) {

                if (SerialPtr != 0 || channel == 0x0f) {
                    p->device->close( channel);
                    p->isopen[channel] = ISOPEN_OPEN;
                    SerialBuffer[SerialPtr] = 0;
                    st = (uint8_t)p->device->open(SerialBuffer, SerialPtr, channel);
                    SerialPtr = 0;
                    if (st) {
                        p->isopen[channel] = ISOPEN_CLOSED;
                        p->device->close(channel);

                    }
                }

                /* open always sets st to 0 even if SERIAL_ERROR is set */
                st = st & (~2);
            }
            p->device->flush(channel);
            break;

        default:
            system->interface->log("Unknown command");
            break;
    }

    return st;
}

auto Traps::setSt(uint8_t st) -> void {
    system->memoryCpu.write( 0x90, system->memoryCpu.read( 0x90 ) | st );
}
}
