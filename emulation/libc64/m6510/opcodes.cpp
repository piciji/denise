
auto M6510::process() -> void {

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
	interruptSampled |= nmiPending | (irqPending & !GET_FLAG_I );

#define READ( addr ) 	dataBus = busRead<false>( addr );
#define READ_LAST( addr ) 	dataBus = busRead<true>( addr );
	
#define WRITE( addr, value )	\
	busWrite( addr, value);

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
		busRead<false, true>( ((absolute & 0xff00) | (absIndexed & 0xff)) );	\
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
		busRead<false, true>( ((absolute & 0xff00) | (absIndexed & 0xff)) ); \
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
	pc = dataBus;				\
	PULL_LAST					\
	pc |= dataBus << 8;

#define RTS	\
	READ_PC_INC					\
	READ( 0x100 | regS )		\
	PULL						\
	pc = dataBus;				\
	PULL						\
	pc |= dataBus << 8;			\
	READ_PC_INC_LAST
	
#define CLC				\
	READ_LAST( pc )		\
	SET_FLAG_C( 0 )
	
#define SEC				\
	READ_LAST( pc )		\
	SET_FLAG_C( 1 )

#define CLI				\
	busAccessUpdateFlagI<false>( pc ); \
	SET_FLAG_I( 0 )
	
#define SEI				\
	busAccessUpdateFlagI<true>( pc ); \
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
	absolute |= dataBus << 8;	\
	PUSH( pc >> 8 )				\
	PUSH( pc & 0xff )			\
	READ_LAST( pc )				\
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
	pc = dataBus;	\
	READ_LAST( (absolute & 0xff00) | ((absolute + 1) & 0xff ) )	\
	pc |= dataBus << 8;
	
#define KILL				\
	READ_PC_INC				\
	READ( 0xffff )			\
	READ( 0xfffe )			\
	READ( 0xfffe )			\
	READ( 0xffff )			\
	killed = true;
	
#define BRANCH( cond )	\
	READ_PC_INC_LAST	\
	if (!cond) return;	\
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
	H1_AND_WRITE( regS ) }	
	
#define SH_ABS_INDEXED( REG_INDEX, REG ) \
	{ ABS_INDEXED( REG_INDEX, true )	\
	H1_AND_WRITE( REG ) }
	
#define GET_IMM_AND_REMEMBER_RDY	\
	dataBus = busRead<true, true>( pc );			\
	INC_PC( 1 )
		
#define ANE	\
	GET_IMM_AND_REMEMBER_RDY	\
	regA = ( regA | (RDY_CYCLE ? 0xee : magicAne) ) & regX & dataBus;	\
	SET_FLAG_ZN( regA )
	
#define LAX( GET )	\
	{ GET##_LAST	\
	regA = regX = dataBus;	\
	SET_FLAG_ZN( dataBus ) }
		
#define LXA	\
	{ GET_IMM_AND_REMEMBER_RDY	\
	regA = regX = (regA | (RDY_CYCLE ? 0xee : magicLax) ) & dataBus;	\
	SET_FLAG_ZN( regA ) }
	
#define LAS	\
	{ GET_ABS_INDEXED_REGY_LAST	\
	regA = dataBus & regS; \
	regX = regS = regA;	\
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
	
//// 
	if (killed) {
		READ( 0xffff )
		return;
	}

	if (interruptSampled)
		return interrupt();
	
	READ_PC_INC		
		
	switch( dataBus ) {
		case 0x00:
			interrupt<true>( );
			break;
			
		case 0x01:
			ORA( GET_INDEXED_INDIRECT )
			break;
			
		case 0x05:
			ORA( GET_ZERO )
			break;
			
		case 0x06:
			ASL( GET_ZERO, SET_ZERO )
			break;
			
		case 0x08:
			PHP
			break;
			
		case 0x09:
			ORA( GET_IMM )
			break;
			
		case 0x0a:
			ASL_IMPLIED
			break;
			
		case 0x0d:
			ORA( GET_ABS )
			break;
			
		case 0x0e:
			ASL( GET_ABS, SET_ABS )
			break;
			
		case 0x10:
			BRANCH( !GET_FLAG_N )
			break;
			
		case 0x11:
			ORA( GET_INDIRECT_INDEXED )
			break;
		
		case 0x15:
			ORA( GET_ZERO_INDEXED_REGX )
			break;
			
		case 0x16:
			ASL( GET_ZERO_INDEXED_REGX, SET_ZERO )
			break;
			
		case 0x18:
			CLC
			break;
			
		case 0x19:
			ORA( GET_ABS_INDEXED_REGY )
			break;
			
		case 0x1a:
			NOP
			break;
			
		case 0x1d:
			ORA( GET_ABS_INDEXED_REGX )
			break;
			
		case 0x1e:
			ASL( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
			break;
			
		case 0x20:
			JSR
			break;
			
		case 0x21:
			AND( GET_INDEXED_INDIRECT )
			break;
			
		case 0x24:
			BIT( GET_ZERO )
			break;
		
		case 0x25:
			AND( GET_ZERO )
			break;
			
		case 0x26:
			ROL( GET_ZERO, SET_ZERO )
			break;
			
		case 0x28:
			PLP
			break;
			
		case 0x29:
			AND( GET_IMM )
			break;
			
		case 0x2a:
			ROL_IMPLIED
			break;
			
		case 0x2c:
			BIT( GET_ABS )
			break;
			
		case 0x2d:
			AND( GET_ABS )
			break;
			
		case 0x2e:
			ROL( GET_ABS, SET_ABS )
			break;
			
		case 0x30:
			BRANCH( GET_FLAG_N )
			break;
			
		case 0x31:
			AND( GET_INDIRECT_INDEXED )
			break;
			
		case 0x35:
			AND( GET_ZERO_INDEXED_REGX )
			break;
			
		case 0x36:
			ROL( GET_ZERO_INDEXED_REGX, SET_ZERO )
			break;
			
		case 0x38:
			SEC
			break;
			
		case 0x39:
			AND( GET_ABS_INDEXED_REGY )
			break;
			
		case 0x3a:
			NOP
			break;
			
		case 0x3d:
			AND( GET_ABS_INDEXED_REGX )
			break;
			
		case 0x3e:
			ROL( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
			break;
			
		case 0x40:
			RTI
			break;
			
		case 0x41:
			EOR( GET_INDEXED_INDIRECT )
			break;
		
		case 0x45:
			EOR( GET_ZERO )
			break;
			
		case 0x46:
			LSR( GET_ZERO, SET_ZERO )
			break;
			
		case 0x48:
			PHA
			break;
			
		case 0x49:
			EOR( GET_IMM )
			break;
			
		case 0x4a:
			LSR_IMPLIED
			break;
			
		case 0x4c:
			JMP_ABS
			break;
			
		case 0x4d:
			EOR( GET_ABS )
			break;
			
		case 0x4e:
			LSR( GET_ABS, SET_ABS )
			break;
			
		case 0x50:
			BRANCH( !GET_FLAG_V )
			break;
			
		case 0x51:
			EOR( GET_INDIRECT_INDEXED )
			break;

		case 0x55:
			EOR( GET_ZERO_INDEXED_REGX )
			break;
			
		case 0x56:
			LSR( GET_ZERO_INDEXED_REGX, SET_ZERO )
			break;
			
		case 0x58:
			CLI
			break;
			
		case 0x59:
			EOR( GET_ABS_INDEXED_REGY )
			break;
			
		case 0x5a:
			NOP
			break;
		
		case 0x5d:
			EOR( GET_ABS_INDEXED_REGX )
			break;

		case 0x5e:
			LSR( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
			break;
			
		case 0x60:
			RTS
			break;
			
		case 0x61:
			ADC( GET_INDEXED_INDIRECT )
			break;
			
		case 0x65:
			ADC( GET_ZERO )
			break;
			
		case 0x66:
			ROR( GET_ZERO, SET_ZERO )
			break;
			
		case 0x68:
			PLA
			break;
			
		case 0x69:
			ADC( GET_IMM )
			break;
			
		case 0x6a:
			ROR_IMPLIED
			break;
			
		case 0x6c:
			JMP_INDIRECT
			break;
			
		case 0x6d:
			ADC( GET_ABS )
			break;
			
		case 0x6e:
			ROR( GET_ABS, SET_ABS )
			break;
			
		case 0x70:
			BRANCH( GET_FLAG_V )
			break;
			
		case 0x71:
			ADC( GET_INDIRECT_INDEXED )
			break;
			
		case 0x75:
			ADC( GET_ZERO_INDEXED_REGX )
			break;
			
		case 0x76:
			ROR( GET_ZERO_INDEXED_REGX, SET_ZERO )
			break;
			
		case 0x78:
			SEI
			break;
		
		case 0x79:
			ADC( GET_ABS_INDEXED_REGY )
			break;
			
		case 0x7a:
			NOP
			break;

		case 0x7d:
			ADC( GET_ABS_INDEXED_REGX )
			break;
			
		case 0x7e:
			ROR( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )	
			break;
			
		case 0x81:
			STORE_INDEXED_INDIRECT( regA )
			break;
			
		case 0x84:
			STORE_ZERO( regY )
			break;
			
		case 0x85:
			STORE_ZERO( regA )
			break;
			
		case 0x86:
			STORE_ZERO( regX )
			break;			
			
		case 0x88:
			DEC_IMPLIED( regY )
			break;
			
		case 0x8a:
			TRANSFER_WITH_FLAG( regX, regA )
			break;
			
		case 0x8c:
			STORE_ABS( regY )
			break;
			
		case 0x8d:
			STORE_ABS( regA )
			break;
			
		case 0x8e:
			STORE_ABS( regX )
			break;
			
		case 0x90:
			BRANCH( !GET_FLAG_C )
			break;
			
		case 0x91:
			STORE_INDIRECT_INDEXED( regA )
			break;
			
		case 0x94:
			STORE_ZERO_INDEXED(regX, regY)
			break;

		case 0x95:
			STORE_ZERO_INDEXED(regX, regA)
			break;

		case 0x96:
			STORE_ZERO_INDEXED(regY, regX)
			break;

		case 0x98:
			TRANSFER_WITH_FLAG( regY, regA )
			break;

		case 0x99:
			STORE_ABS_INDEXED(regY, regA)
			break;
			
		case 0x9a:
			TRANSFER( regX, regS )
			break;
			
		case 0x9d:
			STORE_ABS_INDEXED(regX, regA)
			break;
			
		case 0xa0:
			LD( GET_IMM, regY )
			break;
			
		case 0xa1:
			LD( GET_INDEXED_INDIRECT, regA )
			break;
			
		case 0xa2:
			LD( GET_IMM, regX )
			break;
			
		case 0xa4:
			LD( GET_ZERO, regY )
			break;

		case 0xa5:
			LD( GET_ZERO, regA )
			break;

		case 0xa6:
			LD( GET_ZERO, regX )
			break;
			
		case 0xa8:
			TRANSFER_WITH_FLAG( regA, regY )
			break;
			
		case 0xa9:
			LD( GET_IMM, regA )
			break;
			
		case 0xaa:
			TRANSFER_WITH_FLAG( regA, regX )
			break;

		case 0xac:
			LD( GET_ABS, regY )
			break;
			
		case 0xad:
			LD( GET_ABS, regA )
			break;
			
		case 0xae:
			LD( GET_ABS, regX )
			break;
			
		case 0xb0:
			BRANCH( GET_FLAG_C )
			break;
			
		case 0xb1:
			LD( GET_INDIRECT_INDEXED, regA )
			break;
			
		case 0xb4:
			LD( GET_ZERO_INDEXED_REGX, regY )
			break;
			
		case 0xb5:
			LD( GET_ZERO_INDEXED_REGX, regA )
			break;

		case 0xb6:
			LD( GET_ZERO_INDEXED_REGY, regX )
			break;			
			
		case 0xb8:
			CLV
			break;
			
		case 0xb9:
			LD( GET_ABS_INDEXED_REGY, regA )
			break;
			
		case 0xba:
			TRANSFER_WITH_FLAG( regS, regX )
			break;
			
		case 0xbc:
			LD( GET_ABS_INDEXED_REGX, regY )
			break;
			
		case 0xbd:
			LD( GET_ABS_INDEXED_REGX, regA )
			break;
			
		case 0xbe:
			LD( GET_ABS_INDEXED_REGY, regX )
			break;
			
		case 0xc0:
			CP( GET_IMM, regY )
			break;
			
		case 0xc1:
			CP( GET_INDEXED_INDIRECT, regA )
			break;			

		case 0xc4:
			CP( GET_ZERO, regY )
			break;	
			
		case 0xc5:
			CP( GET_ZERO, regA )
			break;	
			
		case 0xc6:
			DEC( GET_ZERO, SET_ZERO )
			break;
			
		case 0xc8:
			INC_IMPLIED( regY )
			break;
			
		case 0xc9:
			CP( GET_IMM, regA )
			break;
			
		case 0xca:
			DEC_IMPLIED( regX )
			break;
			
		case 0xcc:
			CP( GET_ABS, regY )
			break;
			
		case 0xcd:
			CP( GET_ABS, regA )
			break;
			
		case 0xce:
			DEC( GET_ABS, SET_ABS )
			break;
	
		case 0xd0:
			BRANCH( !GET_FLAG_Z )
			break;
		
		case 0xd1:
			CP( GET_INDIRECT_INDEXED, regA )
			break;			
			
		case 0xd5:
			CP( GET_ZERO_INDEXED_REGX, regA )
			break;	

		case 0xd6:
			DEC( GET_ZERO_INDEXED_REGX, SET_ZERO )
			break;			
			
		case 0xd8:
			CLD
			break;
			
		case 0xd9:
			CP( GET_ABS_INDEXED_REGY, regA )
			break;
			
		case 0xda:
			NOP
			break;
			
		case 0xdd:
			CP( GET_ABS_INDEXED_REGX, regA )
			break;	
			
		case 0xde:
			DEC( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
			break;			
			
		case 0xe0:
			CP( GET_IMM, regX )
			break;				
			
		case 0xe1:
			SBC( GET_INDEXED_INDIRECT )
			break;
			
		case 0xe4:
			CP( GET_ZERO, regX )
			break;	
			
		case 0xe5:
			SBC( GET_ZERO )
			break;		
			
		case 0xe6:
			INC( GET_ZERO, SET_ZERO )
			break;
			
		case 0xe8:
			INC_IMPLIED( regX )
			break;
		
		case 0xe9:
		case 0xeb:
			SBC( GET_IMM )
			break;				
			
		case 0xea:
			NOP
			break;				
			
		case 0xec:
			CP( GET_ABS, regX )
			break;				
			
		case 0xed:
			SBC( GET_ABS )
			break;			
			
		case 0xee:
			INC( GET_ABS, SET_ABS )
			break;			
			
		case 0xf0:
			BRANCH( GET_FLAG_Z )
			break;
			
		case 0xf1:
			SBC( GET_INDIRECT_INDEXED )
			break;
			
		case 0xf5:
			SBC( GET_ZERO_INDEXED_REGX )
			break;			
			
		case 0xf6:
			INC( GET_ZERO_INDEXED_REGX, SET_ZERO )
			break;			
			
		case 0xf8:
			SED
			break;			
			
		case 0xf9:
			SBC( GET_ABS_INDEXED_REGY )
			break;			
			
		case 0xfa:
			NOP
			break;			
			
		case 0xfd:
			SBC( GET_ABS_INDEXED_REGX )
			break;			
			
		case 0xfe:
			INC( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
			break;			
// undocumented			
		case 0x03:
			SLO( GET_INDEXED_INDIRECT, SET_ABS )
			break;
			
		case 0x04:
		case 0x44:
		case 0x64:
			DUMMY( GET_ZERO )
			break;
			
		case 0x07:
			SLO( GET_ZERO, SET_ZERO )
			break;			
			
		case 0x0b:
		case 0x2b:
			ANC
			break;
			
		case 0x0c:
			DUMMY( GET_ABS )
			break;
			
		case 0x0f:
			SLO( GET_ABS, SET_ABS )
			break;
			
		case 0x13:
			SLO( GET_INDIRECT_INDEXED, SET_ABS_INDEXED )
			break;	
			
		case 0x14:
		case 0x34:
		case 0x54:
		case 0x74:
		case 0xd4:
		case 0xf4:
			DUMMY( GET_ZERO_INDEXED_REGX )
			break;
			
		case 0x17:
			SLO( GET_ZERO_INDEXED_REGX, SET_ZERO )
			break;				
			
		case 0x1b:
			SLO( GET_ABS_INDEXED_REGY, SET_ABS_INDEXED )
			break;	

		case 0x1c:
		case 0x3c:
		case 0x5c:
		case 0x7c:
		case 0xdc:
		case 0xfc:
			DUMMY( GET_ABS_INDEXED_REGX )
			break;			
			
		case 0x1f:
			SLO( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
			break;				
			
		case 0x23:
			RLA( GET_INDEXED_INDIRECT, SET_ABS )
			break;
			
		case 0x27:
			RLA( GET_ZERO, SET_ZERO )
			break;
	
		case 0x2f:
			RLA( GET_ABS, SET_ABS )
			break;			
			
		case 0x33:
			RLA( GET_INDIRECT_INDEXED, SET_ABS_INDEXED )
			break;				
			
		case 0x37:
			RLA( GET_ZERO_INDEXED_REGX, SET_ZERO )
			break;		
			
		case 0x3b:
			RLA( GET_ABS_INDEXED_REGY, SET_ABS_INDEXED )
			break;				
		
		case 0x3f:
			RLA( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
			break;
			
		case 0x43:
			SRE( GET_INDEXED_INDIRECT, SET_ABS )
			break;
		
		case 0x47:
			SRE( GET_ZERO, SET_ZERO )
			break;	
			
		case 0x4b:
			ALR
			break;
			
		case 0x4f:
			SRE( GET_ABS, SET_ABS )
			break;			
			
		case 0x53:
			SRE( GET_INDIRECT_INDEXED, SET_ABS_INDEXED )
			break;			
	
		case 0x57:
			SRE( GET_ZERO_INDEXED_REGX, SET_ZERO )
			break;				
			
		case 0x5b:
			SRE( GET_ABS_INDEXED_REGY, SET_ABS_INDEXED )
			break;				
			
		case 0x5f:
			SRE( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
			break;				
			
		case 0x63:
			RRA( GET_INDEXED_INDIRECT, SET_ABS )
			break;

		case 0x67:
			RRA( GET_ZERO, SET_ZERO )
			break;

		case 0x6b:
			ARR
			break;
			
		case 0x6f:
			RRA( GET_ABS, SET_ABS )
			break;		
			
		case 0x73:
			RRA( GET_INDIRECT_INDEXED, SET_ABS_INDEXED )
			break;
		
		case 0x77:
			RRA( GET_ZERO_INDEXED_REGX, SET_ZERO )
			break;			
			
		case 0x7b:
			RRA( GET_ABS_INDEXED_REGY, SET_ABS_INDEXED )
			break;			
	
		case 0x7f:
			RRA( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
			break;			
			
		case 0x80:
		case 0x82:
		case 0x89:
		case 0xc2:
		case 0xe2:
			DUMMY( GET_IMM )
			break;
			
		case 0x83:
			STORE_INDEXED_INDIRECT( regA & regX )
			break;
			
		case 0x87:
			STORE_ZERO( regA & regX )
			break;
			
		case 0x8b:
			ANE
			break;
			
		case 0x8f:
			STORE_ABS( regA & regX )
			break;
			
		case 0x93:
			SHA_INDIRECT_INDEXED
			break;
			
		case 0x97:
			STORE_ZERO_INDEXED(regY, regA & regX)
			break;

		case 0x9b:
			TAS_ABS_INDEXED
			break;
		
		case 0x9c:
			SH_ABS_INDEXED( regX, regY )
			break;
			
		case 0x9e:
			SH_ABS_INDEXED( regY, regX )
			break;
			
		case 0x9f:
			SH_ABS_INDEXED( regY, regA & regX )
			break;
			
		case 0xa3:
			LAX( GET_INDEXED_INDIRECT )
			break;
			
		case 0xa7:
			LAX( GET_ZERO )
			break;
			
		case 0xab:
			LXA
			break;			
			
		case 0xaf:
			LAX( GET_ABS )
			break;			
			
		case 0xb3:
			LAX( GET_INDIRECT_INDEXED )
			break;				
			
		case 0xb7:
			LAX( GET_ZERO_INDEXED_REGY )
			break;			
			
		case 0xbb:
			LAS
			break;
			
		case 0xbf:
			LAX( GET_ABS_INDEXED_REGY )
			break;			
			
		case 0xc3:
			DCP( GET_INDEXED_INDIRECT, SET_ABS )
			break;
			
		case 0xc7:
			DCP( GET_ZERO, SET_ZERO )
			break;
			
		case 0xcb:
			SBX
			break;
			
		case 0xcf:
			DCP( GET_ABS, SET_ABS )
			break;
			
		case 0xd3:
			DCP( GET_INDIRECT_INDEXED, SET_ABS_INDEXED )
			break;		
			
		case 0xd7:
			DCP( GET_ZERO_INDEXED_REGX, SET_ZERO )
			break;			
			
		case 0xdb:			
			DCP( GET_ABS_INDEXED_REGY, SET_ABS_INDEXED )
			break;
			
		case 0xdf:			
			DCP( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
			break;
			
		case 0xe3:
			ISC( GET_INDEXED_INDIRECT, SET_ABS )
			break;
			
		case 0xe7:
			ISC( GET_ZERO, SET_ZERO )
			break;			
			
		case 0xef:
			ISC( GET_ABS, SET_ABS )
			break;			
			
		case 0xf3:
			ISC( GET_INDIRECT_INDEXED, SET_ABS_INDEXED )
			break;			
			
		case 0xf7:
			ISC( GET_ZERO_INDEXED_REGX, SET_ZERO )
			break;			
			
		case 0xfb:
			ISC( GET_ABS_INDEXED_REGY, SET_ABS_INDEXED )
			break;				
			
		case 0xff:
			ISC( GET_ABS_INDEXED_REGX, SET_ABS_INDEXED )
			break;			
		
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
			break;
	}
}
