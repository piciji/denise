
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

namespace LIBC64 {  
    
auto System::dispatcha() -> void {

    c64schedule(true, false)
    c64schedule(true, true)

    c64schedule(false, false)
    c64schedule(false, true)
}
    
}

#undef c64schedule
               