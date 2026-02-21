
namespace LIBC64 {

template<bool mhz2, bool busLogger> auto M6510::process() -> void {

	uint8_t dataBus;
	uint8_t zeroPage;
	uint16_t absolute;

// FLAGS
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

#define SET_STATUS(val)						\
	regP = ((val) & ~(FLAG_Z | FLAG_N)),	\
	flagZ = !((val) & FLAG_Z),				\
	flagN = (val);

#define STATUS	(regP | GET_FLAG_N | FLAG_UNUSED | GET_FLAG_Z)

#define SAMPLE_INTERRUPT	\
	if(nmiPending | (irqPending & !GET_FLAG_I )) control |= IRQ;

#define READ( addr ) 	dataBus = busRead<false, false, mhz2, busLogger>( addr );
#define READ_LAST( addr ) 	dataBus = busRead<true, false, mhz2, busLogger>( addr );

#define WRITE( addr, value )	\
	busWrite<mhz2, busLogger>( addr, value);

#define WRITE_LAST( addr, value )	\
	SAMPLE_INTERRUPT				\
	WRITE( addr, value )

#define INC_PC(value) pc += value;

#define READ_PC_INC			\
	READ( pc )				\
	INC_PC( 1 )

#define READ_PC_INC_LAST	\
	READ_LAST( pc )			\
	INC_PC( 1 )

#define PUSH( value )		\
	WRITE( 0x100 | regS--, value)

#define PUSH_LAST( value )		\
	WRITE_LAST( 0x100 | regS--, value)

#define PULL			\
	READ( 0x100 | ++regS )

#define PULL_LAST				\
	READ_LAST( 0x100 | ++regS )

#define PAGE_CROSSED 	((absIndexed ^ absolute) & 0xff00)
#define RDY_CYCLE (busState & CPU_RDY_CYCLE)

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
	uint16_t absIndexed = absolute + regY;					\
	if (FORCE || PAGE_CROSSED) {		\
		busRead<false, true, mhz2, busLogger>( ((absolute & 0xff00) | (absIndexed & 0xff)) );	\
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
	uint16_t absIndexed = absolute + REG;	\
	if (FORCE || PAGE_CROSSED) {		\
		busRead<false, true, mhz2, busLogger>( ((absolute & 0xff00) | (absIndexed & 0xff)) ); \
	}

// GET
#define GET_INDEXED_INDIRECT		\
	INDEXED_INDIRECT					\
	READ( absolute )

#define GET_INDEXED_INDIRECT_LAST		\
	INDEXED_INDIRECT					\
	READ_LAST( absolute )
//
#define GET_INDIRECT_INDEXED		\
	INDIRECT_INDEXED( true )			\
	READ( absIndexed )

#define GET_INDIRECT_INDEXED_LAST		\
	INDIRECT_INDEXED( false )			\
	READ_LAST( absIndexed )
//
#define GET_ZERO			\
	ZERO				\
	READ( zeroPage )

#define GET_ZERO_LAST		\
	ZERO				\
	READ_LAST( zeroPage )
//
#define GET_ZERO_INDEXED_REGX_LAST	\
	ZERO_PAGE_INDEXED( regX )			\
	READ_LAST( zeroPage )

#define GET_ZERO_INDEXED_REGX	\
	ZERO_PAGE_INDEXED( regX )		\
	READ( zeroPage )
//
#define GET_ZERO_INDEXED_REGY_LAST	\
	ZERO_PAGE_INDEXED( regY )			\
	READ_LAST( zeroPage )

#define GET_ZERO_INDEXED_REGY	\
	ZERO_PAGE_INDEXED( regY )		\
	READ( zeroPage )
//
#define GET_ABS_LAST			\
	ABS							\
	READ_LAST( absolute )

#define GET_ABS			\
	ABS					\
	READ( absolute )
//
#define GET_ABS_INDEXED_REGX_LAST	\
	ABS_INDEXED( regX, false )		\
	READ_LAST( absIndexed )

#define GET_ABS_INDEXED_REGX	\
	ABS_INDEXED( regX, true )		\
	READ( absIndexed )
//
#define GET_ABS_INDEXED_REGY_LAST	\
	ABS_INDEXED( regY, false )		\
	READ_LAST( absIndexed )

#define GET_ABS_INDEXED_REGY	\
	ABS_INDEXED( regY, true )		\
	READ( absIndexed )
//
#define GET_IMM_LAST		\
	READ_PC_INC_LAST

// STORE register
#define STORE_INDEXED_INDIRECT( REG )	\
	INDEXED_INDIRECT					\
	WRITE_LAST( absolute, REG )

#define STORE_ZERO( REG )			\
	ZERO							\
	WRITE_LAST( zeroPage, REG )

#define STORE_ABS( REG )	\
	ABS	\
	WRITE_LAST( absolute, REG )

#define STORE_INDIRECT_INDEXED( REG )	\
	{ INDIRECT_INDEXED( true )	\
	WRITE_LAST( absIndexed, REG ) }

#define STORE_ZERO_INDEXED( INDEDX_REG, REG )	\
	{ ZERO_PAGE_INDEXED( INDEDX_REG )	\
	WRITE_LAST( zeroPage, REG ) }

#define STORE_ABS_INDEXED( INDEDX_REG, REG )	\
	{ ABS_INDEXED( INDEDX_REG, true )	\
	WRITE_LAST( absIndexed, REG ) }

// STORE with dummy write, hence needs more time for calculation
#define SET_ABS_INDEXED_DUMMY( VALUE )	\
	WRITE( absIndexed, dataBus )			\
	WRITE_LAST( absIndexed, VALUE )

#define SET_ABS_DUMMY( VALUE )			\
	WRITE( absolute, dataBus )			\
	WRITE_LAST( absolute, VALUE )

#define SET_ZERO_DUMMY( VALUE )	\
	WRITE( zeroPage, dataBus )		\
	WRITE_LAST( zeroPage, VALUE )


///////////////
#define ORA( GET )		\
	{ GET##_LAST			\
	regA |= dataBus;		\
	SET_FLAG_ZN( regA ) }

#define AND( GET )		\
	{ GET##_LAST			\
	regA &= dataBus;		\
	SET_FLAG_ZN( regA ) }

#define EOR( GET )		\
	{ GET##_LAST			\
	regA ^= dataBus;		\
	SET_FLAG_ZN( regA ) }

#define	ASL( GET, SET )	\
	{ GET						\
	SET_FLAG_C( dataBus & 0x80 )		\
	uint8_t result = dataBus << 1;	\
	SET##_DUMMY( result  )	\
	SET_FLAG_ZN( result ) }

#define ASL_IMPLIED     \
	READ_LAST( pc )		\
	SET_FLAG_C( regA & 0x80 )   \
	regA <<= 1;     \
	SET_FLAG_ZN( regA )

#define	LSR( GET, SET )	\
	{ GET						\
	SET_FLAG_C( dataBus & 1 )		\
	uint8_t result = dataBus >> 1;	\
	SET##_DUMMY( result  )	\
	SET_FLAG_ZN( result ) }

#define LSR_IMPLIED	\
	{ READ_LAST( pc )						\
	SET_FLAG_C( regA & 1 )		\
	regA >>= 1;	\
	SET_FLAG_ZN( regA ) }

#define BIT( GET )					\
	{ GET##_LAST					\
	SET_FLAG_Z( dataBus & regA )	\
	SET_FLAG_N( dataBus )			\
	SET_FLAG_V( dataBus & 0x40 ) }

#define ROL( GET, SET )						\
	{ GET									\
	uint8_t result = (dataBus << 1) | GET_FLAG_C;	\
	SET_FLAG_C( dataBus & 0x80 )			\
	SET##_DUMMY( result )							\
	SET_FLAG_ZN( result ) }

#define ROL_IMPLIED								\
	{ READ_LAST( pc )							\
	uint8_t result = (regA << 1) | GET_FLAG_C;	\
	SET_FLAG_C( regA & 0x80 )					\
	regA = result;								\
	SET_FLAG_ZN( regA ) }

#define ROR( GET, SET )						\
	{ GET									\
	uint8_t result = (dataBus >> 1) | (GET_FLAG_C << 7);	\
	SET_FLAG_C( dataBus & 1 )			\
	SET##_DUMMY( result )							\
	SET_FLAG_ZN( result ) }

#define ROR_IMPLIED								\
	{ READ_LAST( pc )							\
	uint8_t result = (regA >> 1) | (GET_FLAG_C << 7);	\
	SET_FLAG_C( regA & 1 )					\
	regA = result;								\
	SET_FLAG_ZN( regA ) }

#define DEC( GET, SET )	\
	{ GET					\
	uint8_t result = dataBus - 1;	\
	SET##_DUMMY( result )	\
	SET_FLAG_ZN( result ) }

#define DEC_IMPLIED( REG )	\
	READ_LAST( pc )	\
	REG--;	\
	SET_FLAG_ZN( REG )

#define INC( GET, SET )	\
	{ GET					\
	uint8_t result = dataBus + 1;	\
	SET##_DUMMY( result )	\
	SET_FLAG_ZN( result ) }

#define INC_IMPLIED( REG )	\
	READ_LAST( pc )	\
	REG++;	\
	SET_FLAG_ZN( REG )

#define LD( GET, REG )	\
	{ GET##_LAST					\
	REG = dataBus;		\
	SET_FLAG_ZN( dataBus ) }

#define CP( GET, REG )	\
	{ GET##_LAST					\
	uint16_t result = REG - dataBus; \
	SET_FLAG_C( result < 0x100 ) \
	SET_FLAG_ZN( result & 0xff ) }

#define TRANSFER( SRC, TARGET )	\
	READ_LAST( pc )	\
	TARGET = SRC;

#define TRANSFER_REG_S( SRC, TARGET )	\
    TRANSFER( SRC, TARGET )	\
    stepOuts.clear();

#define TRANSFER_WITH_FLAG( SRC, TARGET )	\
	TRANSFER( SRC, TARGET )	\
	SET_FLAG_ZN( SRC )

#define ADC( GET )	\
	{ GET##_LAST			\
	unsigned result = dataBus + regA + GET_FLAG_C;	\
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

#define SBC( GET )	\
	{ GET##_LAST			\
	uint8_t dataInv = ~dataBus;	\
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

#define PHP							\
	READ( pc )						\
	PUSH_LAST( STATUS | FLAG_B )

#define PHA							\
	READ( pc )						\
	PUSH_LAST( regA )

#define PLP						\
	READ( pc )					\
	READ( 0x100 | regS )		\
	PULL_LAST					\
	SET_STATUS( dataBus )

#define PLA	\
	READ( pc )					\
	READ( 0x100 | regS )		\
	PULL_LAST					\
	regA = dataBus;				\
	SET_FLAG_ZN( dataBus )

#define RTI	\
	READ_PC_INC					\
	READ( 0x100 | regS )		\
	PULL						\
	SET_STATUS( dataBus )		\
	PULL						\
	zeroPage = dataBus;				\
	PULL_LAST					\
    if(!stepOuts.empty()) stepOuts.pop_back(); \
	pc = (dataBus << 8) | zeroPage;

#define RTS	\
	READ_PC_INC					\
	READ( 0x100 | regS )		\
	PULL						\
	zeroPage = dataBus;			\
	PULL						\
    if(!stepOuts.empty()) stepOuts.pop_back(); \
	pc = (dataBus << 8) | zeroPage;			\
	READ_PC_INC_LAST

#define CLC				\
	READ_LAST( pc )		\
	SET_FLAG_C( 0 )

#define SEC				\
	READ_LAST( pc )		\
	SET_FLAG_C( 1 )

#define CLI				\
	busAccessUpdateFlagI<false, mhz2, busLogger>( pc ); \
	SET_FLAG_I( 0 )

#define SEI				\
	busAccessUpdateFlagI<true, mhz2, busLogger>( pc ); \
	SET_FLAG_I( 1 )

#define CLV	\
	READ_LAST( pc )		\
	SET_FLAG_V( 0 )

#define CLD	\
	READ_LAST( pc )		\
	SET_FLAG_D( 0 )

#define SED	\
	READ_LAST( pc )		\
	SET_FLAG_D( 1 )


#define NOP				\
	READ_LAST( pc )

#define JSR						\
	READ_PC_INC					\
	absolute = dataBus;			\
	READ( pc )					\
	PUSH( pc >> 8 )				\
	PUSH( pc & 0xff )			\
    appendStepOut(pc + 1);      \
	READ_LAST( pc )				\
	absolute |= dataBus << 8;	\
	pc = absolute;

#define JMP_ABS	\
	READ_PC_INC						\
	absolute = dataBus;				\
	READ_LAST( pc )					\
	absolute |= dataBus << 8;		\
	pc = absolute;

#define JMP_INDIRECT	\
	READ_PC_INC			\
	absolute = dataBus;	\
	READ_PC_INC			\
	absolute |= dataBus << 8;	\
	READ( absolute )	\
	zeroPage = dataBus;	\
	READ_LAST( (absolute & 0xff00) | ((absolute + 1) & 0xff ) )	\
	pc = (dataBus << 8) | zeroPage;

#define KILL				\
	READ_PC_INC				\
	READ( 0xffff )			\
	READ( 0xfffe )			\
	READ( 0xfffe )			\
	READ( 0xffff )			\
    system->jam();          \
	control |= Halt;

#define TRAPPED_KILL    \
    if (!traps.handler()) {   \
        KILL    \
    }

#define BRANCH( cond )	\
	READ_PC_INC_LAST	\
	if (!cond) break;	\
	absolute = pc + int8_t(dataBus);	\
	READ( pc )		\
					\
	if ((pc ^ absolute) & 0xff00) {		\
		READ_LAST( (pc & 0xff00) | (absolute & 0xff) )	\
	}														\
	pc = absolute;

//undocumented
#define	SLO( GET, SET )	\
	{ GET	\
	SET_FLAG_C( dataBus & 0x80 )	\
	uint8_t result = dataBus << 1;	\
	SET##_DUMMY( result )	\
	regA |= result;		\
	SET_FLAG_ZN( regA ) }

#define	RLA( GET, SET )	\
	{ GET	\
	uint8_t result = (dataBus << 1) | GET_FLAG_C;	\
	SET_FLAG_C( dataBus & 0x80 )			\
	SET##_DUMMY( result )	\
	regA &= result;		\
	SET_FLAG_ZN( regA ) }

#define	SRE( GET, SET )	\
	{ GET	\
	SET_FLAG_C( dataBus & 1 )	\
	uint8_t result = dataBus >> 1;	\
	SET##_DUMMY( result )	\
	regA ^= result;		\
	SET_FLAG_ZN( regA ) }

#define DUMMY( GET )	\
	{ GET##_LAST }

#define ANC	\
	AND( GET_IMM )	\
	SET_FLAG_C( GET_FLAG_N )

#define ALR	\
	{ GET_IMM_LAST	\
	regA &= dataBus;	\
	SET_FLAG_C( regA & 1 )	\
	regA >>= 1;	\
	SET_FLAG_ZN( regA ) }

#define TEMP_LAST	dataBus = result;

#define RRA( GET, SET )	\
	{ GET	\
	uint8_t result = (dataBus >> 1) | (GET_FLAG_C << 7);	\
	SET_FLAG_C( dataBus & 1 )	\
	SET##_DUMMY( result )	\
	ADC( TEMP ) }

#define ARR	\
	{ GET_IMM_LAST	\
	dataBus &= regA;	\
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
	regA = result; }

#define H1_AND_WRITE( VALUE )	\
	uint8_t strange = (absolute >> 8) + 1;	\
	uint8_t data = VALUE;	\
	strange &= data;	\
	if ( !RDY_CYCLE )	\
		data = strange;	\
	if (PAGE_CROSSED)	\
		absIndexed = (strange << 8) | (absIndexed & 0xff);    \
    WRITE_LAST( absIndexed, data );

#define SHA_INDIRECT_INDEXED	\
	{ INDIRECT_INDEXED( true )	\
	H1_AND_WRITE( regA & regX ) }

#define TAS_ABS_INDEXED	\
	{ ABS_INDEXED(regY, true)	\
	regS = regA & regX;	\
    stepOuts.clear(); \
	H1_AND_WRITE( regS ) }

#define SH_ABS_INDEXED( REG_INDEX, REG ) \
	{ ABS_INDEXED( REG_INDEX, true )	\
	H1_AND_WRITE( REG ) }

#define GET_IMM_AND_REMEMBER_RDY	\
	dataBus = busRead<true, true, mhz2, busLogger>( pc );			\
	INC_PC( 1 )

#define ANE	\
	GET_IMM_AND_REMEMBER_RDY	\
	regA = ( regA | (RDY_CYCLE ? 0xee : 0xef) ) & regX & dataBus;	\
	SET_FLAG_ZN( regA )

#define LAX( GET )	\
	{ GET##_LAST	\
	regA = regX = dataBus;	\
	SET_FLAG_ZN( dataBus ) }

#define LXA	\
	{ GET_IMM_AND_REMEMBER_RDY	\
	regA = regX = (regA | (RDY_CYCLE ? 0xee : 0xee) ) & dataBus;	\
	SET_FLAG_ZN( regA ) }

#define LAS	\
	{ GET_ABS_INDEXED_REGY_LAST	\
	regA = dataBus & regS; \
	regX = regS = regA;	\
    stepOuts.clear(); \
	SET_FLAG_ZN( regA ) }

#define DCP( GET, SET )	\
	{ GET	\
	uint8_t result = dataBus - 1;	\
	SET##_DUMMY( result )	\
	SET_FLAG_C( regA >= result ) \
	SET_FLAG_ZN( (regA - result) & 0xff ) }

#define SBX	\
	{ GET_IMM_LAST \
	uint16_t result = (regA & regX) - dataBus;	\
	SET_FLAG_C( result < 0x100 )	\
	regX = result & 0xff;	\
	SET_FLAG_ZN( regX ) }

#define ISC( GET, SET )	\
	{ GET	\
	uint8_t result = dataBus + 1;	\
	SET##_DUMMY( result )	\
	SBC( TEMP ) }

    if (control) {
        if (control & Halt) {
            READ( 0xffff )
            return;
        }

        if (control & IRQ) {
            interrupt<false, mhz2, busLogger>();
            pcEdge = pc;
            if (control & (BreakPoint | SoftStop))
                controlBreaks();
            return;
        }

        if (control & ResetRoutine) {
            resetRoutine<mhz2, busLogger>();
        }

        if (control & History)
            loadTrace(historyHandler.getNext());

        READ_PC_INC
        switch( dataBus ) {
            case 0x00:
                interrupt<true, mhz2, busLogger>( );
                break;

            #include "optable.h"
        }
        pcEdge = pc;
        if (control & (BreakPoint | SoftStop))
            controlBreaks();

        return;
    }

	READ_PC_INC
	switch( dataBus ) {
        case 0x00:
            interrupt<true, mhz2, busLogger>( );
            break;

        #include "optable.h"
	}
}

auto M6510::loadTrace(Emulator::HistoryEntry<uint8_t>& entry) -> void {
    uint16_t addr = pcEdge;
    entry.addr = addr;
    entry.flags = getFlags();

    for (int i = 0; i < 3; ++i) {
        entry.mem[i] = memory.peek( addr++ );
    }
}

inline auto M6510::controlBreaks() -> void {
     if ((control & SoftStop) && checkSoftStop(pcEdge)) {
         system->debugPointReached(DebuggerAction::Softstop, pcEdge);
     } else if ((control & BreakPoint) && breakPoints.check(pcEdge)) {
         system->debugPointReached(DebuggerAction::Breakpoint, pcEdge);
     }
}

auto M6510::getFlags() -> uint8_t {
    return STATUS;
}

}
