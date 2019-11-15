
struct {
    bool enable = false;
    bool exit = false;
    uint8_t exitCode;
    unsigned cycles;
    unsigned frames;
    unsigned frameCounter = 0;    
} debugCart;

auto setDebugCart( bool enable, unsigned cycles = 0 ) -> void {
    
    debugCart.enable = enable;
    debugCart.cycles = cycles;
}

auto initDebugCart() -> void {
    
    if (!debugCart.enable)
        return;
        
    if (ntsc)
        debugCart.frames = debugCart.cycles / C64_CYCLES_FRAME_NTSC;
    else
        debugCart.frames = debugCart.cycles / C64_CYCLES_FRAME_PAL; 
    
    if(!debugCart.frames)
        debugCart.frames = 1;
    
    debugCart.frameCounter = 0;  
}
    
inline auto checkDebugCart() -> void {
    
    if (!debugCart.enable)
        return;
    
    if (debugCart.exit) {
        interface->exit( debugCart.exitCode );
        return;
    }
    
    if (!debugCart.cycles)
        return;
    
    if (++debugCart.frameCounter == debugCart.frames) {
        
        interface->exit( 1 );
    }
}
    