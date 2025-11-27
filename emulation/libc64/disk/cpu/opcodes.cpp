
auto M6502::process() -> void {
	
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
	interruptSampled |= nmiPending | (irqPending & !GET_FLAG_I);

#define READ( addr ) 	\
    dataBus = drive->cpuRead( addr );
    
#define READ_LAST( addr ) \
    SAMPLE_INTERRUPT    \
	READ( addr )	    
    
#define WRITE( addr, value )	\
	drive->cpuWrite( addr, value);

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
	WRITE( 0x100 | regS--, value)
	
#define PUSH_LAST( value )		\
	WRITE_LAST( 0x100 | regS--, value)

#define PULL			\
	READ( 0x100 | ++regS )

#define PULL_LAST				\
	READ_LAST( 0x100 | ++regS )
	
#define PAGE_CROSSED 	((absIndexed ^ absolute) & 0xff00)
	
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
	
//
#define SWITCH0 \
    switch(step++) {    \
        case 0:
    
#define SWITCH1 \
        return; \
    case 1:
    
#define SWITCH01    \
    SWITCH0 \
    SWITCH1
    
#define SWITCH2 \
        return; \
    case 2:
    
#define SWITCH3 \
        return; \
    case 3:
    
#define SWITCH_END  \
        step = 0;   \
        return; }    
    
// GET	                    				
#define GET_INDEXED_INDIRECT( LAST, FORCE )		\
    SWITCH0 \
        INDEXED_INDIRECT					\
    SWITCH1 \
        READ##LAST( absolute )

#define GET_INDIRECT_INDEXED( LAST, FORCE )		\
    SWITCH0 \
        INDIRECT_INDEXED( FORCE )			\
    SWITCH1 \
        READ##LAST( absIndexed )
//
#define _GET_ZERO_LAST		\
    SWITCH0 \
        ZERO 				\
    SWITCH1 \
        READ_LAST( zeroPage )
    
#define _GET_ZERO		\
    SWITCH0 \
        ZERO 				\
        READ( zeroPage )    
    
#define GET_ZERO( LAST, FORCE )		\
    _GET_ZERO##LAST
//
#define _GET_ZERO_INDEXED_REGX_LAST	\
    SWITCH0 \
        ZERO_PAGE_INDEXED( regX )			\
    SWITCH1 \
        READ_LAST( zeroPage ) 
    
#define _GET_ZERO_INDEXED_REGX	\
    SWITCH0 \
        ZERO_PAGE_INDEXED( regX )			\
        READ( zeroPage )     
    
#define GET_ZERO_INDEXED_REGX( LAST, FORCE )	\
    _GET_ZERO_INDEXED_REGX##LAST
//		
#define _GET_ZERO_INDEXED_REGY_LAST	\
    SWITCH0 \
        ZERO_PAGE_INDEXED( regY )			\
    SWITCH1 \
        READ_LAST( zeroPage )
    
#define _GET_ZERO_INDEXED_REGY	\
    SWITCH0 \
        ZERO_PAGE_INDEXED( regY )			\
        READ( zeroPage )   
    
#define GET_ZERO_INDEXED_REGY( LAST, FORCE )	\
    _GET_ZERO_INDEXED_REGY##LAST    
//	
#define GET_ABS( LAST, FORCE )			\
    SWITCH0 \
        ABS							\
    SWITCH1 \
        READ##LAST( absolute ) 
	
#define GET_ABS_INDEXED_REGX( LAST, FORCE )	\
    SWITCH0 \
        ABS_INDEXED( regX, FORCE )		\
    SWITCH1 \
        READ##LAST( absIndexed )
	
#define GET_ABS_INDEXED_REGY( LAST, FORCE )	\
    SWITCH0 \
        ABS_INDEXED( regY, FORCE )		\
    SWITCH1 \
        READ##LAST( absIndexed )
	
#define GET_IMM( LAST, FORCE )		\
    SWITCH0 \
    SWITCH1 \
        READ_PC_INC##LAST
	
// STORE register
#define STORE_INDEXED_INDIRECT( REG )	\
    SWITCH0 \
        INDEXED_INDIRECT					\
        readNext = false;   \
    SWITCH1 \
        WRITE_LAST( absolute, REG ) \
    SWITCH_END
	
#define STORE_ZERO( REG )			\
    SWITCH0 \
        ZERO							\
        readNext = false;   \
    SWITCH1 \
        WRITE_LAST( zeroPage, REG ) \
    SWITCH_END

#define STORE_ABS( REG )	\
    SWITCH0 \
        ABS	\
        readNext = false;   \
    SWITCH1 \
        WRITE_LAST( absolute, REG ) \
    SWITCH_END

#define STORE_INDIRECT_INDEXED( REG )	\
    SWITCH0 \
        INDIRECT_INDEXED( true )	\
        readNext = false;   \
    SWITCH1 \
        WRITE_LAST( absIndexed, REG )   \
    SWITCH_END

#define STORE_ZERO_INDEXED( INDEDX_REG, REG )	\
    SWITCH0 \
        ZERO_PAGE_INDEXED( INDEDX_REG )	\
        readNext = false;   \
    SWITCH1 \
        WRITE_LAST( zeroPage, REG ) \
    SWITCH_END

#define STORE_ABS_INDEXED( INDEDX_REG, REG )	\
    SWITCH0 \
        ABS_INDEXED( INDEDX_REG, true )	\
        readNext = false;   \
    SWITCH1 \
        WRITE_LAST( absIndexed, REG )   \
    SWITCH_END
	
// STORE with dummy write, hence needs more time for calculation	
#define SET_ABS_INDEXED_DUMMY	\
        readNext = false;   \
    SWITCH2 \
        WRITE( absIndexed, dataBus )			\
    SWITCH3 \
        WRITE_LAST( absIndexed, _value )
	
#define SET_ABS_DUMMY			\
        readNext = false;   \
    SWITCH2 \
        WRITE( absolute, dataBus )			\
    SWITCH3 \
        WRITE_LAST( absolute, _value )
	
#define SET_ZERO_DUMMY	\
        WRITE( zeroPage, dataBus )		\
        readNext = false;   \
    SWITCH1 \
        WRITE_LAST( zeroPage, _value )
		
	
///////////////
#define ORA( GET )		\
	GET(_LAST, false)			\
	regA |= dataBus;		\
	SET_FLAG_ZN( regA ) \
    SWITCH_END            

#define AND( GET )		\
	GET(_LAST, false)			\
	regA &= dataBus;		\
	SET_FLAG_ZN( regA ) \
    SWITCH_END
    	
#define EOR( GET )		\
	GET(_LAST, false)			\
	regA ^= dataBus;	\
	SET_FLAG_ZN( regA ) \
    SWITCH_END
	
#define	ASL( GET, SET )	\
	GET(, true)						\
	SET_FLAG_C( dataBus & 0x80 )		\
	_value = dataBus << 1;	\
    SET_FLAG_ZN( _value )   \
	SET##_DUMMY  \
    SWITCH_END

#define ASL_IMPLIED     \
SWITCH01    \
	READ_LAST( pc )		\
	SET_FLAG_C( regA & 0x80 )   \
	regA <<= 1;     \
	SET_FLAG_ZN( regA ) \
SWITCH_END
	
#define	LSR( GET, SET )	\
	GET(, true)						\
	SET_FLAG_C( dataBus & 1 )		\
	_value = dataBus >> 1;	\
    SET_FLAG_ZN( _value )    \
	SET##_DUMMY \
    SWITCH_END

#define LSR_IMPLIED	\
SWITCH01    \
	READ_LAST( pc )						\
	SET_FLAG_C( regA & 1 )		\
	regA >>= 1;	\
	SET_FLAG_ZN( regA ) \
SWITCH_END
		
#define BIT( GET )					\
	GET(_LAST, false)					\
	SET_FLAG_Z( dataBus & regA )	\
	SET_FLAG_N( dataBus )			\
	SET_FLAG_V( dataBus & 0x40 )    \
    soBlock = 1;    \
    SWITCH_END    
	
#define ROL( GET, SET )						\
	GET(, true)									\
	_value = (dataBus << 1) | GET_FLAG_C;	\
	SET_FLAG_C( dataBus & 0x80 )			\
    SET_FLAG_ZN( _value )    \
	SET##_DUMMY \
    SWITCH_END
		
#define ROL_IMPLIED								\
SWITCH01    \
	{ READ_LAST( pc )							\
	uint8_t result = (regA << 1) | GET_FLAG_C;	\
	SET_FLAG_C( regA & 0x80 )					\
	regA = result;								\
	SET_FLAG_ZN( regA ) }	\
SWITCH_END    
	
#define ROR( GET, SET )						\
	GET(, true)									\
	_value = (dataBus >> 1) | (GET_FLAG_C << 7);	\
	SET_FLAG_C( dataBus & 1 )			\
    SET_FLAG_ZN( _value )    \
	SET##_DUMMY \
    SWITCH_END
	
#define ROR_IMPLIED								\
SWITCH01    \
	{ READ_LAST( pc )							\
	uint8_t result = (regA >> 1) | (GET_FLAG_C << 7);	\
	SET_FLAG_C( regA & 1 )					\
	regA = result;								\
	SET_FLAG_ZN( regA ) }   \
SWITCH_END    

#define DEC( GET, SET )	\
	GET(, true)					\
	_value = dataBus - 1;	\
    SET_FLAG_ZN( _value )    \
	SET##_DUMMY \
    SWITCH_END
		
#define DEC_IMPLIED( REG )	\
SWITCH01    \
	READ_LAST( pc )	\
	REG--;	\
	SET_FLAG_ZN( REG )	\
SWITCH_END    
	
#define INC( GET, SET )	\
	GET(, true)					\
	_value = dataBus + 1;	\
    SET_FLAG_ZN( _value )    \
	SET##_DUMMY	\
    SWITCH_END
	
#define INC_IMPLIED( REG )	\
SWITCH01    \
	READ_LAST( pc )	\
	REG++;	\
	SET_FLAG_ZN( REG )		\
SWITCH_END   
	
#define LD( GET, REG )	\
	GET(_LAST, false)					\
	REG = dataBus;		\
	SET_FLAG_ZN( dataBus )  \
    SWITCH_END      
	
#define CP( GET, REG )	\
	GET(_LAST, false)					\
	{ uint16_t result = REG - dataBus; \
	SET_FLAG_C( result < 0x100 ) \
	SET_FLAG_ZN( result & 0xff ) } \
    SWITCH_END
		
#define TRANSFER( SRC, TARGET )	\
SWITCH01    \
	READ_LAST( pc )	\
	TARGET = SRC;   \
SWITCH_END

#define TRANSFER_REG_S( SRC, TARGET )	\
    TRANSFER( SRC, TARGET )
	
#define TRANSFER_WITH_FLAG( SRC, TARGET )	\
SWITCH01    \
	READ_LAST( pc )	\
	TARGET = SRC;   \
    SET_FLAG_ZN( SRC )  \
SWITCH_END
	
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
    
#define ADC( GET )	\
    GET(_LAST, false)			\
    _ADC  \
    soBlock = 1;    \
    SWITCH_END

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

#define SBC( GET )	\
    GET(_LAST, false)			\
    _SBC   \
    soBlock = 1;    \
    SWITCH_END
    
#define PHP							\
SWITCH0 \
	READ( pc )						\
SWITCH1 \
    SAMPLE_INTERRUPT    \
    SET_FLAG_B( 1 ) \
    PUSH_STATUS \
SWITCH_END
	
#define PHA							\
SWITCH0 \
	READ( pc )						\
SWITCH1 \
	PUSH_LAST( regA )   \
SWITCH_END
	
#define PLP						\
SWITCH0 \
	READ( pc )					\
	READ( 0x100 | regS )		\
SWITCH1 \
	PULL_LAST					\
	SET_STATUS( dataBus )       \
SWITCH_END
	
#define PLA	\
SWITCH0 \
	READ( pc )					\
	READ( 0x100 | regS )		\
SWITCH1 \
	PULL_LAST					\
	regA = dataBus;				\
	SET_FLAG_ZN( dataBus )  \
SWITCH_END
	
#define RTI	\
SWITCH0 \
	READ_PC_INC					\
	READ( 0x100 | regS )		\
	PULL						\
	SET_STATUS( dataBus )		\
	PULL						\
	pc = dataBus;				\
SWITCH1 \
	PULL_LAST					\
	pc |= dataBus << 8;         \
SWITCH_END

#define RTS	\
SWITCH0 \
	READ_PC_INC					\
	READ( 0x100 | regS )		\
	PULL						\
	pc = dataBus;				\
	PULL						\
	pc |= dataBus << 8;			\
SWITCH1 \
	READ_PC_INC_LAST            \
SWITCH_END
	
#define CLC				\
SWITCH01 \
	READ_LAST( pc )		\
	SET_FLAG_C( 0 ) \
SWITCH_END
	
#define SEC				\
SWITCH01 \
	READ_LAST( pc )		\
	SET_FLAG_C( 1 ) \
SWITCH_END    

#define CLI				\
SWITCH01 \
	READ_LAST( pc )		\
	SET_FLAG_I( 0 ) \
SWITCH_END    
	
#define SEI				\
SWITCH01 \
	READ_LAST( pc )		\
	SET_FLAG_I( 1 ) \
SWITCH_END    
	
#define CLV	\
SWITCH01 \
	READ_LAST( pc )		\
	SET_FLAG_V( 0 )     \
    soBlock = 2;    \
SWITCH_END    
	
#define CLD	\
SWITCH01 \
	READ_LAST( pc )		\
	SET_FLAG_D( 0 ) \
SWITCH_END    

#define SED	\
SWITCH01 \
	READ_LAST( pc )		\
	SET_FLAG_D( 1 ) \
SWITCH_END    
	
#define NOP				\
SWITCH01 \
	READ_LAST( pc ) \
SWITCH_END    
	
#define JSR						\
SWITCH0 \
	READ_PC_INC					\
	absolute = dataBus;			\
	READ( pc )					\
	PUSH( pc >> 8 )				\
	PUSH( pc & 0xff )			\
SWITCH1 \
	READ_LAST( pc )				\
	absolute |= dataBus << 8;	\
	pc = absolute;      \
SWITCH_END    
	
#define JMP_ABS	\
SWITCH0 \
	READ_PC_INC						\
	absolute = dataBus;				\
SWITCH1 \
	READ_LAST( pc )					\
	absolute |= dataBus << 8;		\
	pc = absolute;  \
SWITCH_END    
	
#define JMP_INDIRECT	\
SWITCH0 \
	READ_PC_INC			\
	absolute = dataBus;	\
	READ_PC_INC			\
	absolute |= dataBus << 8;	\
	READ( absolute )	\
	pc = dataBus;	\
SWITCH1 \
	READ_LAST( (absolute & 0xff00) | ((absolute + 1) & 0xff ) )	\
	pc |= dataBus << 8; \
SWITCH_END    
	
#define KILL				\
	READ_PC_INC				\
	READ( 0xffff )			\
	READ( 0xfffe )			\
	READ( 0xfffe )			\
	READ( 0xffff )			\
    system->jam(drive->media); \
	killed = true;
	
#define TRAPPED_KILL   \
    KILL
    
#define BRANCH( cond )	\
SWITCH01 \
	READ_PC_INC_LAST	\
	if (!cond) step = 0;    \
SWITCH2 \
	absolute = pc + int8_t(dataBus);	\
	READ( pc )		\
	if (!((pc ^ absolute) & 0xff00)) {		\
        pc = absolute;  \
        step = 0;   \
    } \
SWITCH3 \
    READ_LAST( (pc & 0xff00) | (absolute & 0xff) )	\
	pc = absolute;  \
SWITCH_END    
	
//undocumented		
#define	SLO( GET, SET )	\
	GET(, true)	\
	SET_FLAG_C( dataBus & 0x80 )	\
	_value = dataBus << 1;	\
    regA |= _value;		\
	SET_FLAG_ZN( regA ) \
	SET##_DUMMY \
    SWITCH_END
	
#define	RLA( GET, SET )	\
	GET(, true)	\
	_value = (dataBus << 1) | GET_FLAG_C;	\
	SET_FLAG_C( dataBus & 0x80 )			\
    regA &= _value;		\
	SET_FLAG_ZN( regA ) \
	SET##_DUMMY \
    SWITCH_END
	
#define	SRE( GET, SET )	\
	GET(, true)	\
	SET_FLAG_C( dataBus & 1 )	\
	_value = dataBus >> 1;	\
    regA ^= _value;		\
	SET_FLAG_ZN( regA ) \
	SET##_DUMMY \
    SWITCH_END

#define DUMMY( GET )	\
	GET(_LAST, false)  \
    SWITCH_END                   
	
#define ANC	\
	GET_IMM(_LAST, false)			\
	regA &= dataBus;		\
	SET_FLAG_ZN( regA ) \
    SET_FLAG_C( GET_FLAG_N )    \
    SWITCH_END	
		
#define ALR	\
	GET_IMM(_LAST, false)	\
	regA &= dataBus;	\
	SET_FLAG_C( regA & 1 )	\
	regA >>= 1;	\
	SET_FLAG_ZN( regA ) \
    SWITCH_END	    
	
#define RRA( GET, SET )	\
	GET(, true)	\
	_value = (dataBus >> 1) | (GET_FLAG_C << 7);	\
	SET_FLAG_C( dataBus & 1 )	\
	SET##_DUMMY \
    dataBus = _value;   \
    _ADC  /* soBlock not needed, V calculation happens in last cycle */   \
    SWITCH_END

#define ARR	\
	GET_IMM(_LAST, false)	\
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
    SWITCH_END	
	
#define H1_AND_WRITE( VALUE )	\
	uint8_t strange = (absolute >> 8) + 1;	\
	strange &= VALUE;	\
	if (PAGE_CROSSED)	\
		absIndexed = (strange << 8) | (absIndexed & 0xff);    \
    WRITE_LAST( absIndexed, strange );  \
	
#define SHA_INDIRECT_INDEXED	\
    SWITCH0    \
        INDIRECT_INDEXED( true )	\
        readNext = false; \
    SWITCH1 \
        H1_AND_WRITE( regA & regX ) \
    SWITCH_END        

#define TAS_ABS_INDEXED	\
    SWITCH0    \
        ABS_INDEXED(regY, true)	\
        readNext = false; \
    SWITCH1 \
        regS = regA & regX;	\
        H1_AND_WRITE( regS ) \
    SWITCH_END 
	
#define SH_ABS_INDEXED( REG_INDEX, REG ) \
    SWITCH0    \
        ABS_INDEXED( REG_INDEX, true )	\
        readNext = false; \
    SWITCH1 \
        H1_AND_WRITE( REG ) \
    SWITCH_END 
		
#define ANE	\
SWITCH01    \
	READ_PC_INC_LAST	\
	regA = ( regA | magicAne ) & regX & dataBus;	\
	SET_FLAG_ZN( regA ) \
SWITCH_END 
	
#define LAX( GET )	\
	GET(_LAST, false)	\
	regA = regX = dataBus;	\
	SET_FLAG_ZN( dataBus )  \
    SWITCH_END
		
#define LXA	\
SWITCH01    \
	{ READ_PC_INC_LAST	\
	regA = regX = ( regA | magicLax ) & dataBus;	\
	SET_FLAG_ZN( regA ) }   \
SWITCH_END 
	
#define LAS	\
	GET_ABS_INDEXED_REGY(_LAST, false)	\
	regA = dataBus & regS; \
	regX = regS = regA;	\
	SET_FLAG_ZN( regA ) \
    SWITCH_END 
	
#define DCP( GET, SET )	\
	GET(, true)	\
	_value = dataBus - 1;	\
    SET_FLAG_C( regA >= _value ) \
	SET_FLAG_ZN( (regA - _value) & 0xff )   \
	SET##_DUMMY \
    SWITCH_END
	
#define SBX	\
	GET_IMM(_LAST, false) \
	{ uint16_t result = (regA & regX) - dataBus;	\
	SET_FLAG_C( result < 0x100 )	\
	regX = result & 0xff;	\
	SET_FLAG_ZN( regX ) }   \
    SWITCH_END 
	
#define ISC( GET, SET )	\
	GET(, true)	\
	_value = dataBus + 1;	\
	SET##_DUMMY \
    dataBus = _value;   \
    _SBC   /* soBlock not needed, V calculation happens in last cycle */     \
    SWITCH_END
	
//// 

    if (!step) {
        readNext = true; // will be reseted before write cycles

        if (unlikely(killed)) {
            READ(0xffff)
            return;
        }

        if (unlikely(interruptSampled))
            return interrupt();
                
        IR = drive->cpuRead( pc++ );
    }
		
	switch( IR ) {
        case 0x00:
            interrupt<true>( );
            break;
        #include "../../m6510/optable.h"
	}
}
