
#include "drive.h"

namespace LIBC64 {   
    
auto Drive::serialize(Emulator::Serializer& s) -> void {

    s.integer( (uint8_t&)type );
    s.integer( expandMemory );
    s.integer( cycleCounter );
    s.integer( synced );
    s.integer( irqIncomming );
    s.integer( speeder );
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
    if (speeder == 12)
        s.array( turboTrans, 512 * 1024);

    s.integer( driveCycles );
    s.integer( accum );
    s.integer( currentHalftrack );
    s.integer( speedZone );
    s.integer( byteReadyOverflow );
    s.integer( readMode );
    s.integer( headOffset );
    s.integer( coilDir );
    s.integer( ue3Counter );
    s.integer( refCyclesPerRevolution );
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
    s.integer( stepperSeekTime );
    s.integer( stepperDelay );
    s.integer( delayInProgress );
    s.integer( nextStep );
    s.integer( wasAttachDetached );
    s.integer( motorOn );
    s.integer( written );
    s.integer( loaded );
    s.integer( clockOut );
    s.integer( dataOut );
    s.integer( atnOut );
    s.integer( motorOff.slowDown );
    s.integer( motorOff.decelerationPoint );
    s.integer( motorOff.delay );
    s.integer( motorOff.pos );
    s.vector( motorOff.chunkSize );
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
    s.integer( wobble );
    s.integer( rpm );
    s.integer( nibble );
    s.integer( profDosAutoSpeed );
    s.integer( prologic40TrackMode );
    s.integer( prologic2Mhz );
    s.integer( extendedMemoryMap );
    s.integer( turboTransVisible );
    s.integer( turboTransPage );
    s.integer( proSpeedControl );
    s.integer( hidden );

    via1.serialize( s );
    via2.serialize( s );
    cpu.serialize( s );

    s.integer( structure.encodingGraceful.status );

    if (operation & DRIVE_MODE_157x) {
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

        if ( (type == Type::D1570) && (side == 1) ) {
            gcrTrack = dummyTrack;
        }

        if (operation & DRIVE_MODE_157x) {
            wd1770.setTrack( gcrTrack, gcrTrack == dummyTrack );
            wd1770.setDiskAccessible( motorOn && loaded );
        }

        // unserialize VIA before to get state of LED
        updateDeviceState();

        if (structure.encodingGraceful.status)
            // state was generated during attaching P64 (gracefully)
            postAttach();

        structure.encodingGraceful.reset();
    }
       
    structure.serialize( s, written );
}

}