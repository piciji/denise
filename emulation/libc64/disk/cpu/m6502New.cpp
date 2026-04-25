
#include "m6502New.h"
#include "../drive/drive.h"
#include "../../system/system.h"

namespace LIBC64 {

#define FLAG_N          0x80
#define FLAG_V			0x40
#define FLAG_UNUSED		0x20
#define FLAG_B			0x10
#define FLAG_D			0x08
#define FLAG_I			0x04
#define FLAG_Z          0x02
#define FLAG_C			0x01

#define SET_FLAG_ZN(val) flagZ = flagN = val; // for speed reasons
#define SET_FLAG_Z(val) flagZ = val;
#define SET_FLAG_N(val) flagN = val;
#define SET_FLAG_C(val)	if(val) regP |= FLAG_C; else regP &= ~FLAG_C;
#define SET_FLAG_V(val) if(val) regP |= FLAG_V; else regP &= ~FLAG_V;
#define SET_FLAG_D(val)	if(val) regP |= FLAG_D; else regP &= ~FLAG_D;
#define SET_FLAG_I(val)	if(val) regP |= FLAG_I; else regP &= ~FLAG_I;
#define SET_FLAG_B(val)	if(val) regP |= FLAG_B; else regP &= ~FLAG_B;

#define GET_FLAG_Z (!flagZ ? FLAG_Z : 0)
#define GET_FLAG_N (flagN & 0x80)
#define GET_FLAG_C (regP & FLAG_C)
#define GET_FLAG_D (regP & FLAG_D)
#define GET_FLAG_V (regP & FLAG_V)
#define GET_FLAG_I (regP & FLAG_I)

#define SAMPLE_INTERRUPT	\
    if(nmiPending | (irqPending & !GET_FLAG_I )) control |= IRQ;

#define STATUS	(regP | GET_FLAG_N | FLAG_UNUSED | GET_FLAG_Z)

#define READ( addr ) 	\
    { if ((control & WatchPoint) && watchPoints.check( addr )) \
        system->debugPointReached(getTheme(), DebuggerAction::Watchpoint, addr); \
    dataBus = drive->cpuRead( addr ); }

#define READ_LAST( addr ) \
    SAMPLE_INTERRUPT    \
    READ( addr )

#define WRITE( addr, value )	\
    { if ((control & WatchPointWrite) && watchPointsWrite.check( addr )) \
        system->debugPointReached(getTheme(), DebuggerAction::WatchpointWrite, addr); \
    drive->cpuWrite( addr, value); }

#define WRITE_LAST( addr, value )	\
    SAMPLE_INTERRUPT				\
    WRITE( addr, value )

#define PUSH_STATUS \
    drive->sync(); /* late detection of possible external overflow, hence STATUS is sampled after SYNC */  \
    drive->ram[ 0x100 | regS-- ] = STATUS;

#define INC_PC(value) pc += value;

#define READ_PC_INC			\
    READ( pc )				\
    INC_PC( 1 )

#define READ_PC_INC_LAST	\
    READ_LAST( pc )			\
    INC_PC( 1 )

#define PUSH( value )		\
    WRITE( 0x100 | regS, value) \
    regS--;

#define PUSH_LAST( value )		\
    WRITE_LAST( 0x100 | regS, value) \
    regS--;

#define PULL			\
    ++regS; \
    READ( 0x100 | regS )

#define PULL_LAST				\
    ++regS; \
    READ_LAST( 0x100 | regS )

#define PAGE_CROSSED 	((absIndexed ^ absolute) & 0xff00)

auto M6502New::power() -> void {
    regS = 0x00;
    regX = 0x00;
    regY = 0x00;
    regA = 0xaa;
    pc = 0x00ff;
    setStatus( 0x02 );
    reset();
}

auto M6502New::reset() -> void {
    irqPending = false;
    nmiPending = false;
    soSample = 0;
    readNext = true;
    soBlock = 0;
    control = ResetRoutine;
    operation = 0;
    M65Debugger::init();
}

auto M6502New::getTheme() -> DebuggerTheme {
    switch( drive->number ) {
        default:
        case 0: return DebuggerTheme::DriveCPU1;
        case 1: return DebuggerTheme::DriveCPU2;
        case 2: return DebuggerTheme::DriveCPU3;
        case 3: return DebuggerTheme::DriveCPU4;
    }
    _unreachable
}

auto M6502New::setStatus(uint8_t val) -> void {
    regP = ((val) & ~(FLAG_Z | FLAG_N));
    flagZ = !((val) & FLAG_Z);
    flagN = (val);
}

auto M6502New::resetRoutine() -> void {
    READ( pc )
    READ( pc )
    READ( pc )
    READ( 0x100 | regS )
    regS--;
    READ( 0x100 | regS )
    regS--;
    READ( 0x100 | regS )
    regS--;

    READ( 0xfffc )
    pc = dataBus;
    READ( 0xfffd )
    pc |= dataBus << 8;
    SET_FLAG_I( 1 )

    control &= ~ResetRoutine;
}

template<bool software> inline auto M6502New::interrupt() -> void {

    if (!software) {
        READ( pc )
        READ( pc )
    } else {
        READ_PC_INC
    }

    PUSH((pc >> 8) & 0xff);
    PUSH( pc & 0xff);

    uint16_t vector = 0xfffe;

    if (nmiPending) {
        nmiPending = false;
        vector = 0xfffa; // a late nmi can hijack irq
    }

    SET_FLAG_B( software )

    PUSH_STATUS

    if constexpr (!software) {
        if ((control & ExceptionPoint) && exceptionPoints.check( vector )) {
            system->debugPointReached(getTheme(), DebuggerAction::ExceptionPoint, vector);
        }
    }

    READ( vector )

    pc = dataBus;

    SET_FLAG_I( 1 )

    READ( vector + 1 )

    pc |= dataBus << 8;

    /**
     * no interrupt polling at the end of this service routine (software break too)
     * so at least one opcode is following before could interrupted again by nmi
     */

    control &= ~IRQ;
}

auto M6502New::setIrq(bool state) -> void {
    // level sensitive
    irqPending = state;
}

auto M6502New::setNmi() -> void {
    // edge sensitive ( triggers only: 0 -> 1)
    nmiPending = true;
}

auto M6502New::triggerSO(uint8_t delay) -> void {
    // edge sensitive
    this->soSample = delay;
}

auto M6502New::handleSo() -> void {
    // a sampled external SO is executed in following first half cycle. when cpu accesses v flag
    // internally in second half cycle, the external change in first half cycle is wasted.
    // instructions like adc, sbc don't calculate in last opcode cycle. there is
    // simply no time because the data fetch for calculation happens in last half cycle.
    // the calculation is done in first cycle of next instruction during the opcode fetch.
    // for simplicity in emulation the calculation is done in context of last instruction cycle.
    // so we need to take care in case of external SO in emulation, because it would
    // execute in wrong order. that's why we use the following "Block" variable,
    // set in the end of overflow accessing instructions.

    if (soBlock) {
        // external overflow is not delayed but prevented
        soBlock--;

        if (soSample) // only a delayed sample could survive this block, because the delay happens outside of CPU (here emulated inside)
            soSample--;

    } else if (soSample) {
        soSample--;

        if (!soSample) {
            SET_FLAG_V(1)
        }
    }
}

auto M6502New::serialize(Emulator::Serializer& s) -> void {
    s.integer( control );
    s.integer( operation );
    s.integer( irqPending );
    s.integer( nmiPending );
    s.integer( pc );
    s.integer( regX );
    s.integer( regY );
    s.integer( regA );
    s.integer( regS );
    s.integer( regP );
    s.integer( flagZ );
    s.integer( flagN );

    s.integer( readNext );

    s.integer( soBlock );
    s.integer( soSample );

    s.integer( zeroPage );
    s.integer( absolute );
    s.integer( absIndexed );
    s.integer( dataBus );
    s.integer( _value );
}

// addressing
#define ZERO						\
    READ_PC_INC						\
    zeroPage = dataBus;

#define INDEXED_INDIRECT			\
    ZERO						\
    READ( zeroPage )				\
    zeroPage += regX;				\
    READ( zeroPage )				\
    absolute = dataBus;				\
    zeroPage += 1;					\
    READ( zeroPage )				\
    absolute |= dataBus << 8;

#define INDIRECT_INDEXED( FORCE )	\
    ZERO						\
    READ( zeroPage )				\
    absolute = dataBus;				\
    zeroPage += 1;					\
    READ( zeroPage )			\
    absolute |= dataBus << 8;		\
    absIndexed = absolute + regY;					\
    if (FORCE || PAGE_CROSSED) {		\
        READ( ((absolute & 0xff00) | (absIndexed & 0xff)) );	\
    }

#define ZERO_PAGE_INDEXED( REG )	\
    ZERO						\
    READ( zeroPage )				\
    zeroPage += REG;

#define ABS							\
    READ_PC_INC						\
    absolute = dataBus;				\
    READ_PC_INC						\
    absolute |= dataBus << 8;

#define ABS_INDEXED( REG, FORCE )			\
    ABS										\
    absIndexed = absolute + REG;	\
    if (FORCE || PAGE_CROSSED) {		\
        READ( ((absolute & 0xff00) | (absIndexed & 0xff)) ); \
    }

#define _N (1 << 8)
#define _NF (2 << 8)

#define NEXT \
    operation |= _N;            \
    return;

#define END \
    operation = 0; \
    if (control) { \
        pcEdge = pc; \
        if (control & (BreakPoint | SoftStop)) \
            controlBreaks(); \
    } \
    return;

// GET
#define GET_ZERO_LAST   				\
    ZERO    \
    NEXT

#define GET_ZERO   				\
    ZERO    \
    READ(zeroPage)

#define GET_INDEXED_INDIRECT			\
    INDEXED_INDIRECT \
    NEXT

#define GET_INDIRECT_INDEXED( FORCE )			\
    INDIRECT_INDEXED( FORCE )       \
    NEXT

#define GET_ABS \
    ABS \
    NEXT

#define GET_ABS_INDEXED_REGY( FORCE ) \
    ABS_INDEXED( regY, FORCE ) \
    NEXT

#define GET_ABS_INDEXED_REGX( FORCE ) \
    ABS_INDEXED( regX, FORCE ) \
    NEXT

#define GET_ZERO_INDEXED_REGX_LAST \
    ZERO_PAGE_INDEXED( regX )			\
    NEXT

#define GET_ZERO_INDEXED_REGX \
    ZERO_PAGE_INDEXED( regX )			\
    READ(zeroPage)

#define GET_ZERO_INDEXED_REGY_LAST \
    ZERO_PAGE_INDEXED( regY )			\
    NEXT

#define GET_ZERO_INDEXED_REGY \
    ZERO_PAGE_INDEXED( regY )			\
    READ(zeroPage)

// SET

#define SET_ABS_DUMMY \
    WRITE( absolute, dataBus ) \
    readNext = false;   \
    operation = _NF | 1; \
    return;

#define SET_ABS_INDEXED_DUMMY \
    WRITE( absIndexed, dataBus ) \
    readNext = false;   \
    operation = _NF | 2; \
    return;

#define SET_ZERO_DUMMY \
    WRITE( zeroPage, dataBus )		\
    readNext = false;   \
    operation = _NF | 3; \
    return;

#define STORE_INDEXED_INDIRECT	\
    INDEXED_INDIRECT		\
    readNext = false;   \
    NEXT

#define STORE_ZERO \
    ZERO			\
    readNext = false;   \
    NEXT

#define STORE_ABS \
    ABS			\
    readNext = false;   \
    NEXT

#define STORE_INDIRECT_INDEXED	\
    INDIRECT_INDEXED( true )	\
    readNext = false;   \
    NEXT

#define STORE_ZERO_INDEXED( INDEDX_REG )	\
    ZERO_PAGE_INDEXED( INDEDX_REG )	\
    readNext = false;   \
    NEXT

#define STORE_ABS_INDEXED( INDEDX_REG )	\
    ABS_INDEXED( INDEDX_REG, true )	\
    readNext = false;   \
    NEXT

///////////////
#define ORA		\
    regA |= dataBus;	\
    SET_FLAG_ZN( regA )  \
    END

#define AND		\
    regA &= dataBus;	\
    SET_FLAG_ZN( regA )  \
    END

#define EOR		\
    regA ^= dataBus;	\
    SET_FLAG_ZN( regA ) \
    END

#define BIT					\
    SET_FLAG_Z( dataBus & regA )	\
    SET_FLAG_N( dataBus )			\
    SET_FLAG_V( dataBus & 0x40 )    \
    soBlock = 1;    \
    END

#define LD( REG )	\
    REG = dataBus;		\
    SET_FLAG_ZN( dataBus )  \
    END

#define _ADC	\
    { unsigned result = dataBus + regA + GET_FLAG_C;	\
    if (GET_FLAG_D) { \
        SET_FLAG_Z( result & 0xff )	\
        result = (regA & 0x0f) + (dataBus & 0x0f) + GET_FLAG_C;	\
        if( result > 0x09 )	result += 0x06;	\
        bool c = result > 0x0f;	\
        result = (regA & 0xf0) + (dataBus & 0xf0) + (c << 4) + (result & 0x0f);	\
        SET_FLAG_N( result )	\
        SET_FLAG_V( ~(regA ^ dataBus) & (regA ^ result) & 0x80 ) \
        if( result > 0x9f ) result += 0x60; \
    } else {			\
        SET_FLAG_ZN( result & 0xff )							\
        SET_FLAG_V( ~(regA ^ dataBus) & (regA ^ result) & 0x80 )\
    }															\
    SET_FLAG_C( result > 0xff )								\
    regA = result & 0xff; }

#define ADC	\
    _ADC  \
    soBlock = 1;    \
    END

#define _SBC	\
    { uint8_t dataInv = ~dataBus;	\
    uint16_t result = regA + dataInv + GET_FLAG_C; \
    SET_FLAG_ZN( result & 0xff )	\
    SET_FLAG_V( (regA ^ dataBus) & (regA ^ result) & 0x80 )	\
    if( GET_FLAG_D ) {       \
        int8_t AL = (regA & 0x0f) - (dataBus & 0x0f) + (GET_FLAG_C ? 0 : -1);	\
        if (AL < 0) AL = ((AL - 0x06) & 0x0F) - 0x10;	\
        int16_t res2 = (regA & 0xf0) - (dataBus & 0xf0) + AL;	\
        if (res2 < 0) res2 -= 0x60;	\
        regA = res2 & 0xff;	\
    } else	\
        regA = result & 0xff;	\
    SET_FLAG_C( result > 0xff ) }

#define SBC	\
    _SBC   \
    soBlock = 1;    \
    END

#define CP( REG )	\
    { uint16_t result = REG - dataBus; \
    SET_FLAG_C( result < 0x100 ) \
    SET_FLAG_ZN( result & 0xff ) } \
    END

#define ASL \
    SET_FLAG_C( dataBus & 0x80 )		\
    _value = dataBus << 1;	\
    SET_FLAG_ZN( _value )

#define ASL_IMPLIED     \
    READ_LAST( pc )		\
    SET_FLAG_C( regA & 0x80 )   \
    regA <<= 1;     \
    SET_FLAG_ZN( regA ) \
    END

#define ROL						\
    _value = (dataBus << 1) | GET_FLAG_C;	\
    SET_FLAG_C( dataBus & 0x80 )			\
    SET_FLAG_ZN( _value )

#define ROL_IMPLIED \
    { READ_LAST( pc )		\
    uint8_t result = (regA << 1) | GET_FLAG_C;	\
    SET_FLAG_C( regA & 0x80 )					\
    regA = result;								\
    SET_FLAG_ZN( regA ) } \
    END

#define ROR						\
    _value = (dataBus >> 1) | (GET_FLAG_C << 7);	\
    SET_FLAG_C( dataBus & 1 )			\
    SET_FLAG_ZN( _value )

#define ROR_IMPLIED								\
    { READ_LAST( pc )							\
    uint8_t result = (regA >> 1) | (GET_FLAG_C << 7);	\
    SET_FLAG_C( regA & 1 )					\
    regA = result;								\
    SET_FLAG_ZN( regA ) }   \
    END

#define	LSR	\
    SET_FLAG_C( dataBus & 1 )		\
    _value = dataBus >> 1;	\
    SET_FLAG_ZN( _value )

#define LSR_IMPLIED	\
    READ_LAST( pc )			\
    SET_FLAG_C( regA & 1 )		\
    regA >>= 1;	\
    SET_FLAG_ZN( regA ) \
    END

#define DEC	\
    _value = dataBus - 1;	\
    SET_FLAG_ZN( _value )

#define DEC_IMPLIED( REG )	\
    READ_LAST( pc )	\
    REG--;	\
    SET_FLAG_ZN( REG )	\
    END

#define INC	\
    _value = dataBus + 1;	\
    SET_FLAG_ZN( _value )

#define INC_IMPLIED( REG )	\
    READ_LAST( pc )	\
    REG++;	\
    SET_FLAG_ZN( REG )		\
    END

#define SLO \
    SET_FLAG_C( dataBus & 0x80 ) \
    _value = dataBus << 1; \
    regA |= _value; \
    SET_FLAG_ZN( regA )

#define	RLA	\
    _value = (dataBus << 1) | GET_FLAG_C;	\
    SET_FLAG_C( dataBus & 0x80 )			\
    regA &= _value;		\
    SET_FLAG_ZN( regA )

#define RRA	\
    { _value = (dataBus >> 1) | (GET_FLAG_C << 7);	\
    SET_FLAG_C( dataBus & 1 )	\
    uint8_t temp = dataBus; \
    dataBus = _value;   \
    _ADC  /* soBlock not needed, V calculation happens in last cycle */   \
    dataBus = temp; }

#define	SRE	\
    SET_FLAG_C( dataBus & 1 )	\
    _value = dataBus >> 1;	\
    regA ^= _value;		\
    SET_FLAG_ZN( regA ) \

#define ALR	\
    regA &= dataBus;	\
    SET_FLAG_C( regA & 1 )	\
    regA >>= 1;	\
    SET_FLAG_ZN( regA ) \
    END

#define ARR	\
    { dataBus &= regA;	\
    uint8_t result = (dataBus >> 1) | (GET_FLAG_C << 7);	\
    if (GET_FLAG_D) { \
        uint8_t AH = dataBus >> 4;	\
        uint8_t AL = dataBus & 15;	\
        SET_FLAG_N( GET_FLAG_C << 7 )	\
        SET_FLAG_Z( result )	\
        SET_FLAG_V( ((dataBus ^ result) & 0x40) != 0 )	\
        if (AL + (AL & 1) > 5) \
        result = (result & 0xf0) | ((result + 6) & 0xf);	\
        SET_FLAG_C( (AH + (AH & 1)) > 5 )	\
        if ( GET_FLAG_C )	\
        result += 0x60;	\
    } else {	\
        SET_FLAG_V( (dataBus & 0x80) ^ ((dataBus & 0x40) << 1) )	\
        SET_FLAG_C( dataBus & 0x80 ) \
        SET_FLAG_ZN( result ) \
    } \
    regA = result; }    \
    soBlock = 1;    \
    END

#define ANC	\
    regA &= dataBus;		\
    SET_FLAG_ZN( regA ) \
    SET_FLAG_C( GET_FLAG_N )    \
    END

#define ANE	\
    READ_PC_INC_LAST	\
    regA = ( regA | 0xee ) & regX & dataBus;	\
    SET_FLAG_ZN( regA ) \
    END

#define LAX \
    regA = regX = dataBus;	\
    SET_FLAG_ZN( dataBus )  \
    END

#define LAS	\
    regA = dataBus & regS; \
    regX = regS = regA;	\
    SET_FLAG_ZN( regA ) \
    END

#define LXA	\
    { READ_PC_INC_LAST	\
    regA = regX = ( regA | 0xee ) & dataBus;	\
    SET_FLAG_ZN( regA ) }   \
    END

#define DCP	\
    _value = dataBus - 1;	\
    SET_FLAG_C( regA >= _value ) \
    SET_FLAG_ZN( (regA - _value) & 0xff )

#define SBX	\
    { uint16_t result = (regA & regX) - dataBus;	\
    SET_FLAG_C( result < 0x100 )	\
    regX = result & 0xff;	\
    SET_FLAG_ZN( regX ) }   \
    END

#define ISC	\
    { _value = dataBus + 1;	\
    uint8_t temp = dataBus; \
    dataBus = _value;   \
    _SBC   /* soBlock not needed, V calculation happens in last cycle */     \
    dataBus = temp; }

#define PHP			\
    SAMPLE_INTERRUPT    \
    SET_FLAG_B( 1 ) \
    PUSH_STATUS \
    END

#define PLA			\
    PULL_LAST					\
    regA = dataBus;				\
    SET_FLAG_ZN( dataBus )  \
    END

#define CLC				\
    READ_LAST( pc )		\
    SET_FLAG_C( 0 ) \
    END

#define CLI				\
    READ_LAST( pc )		\
    SET_FLAG_I( 0 ) \
    END

#define CLV	\
    READ_LAST( pc )		\
    SET_FLAG_V( 0 )     \
    soBlock = 2;    \
    END

#define CLD	\
    READ_LAST( pc )		\
    SET_FLAG_D( 0 ) \
    END

#define SED	\
    READ_LAST( pc )		\
    SET_FLAG_D( 1 ) \
    END

#define SEC \
    READ_LAST( pc ) \
    SET_FLAG_C( 1 ) \
    END

#define SEI				\
    READ_LAST( pc )		\
    SET_FLAG_I( 1 ) \
    END

#define TRANSFER( SRC, TARGET )	\
    READ_LAST( pc )	\
    TARGET = SRC;   \
    END

#define TRANSFER_WITH_FLAG( SRC, TARGET )	\
    READ_LAST( pc )	\
    TARGET = SRC;   \
    SET_FLAG_ZN( SRC )  \
    END

#define JSR						\
    READ_PC_INC					\
    absolute = dataBus;			\
    READ( pc )					\
    PUSH( pc >> 8 )				\
    PUSH( pc & 0xff )			\
    NEXT

#define JMP_INDIRECT	\
    READ_PC_INC			\
    absolute = dataBus;	\
    READ_PC_INC			\
    absolute |= dataBus << 8;	\
    READ( absolute )	\
    pc = dataBus;	\
    NEXT

#define BRANCH_BODY \
    absolute = pc + int8_t(dataBus); \
    READ( pc )		\
    if (!((pc ^ absolute) & 0xff00)) { \
        pc = absolute; \
        END \
    } \
    operation = _NF; \
    return;

#define RTI	\
    READ_PC_INC					\
    READ( 0x100 | regS )		\
    PULL						\
    setStatus( dataBus );		\
    PULL						\
    pc = dataBus;   \
    NEXT

#define RTS	\
    READ_PC_INC					\
    READ( 0x100 | regS )		\
    PULL						\
    pc = dataBus;				\
    PULL						\
    pc |= dataBus << 8;         \
    NEXT

#define KILL				\
    READ_PC_INC				\
    READ( 0xffff )			\
    READ( 0xfffe )			\
    READ( 0xfffe )			\
    READ( 0xffff )			\
    system->jam(drive->media); \
    control |= Halt; \
    END

#define H1_AND_WRITE( VALUE )	\
    { uint8_t strange = (absolute >> 8) + 1;	\
    strange &= VALUE;	\
    if (PAGE_CROSSED)	\
        absIndexed = (strange << 8) | (absIndexed & 0xff);    \
    WRITE_LAST( absIndexed, strange ); }  \
    END

auto M6502New::process() -> void {
    switch (operation) {
        case 0:
            readNext = true;

            if (control) {
                if (control & Halt) {
                    READ( 0xffff )
                    return;
                }

                if (control & IRQ) {
                    interrupt<false>();
                    pcEdge = pc;
                    if (control & (BreakPoint | SoftStop))
                        controlBreaks();
                    return;
                }

                if (control & ResetRoutine) {
                    resetRoutine();
                }

                if (control & History)
                    loadTrace(historyHandler.getNext());

                if ((control & WatchPoint) && watchPoints.check( pc ))
                    system->debugPointReached(getTheme(), DebuggerAction::Watchpoint, pc);
            }

            operation = drive->cpuRead( pc++ );
            switch (operation) {
                case 0x00:
                    interrupt<true>( );
                    END

                case 0x01: // ORA( GET_INDEXED_INDIRECT )
                case 0x21: // AND( GET_INDEXED_INDIRECT )
                case 0x41: // EOR( GET_INDEXED_INDIRECT )
                case 0x61: // ADC( GET_INDEXED_INDIRECT )
                case 0xa1: // LD( GET_INDEXED_INDIRECT, regA )
                case 0xc1: // CP( GET_INDEXED_INDIRECT, regA )
                case 0xe1: // SBC( GET_INDEXED_INDIRECT )
                    GET_INDEXED_INDIRECT

                case 0x05: // ORA( GET_ZERO )
                case 0x24: // BIT( GET_ZERO )
                case 0x25: // AND( GET_ZERO )
                case 0x45: // EOR( GET_ZERO )
                case 0x65: // ADC( GET_ZERO )
                case 0xa4: // LD( GET_ZERO, regY )
                case 0xa5: // LD( GET_ZERO, regA )
                case 0xa6: // LD( GET_ZERO, regX )
                case 0xc4: // CP( GET_ZERO, regY )
                case 0xc5: // CP( GET_ZERO, regA )
                case 0xe4: // CP( GET_ZERO, regX )
                case 0xe5: // SBC( GET_ZERO )
                    GET_ZERO_LAST

                case 0x06: // ASL( GET_ZERO, SET_ZERO )
                    GET_ZERO
                    ASL
                    SET_ZERO_DUMMY

                case 0x26: // ROL( GET_ZERO, SET_ZERO )
                    GET_ZERO
                    ROL
                    SET_ZERO_DUMMY

                case 0x66: // ROR( GET_ZERO, SET_ZERO )
                    GET_ZERO
                    ROR
                    SET_ZERO_DUMMY

                case 0x46: // LSR( GET_ZERO, SET_ZERO )
                    GET_ZERO
                    LSR
                    SET_ZERO_DUMMY

                case 0xc6: // DEC( GET_ZERO, SET_ZERO )
                    GET_ZERO
                    DEC
                    SET_ZERO_DUMMY

                case 0xe6: // INC( GET_ZERO, SET_ZERO )
                    GET_ZERO
                    INC
                    SET_ZERO_DUMMY

                case 0x28: // PLP
                    READ( pc )
                    READ( 0x100 | regS )
                    NEXT

                case 0x09: // ORA( GET_IMM )
                case 0x0a: // ALS IMPLIED
                case 0x29: // AND( GET_IMM )
                case 0x2a: // ROL_IMPLIED
                case 0x10: // BRANCH( !GET_FLAG_N )
                case 0x30: // BRANCH( GET_FLAG_N )
                case 0x38: // SEC
                case 0x18: // CLC
                case 0x3a: // NOP
                case 0x1a: // NOP
                case 0x5a: // NOP
                case 0x7a: // NOP
                case 0xda: // NOP
                case 0xea: // NOP
                case 0xfa: // NOP
                case 0x49: // EOR( GET_IMM )
                case 0x4a: // LSR_IMPLIED
                case 0x50: // BRANCH( !GET_FLAG_V )
                case 0x58: // CLI
                case 0x69: // ADC( GET_IMM )
                case 0x6a: // ROR_IMPLIED
                case 0x70: // BRANCH( GET_FLAG_V )
                case 0x78: // SEI
                case 0x88: // DEC_IMPLIED( regY )
                case 0x8a: // TRANSFER_WITH_FLAG( regX, regA )
                case 0x90: // BRANCH( !GET_FLAG_C )
                case 0x98: // TRANSFER_WITH_FLAG( regY, regA )
                case 0x9a: // TRANSFER( regX, regS )
                case 0xa0: // LD( GET_IMM, regY )
                case 0xa2: // LD( GET_IMM, regX )
                case 0xa8: // TRANSFER_WITH_FLAG( regA, regY )
                case 0xa9: // LD( GET_IMM, regA )
                case 0xaa: // TRANSFER_WITH_FLAG( regA, regX )
                case 0xb0: // BRANCH( GET_FLAG_C )
                case 0xb8: // CLV
                case 0xba: // TRANSFER_WITH_FLAG( regS, regX )
                case 0xc0: // CP( GET_IMM, regY )
                case 0xc8: // INC_IMPLIED( regY )
                case 0xc9: // CP( GET_IMM, regA )
                case 0xca: // DEC_IMPLIED( regX )
                case 0xd0: // BRANCH( !GET_FLAG_Z )
                case 0xd8: // CLD
                case 0xe0: // CP( GET_IMM, regX )
                case 0xe8: // INC_IMPLIED( regX )
                case 0xe9:
                case 0xeb: // SBC( GET_IMM )
                case 0xf0: // BRANCH( GET_FLAG_Z )
                case 0xf8: // SED
                    NEXT

                case 0x08: // PHP
                case 0x48: // PHA
                    READ( pc )
                    NEXT

                case 0x0d: // ORA( GET_ABS )
                case 0x0e: // ASL( GET_ABS, SET_ABS )
                case 0x2c: // BIT( GET_ABS )
                case 0x2d: // AND( GET_ABS )
                case 0x2e: // ROL( GET_ABS, SET_ABS )
                case 0x4d: // EOR( GET_ABS )
                case 0x4e: // LSR( GET_ABS, SET_ABS )
                case 0x6d: // ADC( GET_ABS )
                case 0x6e: // ROR( GET_ABS, SET_ABS )
                case 0xac: // LD( GET_ABS, regY )
                case 0xad: // LD( GET_ABS, regA )
                case 0xae: // LD( GET_ABS, regX )
                case 0xcc: // CP( GET_ABS, regY )
                case 0xcd: // CP( GET_ABS, regA )
                case 0xce: // DEC( GET_ABS, SET_ABS )
                case 0xec: // CP( GET_ABS, regX )
                case 0xed: // SBC( GET_ABS )
                case 0xee: // INC( GET_ABS, SET_ABS )
                    GET_ABS

                case 0x11: // ORA( GET_INDIRECT_INDEXED )
                case 0x31: // AND( GET_INDIRECT_INDEXED )
                case 0x51: // EOR( GET_INDIRECT_INDEXED )
                case 0x71: // ADC( GET_INDIRECT_INDEXED )
                case 0xb1: // LD( GET_INDIRECT_INDEXED )
                case 0xd1: // CP( GET_INDIRECT_INDEXED, regA )
                case 0xf1: // SBC( GET_INDIRECT_INDEXED )
                    GET_INDIRECT_INDEXED(false)

                case 0x15: // ORA( GET_ZERO_INDEXED_REGX )
                case 0x35: // AND( GET_ZERO_INDEXED_REGX )
                case 0x55: // EOR( GET_ZERO_INDEXED_REGX )
                case 0x75: // ADC( GET_ZERO_INDEXED_REGX )
                case 0xb4: // LD( GET_ZERO_INDEXED_REGX, regY )
                case 0xb5: // LD( GET_ZERO_INDEXED_REGX, regA )
                case 0xd5: // CP( GET_ZERO_INDEXED_REGX, regA )
                case 0xf5: // SBC( GET_ZERO_INDEXED_REGX )
                    GET_ZERO_INDEXED_REGX_LAST

                case 0xb6: // LD( GET_ZERO_INDEXED_REGY, regX )
                    GET_ZERO_INDEXED_REGY_LAST

                case 0x16: // ASL( GET_ZERO_INDEXED_REGX, SET_ZERO )
                    GET_ZERO_INDEXED_REGX
                    ASL
                    SET_ZERO_DUMMY

                case 0x36: // ROL( GET_ZERO_INDEXED_REGX, SET_ZERO )
                    GET_ZERO_INDEXED_REGX
                    ROL
                    SET_ZERO_DUMMY

                case 0x56: // LSR( GET_ZERO_INDEXED_REGX, SET_ZERO )
                    GET_ZERO_INDEXED_REGX
                    LSR
                    SET_ZERO_DUMMY

                case 0x76: // ROR( GET_ZERO_INDEXED_REGX, SET_ZERO )
                    GET_ZERO_INDEXED_REGX
                    ROR
                    SET_ZERO_DUMMY

                case 0xd6: // DEC( GET_ZERO_INDEXED_REGX, SET_ZERO )
                    GET_ZERO_INDEXED_REGX
                    DEC
                    SET_ZERO_DUMMY

                case 0xf6: // INC( GET_ZERO_INDEXED_REGX, SET_ZERO )
                    GET_ZERO_INDEXED_REGX
                    INC
                    SET_ZERO_DUMMY


                case 0x19: // ORA( GET_ABS_INDEXED_REGY )
                case 0x39: // AND( GET_ABS_INDEXED_REGY )
                case 0x59: // EOR( GET_ABS_INDEXED_REGY )
                case 0x79: // ADC( GET_ABS_INDEXED_REGY )
                case 0xb9: // LD( GET_ABS_INDEXED_REGY, regA )
                case 0xbe: // LD( GET_ABS_INDEXED_REGY, regX )
                case 0xd9: // CP( GET_ABS_INDEXED_REGY, regA )
                case 0xf9: // SBC( GET_ABS_INDEXED_REGY )
                    GET_ABS_INDEXED_REGY(false)

                case 0x1d: // ORA( GET_ABS_INDEXED_REGX )
                case 0x3d: // AND( GET_ABS_INDEXED_REGX )
                case 0x5d: // EOR( GET_ABS_INDEXED_REGX )
                case 0x7d: // ADC( GET_ABS_INDEXED_REGX )
                case 0xbc: // LD( GET_ABS_INDEXED_REGX, regY )
                case 0xbd: // LD( GET_ABS_INDEXED_REGX, regA )
                case 0xdd: // CP( GET_ABS_INDEXED_REGX, regA )
                case 0xfd: // SBC( GET_ABS_INDEXED_REGX )
                    GET_ABS_INDEXED_REGX( false )

                case 0x1e: // ASL( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
                case 0x3e: // ROL( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
                case 0x5e: // LSR( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
                case 0x7e: // ROR( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
                case 0xde: // DEC( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
                case 0xfe: // INC( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
                    GET_ABS_INDEXED_REGX( true )

                case 0x20: // JSR
                    JSR

                case 0x40: // RTI
                    RTI

                case 0x60: // RTS
                    RTS

                case 0x4c: // JUMP ABS
                    READ_PC_INC
                    absolute = dataBus;
                    NEXT

                case 0x68: // PLA
                    READ( pc )
                    READ( 0x100 | regS )
                    NEXT

                case 0x6c:
                    JMP_INDIRECT

                case 0x81: // STORE_INDEXED_INDIRECT( regA )
                    STORE_INDEXED_INDIRECT

                case 0x84: // STORE_ZERO( regY )
                case 0x85: // STORE_ZERO( regA )
                case 0x86: // STORE_ZERO( regX )
                    STORE_ZERO

                case 0x8c: // STORE_ABS( regY )
                case 0x8d: // STORE_ABS( regA )
                case 0x8e: // STORE_ABS( regX )
                    STORE_ABS

                case 0x91: // STORE_INDIRECT_INDEXED( regA )
                    STORE_INDIRECT_INDEXED

                case 0x94: // STORE_ZERO_INDEXED(regX, regY)
                case 0x95: // STORE_ZERO_INDEXED(regX, regA)
                    STORE_ZERO_INDEXED( regX )

                case 0x96: // STORE_ZERO_INDEXED(regY, regX)
                    STORE_ZERO_INDEXED( regY )

                case 0x99: // STORE_ABS_INDEXED(regY, regA)
                    STORE_ABS_INDEXED(regY)

                case 0x9d: // STORE_ABS_INDEXED(regX, regA)
                    STORE_ABS_INDEXED(regX)


// undocumented
                case 0x03: // SLO( GET_INDEXED_INDIRECT, SET_ABS )
                case 0x23: // RLA( GET_INDEXED_INDIRECT, SET_ABS )
                case 0x43: // SRE( GET_INDEXED_INDIRECT, SET_ABS )
                case 0x63: // RRA( GET_INDEXED_INDIRECT, SET_ABS )
                case 0xa3: // LAX( GET_INDEXED_INDIRECT )
                case 0xc3: // DCP( GET_INDEXED_INDIRECT, SET_ABS )
                case 0xe3: // ISC( GET_INDEXED_INDIRECT, SET_ABS )
                    GET_INDEXED_INDIRECT

                case 0x07: // SLO( GET_ZERO, SET_ZERO )
                    GET_ZERO
                    SLO
                    SET_ZERO_DUMMY

                case 0x27: // RLA( GET_ZERO, SET_ZERO )
                    GET_ZERO
                    RLA
                    SET_ZERO_DUMMY

                case 0x47: // SRE( GET_ZERO, SET_ZERO )
                    GET_ZERO
                    SRE
                    SET_ZERO_DUMMY

                case 0x67: // RRA( GET_ZERO, SET_ZERO )
                    GET_ZERO
                    RRA
                    SET_ZERO_DUMMY

                case 0xc7: // DCP( GET_ZERO, SET_ZERO )
                    GET_ZERO
                    DCP
                    SET_ZERO_DUMMY

                case 0xe7: // ISC( GET_ZERO, SET_ZERO )
                    GET_ZERO
                    ISC
                    SET_ZERO_DUMMY

                case 0x0b: // ANC
                case 0x2b: // RLA( GET_ABS, SET_ABS )
                case 0x4b: // ALR
                case 0x6b: // ARR
                case 0xab: // LXA
                case 0xcb: // SBX
                    NEXT

                case 0x0c: // DUMMY( GET_ABS )
                case 0x0f: // SLO( GET_ABS, SET_ABS )
                case 0x2f: // RLA( GET_ABS, SET_ABS )
                case 0x4f: // SRE( GET_ABS, SET_ABS )
                case 0x6f: // RRA( GET_ABS, SET_ABS )
                case 0xaf: // LAX( GET_ABS )
                case 0xcf: // DCP( GET_ABS, SET_ABS )
                case 0xef: // ISC( GET_ABS, SET_ABS )
                    GET_ABS

                case 0x13: // SLO( GET_INDIRECT_INDEXED, SET_ABS_INDEXED )
                case 0x33: // RLA( GET_INDIRECT_INDEXED, SET_ABS_INDEXED )
                case 0x53: // SRE( GET_INDIRECT_INDEXED, SET_ABS_INDEXED )
                case 0x73: // RRA( GET_INDIRECT_INDEXED, SET_ABS_INDEXED )
                case 0xd3: // DCP( GET_INDIRECT_INDEXED, SET_ABS_INDEXED )
                case 0xf3: // ISC( GET_INDIRECT_INDEXED, SET_ABS_INDEXED )
                    GET_INDIRECT_INDEXED(true)

                case 0xb3: // LAX( GET_INDIRECT_INDEXED )
                    GET_INDIRECT_INDEXED(false)

                case 0x17: // SLO( GET_ZERO_INDEXED_REGX, SET_ZERO )
                    GET_ZERO_INDEXED_REGX
                    SLO
                    SET_ZERO_DUMMY

                case 0x37: // RLA( GET_ZERO_INDEXED_REGX, SET_ZERO )
                    GET_ZERO_INDEXED_REGX
                    RLA
                    SET_ZERO_DUMMY

                case 0x57: // SRE( GET_ZERO_INDEXED_REGX, SET_ZERO )
                    GET_ZERO_INDEXED_REGX
                    SRE
                    SET_ZERO_DUMMY

                case 0x77: // RRA( GET_ZERO_INDEXED_REGX, SET_ZERO )
                    GET_ZERO_INDEXED_REGX
                    RRA
                    SET_ZERO_DUMMY

                case 0xd7: // DCP( GET_ZERO_INDEXED_REGX, SET_ZERO )
                    GET_ZERO_INDEXED_REGX
                    DCP
                    SET_ZERO_DUMMY

                case 0xf7: // ISC( GET_ZERO_INDEXED_REGX, SET_ZERO )
                    GET_ZERO_INDEXED_REGX
                    ISC
                    SET_ZERO_DUMMY

                case 0xb7: // LAX( GET_ZERO_INDEXED_REGY )
                    GET_ZERO_INDEXED_REGY_LAST

                case 0x1b: // SLO( GET_ABS_INDEXED_REGY, SET_ABS_INDEXED )
                case 0x3b: // RLA( GET_ABS_INDEXED_REGY, SET_ABS_INDEXED )
                case 0x5b: // SRE( GET_ABS_INDEXED_REGY, SET_ABS_INDEXED )
                case 0x7b: // RRA( GET_ABS_INDEXED_REGY, SET_ABS_INDEXED )
                case 0xdb: // DCP( GET_ABS_INDEXED_REGY, SET_ABS_INDEXED )
                case 0xfb: // ISC( GET_ABS_INDEXED_REGY, SET_ABS_INDEXED )
                    GET_ABS_INDEXED_REGY( true )

                case 0x1f: // SLO( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
                case 0x3f: // RLA( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
                case 0x5f: // SRE( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
                case 0x7f: // RRA( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
                case 0xdf: // DCP( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
                case 0xff: // ISC( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
                    GET_ABS_INDEXED_REGX( true )

                case 0x83: // STORE_INDEXED_INDIRECT( regA & regX )
                    STORE_INDEXED_INDIRECT

                case 0x87: // STORE_ZERO( regA & regX )
                    STORE_ZERO

                case 0x8f: // STORE_ABS( regA & regX )
                    STORE_ABS

                case 0x93: // SHA_INDIRECT_INDEXED
                    STORE_INDIRECT_INDEXED

                case 0x97: // STORE_ZERO_INDEXED(regY, regA & regX)
                    STORE_ZERO_INDEXED( regY )

                case 0x9b: // TAS_ABS_INDEXED
                case 0x9e: // SH_ABS_INDEXED( regY, regX )
                case 0x9f: // SH_ABS_INDEXED( regY, regA & regX )
                    STORE_ABS_INDEXED( regY )

                case 0x9c: // SH_ABS_INDEXED( regX, regY )
                    STORE_ABS_INDEXED( regX )

                case 0xbb: // LAS
                case 0xbf: // LAX( GET_ABS_INDEXED_REGY )
                    GET_ABS_INDEXED_REGY( false )

                case 0x04: // DUMMY( GET_ZERO )
                case 0x44:
                case 0x64:
                case 0xa7: // LAX( GET_ZERO )
                    GET_ZERO_LAST

                case 0x14: // DUMMY( GET_ZERO_INDEXED_REGX )
                case 0x34:
                case 0x54:
                case 0x74:
                case 0xd4:
                case 0xf4:
                    GET_ZERO_INDEXED_REGX_LAST

                case 0x1c: // DUMMY( GET_ABS_INDEXED_REGX )
                case 0x3c:
                case 0x5c:
                case 0x7c:
                case 0xdc:
                case 0xfc:
                    GET_ABS_INDEXED_REGX( false )

                case 0x80: // DUMMY( GET_IMM )
                case 0x82:
                case 0x89:
                case 0xc2:
                case 0xe2:
                case 0x8b:
                    NEXT

                case 0x02:
                case 0x12:
                case 0x22:
                case 0x32:
                case 0x42:
                case 0x52:
                case 0x62:
                case 0x72:
                case 0x92:
                case 0xb2:
                case 0xd2:
                case 0xf2:
                    KILL
            }
            break;

        case _N | 0xe5: // SBC( GET_ZERO )
        case _N | 0xf5: // SBC( GET_ZERO_INDEXED_REGX )
            READ_LAST( zeroPage )
            SBC

        case _N | 0xe1: // SBC( GET_INDEXED_INDIRECT )
        case _N | 0xed: // SBC( GET_ABS )
            READ_LAST( absolute )
            SBC

        case _N | 0xe9: // SBC( GET_IMM )
        case _N | 0xeb:
            READ_PC_INC_LAST
            SBC

        case _N | 0xf1: // SBC( GET_INDIRECT_INDEXED )
        case _N | 0xf9: // SBC( GET_ABS_INDEXED_REGY )
        case _N | 0xfd: // SBC( GET_ABS_INDEXED_REGX )
            READ_LAST( absIndexed )
            SBC

        case _N | 0xc0: // CP( GET_IMM, regY )
            READ_PC_INC_LAST
            CP(regY)

        case _N | 0xc1: // CP( GET_INDEXED_INDIRECT, regA )
        case _N | 0xcd: // CP( GET_ABS, regA )
            READ_LAST( absolute )
            CP(regA)

        case _N | 0xcc: // CP( GET_ABS, regY )
            READ_LAST( absolute )
            CP(regY)

        case _N | 0xec: // CP( GET_ABS, regX )
            READ_LAST( absolute )
            CP(regX)

        case _N | 0xc4: // CP( GET_ZERO, regY )
            READ_LAST( zeroPage )
            CP(regY)

        case _N | 0xc5: // CP( GET_ZERO, regA )
        case _N | 0xd5: // CP( GET_ZERO_INDEXED_REGX, regA )
            READ_LAST( zeroPage )
            CP(regA)

        case _N | 0xe4: // CP( GET_ZERO, regX )
            READ_LAST( zeroPage )
            CP(regX)

        case _N | 0xc9: // CP( GET_IMM, regA )
            READ_PC_INC_LAST
            CP(regA)

        case _N | 0xe0: // CP( GET_IMM, regX )
            READ_PC_INC_LAST
            CP(regX)

        case _N | 0xd1: // CP( GET_INDIRECT_INDEXED, regA )
        case _N | 0xd9: // CP( GET_ABS_INDEXED_REGY, regA )
        case _N | 0xdd: // CP( GET_ABS_INDEXED_REGX, regA )
            READ_LAST( absIndexed )
            CP(regA)

        case _N | 0xa0: // LD( GET_IMM, regY )
            READ_PC_INC_LAST
            LD(regY)

        case _N | 0xa2: // LD( GET_IMM, regX )
            READ_PC_INC_LAST
            LD(regX)

        case _N | 0xa9: // LD( GET_IMM, regA )
            READ_PC_INC_LAST
            LD(regA)

        case _N | 0xa1: // LD( GET_INDEXED_INDIRECT, regA )
        case _N | 0xad: // LD( GET_ABS, regA )
            READ_LAST( absolute )
            LD(regA)

        case _N | 0xac: // LD( GET_ABS, regY )
            READ_LAST( absolute )
            LD(regY)

        case _N | 0xae: // LD( GET_ABS, regX )
            READ_LAST( absolute )
            LD(regX)

        case _N | 0xa4: // LD( GET_ZERO, regY )
        case _N | 0xb4: // LD( GET_ZERO_INDEXED_REGX, regY )
            READ_LAST( zeroPage )
            LD(regY)

        case _N | 0xa5: // LD( GET_ZERO, regA )
        case _N | 0xb5: // LD( GET_ZERO_INDEXED_REGX, regA )
            READ_LAST( zeroPage )
            LD(regA)

        case _N | 0xa6: // LD( GET_ZERO, regX )
        case _N | 0xb6: // LD( GET_ZERO_INDEXED_REGY, regX )
            READ_LAST( zeroPage )
            LD(regX)

        case _N | 0xb1: // LD( GET_INDIRECT_INDEXED, regA )
        case _N | 0xb9: // LD( GET_ABS_INDEXED_REGY, regA )
        case _N | 0xbd: // LD( GET_ABS_INDEXED_REGX, regA )
            READ_LAST( absIndexed )
            LD(regA)

        case _N | 0xbc: // LD( GET_ABS_INDEXED_REGX, regY )
            READ_LAST( absIndexed )
            LD(regY)

        case _N | 0xbe: // LD( GET_ABS_INDEXED_REGY, regX )
            READ_LAST( absIndexed )
            LD(regX)

        case _N | 0x01: // ORA( GET_INDEXED_INDIRECT )
        case _N | 0x0d: // ORA( GET_ABS )
            READ_LAST(absolute)
            ORA

        case _N | 0x21: // AND( GET_INDEXED_INDIRECT )
        case _N | 0x2d: // AND( GET_ABS )
            READ_LAST(absolute)
            AND

        case _N | 0x41: // EOR( GET_INDEXED_INDIRECT )
        case _N | 0x4d: // EOR( GET_ABS )
            READ_LAST(absolute)
            EOR

        case _N | 0x2c: // BIT( GET_ABS )
            READ_LAST(absolute)
            BIT

        case _N | 0x61: // ADC( GET_INDEXED_INDIRECT )
        case _N | 0x6d: // ADC( GET_ABS )
            READ_LAST(absolute)
            ADC

        case _N | 0x05: // ORA( GET_ZERO )
        case _N | 0x15: // ORA( GET_ZERO_INDEXED_REGX )
            READ_LAST(zeroPage)
            ORA

        case _N | 0x25: // AND( GET_ZERO )
        case _N | 0x35: // AND( GET_ZERO_INDEXED_REGX )
            READ_LAST( zeroPage )
            AND

        case _N | 0x45: // EOR( GET_ZERO )
        case _N | 0x55: // EOR( GET_ZERO_INDEXED_REGX )
            READ_LAST( zeroPage )
            EOR

        case _N | 0x65: // ADC( GET_ZERO )
        case _N | 0x75: // ADC( GET_ZERO_INDEXED_REGX )
            READ_LAST( zeroPage )
            ADC

        case _N | 0x08: // PHP
            PHP

        case _N | 0x09: // ORA( GET_IMM )
            READ_PC_INC_LAST
            ORA

        case _N | 0x29: // AND( GET_IMM )
            READ_PC_INC_LAST
            AND

        case _N | 0x49: // EOR( GET_IMM )
            READ_PC_INC_LAST
            EOR

        case _N | 0x69: // ADC( GET_IMM )
            READ_PC_INC_LAST
            ADC

        case _N | 0x0a: // ALS IMPLIED
            ASL_IMPLIED

        case _N | 0x2a: // ROL_IMPLIED
            ROL_IMPLIED

        case _N | 0x4a: // LSR_IMPLIED
            LSR_IMPLIED

        case _N | 0x6a: // ROR_IMPLIED
            ROR_IMPLIED

        case _N | 0x0e: // ASL( GET_ABS, SET_ABS )
            READ( absolute )
            ASL
            SET_ABS_DUMMY

        case _N | 0x2e: // ROL( GET_ABS, SET_ABS )
            READ(absolute)
            ROL
            SET_ABS_DUMMY

        case _N | 0x6e: // ROR( GET_ABS, SET_ABS )
            READ(absolute)
            ROR
            SET_ABS_DUMMY

        case _N | 0x4e: // LSR( GET_ABS, SET_ABS )
            READ( absolute )
            LSR
            SET_ABS_DUMMY

        case _N | 0xce: // DEC( GET_ABS, SET_ABS )
            READ( absolute )
            DEC
            SET_ABS_DUMMY

        case _N | 0xde: // DEC( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
            READ( absIndexed )
            DEC
            SET_ABS_INDEXED_DUMMY

        case _N | 0xee: // INC( GET_ABS, SET_ABS )
            READ( absolute )
            INC
            SET_ABS_DUMMY

        case _N | 0xfe: // INC( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
            READ( absIndexed )
            INC
            SET_ABS_INDEXED_DUMMY

        case _N | 0x10: // BRANCH( !GET_FLAG_N )
            READ_PC_INC_LAST
            if (GET_FLAG_N) { END }
            BRANCH_BODY

        case _N | 0x30: // BRANCH( GET_FLAG_N )
            READ_PC_INC_LAST
            if (!GET_FLAG_N) { END }
            BRANCH_BODY

        case _N | 0x50: // BRANCH( !GET_FLAG_V )
            READ_PC_INC_LAST
            if (GET_FLAG_V) { END }
            BRANCH_BODY

        case _N | 0x70: // BRANCH( GET_FLAG_V )
            READ_PC_INC_LAST
            if (!GET_FLAG_V) { END }
            BRANCH_BODY

        case _N | 0x90: // BRANCH( !GET_FLAG_C )
            READ_PC_INC_LAST
            if (GET_FLAG_C) { END }
            BRANCH_BODY

        case _N | 0xb0: // BRANCH( GET_FLAG_C )
            READ_PC_INC_LAST
            if (!GET_FLAG_C) { END }
            BRANCH_BODY

        case _N | 0xd0: // BRANCH( !GET_FLAG_Z )
            READ_PC_INC_LAST
            if (GET_FLAG_Z) { END }
            BRANCH_BODY

        case _N | 0xf0: // BRANCH( GET_FLAG_Z )
            READ_PC_INC_LAST
            if (!GET_FLAG_Z) { END }
            BRANCH_BODY

        case _NF: // BRANCH
            READ_LAST( (pc & 0xff00) | (absolute & 0xff) )
            pc = absolute;
            END

        case _N | 0x18: // CLC
            CLC

        case _N | 0x38: // SEC
            SEC

        case _N | 0x58: // CLI
            CLI

        case _N | 0x78: // SEI
            SEI

        case _N | 0xf8: // SED
            SED

        case _N | 0xb8: // CLV
            CLV

        case _N | 0xd8: // CLD
            CLD

        case _N | 0x11: // ORA( GET_INDIRECT_INDEXED )
        case _N | 0x19: // ORA( GET_ABS_INDEXED_REGY )
        case _N | 0x1d: // ORA( GET_ABS_INDEXED_REGX )
            READ_LAST( absIndexed )
            ORA

        case _N | 0x31: // AND( GET_INDIRECT_INDEXED )
        case _N | 0x39: // AND( GET_ABS_INDEXED_REGY )
        case _N | 0x3d: // AND( GET_ABS_INDEXED_REGX )
            READ_LAST(absIndexed)
            AND

        case _N | 0x51: // EOR( GET_INDIRECT_INDEXED )
        case _N | 0x59: // EOR( GET_ABS_INDEXED_REGY )
        case _N | 0x5d: // EOR( GET_ABS_INDEXED_REGX )
            READ_LAST(absIndexed)
            EOR

        case _N | 0x71: // ADC( GET_INDIRECT_INDEXED )
        case _N | 0x79: // ADC( GET_ABS_INDEXED_REGY )
        case _N | 0x7d: // ADC( GET_ABS_INDEXED_REGX )
            READ_LAST(absIndexed)
            ADC

        case _N | 0x1a: // NOP
        case _N | 0x3a:
        case _N | 0x5a:
        case _N | 0x7a:
        case _N | 0xda:
        case _N | 0xea:
        case _N | 0xfa:
            READ_LAST( pc )
            END

        case _N | 0x1e: // ASL( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
            READ( absIndexed )
            ASL
            SET_ABS_INDEXED_DUMMY

        case _N | 0x3e: // ROL( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
            READ(absIndexed)
            ROL
            SET_ABS_INDEXED_DUMMY

        case _N | 0x5e: // LSR( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
            READ( absIndexed )
            LSR
            SET_ABS_INDEXED_DUMMY

        case _N | 0x7e: // ROR( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
            READ( absIndexed )
            ROR
            SET_ABS_INDEXED_DUMMY

        case _N | 0x20: // JSR
            READ_LAST( pc )
            absolute |= dataBus << 8;
            pc = absolute;
            END

        case _N | 0x24: // BIT( GET_ZERO )
            READ_LAST( zeroPage )
            BIT

        case _N | 0x28: // PLP
            PULL_LAST
            setStatus( dataBus );
            END

        case _N | 0x40: // RTI
            PULL_LAST
            pc |= dataBus << 8;
            END

        case _N | 0x60: // RTS
            READ_PC_INC_LAST
            END

        case _N | 0x48: // PHA
            PUSH_LAST( regA )
            END

        case _N | 0x4c: // JUMP ABS
            READ_LAST( pc )
            absolute |= dataBus << 8;
            pc = absolute;
            END

        case _N | 0x68: // PLA
            PLA

        case _N | 0x6c: // JMP_INDIRECT
            READ_LAST( (absolute & 0xff00) | ((absolute + 1) & 0xff ) )
            pc |= dataBus << 8;
            END

        case _N | 0x88: // DEC_IMPLIED( regY )
            DEC_IMPLIED( regY )

        case _N | 0xca: // DEC_IMPLIED( regX )
            DEC_IMPLIED( regX )

        case _N | 0xc8: // INC_IMPLIED( regY )
            INC_IMPLIED( regY )

        case _N | 0xe8: // INC_IMPLIED( regX )
            INC_IMPLIED( regX )

        case _N | 0x8a: // TRANSFER_WITH_FLAG( regX, regA )
            TRANSFER_WITH_FLAG( regX, regA )

        case _N | 0x98: // TRANSFER_WITH_FLAG( regY, regA )
            TRANSFER_WITH_FLAG( regY, regA )

        case _N | 0xa8: // TRANSFER_WITH_FLAG( regA, regY )
            TRANSFER_WITH_FLAG( regA, regY )

        case _N | 0xaa: // TRANSFER_WITH_FLAG( regA, regX )
            TRANSFER_WITH_FLAG( regA, regX )

        case _N | 0xba: // TRANSFER_WITH_FLAG( regS, regX )
            TRANSFER_WITH_FLAG( regS, regX )

        case _N | 0x9a: // TRANSFER( regX, regS )
            TRANSFER( regX, regS )

// Writes
        case _N | 0x84: // STORE_ZERO( regY )
        case _N | 0x94: // STORE_ZERO_INDEXED(regX, regY)
            WRITE_LAST( zeroPage, regY )
            END

        case _N | 0x85: // STORE_ZERO( regA )
        case _N | 0x95: // STORE_ZERO_INDEXED(regX, regA)
            WRITE_LAST( zeroPage, regA )
            END

        case _N | 0x86: // STORE_ZERO( regX )
        case _N | 0x96: // STORE_ZERO_INDEXED(regY, regX)
            WRITE_LAST( zeroPage, regX )
            END

        case _N | 0x8c: // STORE_ABS( regY )
            WRITE_LAST( absolute, regY )
            END

        case _N | 0x8d: // STORE_ABS( regA )
        case _N | 0x81: // STORE_INDEXED_INDIRECT( regA )
            WRITE_LAST( absolute, regA )
            END

        case _N | 0x8e: // STORE_ABS( regX )
            WRITE_LAST( absolute, regX )
            END

        case _N | 0x9d: // STORE_ABS_INDEXED(regX, regA)
        case _N | 0x99: // STORE_ABS_INDEXED(regY, regA )
        case _N | 0x91: // STORE_INDIRECT_INDEXED( regA )
            WRITE_LAST( absIndexed, regA );
            END

        case _NF | 1:
            WRITE_LAST( absolute, _value )
            END

        case _NF | 2:
            WRITE_LAST( absIndexed, _value )
            END

        case _NF | 3:
            WRITE_LAST( zeroPage, _value )
            END

// undocumented
        case _N | 0xe3: // ISC( GET_INDEXED_INDIRECT, SET_ABS )
        case _N | 0xef: // ISC( GET_ABS, SET_ABS )
            READ(absolute)
            ISC
            SET_ABS_DUMMY

        case _N | 0xc3: // DCP( GET_INDEXED_INDIRECT, SET_ABS )
        case _N | 0xcf: // DCP( GET_ABS, SET_ABS )
            READ(absolute)
            DCP
            SET_ABS_DUMMY

        case _N | 0xf3: // ISC( GET_INDIRECT_INDEXED, SET_ABS_INDEXED )
        case _N | 0xfb: // ISC( GET_ABS_INDEXED_REGY, SET_ABS_INDEXED )
        case _N | 0xff: // ISC( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
            READ(absIndexed)
            ISC
            SET_ABS_INDEXED_DUMMY

        case _N | 0xd3: // DCP( GET_INDIRECT_INDEXED, SET_ABS_INDEXED )
        case _N | 0xdb: // DCP( GET_ABS_INDEXED_REGY, SET_ABS_INDEXED )
            READ(absIndexed)
            DCP
            SET_ABS_INDEXED_DUMMY

        case _N | 0xa7: // LAX( GET_ZERO )
        case _N | 0xb7: // LAX( GET_ZERO_INDEXED_REGY )
            READ_LAST( zeroPage )
            LAX

        case _N | 0xaf: // LAX( GET_ABS )
            READ_LAST( absolute )
            LAX

        case _N | 0x03: // SLO( GET_INDEXED_INDIRECT, SET_ABS )
        case _N | 0x0f: // SLO( GET_ABS, SET_ABS )
            READ(absolute)
            SLO
            SET_ABS_DUMMY

        case _N | 0x23: // RLA( GET_INDEXED_INDIRECT, SET_ABS )
        case _N | 0x2f: // RLA( GET_ABS, SET_ABS )
            READ(absolute)
            RLA
            SET_ABS_DUMMY

        case _N | 0x6f: // RRA( GET_ABS, SET_ABS )
            READ(absolute)
            RRA
            SET_ABS_DUMMY

        case _N | 0x43: // SRE( GET_INDEXED_INDIRECT, SET_ABS )
        case _N | 0x4f: // SRE( GET_ABS, SET_ABS )
            READ(absolute)
            SRE
            SET_ABS_DUMMY

        case _N | 0x63: // RRA( GET_INDEXED_INDIRECT, SET_ABS )
            READ(absolute)
            RRA
            SET_ABS_DUMMY

        case _N | 0x0b:
        case _N | 0x2b:
            READ_PC_INC_LAST
            ANC

        case _N | 0x33: // RLA( GET_INDIRECT_INDEXED, SET_ABS_INDEXED )
        case _N | 0x3b: // RLA( GET_ABS_INDEXED_REGY, SET_ABS_INDEXED )
        case _N | 0x3f: // RLA( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
            READ(absIndexed)
            RLA
            SET_ABS_INDEXED_DUMMY

        case _N | 0x13: // SLO( GET_INDIRECT_INDEXED, SET_ABS_INDEXED )
        case _N | 0x1b: // SLO( GET_ABS_INDEXED_REGY, SET_ABS_INDEXED )
        case _N | 0x1f: // SLO( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
            READ(absIndexed)
            SLO
            SET_ABS_INDEXED_DUMMY

        case _N | 0x53: // SRE( GET_INDIRECT_INDEXED, SET_ABS_INDEXED )
        case _N | 0x5b: // SRE( GET_ABS_INDEXED_REGY, SET_ABS_INDEXED )
        case _N | 0x5f: // SRE( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
            READ(absIndexed)
            SRE
            SET_ABS_INDEXED_DUMMY

        case _N | 0x73: // RRA( GET_INDIRECT_INDEXED, SET_ABS_INDEXED )
        case _N | 0x7b: // RRA( GET_ABS_INDEXED_REGY, SET_ABS_INDEXED )
        case _N | 0x7f: // RRA( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
            READ(absIndexed)
            RRA
            SET_ABS_INDEXED_DUMMY

        case _N | 0xdf: // DCP( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
            READ(absIndexed)
            DCP
            SET_ABS_INDEXED_DUMMY

        case _N | 0x0c: // DUMMY( GET_ABS )
            READ_LAST( absolute )
            END

        case _N | 0x4b: // ALR
            READ_PC_INC_LAST
            ALR

        case _N | 0x6b: // ARR
            READ_PC_INC_LAST
            ARR

        case _N | 0x8b:
            ANE

        case _N | 0x8f: // STORE_ABS( regA & regX )
        case _N | 0x83: // STORE_INDEXED_INDIRECT( regA & regX )
            WRITE_LAST( absolute, regA & regX )
            END

        case _N | 0x87: // STORE_ZERO( regA & regX )
        case _N | 0x97: // STORE_ZERO_INDEXED(regY, regA & regX)
            WRITE_LAST( zeroPage, regA & regX )
            END

        case _N | 0x93: // SHA_INDIRECT_INDEXED
        case _N | 0x9f: // SH_ABS_INDEXED( regX, regA & regX )
            H1_AND_WRITE( regA & regX )

        case _N | 0x9b: // TAS_ABS_INDEXED
            regS = regA & regX;
            H1_AND_WRITE( regS )

        case _N | 0x9c: // SH_ABS_INDEXED( regX, regY )
            H1_AND_WRITE( regY )

        case _N | 0x9e: // SH_ABS_INDEXED( regX, regX )
            H1_AND_WRITE( regX )

        case _N | 0xa3: // LAX( GET_INDEXED_INDIRECT )
            READ_LAST( absolute )
            LAX

        case _N | 0xab: // LXA
            LXA

        case _N | 0xb3: // LAX( GET_INDIRECT_INDEXED )
        case _N | 0xbf: // LAX( GET_ABS_INDEXED_REGY )
            READ_LAST( absIndexed )
            LAX

        case _N | 0xbb: // LAS
            READ_LAST( absIndexed )
            LAS

        case _N | 0xcb: // SBX
            READ_PC_INC_LAST
            SBX

        case _N | 0x04: // DUMMY( GET_ZERO )
        case _N | 0x44:
        case _N | 0x64:
        case _N | 0x14: // DUMMY( GET_ZERO_INDEXED_REGX )
        case _N | 0x34:
        case _N | 0x54:
        case _N | 0x74:
        case _N | 0xd4:
        case _N | 0xf4:
            READ_LAST(zeroPage)
            END

        case _N | 0x1c:
        case _N | 0x3c:
        case _N | 0x5c:
        case _N | 0x7c:
        case _N | 0xdc:
        case _N | 0xfc:
            READ_LAST( absIndexed )
            END

        case _N | 0x80: // DUMMY( GET_IMM )
        case _N | 0x82:
        case _N | 0x89:
        case _N | 0xc2:
        case _N | 0xe2:
            READ_PC_INC_LAST
            END
    }
}

auto M6502New::parseExpressionValue(const std::string& input, int& pos) -> uint32_t {
    for (auto& cond : DebuggerSnapshot::breakConditions) {
        std::string token = cond.ident;
        if (input.compare(pos, token.size(), token) == 0) {
            pos += token.size();

            switch (cond.vector) {
                default: return 0;
                case 0: return system->vicII->getVcounter();
                case 1: return system->vicII->getCycle();
                case 2: return pc;
                case 3: return regX;
                case 4: return regY;
                case 5: return regA;
                case 6: return regS;
                case 7: return regP;

                case 12: return irqPending;
                case 13: return nmiPending;

                case 14: return !!(regP & 1);
                case 15: return !!(regP & 2);
                case 16: return !!(regP & 4);
                case 17: return !!(regP & 8);
                case 18: return !!(regP & 0x10);
                case 19: return !!(regP & 0x40);
                case 20: return !!(regP & 0x80);

                case 100:
                case 101:
                case 102:
                case 103:
                case 104:
                case 105: {
                    int radix = 10;
                    if (input.compare(pos, 1, "$") == 0) {
                        radix = 16;
                        pos++;
                    }
                    const char* start = input.c_str() + pos;
                    char* end;
                    uint32_t value = std::strtoul(start, &end, radix);
                    if (start != end) {
                        pos += (end - start);
                        return system->peekMemoryByIdent( value, cond.vector );
                    }
                    return 0;
                }
            }
        }
    }
    return 0;
}

auto M6502New::updateSnapshot(DebuggerSnapshot& snap) -> void {
    auto& s = snap.drives[drive->number];

    s.pc = pc;
    s.pcEdge = pcEdge;
    s.regA = regA;
    s.regX = regX;
    s.regY = regY;
    s.regS = 0x100 | regS;
    s.flags = STATUS;
}

auto M6502New::peek(uint16_t addr) -> uint8_t {
    return drive->cpuRead<true>( addr );
}

auto M6502New::flagDebugAction(int action, bool state) -> void {
    if (state)
        control |= action;
    else
        control &= ~action;
}

auto M6502New::loadTrace(Emulator::HistoryEntry<uint8_t>& entry) -> void {
    uint16_t addr = pcEdge;
    entry.addr = addr;
    entry.flags = STATUS;

    for (int i = 0; i < 3; ++i) {
        entry.mem[i] = peek( addr++ );
    }
}

auto M6502New::controlBreaks() -> void {
    if ((control & SoftStop) && checkSoftStop(pcEdge)) {
        system->debugPointReached(getTheme(), DebuggerAction::Softstop, pcEdge);
    } else if ((control & BreakPoint) && breakPoints.check(pcEdge)) {
        system->debugPointReached(getTheme(), DebuggerAction::Breakpoint, pcEdge);
    }
}

}
