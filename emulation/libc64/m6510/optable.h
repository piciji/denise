
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
    TRAPPED_KILL
    break;
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
