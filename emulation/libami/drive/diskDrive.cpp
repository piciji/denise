
#include "diskDrive.h"
#include "../agnus/agnus.h"
#include "../paula/paula.h"
#include "../interface.h"
#include "../system/system.h"
#include "instantDiskDrive.cpp"

#define LIBAMI_MOTOR_ACCELERATION_CYCLES Agnus::msecToDMACycles(360)
#define LIBAMI_MOTOR_DECELERATION_CYCLES Agnus::msecToDMACycles(480)
#define LIBAMI_DSK_CHANGE_CYCLES Agnus::msecToDMACycles(1900)

namespace LIBAMI {

typedef Emulator::Interface::DriveSound DriveSound;

DiskDrive::DiskDrive(uint8_t number, System* system, Agnus& agnus, Cia<MOS_8520>& cia)
: number(number), system(system), agnus(agnus), paula(agnus.paula), cia(cia), structure(agnus) {
    interface = system->interface;
    media = &interface->mediaGroups[Interface::MediaGroupIdDisk].media[number];
    track = getDummyTrack();
    written = false;
    cylinder = 0;
    side = 0;

    structure.write = [this, system](uint8_t* buffer, unsigned length, unsigned offset) {
        return system->interface->writeMedia( media, buffer, length, offset );
    };

    structure.readAssigned = [this, system](uint8_t*& buffer) {
        return system->interface->readAssignedMedia( media, buffer );
    };

    structure.writeAssigned = [this, system](uint8_t* buffer, unsigned length) {
        return system->interface->writeAssignedMedia( media, buffer, length );
    };
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

template<bool update> auto DiskDrive::readByte(int& dmaCycles) -> uint8_t {
    // todo: simulate deceleration more realistic (like d64)
    // e.g. there are some C64 games which expect to read some data from disc after motor has stopped.
    if (!motorSpinning() || !inserted)
        return 0;

    if (stepSettleClock) progressStepper();

    dmaCycles = 8 * 7;
    if constexpr (update) {
        int _bits = track->bits;
        int refCyclesPerRevolutionScaled = refCyclesPerRevolution << 3;

        accum += refCyclesPerRevolutionScaled - _bits * 8 * 7;

        if (accum > _bits) {
            dmaCycles -= 1;
            accum -= _bits;
        } else if (accum < (-1 * _bits)) {
            dmaCycles += 1;
            accum += _bits;
        }
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

    // weak bits (no magnetization) for tracks, which are not included in ADF
    // encoded ADF has always valid MFM, so there are no longer sequences with zeros than 10001
    if (!byte) {
        if (randCounter) { // continuous oscillations
            for (int i = 0; i < 8; i++)
                byte |= ((randomizer.xorShift() >> 16 ) & 1) << i;
        } else {
            byte |= 1;
            randCounter = 1; // first oscillation
        }
    } else
        randCounter = 0;

    return byte;
}

auto DiskDrive::writeByte(uint8_t byte) -> void {
    if (!motorSpinning() || !inserted)
        return;

    if (stepSettleClock) progressStepper();

    // no support for writing a custom bitcell width while adjusting motor speed.
    // basically possible in EXT ADF, but only with compromises ... see explanation in fdc.cpp
    accum = 0;

    unsigned byteOffset = headOffset >> 3;
    unsigned curOffset = byteOffset;

    if (++byteOffset >= track->length) {
        headOffset = 0;
        if (selected)
            cia.setFlag();
    } else
        headOffset = byteOffset << 3;

    if (!selected || structure.writeProtected)
        return;

    track->data[curOffset] = byte;

    if (!written)
        written = true;
    track->options |= 1; // track data has changed, host have to write back
}

template<bool update> auto DiskDrive::readBit(int& dmaCycles) -> bool {
    if (!motorSpinning() || !inserted)
        return false;

    if (stepSettleClock) progressStepper();

    dmaCycles = 7;
    if constexpr(update) {
        int _bits = track->bits;
        accum += (int)refCyclesPerRevolution - _bits * 7;

        if (accum > _bits) {
            dmaCycles -= 1;
            accum -= _bits;
        } else if (accum < (-1 * _bits)) {
            dmaCycles += 1;
            accum += _bits;
        }
    }

    unsigned byte = headOffset >> 3;
    uint8_t bit = (~headOffset) & 7; // msb is next

    headOffset++;
    if ( headOffset >= track->bits ) {
        if ((structure.type != DiskStructure::ADF) || (headOffset >= (track->length << 3))) {
            headOffset = 0;
            if (selected)
                cia.setFlag();
        }
    }

    if (!selected)
        return false;

    bool state = (track->data[byte] >> bit) & 1;

    // weak bits (no magnetization)
    if (!state) {
        // after a flux change it takes a while until first oscillation happens.
        if (++randCounter == 7) {
            state = true; // oscillation
            // continuous oscillation without any new flux change happens much faster
            randCounter -= ((randomizer.xorShift() >> 16) & 3) + 1;
        }
    } else
        randCounter = 0;

    return state;
}

template<bool update> auto DiskDrive::readBitIPF(int& dmaCycles) -> bool {
    if (!motorSpinning() || !inserted)
        return false;

    if (stepSettleClock) progressStepper();

    unsigned byte = headOffset >> 3;
    uint8_t bit = (~headOffset) & 7; // msb is next

    headOffset++;
    if ( headOffset >= track->bits ) {
        headOffset = 0;
        structure.loadNextRevIPF(*track);
        if (selected)
            cia.setFlag();
    }

    dmaCycles = 7;

    if constexpr(update) {
        int _bits = track->bits;

        if (track->cellWidth) {
            int cellSpeed = track->cellWidth[headOffset >> 3];
            int oneCycle = (_bits * cellSpeed) / 1000;
            accum += (int)refCyclesPerRevolution - ((_bits * 7 * cellSpeed) / 1000);

            if (accum > oneCycle) {
                dmaCycles -= 1;
                accum -= oneCycle;
            } else if (accum < (-1 * oneCycle)) {
                dmaCycles += 1;
                accum += oneCycle;
            }
        } else {
            accum += (int)refCyclesPerRevolution - _bits * 7;

            if (accum > _bits) {
                dmaCycles -= 1;
                accum -= _bits;
            } else if (accum < (-1 * _bits)) {
                dmaCycles += 1;
                accum += _bits;
            }
        }
    }

    if (!selected)
        return false;

    return (track->data[byte] >> bit) & 1;
}

auto DiskDrive::writeBit(bool state) -> void {
    if (!motorSpinning() || !inserted)
        return;

    if (stepSettleClock) progressStepper();

    // no support for writing a custom bit cell width while adjusting motor speed.
    // basically possible in EXT ADF, but only with compromises ... see explanation in fdc.cpp
    accum = 0;

    unsigned byte = headOffset >> 3;
    uint8_t bit = (~headOffset) & 7; // msb is next

    headOffset++;
    if ( headOffset >= track->bits ) {
        headOffset = 0;
        if (selected)
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
    track->options |= 1; // track data has changed, host have to write back
}

auto DiskDrive::setStandardTiming() -> void {
    if (structure.setStandardTiming(*track))
        reset();
}

auto DiskDrive::reset() -> void {
    accum = 0;
    randCounter = 0;
}

auto DiskDrive::progressStepper() -> void {
    int64_t delay = agnus.fallBackCycles(stepSettleClock);
    if (delay >= stepperSeekTime) {
        // Continuous stepping can be done very quickly.
        // The last step, however, requires about 10 ms until data can be reliably read.
        // The data read during stepping is hard to emulate. The magnetization of connected tracks influence each other.
        stepSettleClock = 0;
        step(nextStep, true);
    }
}

auto DiskDrive::attach(uint8_t* data, unsigned size) -> bool {
    if (!structure.attach(data, size))
        return false;

    headOffset = 0;
    inserted = true;
    stepSettleClock = 0;
    accum = 0;
    wobblePos = 0;
    wobbleLimit = wobble >> 1;
    randomizeRpm(agnus.frequency(), true);
    updateRpm();

    if (driveSound && system->powerOn && system->isDisplayFrame())
        interface->mixDriveSound( media, DriveSound::FloppyInsert );

    updateTrack();
    return true;
}

auto DiskDrive::detach() -> void {
    write();
    structure.detach();
    dskChangeClock = agnus.clock;
    stepSettleClock = 0;
    if (driveSound && inserted && system->powerOn && system->isDisplayFrame())
        interface->mixDriveSound( media, DriveSound::FloppyEject );

    dskChange = true;
    inserted = false;
    track = getDummyTrack();
}

auto DiskDrive::writeProtect(bool state) -> void {
    structure.writeProtected = state;
    // simulate disk out -> in (Final Mission expects this)
    if (system->powerOn) {
        dskChangeClock = system->agnus.clock;
        dskChange = true;
    }
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

    structure.storeWrittenTracks(media);
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
    randCounter = 0;
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

    selected = !selectedLine;
    if (selectedLineOld && !selectedLine) {
        idPos = (idPos + 1) & 31;
        setMotor( !(value & oldValue & 0x80) );
    }

    side = (value & 4) ? 0 : 1;

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
    if (!structure.hd && (number == 0))
        return 0;

    if (!inserted || !structure.hd)
        return 0xffffffff;

    return 0xaaaaaaaa; // hd
}

inline auto DiskDrive::motorSpinning() -> bool {
    if (motor && (motorSpeed == 100)) return true;
    if (!motor && (motorSpeed == 0)) return false;

    unsigned speed = getMotorSpeed();

    if (motor && (speed == 100)) motorSpeed = 100;
    else if (!motor && (speed == 0)) motorSpeed = 0;

    return motor || (speed > 87);
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
    if (motor)
        reset();

    updateDeviceState();
    if (driveSound && system->isDisplayFrame())
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

    if (driveSound && system->isDisplayFrame())
        interface->mixDriveSound( media, DriveSound::FloppyStep, cylinder );

    if (updTrack)
        updateTrack();
}

inline auto DiskDrive::updateTrack() -> void {
    unsigned oldHeadOffset = headOffset;
    DiskStructure::Track* oldTrack = track;
    track = &structure.tracks[(cylinder << 1) | side];

    if (oldTrack && oldTrack->bits && oldHeadOffset) {
        if (structure.type == DiskStructure::ADF) {
            if (oldTrack->length != track->length)
                headOffset = ((((uint64_t)oldHeadOffset >> 3) * (uint64_t)track->length) / (uint64_t)oldTrack->length) << 3;
        } else {
            if (oldTrack->bits != track->bits)
                 headOffset = ((uint64_t)oldHeadOffset * (uint64_t)track->bits) / (uint64_t)oldTrack->bits;
        }
    } else
        headOffset = 0;

    paula.turbo = ((structure.type != DiskStructure::IPF) || !track->cellWidth) ? paula.turboRequested : 0;
    randCounter = 0;

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

auto DiskDrive::randomizeRpm(unsigned frequency, bool prevent) -> void {
    // drive speed is 300 rounds per minute
    // more realistic speed wobbles between 299,75 - 300,25
    // so we could generate a random number in a range of 0.5
    // generating random integer numbers is easier, lets scale up
    // 0.5 rpm * 100 = 50
    // 300 rpm * 100 = 30000
    // unsigned adjusted = rpm + (rand() % (wobble + 1) ) - (wobble / 2);

    unsigned long long cyclesPerRevolution = frequency / 5;

    if (!prevent && wobble) {
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

auto DiskDrive::updateDeviceState(bool force) -> void {
    // drive LED is hardwired to motor state
    if (!connected)
        return;

    if (force || (selected && system->isDisplayFrame()))
        interface->updateDeviceState( media, paula.fdcWriteMode(), (cylinder << 1) | side, motor, !motor );
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
    s.integer(cellSpeedDeprecated);

    if (s.mode() == Emulator::Serializer::Mode::Load)
        track = &structure.tracks[(cylinder << 1) | side];

    if (light)
        return;

    if (number == 0) {
        s.integer(DiskDrive::rpm);
        s.integer(DiskDrive::wobble);
        s.integer(DiskDrive::wobblePos);
        s.integer(DiskDrive::wobbleLimit);
        s.integer(DiskDrive::refCyclesPerRevolutionBase);
        s.integer(DiskDrive::stepperSeekTimeBase);
        s.integer(DiskDrive::stepperMinTimeBase);
    }

    if (s.mode() == Emulator::Serializer::Mode::Load) {
        updateDeviceState(true);
    }

    structure.serialize( s, written );
}

template auto DiskDrive::readByte<false>(int& dmaCycles) -> uint8_t;
template auto DiskDrive::readByte<true>(int& dmaCycles) -> uint8_t;

template auto DiskDrive::readBit<false>(int& dmaCycles) -> bool;
template auto DiskDrive::readBit<true>(int& dmaCycles) -> bool;

template auto DiskDrive::readBitIPF<false>(int& dmaCycles) -> bool;
template auto DiskDrive::readBitIPF<true>(int& dmaCycles) -> bool;

}
