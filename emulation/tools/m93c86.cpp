
#include <cstdint>
#include "m93c86.h"

namespace Emulator {

M93C86::M93C86() {
    data = nullptr;
	org = false;    // 8 Bit mode
}

M93C86::~M93C86() {

}

auto M93C86::reset() -> void {
    cs = false;    
    clockState = false;
    dataIn = false;
    dataOut = false;

    clockCount = 0;
    commandReg = 0;
    outShiftReg = 0;
    status = Status::Init;
    command = Command::None;

    dirty = false;
}

auto M93C86::setData( uint8_t* data ) -> void {

    this->data = data;
}

auto M93C86::orgSelect(bool state) -> void {

    org = state;
}

auto M93C86::chipSelect(bool state) -> void {

    if (!cs && state && !clockState)
        resetCommandReg();

    else if (cs && !state) {

        switch (command) {
            case Command::Read:
                command = Command::None;
                break;
            case Command::Erase:
            case Command::EraseAll:
            case Command::Write:
            case Command::WriteAll:
                // check if a pending write or erase command is scheduled
                if (status == Status::Pending) {
                    // faster chips need 5 ms, slower ones 10 ms
                    events->add( &writeCycle, 5000, Emulator::SystemTimer::UpdateExisting );

                    status = Status::Busy;
                }
                break;
        }
    }

    cs = state;
}

auto M93C86::write(bool state) -> void {

    dataIn = state;
}

auto M93C86::read() -> bool {
    if (cs) {
        switch (status) {
            case Status::Busy:
                return 0;
            case Status::Ready:
                return 1;
            default:
                break;
        }
    }

    return dataOut;
}

auto M93C86::clock(bool state) -> void {

    if (cs && !clockState && state)
        clock();

    clockState = state;
}

// 0 => 1 transition
auto M93C86::clock() -> void {

    if (status == Status::Busy)
        return;

    if (status == Status::Pending) {
		
		if (command == Command::Read) {

			if (clockCount == 0)
				outShiftReg = data[ addr++ ];

			dataOut = (outShiftReg >> 7) & 1;

			outShiftReg <<= 1;

			clockCount++;

			addr &= (M93C86_SIZE - 1);

			clockCount &= 7;

			return;
		}
		
        // self timed cycle will be aborted ( erase(all), write(all) )
        // only falling edge on CS starts a pending self timed cycle
        status = Status::Init;
        command = Command::None;
    }

    commandReg <<= 1;
    commandReg |= dataIn;

    switch(++clockCount) {
        case 1:
            if (dataIn == 0)
                resetCommandReg();
            else
                status = Status::Init;
            break;
        case 3:
            switch( commandReg & 3 ) {
                default: break;
                case 1: command = Command::Write; break;
                case 2: command = Command::Read; break;
                case 3: command = Command::Erase; break;
            }
            break;
        case 5:
            switch( commandReg & 15 ) {
                default: break;
                case 0: command = Command::WriteDisable; break;
                case 1: command = Command::WriteAll; break;
                case 2: command = Command::EraseAll; break;
                case 3: command = Command::WriteEnable; break;
            }
            break;
        case 13:
            if (org == 0)
                break;

            addr = (commandReg & 0x3ff) << 1;
            // fall through
        case 14:
            if (org == 0)
                addr = commandReg & 0x7ff;

            switch (command) {
                default: break;
                case Command::Read:
                    dataOut = 0;
					status = Status::Pending;
                    resetCommandReg();
                    break;
                case Command::WriteEnable:
                    enableWrite = true;
                    resetCommandReg();
                    command = Command::None;
                    break;
                case Command::WriteDisable:
                    enableWrite = false;
                    resetCommandReg();
                    command = Command::None;
                    break;
				case Command::WriteAll:
					addr = 0;
					break;
                case Command::EraseAll:
                    addr = 0;
                case Command::Erase:
                    if (enableWrite) {
                        value = 0xffff;
                        status = Status::Pending;
                    }
                    resetCommandReg();
                    break;
            }
            break;
        case 22:
            if (org == 1)
                break;
        case 29:
            if (enableWrite) {
                value = commandReg & (org == 0 ? 0xff : 0xffff);
                status = Status::Pending;
            }
            resetCommandReg();
            break;
    }
}

auto M93C86::setEvents( SystemTimer* events ) -> void {

    this->events = events;

    writeCycle = [this]() {
        if (!dirty)
            written();

        dirty = true;

        if (org == 0) {
            data[addr++] = value;
        } else {
            data[addr++] = (value >> 8) & 0xff;
            data[addr++] = value & 0xff;
        }

        addr &= (M93C86_SIZE - 1);

        if (addr && ((command == Command::WriteAll) || (command == Command::EraseAll)) )
            this->events->add(&writeCycle, 5000, Emulator::SystemTimer::UpdateExisting);
        else
            status = Status::Ready;
    };

    events->registerCallback( { {&writeCycle, 1} } );
}

auto M93C86::resetCommandReg() -> void {
    clockCount = 0;
    commandReg = 0;
}

auto M93C86::serialize(Emulator::Serializer& s) -> void {

    s.integer((uint8_t&)command);
    s.integer((uint8_t&)status);
    s.integer(dirty);

    s.integer(cs);
    s.integer(org);
    s.integer(clockState);
    s.integer(dataIn);
    s.integer(dataOut);

    s.integer(clockCount);
    s.integer(commandReg);
    s.integer(enableWrite);
    s.integer(addr);
    s.integer(value);
    s.integer(outShiftReg);

}

}
