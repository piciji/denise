
#include "iec.h"
#include "../system/system.h"

namespace LIBC64 {

// 1. main thread (c64) runs some cycles, drive thread waits
// 2. main thread transfers number of consumed cycles to drive thread
// 3. drive thread runs for this number of cycles
// 4. main thread doesn't wait for drive thread and goes on
// 5. depending on which thread finishes his work first, threads wait for each other
// 6. go to second step and repeat
// NOTE: main thread runs a few cycles or stops before an IEC read/write, then
// synchronizes with drive thread and goes on. in the case of IEC access, main thread
// and drive thread don't run in parallel. main thread waits for drive thread to keep up
// before data is transferred in a cycle exact way.

// we have to control the behavior when c64 and drive(s) access IEC BUS at nearly the same time.
// if both read or write, it doesn't matter which side access BUS first.
// if one side reads and the other writes, we have to carefully control BUS access.
// when accessing exactly the same time, a read happens before a writing.
// to find out the time window for a writing to be reflected by a read, we need to know some facts.

// 1. how late in a cycle, data can change to be read back by CPU ?
// synertek hardware manual says for 1 MHz operation.
// address is guaranteed to be stable 300 ns after the leading edge of phase 1.
// a memory device has ~575 ns to make data available on the bus.
// means a VIA BUS read can detect data changes no later than 875 ns within a cycle.
// a CIA read operates not exactly at 1 Mhz. pal is 985248 (862 ns) and ntsc is 1022727 (895 ns).
// so a safe value could be 850 ns for all participants, don't know how far we 
// should approach the value to reliably read back.

// 2. how long it takes a write is visible for other IEC BUS connected devices ?
// it takes ~0.3 cycle.

// Note 1: when c64 gives back control for VIA to keep up, the actual cycle is not counted yet.
// Note 2: VIA and CIA cycles don't have same length, therefore the alignment is changing each cycle.
// Note 3: emulated drive CPU can not sync back between half cycles.
//  doing so would result in a speed hit and additional complexity of CPU class.
//  main cpu and drive cpu access IEC BUS in second half cycle, so there is no accuracy loss
//  by syncing in full cycles.

// scenario 1: CIA writes, VIA reads
// 1.3 cycles (1 write + 0.3 delay) - 0.85 (data change time) = 0.45
// a VIA read before (0.45 * drive cycle duration) shouldn't be affected by a cia write
// 2 Mhz: 1.3 cycles (1 write + 0.3 delay) - 0.425 (data change time) = 0.875

// scenario 2: CIA reads, VIA writes
// 0.85 ( data change time ) - 1.3 cycles (1 write + 0.3 delay) = -0.45
// a VIA write before (-0.45 * drive cycle duration) should affect a CIA read
// 2Mhz: 0.425 ( data change time ) - 1.3 cycles (1 write + 0.3 delay) = -0.875

IecBus::IecBus(System* system, Emulator::Interface::MediaGroup* mediaGroup) :
system(system),
sysTimer(system->sysTimer) {

    // max 4 drives will be supported
    for( unsigned i = 0; i < 4; i++ ) {    
        Drive* drive = new Drive( i, system, *this, &mediaGroup->media[i] );
        
        drives.push_back( drive );
    }     

    idle = true;
    powerOn = false;
    kill = false;
    threadInitialized = false;
    cpuBurner = 0;
    cpuBurnerRequested = 0;
}

IecBus::~IecBus() {
    if (threadInitialized) {
        waitForDrives();
        idle.store(0);
        kill.store(1);
        cv.notify_one();
    }

    for( auto drive : drives )
        delete drive;
}

auto IecBus::initThread() -> void {
    threadInitialized = true;

    std::thread worker( [this] {
        std::chrono::milliseconds duration(5);
        std::mutex cvM;
        std::unique_lock<std::mutex> lk(cvM);

        if (this->system->interface->setThreadPriority(Emulator::Interface::ThreadPriority::High )) {
            // system->interface->log("IEC prio change high");
        }

        while(1) {
            ready = false;

            // let drive thread wait until main thread signals a safe start
            while (!ready.load()) {

                if (idle.load()) {
                    if (cv.wait_for(lk, duration, [this](){ return ready.load() || kill.load(); })) {
                        if (kill) {
#if defined(_WIN32) || defined(__APPLE__)
#else
                            std::this_thread::sleep_for(std::chrono::milliseconds(20));
#endif
                            kill = false;
                            return;
                        }
                        break;
                    }
                } else {
                    if (kill) {
#if defined(_WIN32) || defined(__APPLE__)
#else
                        std::this_thread::sleep_for(std::chrono::milliseconds(20));
#endif
                        kill = false;
                        return;
                    }
                    std::this_thread::yield();
                }
            }

            run();
        }
    } );
    worker.detach();
}

auto IecBus::setPowerThread( unsigned value ) -> void {
    cpuBurnerRequested = value;

    updateIdleState();          
}

auto IecBus::updateIdleState() -> void {
    bool _idle = idle;

    cpuBurner = ((cpuBurnerRequested == 2) && (drivesConnected > 2)) ? 1 : cpuBurnerRequested;
    idle = (powerOn && drivesConnected > 0) ? (cpuBurner != 1) : true;

    if (!threadInitialized && cpuBurner)
        initThread();
    else if (_idle && !idle)
        cv.notify_one();
}

auto IecBus::run() -> void {
    Drive* useDrive;
    
    while (1) {
        // we have to sort enabled drives. first run the drives most behind the c64,
        // so a possible read/write to iec bus is executed at the right order.
        // second when more drives execute same time a possible read should happen
        // before a write, otherwise the write affect the read and that would be time
        // shifting when happen the exact same half cycle.
        // NOTE: all drives run together fully cycle aligned. in real system it would
        // do it by coincidence only.
        // but this way is not wrong and easier to compare which drives access iec bus
        // at the same time.
        // Maverick Dual Drive copy depends on correct drive <> drive synchronisation.
        
        useDrive = nullptr;
        
        for (auto drive : drivesEnabled) {
            
            if (drive->synced)
                continue;
            
            if (!useDrive) {
                useDrive = drive;
                continue;
            }
                            
            if (drive->cycleCounter < useDrive->cycleCounter)
                useDrive = drive;
            
            else if (drive->cycleCounter == useDrive->cycleCounter) {
                
                if (drive->cpu.isReadNext() > useDrive->cpu.isReadNext())
                    useDrive = drive;    
            }
        }
        
        if (!useDrive)
            break;
        
        useDrive->cpu.process();
        useDrive->synced = useDrive->cycleCounter >= useDrive->syncPos;
    }     
}

auto IecBus::syncDrivesEachCycle( ) -> void {
    if (!drivesConnected)
        return;

    for (auto drive : drivesEnabled) {
        drive->cycleCounter -= drive->frequency;
        drive->synced = drive->cycleCounter >= drive->syncPosRead;
    }
    sysClock = sysTimer.clock;

    run();
}

template<bool ciaAccess> auto IecBus::syncDrives( int direction ) -> bool {
    if ( drivesConnected == 0 )
        return false;
      
    unsigned _delay = sysTimer.fallBackCycles( sysClock );
    
    if (!ciaAccess && (_delay < ((cpuBurner == 1) ? 100 : 3000) ) )
        return true;

    if (cpuBurner)
        waitForDrives();

    // drive cpu runs at 1 Mhz, pal c64 is slightly slower, NTSC c64 slightly faster.
    // to sync c64 and drive clock we use a simple proportional scaling term
    // when:
    // cpu clock (PAL or NTSC clock) = drive clock (1.000.000)
    // then:
    // x cpu cycles = x drive cycles
    // cpu clock * x drive cycles = drive clock * x cpu cycles                
    // drive->cycleCounter -= cycleCounterTemp * 1000000; // cpu cycles * drive clock 

    for (auto drive : drivesEnabled) {
        drive->setSyncPos( direction );
        drive->cycleCounter -= _delay * drive->frequency;
        drive->synced = drive->cycleCounter >= drive->syncPos;
    }
        
    sysClock = sysTimer.clock; // reset for next run 
    
    // now let the drive thread catch up with the main thread   
    if ( ciaAccess || !cpuBurner )
        run();
    else {
        ready.store(1); // run concurrent
        if (cpuBurner == 2)
            cv.notify_one();
    }

    return true;
}

auto IecBus::serialShift(bool bit) -> void {
    if (!syncDrives<true>(1))
        return;

    for (auto drive : drivesEnabled) {
        if (!drive->dataDirection)
            drive->cia.serialIn( bit );
    }
}

auto IecBus::readParallelWithHandshake() -> uint8_t {
    uint8_t out = 0xff;
    if(!syncDrives<true>(0))
        return out;

    for (auto drive : drivesEnabled) {
        if (drive->operation & DRIVE_HAS_PIA) {
            if (drive->speeder == 10) {
                drive->pia.cb1In(false);
                out &= drive->pia.iob;
            } else {
                drive->pia.ca1In(false);
                out &= drive->pia.ioa;
            }
        } else {
            if (drive->operation & DRIVE_MODE_157x) {
                if (drive->operation & DRIVE_HAS_EXTRA_CIA) {
                    drive->ciaSpeeder.setFlag();
                    out &= drive->ciaSpeeder.lines.iob;
                } else {
                    drive->cia.setFlag();
                    out &= drive->cia.lines.iob;
                }
            } else {
                drive->via1.cb1In(false);
                out &= drive->via1.lines.ioa;
            }
        }
    }
    return out;
}

auto IecBus::readParallel() -> uint8_t {
    uint8_t out = 0xff;
    if (!syncDrives<true>(0))
        return out;

    for (auto drive : drivesEnabled) {
        if (drive->operation & DRIVE_HAS_PIA) {
            if (drive->speeder == 10) {
                out &= drive->pia.iob;
            } else {
                out &= drive->pia.ioa;
            }
        } else {
            if (drive->operation & DRIVE_MODE_157x) {
                if (drive->operation & DRIVE_HAS_EXTRA_CIA)
                    out &= drive->ciaSpeeder.lines.iob;
                else
                    out &= drive->cia.lines.iob;
            } else {
                out &= drive->via1.lines.ioa;
            }
        }
    }
    return out;
}

auto IecBus::writeParallelHandshake() -> void {
    if (!syncDrives<true>(0))
        return;

    for (auto drive : drivesEnabled) {
        if (drive->operation & DRIVE_HAS_PIA) {
            if (drive->speeder == 10) {
                drive->pia.cb1In(false);
            } else {
                drive->pia.ca1In(false);
            }
        } else {
            if (drive->operation & DRIVE_MODE_157x) {
                if (drive->operation & DRIVE_HAS_EXTRA_CIA)
                    drive->ciaSpeeder.setFlag();
                else
                    drive->cia.setFlag();
            } else
                drive->via1.cb1In(false);
        }
    }
}

auto IecBus::powerOff() -> void {
    
    idle = true;
    powerOn = false;
    waitForDrives();
    
    for( auto drive : drives )
        drive->powerOff();
}

auto IecBus::power() -> void {
    atnOut = clockOut = dataOut = 1;
    
    port = 0xc0;
    lastByte = 0;
    ready = false;
    sysClock = sysTimer.clock;
            
    for( auto drive : drivesEnabled ) {                   
        drive->power();
    }

    powerOn = true;
    updateIdleState();
}

auto IecBus::writeCia( uint8_t byte ) -> bool {
    // let drives catch up
    bool result = syncDrives<true>( 1 );
    
    bool atnBefore = atnOut;
    // for better readability we put out signals on separate variables
    atnOut = (byte >> 3) & 1;
    clockOut = (byte >> 4) & 1;
    dataOut = (byte >> 5) & 1;
    
    if (result) {
        if (atnBefore != atnOut) { 
            for( auto drive : drivesEnabled ) {
                // attention please :-) there is a transition of ca1 pin ( via 1 chip )
                // for all connected drives.
                // NOTE: drive cpus can't give back control after each single cycle,
                // means drive cpus could run ahead a few cycles when syncing back.
                // but a drive cpu can give back control before an interrupt sample cycle,
                // so we can not miss an interrupt when main thread triggers a ca1 transition
                // and drive thread runs ahead.
                drive->setViaTransition( atnOut ? 0 : 1 );
            }
        }                               

        for (auto drive : drivesEnabled)       
            drive->updateBus(); // because of possible atn change 
    }
    
    updatePort();  
    
    byte &= 0x38;
    
    bool change = byte != lastByte;
    
    lastByte = byte;
    
    return change;
}

auto IecBus::updatePort() -> void {
    
    // the iec bus uses the same lines for both in / out going transfers.
    // so if there is no drive present, the cia reads back the same values for the 'in' marked pins
    port = (dataOut << 7) | (clockOut << 6);
    
    for (auto drive : drivesEnabled) {
        if (drive->hidden)
            continue;

        // override it by data sent from drives
        // Note: bit state 1 means "false", and 0 means "true"
        // a line will become "false" (released) only if all devices signal false
        // a line will become "true" (pulled down) if one or more devices signal true
        port &= (drive->dataOut << 7) | (drive->clockOut << 6);
    }        
}

auto IecBus::readVia() -> uint8_t {
    // bit 0: data in, bit 2: clock in, bit 7: atn in    
    return (port >> 7) | ((port >> 4) & 4) | (atnOut << 7);
}

auto IecBus::readCia() -> uint8_t {
    // let drives catch up
    syncDrives<true>( -1 );
	
    return port;
}

auto IecBus::setDrivesEnabled( uint8_t count ) -> void {
    
    drivesConnected = count;
    
    drivesEnabled.clear();
            
    for( auto drive : drives ) {                
        
        if ( drive->number >= count )
            break;
        
        drivesEnabled.push_back( drive );
    }
}

auto IecBus::hideDrive( Emulator::Interface::Media* media ) -> void {
    // games like "Roland's Ratrace" look for drives and crash if find one as kind of copy protection
    drives[ media->id ]->hide();
}

auto IecBus::resetDrive( Emulator::Interface::Media* media) -> void {

    waitForDrives();

    system->diskIdleOff();

    drives[media->id]->power();
}

auto IecBus::setDriveType(Drive::Type type) -> void {

    system->secondDriveCable.burstPossible = (type == Drive::Type::D1570) || (type == Drive::Type::D1571);
    system->secondDriveCable.parallelPossible = true;
    system->burstOrParallelUpdate();

    for( auto drive : drives )
        drive->setType( type );
}

auto IecBus::setDriveSpeed(unsigned rpmScaled) -> void {
    Drive::setSpeed( rpmScaled );
}

auto IecBus::getDriveSpeed() -> unsigned {
    return Drive::rpm;
}

auto IecBus::enableDeceleration(bool state) -> void {
    Drive::enableDeceleration = state;
}

auto IecBus::hasDeceleration() -> bool {
    return Drive::enableDeceleration;
}

auto IecBus::setDriveWobble(unsigned wobbleScaled) -> void {
    Drive::setWobble( wobbleScaled );
}

auto IecBus::getDriveWobble() -> unsigned {
    return Drive::wobble;
}

auto IecBus::setStepperSeekTime(unsigned stepperSeekTimeScaled) -> void {
    for( auto drive : drives )
        drive->setStepperSeekTime( stepperSeekTimeScaled );
}
    
auto IecBus::setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void {
    
    for( auto drive : drives )
        drive->setFirmware( typeId, data, size );
}

auto IecBus::disalignTracks(bool state) -> void {

    for( auto drive : drives )
        drive->structure.disalignTracks = state;
}

auto IecBus::resetDriveState() -> void {
    
    for( auto drive : drivesEnabled )
        drive->updateIdleDeviceState();
}

auto IecBus::setCpuCyclesPerSecond( unsigned cycles ) -> void {
    cpuCylcesPerSecond = cycles;
}

auto IecBus::randomizeRpm() -> void {
    Drive::randomizeRpm(drivesEnabled);
}

auto IecBus::updateSerializationSize() -> void {

    for (auto drive : drivesEnabled) {
        if (drive->written)
            drive->structure.updateSerializationSize();
    }
}

auto IecBus::insertDiskGracefully() -> void {

    diskInsertInProgress = false;
    for (auto drive : drivesEnabled) {
        if (drive->structure.encodingGraceful.status) {
            drive->structure.prepareP64Graceful();

            if (drive->structure.encodingGraceful.status) {
                diskInsertInProgress = true;
            } else {
                drive->postAttach();
            }
        }
    }
}

auto IecBus::attach( Emulator::Interface::Media* media, uint8_t* data, unsigned size, bool loadGracefully ) -> void {

    system->diskIdleOff();
    
    drives[ media->id ]->attach( data, size, loadGracefully );
}

auto IecBus::detach( Emulator::Interface::Media* media ) -> void {
            
    drives[ media->id ]->detach();
}

auto IecBus::writeProtect( Emulator::Interface::Media* media, bool state ) -> void {
    drives[ media->id ]->setWriteProtect( state );
}

auto IecBus::isWriteProtected( Emulator::Interface::Media* media ) -> bool {
    return drives[ media->id ]->writeProtected;
}

auto IecBus::getDiskListing(Emulator::Interface::Media* media) -> std::vector<Emulator::Interface::Listing>& {
    
    return drives[ media->id ]->structure.getListing( );
}

auto IecBus::selectListing( Emulator::Interface::Media* media, unsigned pos, uint8_t options ) -> void {
    
    drives[ media->id ]->structure.selectListing( pos, options );
}

auto IecBus::selectListing( Emulator::Interface::Media* media,  std::string fileName, uint8_t options ) -> void {

    drives[ media->id ]->structure.selectListing( fileName, options );
}

auto IecBus::wasAutostarted() -> bool {
    for (auto drive : drivesEnabled) {
        if (drive->structure.autoStarted)
            return true;
    }
    return false;
}

auto IecBus::serialize(Emulator::Serializer& s) -> void {
    // depends on region, enabled drives, firmware and disk images.
    
    waitForDrives();
    
    s.integer( atnOut );
    s.integer( clockOut );
    s.integer( dataOut );
    s.integer( port );
    s.integer( sysClock );
    s.integer( cpuCylcesPerSecond );
    s.integer( drivesConnected );
    s.integer( lastByte );
       
    if (s.mode() == Emulator::Serializer::Mode::Load) {
        setDrivesEnabled( drivesConnected );
        updateIdleState();
    }
    
    for (auto drive : drivesEnabled)        
        drive->serialize( s );
}

auto IecBus::serializeLight(Emulator::Serializer& s) -> void {
    
    waitForDrives();

    s.integer( sysClock );
    s.integer( atnOut );
    s.integer( clockOut );
    s.integer( dataOut );
    s.integer( port );
    s.integer( lastByte );
    
    s.integer( drivesConnected );
    
    if (s.mode() == Emulator::Serializer::Mode::Save) {
        // disable all drives for runahead
        drivesConnected = 0;
    }
}

auto IecBus::resetTicks() -> void {
	sysClock = sysTimer.clock;
}

auto IecBus::setExpandedMemory( Drive::ExpandedMemMode expandedMemMode, bool state ) -> void {
    for( auto drive : drives )
        drive->setExpandedMemory( expandedMemMode, state );
}

auto IecBus::getExpandedMemory( Drive::ExpandedMemMode expandedMemMode ) -> bool {
    return (drives[0]->expandMemory & (uint8_t)expandedMemMode) ? true : false;
}

auto IecBus::setSpeeder(uint8_t speeder) -> void {
    for( auto drive : drives )
        drive->setSpeeder( speeder );
}

auto IecBus::updateDriveSounds() -> void {
    for( auto drive : drives ) {
        if (drive->motorOn)
            system->interface->mixDriveSound( drive->media, Emulator::Interface::DriveSound::FloppySpin );
    }
}

template auto IecBus::syncDrives<false>( int direction ) -> bool;

}
