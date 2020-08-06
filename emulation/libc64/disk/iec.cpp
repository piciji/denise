
#include "iec.h"
#include "../../tools/thread.h"

namespace LIBC64 {
    
IecBus* iecBus;
   
// 1. main thread (c64) runs some cycles, drive thread waits
// 2. main thread transfers amount of consumed cycles to drive thread
// 3. drive thread runs for this amount of cycles
// 4. main thread doesn't wait for drive thread and goes on (real parallel)
// 5. depending on which thread finish his work first, threads waits for each other
// 6. go to second step and repeat
// NOTE: main thread runs a few cycles or stops before a iec read/write, then
// synchronizes with drive thread and goes on. in case of iec access, main thread
// and drive thread don't run in parallel. main thread waits for drive thread to keep up
// before data is transfered in a cycle exact way.                 

// we have to control the behaviour when c64 and drive(s) access iec bus at nearly same time.
// if both reads or writes it doesn't matter which side access bus first.
// if one side reads and the other writes we have to carefully control bus access.
// when accessing exactly same time a read happens before a write.
// to find out the time window for a write to be reflected by a read we need to know
// some facts.
// 1. how late in a cycle data can change to be readed back by cpu ?

// synertek hardware manual says for 1 MHz operation.
// address is guaranted to be stable 300 ns after leading edge of phase 1.
// a memory device has ~575 ns to make data available on the bus.
// means a via bus read can detect data changes no later than 875 ns within a cycle.
// a cia read operates not exactly at 1 Mhz. pal is 985248 (862 ns) and ntsc is 1022727 (895 ns).
// so a safe value could be 850 ns for all participants, don't know how far we 
// should approach the value in order to reliably read back.

// 2. how long it takes a write is visible for other iec bus connected devices ?
// it takes ~0.3 cycle.

// Note 1: when c64 gives back control for via to keep up, the actual cycle is not counted yet.
// Note 2: via and cia cycles don't have same length, therefore the alignment is changed each cycle.
// Note 3: drive cpu can not sync back between half cycles.
//  doing so would result in a speed hit and additional complexity of cpu class.
//  main cpu and drive cpu access iec bus in second half cycle, so there is no accuracy loss
//  by syncing in full cycles.

// scenario 1: cia writes, via reads
// a via read before (0.45 * drive cycle duration) shouldn't be affected by a cia write
// 0.45 + 0.85 ( data change time ) = 1.3 cycles = 1 write cycle + 0.3 write delay

// scenario 2: cia reads, via writes
// a via write before (-0.45 * drive cycle duration) should affect a cia read
// -0.45 + 1 write cycle + 0.3 write delay = 0.85 ( data change time )  

IecBus::IecBus() {    
    // max 4 drives will be supported
    for( unsigned i = 0; i < 4; i++ ) {    
        Drive1541* drive = new Drive1541( i );
        
        drives.push_back( drive );
    }     
    
    idle = true;
    powerOn = false;
        
    std::thread worker( [this] {       
               
        std::chrono::milliseconds duration(5);
        std::mutex cvM;
        std::unique_lock<std::mutex> lk(cvM);
        
        while(1) {
            
            ready = false;
            
            // let drive thread wait until main thread signals a safe start
            while (!ready.load()) {

                if (idle.load()) {
                    if (cv.wait_for(lk, duration, [this](){ return ready.load(); }))
                        break;                    
                } else
                    std::this_thread::yield();                         
            }
            
            run();
        }       
      
    } );    
    
    Emulator::setThreadPriorityRealtime( worker );
    
    worker.detach();
}  

IecBus::~IecBus() {
    
    for( auto drive : drives )
        delete drive;
}

auto IecBus::setPowerThread( bool state ) -> void {

    cpuBurnerRequested = cpuBurner = state;
    
    updateIdleState();          
}

auto IecBus::setFastForward( bool state ) -> void {

    cpuBurner = (state && (drivesConnected > 1) ) ? state : cpuBurnerRequested;
    
    updateIdleState();          
}

auto IecBus::run() -> void {
    Drive1541* useDrive;
    
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
                
                if (drive->cpu->isReadNext() > useDrive->cpu->isReadNext())
                    useDrive = drive;    
            }
        }
        
        if (!useDrive)
            break;
        
        useDrive->cpu->process();
        useDrive->synced = useDrive->cycleCounter >= syncPos;                
    }     
}

inline auto IecBus::waitForDrives() -> void {
    // wait for drive thread to finish his work    
    while ( ready.load() ) {
        // in case of main thread and drive thread run on different cores,
        // we can wait in a tight loop because of true parallelism.
        // if not so because cpu is single core or there are another cpu intense
        // applications running aside this emulator, OS scheduler could be run both
        // threads on same core. In this case we can not wait in a tight loop like this,
        // because of both threads run in interleaved mode. That means each thread is running
        // for a short time frame. There is no true parallelism. In a tight loop the OS scheduler
        // can not transfer control to another thread until loop is finished.
        // We get a typical dead lock. The second thread isn't progressing, so the first thread        
        // waits for ever. We have to transfer control programmatically. That's costly even when
        // running on different cores. But from a c++ point of view we don't know how OS scheduler
        // spread threads.
        std::this_thread::yield();
    }
}

auto IecBus::syncDrives( int64_t _syncPos, bool ciaAccess ) -> void {
    // no disk drives connected
    if ( drivesConnected == 0 )
        return;
      
    if (!ciaAccess && (cycleCounter < (cpuBurner ? 100 : 3000) ) )
        return;
    
    if (threaded)
        waitForDrives();
    
    syncPos = _syncPos;
    
    // drive cpu runs at 1 Mhz, pal c64 is slightly slower, ntsc c64 slighter faster.
    // to sync c64 and drive clock we use a simple proportional scaling term
    // when:
    // cpu clock (pal or ntsc clock) = drive clock (1.000.000)
    // then:
    // x cpu cycles = x drive cycles
    // cpu clock * x drive cycles = drive clock * x cpu cycles                
    // drive->cycleCounter -= cycleCounterTemp * 1000000; // cpu cycles * drive clock 
    int64_t _temp = cycleCounter * 1000000;
    
    for (auto drive : drivesEnabled) {
        drive->cycleCounter -= _temp;
        drive->synced = drive->cycleCounter >= syncPos;
    }
        
    cycleCounter = 0; // reset for next run 
    
    // now let the drive thread catch up with the main thread   
    if ( ciaAccess || !threaded ) {
        run();        
        
    } else {
        // if no cia access, run concurrent
        ready.store(1); 
        if (!cpuBurner)
            cv.notify_one();
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

    powerOn = true;
    cpuBurner = cpuBurnerRequested;
    updateIdleState();

    syncPos = 0;
    syncPosRead = (int64_t)(-0.455 * (double)cpuCylcesPerSecond);
    syncPosWrite = (int64_t)(0.455 * (double)cpuCylcesPerSecond);
    cycleCounter = ~0;
            
    for( auto drive : drivesEnabled ) {                   
        drive->power();
    }
}
    
auto IecBus::writeCia( uint8_t byte ) -> bool {
    // let drives catch up
    syncDrives( syncPosWrite, true );      
    
    bool atnBefore = atnOut;
    // for better readability we put out signals on separate variables
    atnOut = (byte >> 3) & 1;
    clockOut = (byte >> 4) & 1;
    dataOut = (byte >> 5) & 1;
    
    if (drivesConnected) {
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
    
    // the iec bus uses the same lines for both in / out going transfer.
    // so if there is no drive present
    // the cia reads back the same values for the 'in' marked pins
    port = (dataOut << 7) | (clockOut << 6);
    
    for (auto drive : drivesEnabled) {
        
        // override it by data sended from drives
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
    syncDrives( syncPosRead, true ); 
    
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
    
    threaded = drivesEnabled.size() > 0; 
}

auto IecBus::setDriveSpeed(double rpm, double wobble) -> void {
    for( auto drive : drives )
        drive->setSpeed( rpm, wobble );
}
    
auto IecBus::setFirmware(uint8_t* rom, unsigned romSize) -> void {
    
    for( auto drive : drives )
        drive->setFirmware( rom, romSize );
}

auto IecBus::resetDriveState() -> void {
    
    for( auto drive : drivesEnabled )
        system->interface->updateDriveState( drive->getMedia(), Drive1541::TrackState::NoOperation, 0 );
}

auto IecBus::setCpuCyclesPerSecond( unsigned cycles ) -> void {
    cpuCylcesPerSecond = cycles;
}

auto IecBus::randomizeRpm() -> void {

    for (auto drive : drivesEnabled)
        drive->randomizeRpm();
}

auto IecBus::attach( Emulator::Interface::Media* media, uint8_t* data, unsigned size ) -> void {
    
    drives[ media->id ]->attach( media, data, size );   
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
    
    return drives[ media->id ]->structure1541.getListing();
}

auto IecBus::selectListing( Emulator::Interface::Media* media, unsigned pos ) -> void {
    
    drives[ media->id ]->structure1541.selectListing( pos );
}

auto IecBus::updateIdleState() -> void {
    
    idle = (powerOn && drivesConnected > 0) ? !cpuBurner : true;
}

auto IecBus::serialize(Emulator::Serializer& s) -> void {
    // depends on region, enabled drives, firmware and disk images.
    
    waitForDrives();
    
    s.integer( atnOut );
    s.integer( clockOut );
    s.integer( dataOut );
    s.integer( port );
    s.integer( syncPos );
    s.integer( syncPosRead );
    s.integer( syncPosWrite );
    s.integer( cycleCounter );
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
    
    s.integer( cycleCounter );
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

}
