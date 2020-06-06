
namespace LIBC64 {  
    
auto System::dispatcha() -> void {
   
    if ( (vicII->useSequencer() == true) && (true == (expansionPort != noExpansion) ) ) {
        
        cpuCtx->sync = [this]() {
            iecBus->countTicks();
            events.process();
            powerSupply->clock();
            cia1->clock();
            vicII->clock();
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
            vicII->clock();
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
