
#include "diskDrive.h"
#include "../agnus/agnus.h"
#include "../paula/paula.h"
#include "../interface.h"
#include "../system/system.h"
#include "instantDiskDrive.cpp"

#define LIBAMI_MOTOR_ACCELERATION_CYCLES agnus.msecToDMACycles(360)
#define LIBAMI_MOTOR_DECELERATION_CYCLES agnus.msecToDMACycles(480)
#define LIBAMI_DSK_CHANGE_CYCLES agnus.msecToDMACycles(1700)

namespace LIBAMI {

typedef Emulator::Interface::DriveSound DriveSound;

DiskDrive::DiskDrive(uint8_t number, System* system, Agnus& agnus, Cia<MOS_8520>& cia)
: number(number), system(system), agnus(agnus), cia(cia), structure(agnus) {
    interface = system->interface;
    media = &interface->mediaGroups[Interface::MediaGroupIdDisk].media[number];
    track = getDummyTrack();
}

unsigned DiskDrive::rpm = 30000;
unsigned DiskDrive::wobble = 50;
int DiskDrive::wobblePos = 0;
int DiskDrive::wobbleLimit = 25;
unsigned DiskDrive::stepperSeekTimeBase = 0;
unsigned DiskDrive::stepperMinTimeBase = 0;
// PAL: 28375160 (master clock) / 8 (to DMA clock) / 5 (revs per sec)
// updated each frame, depends on region
unsigned DiskDrive::refCyclesPerRevolutionBase = 709379;

auto DiskDrive::readByte(uint16_t& dmaCycles, bool upd) -> uint8_t {
    // todo: simulate deceleration more realistic (like d64)
    // e.g. there are some C64 games which expect to read some data from disc after motor has stopped.
    if ((!motor && (motorSpeed < 20)) || !inserted)
        return 0;

    if (stepSettleClock) progressStepper();

    if (upd) {
        int refCyclesPerRevolutionScaled = refCyclesPerRevolution << 3;
        accum += track->bits * dmaCycles;
        accum -= refCyclesPerRevolutionScaled;

        int todo = refCyclesPerRevolutionScaled - accum;
        // Depending on the current motor speed, it is determined how many DMA cycles are necessary until the next byte is read.
        // That way, we don't have to work with fractional numbers. carry is remembered in "accum".

        dmaCycles = (todo + (track->bits >> 1)) / track->bits;
    }
    unsigned byteOffset = headOffset >> 3;
    uint8_t byte = track->data[byteOffset];

    if (++byteOffset >= track->length) {
        headOffset = 0;
        if (selected)
            cia.setFlag();
    } else
        headOffset = byteOffset << 3;

    if (!selected)
        return 0;

    return byte;
}

auto DiskDrive::rotate(unsigned dmaCycles, bool reset) -> void {
    if (!motor || !inserted || !dmaCycles) {
        if (reset)
            accum = 0;
        return;
    }

    if (stepSettleClock) progressStepper();

    unsigned refCyclesPerRevolutionScaled = refCyclesPerRevolution << 3;
    accum += track->bits * dmaCycles;

    while (1) {
        if (accum >= refCyclesPerRevolutionScaled) {
            accum -= refCyclesPerRevolutionScaled;
            headOffset += 8;
            if (headOffset >= track->bits) {
                headOffset -= track->bits;
                if (selected)
                    cia.setFlag();
            }
        } else {
            if (accum >= refCyclesPerRevolution) {
                accum -= refCyclesPerRevolution;
                if (++headOffset >= track->bits) {
                    headOffset = 0;
                    if (selected)
                        cia.setFlag();
                }
            } else
                break;
        }
    }
    if (reset)
        accum = 0;
}

auto DiskDrive::readBit(uint16_t& dmaCycles, bool upd) -> bool {
    if ((!motor && (motorSpeed < 20)) || !inserted )
        return false;

    if (stepSettleClock) progressStepper();

    if (upd) {
        // track bit field in header determines bitcell width
        accum += track->bits * dmaCycles;
        accum -= refCyclesPerRevolution;

        unsigned todo = refCyclesPerRevolution - accum;
        // Depending on the current motor speed and bit cell width noted in the header, it is determined how many DMA cycles are necessary
        // until the next bit is read. That way, we don't have to work with fractional numbers. Transfer is remembered in "accum".
        dmaCycles = (todo + (track->bits >> 1) ) / track->bits;
    }

    unsigned byte = headOffset >> 3;
    uint8_t bit = (~headOffset) & 7; // msb is next

    headOffset++;
    if ( headOffset >= track->bits ) {
        headOffset = 0;
        cia.setFlag();
    }

    if (!selected)
        return false;

    bool state = (track->data[byte] >> bit) & 1;

    // weak bits
    if (state)
        randCounter = ( (randomizer.xorShift() >> 16 ) & 7) + 51; // 14.5 - 16.5
    else {
        if (dmaCycles >= randCounter) {
            state = true; // oscilation
            randCounter = ( (randomizer.xorShift() >> 16 ) % 31) + 44;
        } else
            randCounter -= dmaCycles;
    }

    return state;
}

auto DiskDrive::writeBit(bool state) -> void {
    if ((!motor && (motorSpeed < 20)) || !inserted )
        return;

    if (stepSettleClock) progressStepper();

    // no support for writing a custom bitcell width while adjusting motor speed.
    // basically possible in EXT ADF, but only with compromises ... see explanation in fdc.cpp
    accum = 0;

    unsigned byte = headOffset >> 3;
    uint8_t bit = (~headOffset) & 7; // msb is next

    headOffset++;
    if ( headOffset >= track->bits ) {
        headOffset = 0;
        cia.setFlag();
    }

    if (!selected || structure.writeProtected)
        return;

    if (state)
        track->data[byte] |= 1 << bit;
    else
        track->data[byte] &= ~(1 << bit);

    if (!written)
        written = true;
    track->written |= 1; // track data has changed, host have to write back
}

auto DiskDrive::adjustHead(int offset) -> void {
    if (offset >= 0) {
        headOffset += offset;
        if (headOffset >= track->bits)
            headOffset -= track->bits;
    } else {
        if (headOffset >= offset)
            headOffset -= offset;
        else
            headOffset = track->bits - (offset - headOffset);
    }
}

auto DiskDrive::progressStepper() -> void {
    int64_t delay = agnus.fallBackCycles(stepSettleClock);
    if (delay >= stepperSeekTime) {
        // Continuous stepping can be done very quickly.
        // The last step, however, requires about 10 ms until data can be reliably read.
        // The data read during stepping is hard to emulate. The magnetizations of connected tracks influence each other.
        stepSettleClock = 0;
        step(nextStep, true);
    }
}

auto DiskDrive::attach(uint8_t* data, unsigned size) -> bool {
    if (!structure.attach(data, size))
        return false;

    inserted = true;
    stepSettleClock = 0;
    accum = 0;
    wobblePos = 0;
    wobbleLimit = wobble >> 1;
    updateRpm();

    if (driveSound && system->powerOn && system->displayFrame())
        interface->mixDriveSound( media, DriveSound::FloppyInsert );

    updateTrack();
    return true;
}

auto DiskDrive::detach() -> void {
    write();
    structure.detach();
    dskChangeClock = agnus.clock;
    stepSettleClock = 0;
    if (driveSound && inserted && system->powerOn && system->displayFrame())
        interface->mixDriveSound( media, DriveSound::FloppyEject );

    dskChange = true;
    inserted = false;
    track = getDummyTrack();
}

auto DiskDrive::write() -> void {
    if (!written)
        return;

    written = false;

    if (structure.serializationSize) {
        system->serializationSize -= structure.serializationSize;
        structure.serializationSize = 0;
    }

    if (!inserted)
        return;

    if (!interface->questionToWrite(media))
        return;

    structure.storeWrittenTracks();
}

auto DiskDrive::power() -> void {
    selected = false;
    motor = false;
    idPos = 0;
    motorClock = 0;
    motorSpeed = 0;
    dskChangeClock = 0;
    dskChange = true;
    stepClock = agnus.clock;
    stepSettleClock = 0;
    nextStep = 0;
    cylinder = 0;
    side = 0;
    headOffset = 0;
    accum = 0;
    stepperSeekTime = (stepperSeekTimeBase * agnus.frequency()) / 10000;
    stepperMinTime = (stepperMinTimeBase * agnus.frequency()) / 10000;
    structure.serializationSize = 0;
    randomizer.initXorShift( 0x1234abcd );
    randCounter = ~0;
    wobblePos = 0;
    wobbleLimit = wobble >> 1;

    if (driveSound && inserted && !system->powerOn)
        interface->mixDriveSound(media, DriveSound::FloppyInsert);
}

auto DiskDrive::powerOff() -> void {
    motor = false;
    write();
}

auto DiskDrive::readCiaPortA() -> uint8_t {
    uint8_t out = 0xff;

    if (selected) {
        auto speed = getMotorSpeed();

        if ((!motor && (speed == 0)) || (motor && (speed < 2))) {
            if (getId() & (1 << (31 - idPos))) out &= ~0x20;
        } else if (inserted) {
            if ((!motor && (speed > 98)) || (motor && (speed == 100))) out &= ~0x20;
        }

        if (cylinder == 0) out &= ~0x10;
        if (structure.writeProtected) out &= ~8;
        if (dskChange) out &= ~4;
    }

    return out;
}

auto DiskDrive::writeCiaPortB(uint8_t value, uint8_t oldValue) -> void {
    bool selectedLine = value & (8 << number);
    bool selectedLineOld = oldValue & (8 << number);
    bool sideBefore = (oldValue & 4) ? 0 : 1;

    if (selectedLineOld && !selectedLine) {
        idPos = (idPos + 1) & 31;
        setMotor( !(value & oldValue & 0x80) );
    }

    side = (value & 4) ? 0 : 1;
    selected = !selectedLine;

    if (!selectedLineOld && (value & 1) && (!(oldValue & 1)) ) { // step
        if (inserted) {
            if (dskChangeClock ) {
                if (agnus.fallBackCycles(dskChangeClock) > LIBAMI_DSK_CHANGE_CYCLES) {
                    dskChange = false;
                    dskChangeClock = 0;
                }
            } else
                dskChange = false;
        }

        if (stepSettleClock) {
            step(nextStep, false);
        }

        if (!stepperSeekTime) {
            step(value & 2, true);
        } else {
            nextStep = value & 2;
            stepSettleClock = agnus.clock;
        }
    } else if (side != sideBefore)
        updateTrack();
}

auto DiskDrive::getId() -> unsigned { // no emulation of a HD drive with inserted DD disk.
    if (number == 0)
        return 0;

    if (!inserted || !structure.hd)
        return 0xffffffff;

    return 0xaaaaaaaa; // hd
}

auto DiskDrive::getMotorSpeed() -> unsigned {
    int64_t cycles = agnus.fallBackCycles(motorClock);
    int percent;

    if (motor) {
        if (cycles >= LIBAMI_MOTOR_ACCELERATION_CYCLES) return 100;
        percent = ((100.0 * (unsigned)cycles) / LIBAMI_MOTOR_ACCELERATION_CYCLES) + 0.5;
        percent += motorSpeed;
        return (percent > 100) ? 100 : percent;
    }

    if (cycles >= LIBAMI_MOTOR_DECELERATION_CYCLES) return 0;
    percent = ((100.0 * (unsigned)cycles) / LIBAMI_MOTOR_DECELERATION_CYCLES) + 0.5;
    percent = (int)motorSpeed - percent;
    return (percent < 0) ? 0 : percent;
}

auto DiskDrive::setMotor(bool state) -> void {
    if (state == motor)
        return;

    if (!state)
        idPos = 0;

    motorSpeed = getMotorSpeed();
    motorClock = agnus.clock;
    motor = state;

    updateDeviceState();
    if (driveSound && system->displayFrame())
        interface->mixDriveSound( media, state ? DriveSound::FloppySpinUp : DriveSound::FloppySpinDown );

    system->hintObserverMotorChange( motor );
}

auto DiskDrive::step(bool dir, bool updTrack) -> void {

    // The drives used for the Amiga are guaranteed to get to the next
    // track within 3 milliseconds. Some drives will support a much
    // faster rate, others will fail.
    if (stepperMinTime && (agnus.fallBackCycles(stepClock) < stepperMinTime))
        return;

    if (dir) {
        if (cylinder > 0)
            cylinder--;
    } else {
        if (cylinder < 83)
            cylinder++;
    }

    //system->interface->log("s",1);
    //system->interface->log(cylinder,0,1);

    stepClock = agnus.clock;

    if (driveSound && system->displayFrame())
        interface->mixDriveSound( media, DriveSound::FloppyStep, cylinder );

    if (updTrack)
        updateTrack();

    randCounter = ~0;
}

inline auto DiskDrive::updateTrack() -> void {
    accum = 0;
    track = &structure.tracks[(cylinder << 1) | side];
    updateDeviceState();
}

auto DiskDrive::updateRpm() -> void {
    // to simulate the halved speed of HD disks
    refCyclesPerRevolution = refCyclesPerRevolutionBase << structure.hd;
}

auto DiskDrive::setSpeed(unsigned rpmScaled) -> void {
    rpm = rpmScaled;
}

auto DiskDrive::setWobble(unsigned wobbleScaled) -> void {
    wobble = wobbleScaled;
    wobblePos = 0;
    wobbleLimit = wobble >> 1;
}

auto DiskDrive::randomizeRpm(unsigned frequency) -> void {
    // supported for EXT ADF only

    // drive speed is 300 rounds per minute
    // more realistic speed wobbles between 299,75 - 300,25
    // so we could generate a random number in a range of 0.5
    // generating random integer numbers is easier, lets scale up
    // 0.5 rpm * 100 = 50
    // 300 rpm * 100 = 30000
    //unsigned adjusted = rpm + (rand() % (wobble + 1) ) - (wobble / 2);

    unsigned long long cyclesPerRevolution = frequency / 5;

    if (wobble) {
        if (wobbleLimit < 0) { // neg
            if (--wobblePos < wobbleLimit) {
                wobbleLimit = wobble >> 1;
            }
        } else {
            if (++wobblePos > wobbleLimit) {
                wobbleLimit = -(int)(wobble >> 1);
            }
        }

        unsigned adjusted = rpm + wobblePos;

        // drive speed is 300 rounds per minute, means 300 / 60 = 5 rounds per second.
        refCyclesPerRevolutionBase = (30000ULL * cyclesPerRevolution) / adjusted;
    } else {

        refCyclesPerRevolutionBase = (30000ULL * cyclesPerRevolution) / rpm;
    }
}

auto DiskDrive::setStepperSeekTime( unsigned stepperSeekTimeScaled ) -> void {
    stepperSeekTimeBase = stepperSeekTimeScaled;
}

auto DiskDrive::setStepperMinTime( unsigned stepperMinTimeScaled ) -> void {
    stepperMinTimeBase = stepperMinTimeScaled;
}

auto DiskDrive::updateDeviceState() -> void {
    // drive LED is hardwired to motor state
    // FDC (Paula) enables write mode if drive is selected
    if (connected && selected && system->displayFrame())
        interface->updateDeviceState( media, agnus.paula.fdcWriteMode(), (cylinder << 1) | side, motor, !motor );
}

auto DiskDrive::enableSounds(bool state) -> void {
    driveSound = state;

    if (state && motor)
        interface->mixDriveSound( media, DriveSound::FloppySpin );
}

auto DiskDrive::getDummyTrack() -> DiskStructure::Track* {
    static DiskStructure::Track* dummyTrack = nullptr;

    if (!dummyTrack) { // one time init, instance doesn't matter
        dummyTrack = new DiskStructure::Track;
        structure.initTrack( *dummyTrack );
    }

    return dummyTrack;
}

auto DiskDrive::serialize(Emulator::Serializer& s, bool light) -> void {
    s.integer(connected);
    if (!connected && light)
        return;

    s.integer(selected);
    s.integer(motor);
    s.integer(inserted);
    s.integer(idPos);
    s.integer(motorClock);
    s.integer(motorSpeed);
    s.integer(dskChangeClock);
    s.integer(dskChange);
    s.integer(written);
    s.integer(cylinder);
    s.integer(side);
    s.integer(headOffset);
    s.integer(refCyclesPerRevolution);
    s.integer(accum);
    s.integer(stepClock);
    s.integer(stepSettleClock);
    s.integer(nextStep);
    s.integer(stepperSeekTime);
    s.integer(stepperMinTime);
    s.integer(randomizer.xorShift32);
    s.integer(randCounter);

    if (light)
        return;

    if (s.mode() == Emulator::Serializer::Mode::Load)
        updateTrack();

    structure.serialize( s, written );
}

}
