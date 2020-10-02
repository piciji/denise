
namespace LIBC64 {  
    
auto System::dispatcha() -> void {
   
    if ( (vicII->useSequencer() == true) && (true == (expansionPort != noExpansion) ) ) {
        
        cpuCtx->sync = [this]() {
            sysTimer.process();
            cia1->clock();
            vicII->clock();
            cia2->clock();
            expansionPort->clock();
        };  
        
    } else if ( (vicII->useSequencer() == true) && (false == (expansionPort != noExpansion) ) ) { 
        
        cpuCtx->sync = [this]() {
            sysTimer.process();
            cia1->clock();
            vicII->clock();
            cia2->clock();
        };  
    } else if ( (vicII->useSequencer() == false) && (true == (expansionPort != noExpansion) ) ) { 
        
        cpuCtx->sync = [this]() {
            sysTimer.process();
            cia1->clock();
            vicII->clockSilence();
            cia2->clock();
            expansionPort->clock();
        };  
        
    } else if ( (vicII->useSequencer() == false) && (false == (expansionPort != noExpansion) ) ) { 
        
        cpuCtx->sync = [this]() {
            sysTimer.process();
            cia1->clock();
            vicII->clockSilence();
            cia2->clock();
        };  
    }
}
    
}
