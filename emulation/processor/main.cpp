
#include "m65xx/model.h"
#include "m680x0/model.h"
#include "m680x0/unittest/tester.h"

auto run65series() -> void {

    auto ctx = MOS65FAMILY::createContext();
    
    auto m6502 = MOS65FAMILY::create6502();
    
    ctx->read = [&](uint16_t address) {
        
        if (address == 0xfffc) return 0;        
        if (address == 0xfffd) return 0;       
        if (address == 0x0) return 0xd8;
        if (address == 0x1) return 0x38;
        if (address == 0x2) return 0xa9;
        if (address == 0x3) return 0xb0; //1
        if (address == 0x4) return 0x6b;
        if (address == 0x5) return 0xca; //2
        if (address == 0x6) return 0xea;
        if (address == 0x7) return 0xea;
        
		return 0;
	};
    
    m6502->setContext( ctx );
    
    m6502->power();
    
    m6502->process();
    
    m6502->process();
    
    m6502->process();
    
    m6502->process();    
}

auto run68series() -> void {

    M68FAMILY::M68000Tester* tester = new M68FAMILY::M68000Tester;
    tester->setContext(M68FAMILY::createContext());
    tester->run();
    tester->errorCount();
    return;

    //example usage
    M68Context* ctx = M68FAMILY::createContext();
    M68Context* ctx2 = M68FAMILY::createContext();
    M68Context* ctx3 = M68FAMILY::createContext();
    ctx2->portSize = [](uint32_t adr) {
        return M68Context::Word;
    };

    M68Model* obj = M68FAMILY::create68000();
    obj->setContext(ctx);
    obj->setInterruptType(M68Model::AUTO_VECTOR);
    auto obj2 = M68FAMILY::create68020();
    obj2->setContext(ctx2);
    obj->power();
    M68Model* obj3 = M68FAMILY::create68010();
    obj3->setContext(ctx3);
    obj3->getFunctionCodes();
}

auto main(int argc, char **argv) -> int {	
		
    run65series();
    
    run68series();
    
	return 0;
}
