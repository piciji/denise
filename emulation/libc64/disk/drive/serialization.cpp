
#include "drive.h"

namespace LIBC64 {   
    
auto Drive::serialize(Emulator::Serializer& s) -> void {

    s.integer( (uint8_t&)type );
    s.integer( expandMemory );
    s.integer( cycleCounter );
    s.integer( synced );
    s.integer( irqIncomming );
    s.integer( speeder );

    if (operation & DRIVE_MODE_158x)
        s.array( ram, 8 * 1024 );
    else
        s.array( ram, 2 * 1024 );

    if (expandMemory & (uint8_t)ExpandedMemMode::M20)
        s.array( ram20To3F, 8 * 1024 );
    if (expandMemory & (uint8_t)ExpandedMemMode::M40)
        s.array( ram40To5F, 8 * 1024);
    if (expandMemory & (uint8_t)ExpandedMemMode::M60)
        s.array( ram60To7F, 8 * 1024);
    if (expandMemory & (uint8_t)ExpandedMemMode::M80)
        s.array( ram80To9F, 8 * 1024);
    if (expandMemory & (uint8_t)ExpandedMemMode::MA0)
        s.array( ramA0ToBF, 8 * 1024);
    if (speeder == 12) {
        if (!turboTrans)
            turboTrans = new uint8_t[ 512 * 1024 ];
        s.array( turboTrans, 512 * 1024);
    }

    s.integer( driveCycles );
    s.integer( accum );
    s.integer( currentHalftrack );
    s.integer( speedZone );
    s.integer( byteReadyOverflow );
    s.integer( readMode );
    s.integer( headOffset );
    s.integer( coilDir );
    s.integer( ue3Counter );
    s.integer( uf6aFlipFlop );
    s.integer( comperatorFlipFlop );
    s.integer( ue7Counter );
    s.integer( uf4Counter );
    s.integer( randomizer.xorShift32 );
    s.integer( randCounter );
    s.integer( writeValue );
    s.integer( readBuffer );
    s.integer( writeBuffer );
    s.integer( attachDelay );
    s.integer( mechanics.stepperDelay );
    s.integer( mechanics.motorDelay );
    s.integer( mechanics.refCycles );
    s.integer( delayInProgress );
    s.integer( nextStep );
    s.integer( wasAttachDetached );
    s.integer( motorOn );
    s.integer( written );
    s.integer( loaded );
    s.integer( clockOut );
    s.integer( dataOut );
    s.integer( atnOut );
    s.integer( writeProtected );
    s.integer( pulseIndex );
    s.integer( pulseDelta );
    s.integer( pulseDuration );
    s.integer( latchedByte );
    s.integer( byteReady );
    s.integer( ca1Line );
    s.integer( dataDirection );
    s.integer( frequency );
    s.integer( side );
    s.integer( operation );
    s.integer( syncPos );
    s.integer( nibble );
    s.integer( profDosAutoSpeed );
    s.integer( prologic40TrackMode );
    s.integer( prologic2Mhz );
    s.integer( extendedMemoryMap );
    s.integer( turboTransVisible );
    s.integer( turboTransPage );
    s.integer( proSpeedControl );
    s.integer( hidden );
    s.integer( dskChange );

    if (number == 0) {
        s.integer(Drive::rpm);
        s.integer(Drive::wobble);
        s.integer(Drive::wobblePos);
        s.integer(Drive::wobbleLimit);
        s.integer(Drive::refCyclesPerRevolution);
        s.integer(Drive::Mechanics::enabled);
        s.integer(Drive::Mechanics::acceleration);
        s.integer(Drive::Mechanics::deceleration);
        s.integer(Drive::Mechanics::stepperSeekTime);
        s.integer((uint8_t&)Drive::globalType);
    }

    if ((operation & DRIVE_MODE_158x) == 0) {
        via1.serialize( s );
        via2.serialize( s );
    }
    cpu.serialize( s );

    if (operation & (DRIVE_MODE_157x | DRIVE_MODE_158x) ) {
        cia.serialize(s);
        wd1770.serialize(s);
    }

    if (operation & DRIVE_HAS_PIA)
        pia.serialize(s);
    if (operation & DRIVE_HAS_EXTRA_CIA)
        ciaSpeeder.serialize(s);

    if (s.mode() == Emulator::Serializer::Mode::Load) {

        updateCycleSpeed( use2Mhz() );

        setFirmwareByType();

        gcrTrack = structure.getTrackPtr( side, currentHalftrack );

        if (operation & (DRIVE_MODE_157x  | DRIVE_MODE_158x) ) {
            wd1770.setTrack( gcrTrack );
            wd1770.setDiskAccessible( motorOn && loaded );
        }

        // unserialize VIA, CIA before to get state of LED
        (operation & DRIVE_MODE_158x) ? updateDeviceState1581() : updateDeviceState();
    }
       
    structure.serialize( s, written );
}

}