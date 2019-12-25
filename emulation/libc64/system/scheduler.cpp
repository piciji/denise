
#define c64schedule(_useSequencer, _useExpansion)                                                               \
    if ( (vicII->useSequencer() == _useSequencer) && (_useExpansion == (expansionPort != noExpansion) ) ) {   \
                                                                                                                \
        cpuCtx->syncLo = [this]() {                                                                             \
            events.process();                                                                                   \
            powerSupply->tick();                                                                                \
            cia1->processLo();                                                                                  \
            vicII->phase1<_useSequencer>();                                                      \
            cia2->processLo();                                                                                  \
            sid->phase1();                                                                                      \
            if (_useExpansion ) \
                expansionPort->cycleLo();                                                                           \
        };                                                                                                      \
                                                                                                                \
        cpuCtx->syncHi = [this]() {                                                                             \
            /* let the cia1 process before Vic, so Vic can latch a lightpen trigger late this half cycle */     \
            if (_useExpansion) \
                expansionPort->cycleHi();                                                                           \
            cia1->processHi();                                                                                  \
            vicII->phase2<_useSequencer>();                                                      \
            cia2->processHi();                                                                                  \
            sid->phase2();                                                                                      \
            tape->clock();                                                                                      \
            iecBus->countTicks();                                                                               \
            input->clock();                                                                                     \
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
               