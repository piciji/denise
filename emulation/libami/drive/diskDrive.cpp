
#include "diskDrive.h"
#include "../agnus/agnus.h"
#include "../paula/paula.h"
#include "../system/system.h"

#define LIBAMI_MOTOR_ACCELERATION_CYCLES agnus.msecToDMACycles(360)
#define LIBAMI_MOTOR_DECELERATION_CYCLES agnus.msecToDMACycles(480)
#define LIBAMI_DSK_CHANGE_CYCLES agnus.msecToDMACycles(1700)
#define LIBAMI_STEP_CYCLES agnus.msecToDMACycles(228 * 2)

namespace LIBAMI {

typedef Emulator::Interface::DriveSound DriveSound;

DiskDrive::DiskDrive(uint8_t number, System* system, Agnus& agnus, Cia<MOS_8520>& cia)
: system(system), number(number), agnus(agnus), cia(cia), structure(agnus) {
    this->interface = system->interface;
    media = &interface->mediaGroups[Interface::MediaGroupIdDisk].media[number];
}

unsigned DiskDrive::rpm = 30000;
unsigned DiskDrive::wobble = 50;
unsigned DiskDrive::stepperSeekTimeBase = 0;
// PAL: 28375160 (master clock) / 8 (to DMA clock) / 5 (revs per sec)
// updated each frame, depends on region
unsigned DiskDrive::refCyclesPerRevolutionBase = 709379;

auto DiskDrive::readADF() -> uint8_t {
    // todo: simulate deceleration more realistic (like d64)
    // e.g. there are some C64 games which expect to read some data from disc after motor has stopped.
    if ((!motor && (motorSpeed < 20)) || !inserted )
        return 0;

    if (stepSettleClock) progressStepper();

    uint8_t data = track->data[headOffset]; // no sanity check needed, all possible tracks were created before

    if (++headOffset == track->length) {
        headOffset = 0;
        cia.setFlag();
    }

    return data;
}

auto DiskDrive::writeADF(uint8_t data) -> void {
    if ((!motor && (motorSpeed < 20)) || !inserted )
        return;

    if (stepSettleClock) progressStepper();

    unsigned offset = headOffset;
    if (++headOffset == track->length) {
        headOffset = 0;
        cia.setFlag();
    }

    if (structure.writeProtected)
        return;

    track->data[offset] = data;

    if (!written)
        written = true;

    track->written |= 1; // track data has changed, host have to write back
}

auto DiskDrive::readEXT(uint8_t& dmaCycles) -> bool {
    if ((!motor && (motorSpeed < 20)) || !inserted )
        return false;

    if (stepSettleClock) progressStepper();

    accum += track->bits * dmaCycles;

    // track bit field in header determines bit cell width
    if (accum >= refCyclesPerRevolution) {
        accum -= refCyclesPerRevolution;
        dmaCycles = accum / track->bits;
        return readBit();
    }

    return false;
}

inline auto DiskDrive::readBit() -> bool {
    unsigned byte = headOffset >> 3;
    uint8_t bit = (~headOffset) & 7; // msb is next

    headOffset++;
    if ( headOffset >= track->bits ) {
        headOffset = 0;
        cia.setFlag();
    }

    return (track->data[byte] >> bit) & 1;
}

auto DiskDrive::writeEXT(unsigned dmaCycles, bool bit) -> void {
    if ((!motor && (motorSpeed < 20)) || !inserted )
        return;

    if (stepSettleClock) progressStepper();

    accum += track->bits * dmaCycles;

    if (accum >= refCyclesPerRevolution) {
        accum -= refCyclesPerRevolution;
    }

    // controller writes with a fixed bit cell width
    writeBit(bit);
}

inline auto DiskDrive::writeBit( bool state ) -> void {
    unsigned byte = headOffset >> 3;
    uint8_t bit = (~headOffset) & 7; // msb is next

    headOffset++;

    if ( headOffset >= track->bits )
        headOffset = 0; // wrap around the ring buffer

    if (structure.writeProtected)
        return;

    if (state)
        track->data[byte] |= 1 << bit;
    else
        track->data[byte] &= ~(1 << bit);

    if (!written)
        written = true;

    track->written |= 1; // track data has changed, host have to write back
}

auto DiskDrive::progressStepper() -> void {
    uint64_t delay = agnus.fallBackCycles(stepSettleClock);
    if (delay >= stepperSeekTime) {
        stepSettleClock = 0;
        step(nextStep, true);
    }
}

auto DiskDrive::attach(uint8_t* data, unsigned size) -> bool {
    if (!structure.attach(data, size))
        return false;

    inserted = true;
    stepSettleClock = 0;
    updateRpm();

    if (driveSound && system->powerOn)
        interface->mixDriveSound( media, DriveSound::FloppyInsert );

    return true;
}

auto DiskDrive::detach() -> void {
    structure.detach();
    dskChangeClock = agnus.clock;
    stepSettleClock = 0;

    if (driveSound && inserted && system->powerOn)
        interface->mixDriveSound( media, DriveSound::FloppyEject );

    dskChange = true;
    inserted = false;
}

auto DiskDrive::power() -> void {
    dskChangeClock = 0;
    dskChange = true;
    motor = false;
    stepClock = agnus.clock;
    stepperSeekTime = (stepperSeekTimeBase * agnus.frequency()) / 10000;

    if (driveSound && inserted && !system->powerOn)
        interface->mixDriveSound(media, DriveSound::FloppyInsert);
}

auto DiskDrive::powerOff() -> void {
    motor = false;
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

    if (selectedLineOld && !selectedLine) {
        idPos = (idPos + 1) & 31;
        setMotor( !(value & oldValue & 0x80) );
    }

    side = (value & 4) ? 0 : 1;
    selected = !selectedLine;
    if (!selectedLineOld && ((value & 1) > (oldValue & 1))) {
        if (inserted && dskChangeClock) {
            if (agnus.fallBackCycles(dskChangeClock) > LIBAMI_DSK_CHANGE_CYCLES) {
                dskChange = false;
                dskChangeClock = 0;
            }
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
    }
}

auto DiskDrive::getId() -> unsigned {
    if (number == 0)
        return 0;

    if (!inserted || !structure.hd)
        return 0xffffffff;

    return 0xaaaaaaaa; // hd
}

auto DiskDrive::getMotorSpeed() -> unsigned {
    auto cycles = agnus.fallBackCycles(motorClock);
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
    if (driveSound)
        interface->mixDriveSound( media, state ? DriveSound::FloppySpinUp : DriveSound::FloppySpinDown );
}

auto DiskDrive::step(bool dir, bool updTrack) -> void {
    // todo emulate stepping more realistic.
    // Currently, only too fast stepping is prevented. Continuous stepping can be done very quickly.
    // The last step, however, requires about 15 ms until data can be reliably read.
    // The data read during stepping is hard to emulate. The magnetizations of connected tracks influence each other.
    if (agnus.fallBackCycles(stepClock) < LIBAMI_STEP_CYCLES)
        return;

    if (dir) {
        if (cylinder > 0)
            cylinder--;
    } else {
        if (cylinder < 83)
            cylinder++;
    }

    stepClock = agnus.clock;

    if (driveSound)
        interface->mixDriveSound( media, DriveSound::FloppyStep, (cylinder << 1) | side );

    if (updTrack)
        updateTrack();
}

inline auto DiskDrive::updateTrack() -> void {
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
}

auto DiskDrive::randomizeRpm(unsigned frequency) -> void {
    // supported for EXT ADF only

    // drive speed is 300 rounds per minute
    // more realistic speed wobbles between 299,75 - 300,25
    // so we could generate a random number in a range of 0.5
    // generating random integer numbers is easier, lets scale up
    // 0.5 rpm * 100 = 50
    // 300 rpm * 100 = 30000
    unsigned adjusted = rpm + (rand() % (wobble + 1) ) - (wobble / 2);

    // drive speed is 300 rounds per minute, means 300 / 60 = 5 rounds per second.
    unsigned long long cyclesPerRevolution = frequency / 5;
    refCyclesPerRevolutionBase = (30000ULL * cyclesPerRevolution) / adjusted;
}

auto DiskDrive::setStepperSeekTime( unsigned stepperSeekTimeScaled ) -> void {
    stepperSeekTimeBase = stepperSeekTimeScaled;
}

auto DiskDrive::updateDeviceState() -> void {
    // drive LED is hardwired to motor state
    // FDC (Paula) enables write mode if drive is selected
    if (connected && selected)
        interface->updateDeviceState( media, agnus.paula.fdcWriteMode(), (cylinder << 1) | side, motor, !motor );
}

auto DiskDrive::enableSounds(bool state) -> void {
    driveSound = state;

    if (state && motor)
        interface->mixDriveSound( media, DriveSound::FloppySpin );
}

}
