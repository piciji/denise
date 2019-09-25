//
//#include "m6502.h"
//
//#define COMMA ,
//#define op(id, name, ...) case id: return name(__VA_ARGS__);
//#define fp(name) &M6502::_##name
//#define UO //undocumented opcode but always predictable
///**
// * don't use these kind of opcodes
// * some results differs between visual6502 and real cpu
// * Visual6502 is a digital representation so it can not handle race conditions that good
// * real cpu could produce different results, depending on a lot of things like heat, cpu version, bus usage and so on
// */
//#define UUO //unstable undocumented opcode
//
//namespace MOS65FAMILY {
//
//auto M6502::decode( uint8_t IR ) -> void {
//
//	switch( IR ) {
		op(0x00, brk) 
		op(0x01, indexedIndirect, fp(ora)) 
UO      op(0x02, kill) 
UO      op(0x03, indexedIndirectM, fp(asl), fp(ora) ) //SLO (ASO)    
UO      op(0x04, zeroPage) 
        op(0x05, zeroPage, fp(ora)) 
        op(0x06, zeroPageM, fp(asl)) 
UO      op(0x07, zeroPageM, fp(asl), fp(ora)) //SLO (ASO)    
		op(0x08, php) 
		op(0x09, immediate<RegA>, fp(ora)) 
		op(0x0a, implied<RegA>, fp(asl)) 
UO		op(0x0b, immediateAnc) 
UO      op(0x0c, absolute); 
		op(0x0d, absolute, fp(ora)); 
		op(0x0e, absoluteM, fp(asl)) 
UO		op(0x0f, absoluteM, fp(asl), fp(ora)) //SLO (ASO)  
		op(0x10, branch<FlagN>, false) 
        op(0x11, indirectIndexed, fp(ora)) 
UO      op(0x12, kill) 
UO      op(0x13, indirectIndexedM, fp(asl), fp(ora) )  //SLO (ASO)    
UO      op(0x14, zeroPageIndexed<RegX>) 
        op(0x15, zeroPageIndexed<RegX>, fp(ora)) 
        op(0x16, zeroPageIndexedM, fp(asl)) 
UO      op(0x17, zeroPageIndexedM, fp(asl), fp(ora)) //SLO (ASO)    
		op(0x18, clear<FlagC>) 
		op(0x19, absoluteIndexed<RegY>, fp(ora)) 
UO      op(0x1a, nop) 
UO		op(0x1b, absoluteIndexedM<RegY>, fp(asl), fp(ora)) //SLO (ASO)    
UO      op(0x1c, absoluteIndexed<RegX>) 
		op(0x1d, absoluteIndexed<RegX>, fp(ora)) 
		op(0x1e, absoluteIndexedM<RegX>, fp(asl)) 
UO		op(0x1f, absoluteIndexedM<RegX>, fp(asl), fp(ora)) //SLO (ASO) 
		op(0x20, jsrAbsolute) 
		op(0x21, indexedIndirect, fp(and)) 
UO      op(0x22, kill) 
UO      op(0x23, indexedIndirectM, fp(rol), fp(and) ) //rla 
        op(0x24, zeroPage, fp(bit)) 
        op(0x25, zeroPage, fp(and)) 
        op(0x26, zeroPageM, fp(rol)) 
UO      op(0x27, zeroPageM, fp(rol), fp(and)) //rla 
		op(0x28, plp) 
		op(0x29, immediate<RegA>, fp(and)) 
		op(0x2a, implied<RegA>, fp(rol)) 
UO		op(0x2b, immediateAnc) 
		op(0x2c, absolute, fp(bit)) 
		op(0x2d, absolute, fp(and)) 
		op(0x2e, absoluteM, fp(rol)) 
UO		op(0x2f, absoluteM, fp(rol), fp(and)) //rla 
		op(0x30, branch<FlagN>, true) 
        op(0x31, indirectIndexed, fp(and)) 
UO      op(0x32, kill) 
UO      op(0x33, indirectIndexedM, fp(rol), fp(and) ) //rla 
UO      op(0x34, zeroPageIndexed<RegX>) 
        op(0x35, zeroPageIndexed<RegX>, fp(and)) 
        op(0x36, zeroPageIndexedM, fp(rol)) 
UO      op(0x37, zeroPageIndexedM, fp(rol), fp(and)) //rla 
		op(0x38, set<FlagC>) 
		op(0x39, absoluteIndexed<RegY>, fp(and)) 
UO      op(0x3a, nop) 
UO		op(0x3b, absoluteIndexedM<RegY>, fp(rol), fp(and)) //rla 
UO      op(0x3c, absoluteIndexed<RegX>) 
		op(0x3d, absoluteIndexed<RegX>, fp(and)) 
		op(0x3e, absoluteIndexedM<RegX>, fp(rol)) 
UO		op(0x3f, absoluteIndexedM<RegX>, fp(rol), fp(and)) //rla 
		op(0x40, rti) 
		op(0x41, indexedIndirect, fp(eor)) 
UO      op(0x42, kill) 
UO      op(0x43, indexedIndirectM, fp(lsr), fp(eor) ) //SRE (LSE) 
UO      op(0x44, zeroPage) 
        op(0x45, zeroPage, fp(eor)) 
        op(0x46, zeroPageM, fp(lsr)) 
UO      op(0x47, zeroPageM, fp(lsr), fp(eor)) //SRE (LSE) 
        op(0x48, pha) 
		op(0x49, immediate<RegA>, fp(eor)) 
		op(0x4a, implied<RegA>, fp(lsr)) 
UO      op(0x4b, immediateAlr)  //ALR (ASR) 
		op(0x4c, jmpAbsolute) 
		op(0x4d, absolute, fp(eor)) 
		op(0x4e, absoluteM, fp(lsr)) 
UO		op(0x4f, absoluteM, fp(lsr), fp(eor)) //SRE (LSE) 
		op(0x50, branch<FlagV>, false) 
        op(0x51, indirectIndexed, fp(eor)) 
UO      op(0x52, kill) 
UO      op(0x53, indirectIndexedM, fp(lsr), fp(eor) ) //SRE (LSE) 
UO      op(0x54, zeroPageIndexed<RegX>) 
        op(0x55, zeroPageIndexed<RegX>, fp(eor)) 
        op(0x56, zeroPageIndexedM, fp(lsr)) 
UO      op(0x57, zeroPageIndexedM, fp(lsr), fp(eor)) //SRE (LSE) 
		op(0x58, clear<FlagI>) 
		op(0x59, absoluteIndexed<RegY>, fp(eor)) 
UO      op(0x5a, nop) 
UO		op(0x5b, absoluteIndexedM<RegY>, fp(lsr), fp(eor)) //SRE (LSE) 
UO      op(0x5c, absoluteIndexed<RegX>) 
		op(0x5d, absoluteIndexed<RegX>, fp(eor)) 
		op(0x5e, absoluteIndexedM<RegX>, fp(lsr)) 
UO		op(0x5f, absoluteIndexedM<RegX>, fp(lsr), fp(eor)) //SRE (LSE) 
		op(0x60, rts) 
		op(0x61, indexedIndirect, fp(adc)) 
UO      op(0x62, kill) 
UO      op(0x63, indexedIndirectM, fp(ror), fp(adc) ) //rra 
UO      op(0x64, zeroPage) 
        op(0x65, zeroPage, fp(adc)) 
        op(0x66, zeroPageM, fp(ror)) 
UO      op(0x67, zeroPageM, fp(ror), fp(adc)) //rra 
        op(0x68, pla) 
		op(0x69, immediate<RegA>, fp(adc)) 
		op(0x6a, implied<RegA>, fp(ror)) 
UO      op(0x6b, immediateArr) //arr 
		op(0x6c, jmpIndirect) 
		op(0x6d, absolute, fp(adc)) 
		op(0x6e, absoluteM, fp(ror)) 	
UO		op(0x6f, absoluteM, fp(ror), fp(adc)) //rra 
		op(0x70, branch<FlagV>, true) 
        op(0x71, indirectIndexed, fp(adc)) 
UO      op(0x72, kill) 
UO      op(0x73, indirectIndexedM, fp(ror), fp(adc) ) //rra 
UO      op(0x74, zeroPageIndexed<RegX>) 
        op(0x75, zeroPageIndexed<RegX>, fp(adc)) 
        op(0x76, zeroPageIndexedM, fp(ror)) 
UO      op(0x77, zeroPageIndexedM, fp(ror), fp(adc))  //rra 
		op(0x78, set<FlagI>) 
		op(0x79, absoluteIndexed<RegY>, fp(adc)) 
UO      op(0x7a, nop) 
UO		op(0x7b, absoluteIndexedM<RegY>, fp(ror), fp(adc)) //rra 
UO      op(0x7c, absoluteIndexed<RegX>) 
		op(0x7d, absoluteIndexed<RegX>, fp(adc)) 
		op(0x7e, absoluteIndexedM<RegX>, fp(ror)) 
UO		op(0x7f, absoluteIndexedM<RegX>, fp(ror), fp(adc)) //rra 
UO      op(0x80, immediate) 
        op(0x81, indexedIndirectW<RegA>)  
UO      op(0x82, immediate) 
UO      op(0x83, indexedIndirectW<RegAX>) //SAX (AXS, AAX) 
        op(0x84, zeroPageW<RegY>) 
        op(0x85, zeroPageW<RegA>) 
        op(0x86, zeroPageW<RegX>) 
UO      op(0x87, zeroPageW<RegAX>) //sax 
		op(0x88, implied<RegY>, fp(dec)) 
UO      op(0x89, immediate) //nop 
        op(0x8a, transfer<RegX COMMA RegA>, 1) 
UUO     op(0x8b, immediateAne) //ANE (XAA) 
		op(0x8c, absoluteW<RegY>) 
		op(0x8d, absoluteW<RegA>) 
		op(0x8e, absoluteW<RegX>) 
UO      op(0x8f, absoluteW<RegAX>) 
		op(0x90, branch<FlagC>, false) 
        op(0x91, indirectIndexedW) 
UO      op(0x92, kill) 
UUO     op(0x93, indirectIndexedWAhx) //SHA (AXA, AHX) 
        op(0x94, zeroPageIndexedW<RegX COMMA RegY>) 
        op(0x95, zeroPageIndexedW<RegX COMMA RegA>) 
        op(0x96, zeroPageIndexedW<RegY COMMA RegX>) 
UO      op(0x97, zeroPageIndexedW<RegY COMMA RegAX>)  //sax 
        op(0x98, transfer<RegY COMMA RegA>, 1) 
		op(0x99, absoluteIndexedW<RegY COMMA RegA>) 
        op(0x9a, transfer<RegX COMMA RegS>, 0) 
UUO     op(0x9b, absoluteIndexedWTas) //tas 
UUO     op(0x9c, absoluteIndexedWSh<RegX COMMA RegY>) //shy 
		op(0x9d, absoluteIndexedW<RegX COMMA RegA>) 
UUO     op(0x9e, absoluteIndexedWSh<RegY COMMA RegX>) //shx 
UUO     op(0x9f, absoluteIndexedWAhx) //SHA (AXA, AHX) 
		op(0xa0, immediate<RegY>, fp(ld)) //ldy 
		op(0xa1, indexedIndirect, fp(ld)) 
		op(0xa2, immediate<RegX>, fp(ld)) //ldx 
UO		op(0xa3, indexedIndirectLax) 
        op(0xa4, zeroPage<RegY>, fp(ld)) 
        op(0xa5, zeroPage<RegA>, fp(ld)) 
        op(0xa6, zeroPage<RegX>, fp(ld)) 
UO		op(0xa7, zeroPageLax) 
        op(0xa8, transfer<RegA COMMA RegY>, 1) 
		op(0xa9, immediate<RegA>, fp(ld)) //lda 
        op(0xaa, transfer<RegA COMMA RegX>, 1) 
UUO		op(0xab, immediateLax) 
		op(0xac, absolute<RegY>, fp(ld)) 
		op(0xad, absolute<RegA>, fp(ld)) 
		op(0xae, absolute<RegX>, fp(ld)) 
UO		op(0xaf, absoluteLax) 
		op(0xb0, branch<FlagC>, true) 
        op(0xb1, indirectIndexed, fp(ld)) 
UO      op(0xb2, kill) 
UO		op(0xb3, indirectIndexedLax) 
        op(0xb4, zeroPageIndexed<RegX COMMA RegY>, fp(ld)) 
        op(0xb5, zeroPageIndexed<RegX COMMA RegA>, fp(ld)) 
        op(0xb6, zeroPageIndexed<RegY COMMA RegX>, fp(ld)) 
UO		op(0xb7, zeroPageIndexedLax) 
		op(0xb8, clear<FlagV>) 
		op(0xb9, absoluteIndexed<RegY>, fp(ld)) 
        op(0xba, transfer<RegS COMMA RegX>, 1) 
UUO     op(0xbb, absoluteIndexedLas) //LAS (LAR) 
		op(0xbc, absoluteIndexed<RegX COMMA RegY>, fp(ld)) 
		op(0xbd, absoluteIndexed<RegX COMMA RegA>, fp(ld)) 
		op(0xbe, absoluteIndexed<RegY COMMA RegX>, fp(ld)) 
UO		op(0xbf, absoluteIndexedLax) 
		op(0xc0, immediate<RegY>, fp(cpy)) 
		op(0xc1, indexedIndirect, fp(cmp)) 
UO      op(0xc2, immediate) //nop 
UO      op(0xc3, indexedIndirectM, fp(dec), fp(cmp) ) //DCP (DCM) 
        op(0xc4, zeroPage<RegY>, fp(cpy)) 
        op(0xc5, zeroPage<RegA>, fp(cmp)) 
        op(0xc6, zeroPageM, fp(dec)) 
UO      op(0xc7, zeroPageM, fp(dec), fp(cmp)) //DCP (DCM) 
		op(0xc8, implied<RegY>, fp(inc)) 
		op(0xc9, immediate<RegA>, fp(cmp)) 
		op(0xca, implied<RegX>, fp(dec)) 
UO		op(0xcb, immediateSbx)  //SBX (AXS, SAX) 
		op(0xcc, absolute<RegY>, fp(cpy)) 
		op(0xcd, absolute<RegA>, fp(cmp)) 
		op(0xce, absoluteM, fp(dec)) 	
UO		op(0xcf, absoluteM, fp(dec), fp(cmp)) //DCP (DCM) 
		op(0xd0, branch<FlagZ>, false) 
        op(0xd1, indirectIndexed, fp(cmp)) 
UO      op(0xd2, kill) 
UO      op(0xd3, indirectIndexedM, fp(dec), fp(cmp) ) //DCP (DCM) 
UO      op(0xd4, zeroPageIndexed<RegX>) 
        op(0xd5, zeroPageIndexed<RegX>, fp(cmp)) 
        op(0xd6, zeroPageIndexedM, fp(dec)) 
UO      op(0xd7, zeroPageIndexedM, fp(dec), fp(cmp)) //DCP (DCM) 
		op(0xd8, clear<FlagD>) 
		op(0xd9, absoluteIndexed<RegY>, fp(cmp)) 
UO      op(0xda, nop) 
UO		op(0xdb, absoluteIndexedM<RegY>, fp(dec), fp(cmp)) //DCP (DCM) 
UO      op(0xdc, absoluteIndexed<RegX>) 
		op(0xdd, absoluteIndexed<RegX>, fp(cmp)) 
		op(0xde, absoluteIndexedM<RegX>, fp(dec)) 
UO		op(0xdf, absoluteIndexedM<RegX>, fp(dec), fp(cmp)) //DCP (DCM) 
		op(0xe0, immediate<RegX>, fp(cpx)) 
		op(0xe1, indexedIndirect, fp(sbc)) 
UO      op(0xe2, immediate) //nop 
UO      op(0xe3, indexedIndirectM, fp(inc), fp(sbc) ) //ISC (ISB, INS) 
        op(0xe4, zeroPage<RegX>, fp(cpx)) 
        op(0xe5, zeroPage<RegA>, fp(sbc)) 
        op(0xe6, zeroPageM, fp(inc)) 
UO      op(0xe7, zeroPageM, fp(inc), fp(sbc)) //ISC (ISB, INS) 
		op(0xe8, implied<RegX>, fp(inc)) 
		op(0xe9, immediate<RegA>, fp(sbc)) 
		op(0xea, nop)  
UO      op(0xeb, immediate<RegA>, fp(sbc)) 
		op(0xec, absolute<RegX>, fp(cpx)) 
		op(0xed, absolute<RegA>, fp(sbc)) 
		op(0xee, absoluteM, fp(inc)) 
UO		op(0xef, absoluteM, fp(inc), fp(sbc)) //ISC (ISB, INS) 
		op(0xf0, branch<FlagZ>, true) 
        op(0xf1, indirectIndexed, fp(sbc)) 
UO      op(0xf2, kill) 
UO      op(0xf3, indirectIndexedM, fp(inc), fp(sbc) ) //ISC (ISB, INS) 
UO      op(0xf4, zeroPageIndexed<RegX>) 
        op(0xf5, zeroPageIndexed<RegX>, fp(sbc)) 
        op(0xf6, zeroPageIndexedM, fp(inc)) 
UO      op(0xf7, zeroPageIndexedM, fp(inc), fp(sbc)) //ISC (ISB, INS) 
		op(0xf8, set<FlagD>) 
		op(0xf9, absoluteIndexed<RegY>, fp(sbc)) 
UO      op(0xfa, nop) 
UO		op(0xfb, absoluteIndexedM<RegY>, fp(inc), fp(sbc)) //ISC (ISB, INS) 
UO      op(0xfc, absoluteIndexed<RegX>) 
		op(0xfd, absoluteIndexed<RegX>, fp(sbc)) 
		op(0xfe, absoluteIndexedM<RegX>, fp(inc)) 
UO		op(0xff, absoluteIndexedM<RegX>, fp(inc), fp(sbc)) //ISC (ISB, INS) 				            
//	}
//}
//
//}
//
//#undef op
//#undef fp
//#undef UO
//#undef UUO
//#undef COMMA	
