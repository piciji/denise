
#define c64schedule(_useSequencer, _useExpansion)                                                               \
    if ( (vicII->useSequencer() == _useSequencer) && (_useExpansion == (expansionPort != noExpansion) ) ) {     \
                                                                                                                \
        cpuCtx->sync = [this]() {                                                                               \
            iecBus->countTicks();                                                                               \
            events.process();                                                                                   \
            powerSupply->clock();                                                                               \
            cia1->clock();                                                                                      \
            vicII->clock<_useSequencer>();                                                                      \
            cia2->clock();                                                                                      \
            sid->clock();                                                                                       \
            tape->clock();                                                                                      \
            input->clock();                                                                                     \
            if (_useExpansion )                                                                                 \
                expansionPort->clock();                                                                         \
        };                                                                                                      \
                                                                                                                \
        return;                                                                                                 \
    }

#define c64scheduleRunAhead(_useExpansion)                                                                      \
    if ( _useExpansion == (expansionPort != noExpansion) ) {                                                    \
                                                                                                                \
        cpuCtx->sync = [this]() {                                                                               \
            iecBus->countTicks();                                                                               \
            events.process();                                                                                   \
            powerSupply->clock();                                                                               \
            cia1->clock();                                                                                      \
            vicII->clockSilence();                                                                              \
            cia2->clock();                                                                                      \
            sid->clock();                                                                                       \
            tape->clock();                                                                                      \
            input->clock();                                                                                     \
            if (_useExpansion )                                                                                 \
                expansionPort->clock();                                                                         \
        };                                                                                                      \
                                                                                                                \
        return;                                                                                                 \
    }

namespace LIBC64 {  
    
auto System::dispatcha() -> void {

//    if (runAhead.frames && runAhead.pos && !fastForward.config) {
//        c64scheduleRunAhead(true)
//        c64scheduleRunAhead(false)
//        
//    } else {        
//        c64schedule(true, false)
//        c64schedule(true, true)
//
//        c64schedule(false, false)
//        c64schedule(false, true)
//    }
    
    
    if ( (vicII->useSequencer() == true) && (true == (expansionPort != noExpansion) ) ) {
        
        cpuCtx->sync = [this]() {
            iecBus->countTicks();
            events.process();
            powerSupply->clock();
            cia1->clock();
            vicII->clock<true>();
            cia2->clock();
            sid->clock();
            tape->clock();
            input->clock();
            expansionPort->clock();
        };  
        
    } else if ( (vicII->useSequencer() == true) && (false == (expansionPort != noExpansion) ) ) { 
        
        cpuCtx->sync = [this]() {
            iecBus->countTicks();
            events.process();
            powerSupply->clock();
            cia1->clock();
            vicII->clock<true>();
            cia2->clock();
            sid->clock();
            tape->clock();
            input->clock();
        };  
    } else if ( (vicII->useSequencer() == false) && (true == (expansionPort != noExpansion) ) ) { 
        
        cpuCtx->sync = [this]() {
            iecBus->countTicks();
            events.process();
            powerSupply->clock();
            cia1->clock();
            vicII->clockSilence();
            cia2->clock();
            sid->clock();
            tape->clock();
            input->clock();
            expansionPort->clock();
        };  
        
    } else if ( (vicII->useSequencer() == false) && (false == (expansionPort != noExpansion) ) ) { 
        
        cpuCtx->sync = [this]() {
            iecBus->countTicks();
            events.process();
            powerSupply->clock();
            cia1->clock();
            vicII->clockSilence();
            cia2->clock();
            sid->clock();
            tape->clock();
            input->clock();
        };  
    }
}
    
}

#undef c64schedule
#undef c64scheduleRunAhead
