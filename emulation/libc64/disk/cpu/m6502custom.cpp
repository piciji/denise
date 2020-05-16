
#include "m6502custom.h"

#define A ctx->a
#define X ctx->x
#define Y ctx->y
#define S ctx->s
#define PC ctx->pc

#define C ctx->c
#define Z ctx->z
#define I ctx->i
#define D ctx->d
#define V ctx->v
#define N ctx->n

#include "../../../processor/m65xx/m6502/externalOverflow.h"
#include "m6502opcodes.cpp"

#define PAGE_CROSSED(x, y) ( ( (uint16_t)x >> 8 ) != ( (uint16_t)y >> 8) )
#define GET_INDEX_REG regIndex == RegX ? X : ( regIndex == RegY ? Y : X )

namespace LIBC64 {

auto M6502Custom::serialize(Emulator::Serializer& s) -> void {
    
    s.integer( step );
    s.integer( readNext );    
}
    
auto M6502Custom::power() -> void {        
    
    M6502::power();
    
    step = 0;        
    
    readNext = true;
}

auto M6502Custom::reset() -> void {

    M6502::reset();
    
    _readPC();
    _readPC();
    _readPC();

    _read(0x100 | ctx->s--);
    
    _read(0x100 | ctx->s--);
    _read(0x100 | ctx->s--);

    setPCL(_read(0xfffc));
    ctx->i = true; 
    setPCH(_read(0xfffd));
}
    
auto M6502Custom::process() -> void {    
    
    if (step == 0) {     
        readNext = true; // will be reseted before write cycles
        
        if (ctx->killed) {
            _read(0xffff);
            return;
        }

        if (ctx->interruptSampled)
            return _interrupt();

        ctx->IR = _readPCInc();             
    }
    
    _decode( ctx->IR );       
}

auto M6502Custom::_decode( uint8_t IR ) -> void {
    #define fp(name) &M6502Custom::_##name
    #define COMMA ,
    #define op(id, name, ...) case id: return _##name(__VA_ARGS__);
    #define UO //undocumented opcode but always predictable
    /**
     * don't use these kind of opcodes
     * some results differs between visual6502 and real cpu
     * Visual6502 is a digital representation so it can not handle race conditions that good
     * real cpu could produce different results, depending on a lot of things like heat, cpu version, bus usage and so on
     */
    #define UUO //unstable undocumented opcode

    switch( IR ) {
        #include "../../../processor/m65xx/m6502/optable.cpp"
    }

    #undef op
    #undef UO
    #undef UUO
    #undef COMMA	
    #undef fp
}

auto M6502Custom::detectIrq() -> void {

    ctx->irqPending = ctx->irqLine;
}

inline auto M6502Custom::sampleIrq() -> void {

    // happens during last cpu cycle    
    ctx->interruptSampled |= ctx->irqPending & ~ctx->i;
}

auto M6502Custom::_read( uint16_t addr, bool lastCycle ) -> uint8_t {         
    static uint8_t data;
    
    ctx->sync();                  
    
    handleSo();
	
    if (lastCycle)
        sampleIrq();           
    
    data = ctx->read(addr);
        
    ctx->syncHi();    
	
    ctx->irqPending = ctx->irqLine;
    
    return data;
}

auto M6502Custom::_write( uint16_t addr, uint8_t data, bool lastCycle ) -> void {    
    
    ctx->sync();
    
    if (lastCycle)
        sampleIrq();    
    
    ctx->data2 = data; 
    
    handleSo();           
    
    ctx->write(addr, ctx->data2);
	
	ctx->syncHi();    
    
    ctx->irqPending = ctx->irqLine;
}

inline auto M6502Custom::_loadZeroPage( uint8_t addr, bool lastCycle ) -> uint8_t {
    
    return _read( 0x0000 | addr, lastCycle );
}

inline auto M6502Custom::_storeZeroPage( uint8_t addr, uint8_t data, bool lastCycle ) -> void {
    
    _write( 0x0000 | addr, data, lastCycle );
}

inline auto M6502Custom::_readPCInc( bool lastCycle ) -> uint8_t {
    
    return _read( ctx->pc++, lastCycle );
}

inline auto M6502Custom::_readPC( bool lastCycle ) -> uint8_t {
    
    return _read( ctx->pc, lastCycle );
}

inline auto M6502Custom::_pushStack( uint8_t data, bool lastCycle ) -> void {
    
    _write( 0x100 | ctx->s--, data, lastCycle );
}

inline auto M6502Custom::_pullStack( bool lastCycle ) -> uint8_t {

    return _read( 0x100 | ++ctx->s, lastCycle );
}


// (d,x)
inline auto M6502Custom::_indexedIndirectAdr() -> void {
	
	ctx->zeroPage = _readPCInc();
	_read( ctx->zeroPage ); //need time for adding x register
    
	ctx->absolute = _loadZeroPage( ctx->zeroPage + ctx->x );
	ctx->absolute |= _loadZeroPage( ctx->zeroPage + ctx->x + 1 ) << 8;	
}

// (d), y
inline auto M6502Custom::_indirectIndexedAdr( bool forceExtraCycle ) -> void {
    
    ctx->zeroPage = _readPCInc();

    ctx->absolute = _loadZeroPage( ctx->zeroPage );
    ctx->absolute |= _loadZeroPage( ctx->zeroPage + 1 ) << 8;    

    ctx->absIndexed = ctx->absolute + ctx->y;
    
    ctx->boundaryCrossing = PAGE_CROSSED(ctx->absolute, ctx->absolute + ctx->y); 

    if (forceExtraCycle || ctx->boundaryCrossing)
        _read((ctx->absolute & 0xff00) | (ctx->absIndexed & 0xff));	
}

// d,x  d,y
template<M6502Reg regIndex> inline auto M6502Custom::_zeroPageIndexedAdr( ) -> void {
    
    ctx->zeroPage = _readPCInc();
    _loadZeroPage( ctx->zeroPage );
    
    ctx->zeroPage += (GET_INDEX_REG);
}

// a
inline auto M6502Custom::_absoluteAdr( ) -> void {
	
	ctx->absolute = _readPCInc();
	ctx->absolute |= _readPCInc() << 8;	
}

// a,x  a,y
template<M6502Reg regIndex> inline auto M6502Custom::_absoluteIndexedAdr( bool forceExtraCycle ) -> void {
	
	_absoluteAdr();
   
    ctx->boundaryCrossing = PAGE_CROSSED(ctx->absolute, ctx->absolute + (GET_INDEX_REG));
	
	ctx->absIndexed = ctx->absolute + (GET_INDEX_REG);

	if (forceExtraCycle || ctx->boundaryCrossing)
		_read((ctx->absolute & 0xff00) | (ctx->absIndexed & 0xff));
}

auto M6502Custom::_interrupt(bool software) -> void {

    if (!software) {
        _readPC();
        _readPC();
    } else
        _readPCInc();

    _pushStack((PC >> 8) & 0xff);
    _pushStack(PC & 0xff);

    ctx->vector = 0xfffe;

    ctx->storeFlags = true;
    _pushStack(getFlags() | (software ? 0x30 : 0x20));
    ctx->storeFlags = false;

    ctx->dataW = _read(ctx->vector);

    I = 1;

    ctx->dataW |= _read(ctx->vector + 1) << 8;

    PC = ctx->dataW;

    ctx->interruptSampled = false;
}
    
}

#undef PAGE_CROSSED
#undef GET_INDEX_REG
