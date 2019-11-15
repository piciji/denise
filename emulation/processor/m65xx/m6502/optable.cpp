
#include "m6502.h"

#define op(id, name, ...) case id: return name(__VA_ARGS__);
#define fp(name) &M6502::_##name
#define UO //undocumented opcode but always predictable
/**
 * don't use these kind of opcodes
 * some results differs between visual6502 and real cpu
 * Visual6502 is a digital representation so it can not handle race conditions that good
 * real cpu could produce different results, depending on a lot of things like heat, cpu version, bus usage and so on
 */
#define UUO //unstable undocumented opcode

namespace MOS65FAMILY {

auto M6502::decode( uint8_t IR ) -> void {

	switch( IR ) {
		op(0x00, brk) 
		op(0x01, indexedIndirect, fp(ora)) 
UO      op(0x02, kill) 
UO      op(0x03, indexedIndirectM, fp(asl), fp(ora) ) //SLO (ASO)    
UO      op(0x04, zeroPage) 
        op(0x05, zeroPage, fp(ora)) 
        op(0x06, zeroPageM, fp(asl)) 
UO      op(0x07, zeroPageM, fp(asl), fp(ora)) //SLO (ASO)    
		op(0x08, php) 
		op(0x09, immediate, fp(ora), A) 
		op(0x0a, implied, fp(asl), A) 
UO		op(0x0b, immediateAnc) 
UO      op(0x0c, absolute); 
		op(0x0d, absolute, fp(ora)); 
		op(0x0e, absoluteM, fp(asl)) 
UO		op(0x0f, absoluteM, fp(asl), fp(ora)) //SLO (ASO)  
		op(0x10, branch, N, false) 
        op(0x11, indirectIndexed, fp(ora)) 
UO      op(0x12, kill) 
UO      op(0x13, indirectIndexedM, fp(asl), fp(ora) )  //SLO (ASO)    
UO      op(0x14, zeroPageIndexed, X) 
        op(0x15, zeroPageIndexed, X, fp(ora)) 
        op(0x16, zeroPageIndexedM, fp(asl)) 
UO      op(0x17, zeroPageIndexedM, fp(asl), fp(ora)) //SLO (ASO)    
		op(0x18, clear, C) 
		op(0x19, absoluteIndexed, Y, fp(ora)) 
UO      op(0x1a, nop) 
UO		op(0x1b, absoluteIndexedM, Y, fp(asl), fp(ora)) //SLO (ASO)    
UO      op(0x1c, absoluteIndexed, X) 
		op(0x1d, absoluteIndexed, X, fp(ora)) 
		op(0x1e, absoluteIndexedM, X, fp(asl)) 
UO		op(0x1f, absoluteIndexedM, X, fp(asl), fp(ora)) //SLO (ASO) 
		op(0x20, jsrAbsolute) 
		op(0x21, indexedIndirect, fp(and)) 
UO      op(0x22, kill) 
UO      op(0x23, indexedIndirectM, fp(rol), fp(and) ) //rla 
        op(0x24, zeroPage, fp(bit)) 
        op(0x25, zeroPage, fp(and)) 
        op(0x26, zeroPageM, fp(rol)) 
UO      op(0x27, zeroPageM, fp(rol), fp(and)) //rla 
		op(0x28, plp) 
		op(0x29, immediate, fp(and), A) 
		op(0x2a, implied, fp(rol), A) 
UO		op(0x2b, immediateAnc) 
		op(0x2c, absolute, fp(bit)) 
		op(0x2d, absolute, fp(and)) 
		op(0x2e, absoluteM, fp(rol)) 
UO		op(0x2f, absoluteM, fp(rol), fp(and)) //rla 
		op(0x30, branch, N, true) 
        op(0x31, indirectIndexed, fp(and)) 
UO      op(0x32, kill) 
UO      op(0x33, indirectIndexedM, fp(rol), fp(and) ) //rla 
UO      op(0x34, zeroPageIndexed, X) 
        op(0x35, zeroPageIndexed, X, fp(and)) 
        op(0x36, zeroPageIndexedM, fp(rol)) 
UO      op(0x37, zeroPageIndexedM, fp(rol), fp(and)) //rla 
		op(0x38, set, C) 
		op(0x39, absoluteIndexed, Y, fp(and)) 
UO      op(0x3a, nop) 
UO		op(0x3b, absoluteIndexedM, Y, fp(rol), fp(and)) //rla 
UO      op(0x3c, absoluteIndexed, X) 
		op(0x3d, absoluteIndexed, X, fp(and)) 
		op(0x3e, absoluteIndexedM, X, fp(rol)) 
UO		op(0x3f, absoluteIndexedM, X, fp(rol), fp(and)) //rla 
		op(0x40, rti) 
		op(0x41, indexedIndirect, fp(eor)) 
UO      op(0x42, kill) 
UO      op(0x43, indexedIndirectM, fp(lsr), fp(eor) ) //SRE (LSE) 
UO      op(0x44, zeroPage) 
        op(0x45, zeroPage, fp(eor)) 
        op(0x46, zeroPageM, fp(lsr)) 
UO      op(0x47, zeroPageM, fp(lsr), fp(eor)) //SRE (LSE) 
        op(0x48, pha) 
		op(0x49, immediate, fp(eor), A) 
		op(0x4a, implied, fp(lsr), A) 
UO      op(0x4b, immediateAlr)  //ALR (ASR) 
		op(0x4c, jmpAbsolute) 
		op(0x4d, absolute, fp(eor)) 
		op(0x4e, absoluteM, fp(lsr)) 
UO		op(0x4f, absoluteM, fp(lsr), fp(eor)) //SRE (LSE) 
		op(0x50, branch, V, false) 
        op(0x51, indirectIndexed, fp(eor)) 
UO      op(0x52, kill) 
UO      op(0x53, indirectIndexedM, fp(lsr), fp(eor) ) //SRE (LSE) 
UO      op(0x54, zeroPageIndexed, X) 
        op(0x55, zeroPageIndexed, X, fp(eor)) 
        op(0x56, zeroPageIndexedM, fp(lsr)) 
UO      op(0x57, zeroPageIndexedM, fp(lsr), fp(eor)) //SRE (LSE) 
		op(0x58, clear, I) 
		op(0x59, absoluteIndexed, Y, fp(eor)) 
UO      op(0x5a, nop) 
UO		op(0x5b, absoluteIndexedM, Y, fp(lsr), fp(eor)) //SRE (LSE) 
UO      op(0x5c, absoluteIndexed, X) 
		op(0x5d, absoluteIndexed, X, fp(eor)) 
		op(0x5e, absoluteIndexedM, X, fp(lsr)) 
UO		op(0x5f, absoluteIndexedM, X, fp(lsr), fp(eor)) //SRE (LSE) 
		op(0x60, rts) 
		op(0x61, indexedIndirect, fp(adc)) 
UO      op(0x62, kill) 
UO      op(0x63, indexedIndirectM, fp(ror), fp(adc) ) //rra 
UO      op(0x64, zeroPage) 
        op(0x65, zeroPage, fp(adc)) 
        op(0x66, zeroPageM, fp(ror)) 
UO      op(0x67, zeroPageM, fp(ror), fp(adc)) //rra 
        op(0x68, pla) 
		op(0x69, immediate, fp(adc), A) 
		op(0x6a, implied, fp(ror), A) 
UO      op(0x6b, immediateArr) //arr 
		op(0x6c, jmpIndirect) 
		op(0x6d, absolute, fp(adc)) 
		op(0x6e, absoluteM, fp(ror)) 	
UO		op(0x6f, absoluteM, fp(ror), fp(adc)) //rra 
		op(0x70, branch, V, true) 
        op(0x71, indirectIndexed, fp(adc)) 
UO      op(0x72, kill) 
UO      op(0x73, indirectIndexedM, fp(ror), fp(adc) ) //rra 
UO      op(0x74, zeroPageIndexed, X) 
        op(0x75, zeroPageIndexed, X, fp(adc)) 
        op(0x76, zeroPageIndexedM, fp(ror)) 
UO      op(0x77, zeroPageIndexedM, fp(ror), fp(adc))  //rra 
		op(0x78, set, I) 
		op(0x79, absoluteIndexed, Y, fp(adc)) 
UO      op(0x7a, nop) 
UO		op(0x7b, absoluteIndexedM, Y, fp(ror), fp(adc)) //rra 
UO      op(0x7c, absoluteIndexed, X) 
		op(0x7d, absoluteIndexed, X, fp(adc)) 
		op(0x7e, absoluteIndexedM, X, fp(ror)) 
UO		op(0x7f, absoluteIndexedM, X, fp(ror), fp(adc)) //rra 
UO      op(0x80, immediate) 
        op(0x81, indexedIndirectW, A)  
UO      op(0x82, immediate) 
UO      op(0x83, indexedIndirectW, A & X ) //SAX (AXS, AAX) 
        op(0x84, zeroPageW, Y) 
        op(0x85, zeroPageW, A) 
        op(0x86, zeroPageW, X) 
UO      op(0x87, zeroPageW, A & X) //sax 
		op(0x88, implied, fp(dec), Y) 
UO      op(0x89, immediate) //nop 
        op(0x8a, transfer, X, A, 1) 
UUO     op(0x8b, immediateAne) //ANE (XAA) 
		op(0x8c, absoluteW, Y) 
		op(0x8d, absoluteW, A) 
		op(0x8e, absoluteW, X) 
UO      op(0x8f, absoluteW, A & X) 
		op(0x90, branch, C, false) 
        op(0x91, indirectIndexedW, A) 
UO      op(0x92, kill) 
UUO     op(0x93, indirectIndexedWAhx) //SHA (AXA, AHX) 
        op(0x94, zeroPageIndexedW, X, Y) 
        op(0x95, zeroPageIndexedW, X, A) 
        op(0x96, zeroPageIndexedW, Y, X) 
UO      op(0x97, zeroPageIndexedW, Y, X & A)  //sax 
        op(0x98, transfer, Y, A, 1) 
		op(0x99, absoluteIndexedW, Y, A) 
        op(0x9a, transfer, X, S, 0) 
UUO     op(0x9b, absoluteIndexedWTas) //tas 
UUO     op(0x9c, absoluteIndexedWSh, X, Y) //shy 
		op(0x9d, absoluteIndexedW, X, A) 
UUO     op(0x9e, absoluteIndexedWSh, Y, X) //shx 
UUO     op(0x9f, absoluteIndexedWAhx) //SHA (AXA, AHX) 
		op(0xa0, immediate, fp(ld), Y) //ldy 
		op(0xa1, indexedIndirect, fp(ld)) 
		op(0xa2, immediate, fp(ld), X) //ldx 
UO		op(0xa3, indexedIndirectLax) 
        op(0xa4, zeroPage, fp(ld), Y) 
        op(0xa5, zeroPage, fp(ld), A) 
        op(0xa6, zeroPage, fp(ld), X) 
UO		op(0xa7, zeroPageLax) 
        op(0xa8, transfer, A, Y, 1) 
		op(0xa9, immediate, fp(ld), A) //lda 
        op(0xaa, transfer, A, X, 1) 
UUO		op(0xab, immediateLax) 
		op(0xac, absolute, fp(ld), Y) 
		op(0xad, absolute, fp(ld), A) 
		op(0xae, absolute, fp(ld), X) 
UO		op(0xaf, absoluteLax) 
		op(0xb0, branch, C, true) 
        op(0xb1, indirectIndexed, fp(ld)) 
UO      op(0xb2, kill) 
UO		op(0xb3, indirectIndexedLax) 
        op(0xb4, zeroPageIndexed, X, fp(ld), Y) 
        op(0xb5, zeroPageIndexed, X, fp(ld), A) 
        op(0xb6, zeroPageIndexed, Y, fp(ld), X) 
UO		op(0xb7, zeroPageIndexedLax) 
		op(0xb8, clear, V) 
		op(0xb9, absoluteIndexed, Y, fp(ld)) 
        op(0xba, transfer, S, X, 1) 
UUO     op(0xbb, absoluteIndexedLas) //LAS (LAR) 
		op(0xbc, absoluteIndexed, X, fp(ld), Y) 
		op(0xbd, absoluteIndexed, X, fp(ld), A) 
		op(0xbe, absoluteIndexed, Y, fp(ld), X) 
UO		op(0xbf, absoluteIndexedLax) 
		op(0xc0, immediate, fp(cpy), Y) 
		op(0xc1, indexedIndirect, fp(cmp)) 
UO      op(0xc2, immediate) //nop 
UO      op(0xc3, indexedIndirectM, fp(dec), fp(cmp) ) //DCP (DCM) 
        op(0xc4, zeroPage, fp(cpy), Y) 
        op(0xc5, zeroPage, fp(cmp), A) 
        op(0xc6, zeroPageM, fp(dec)) 
UO      op(0xc7, zeroPageM, fp(dec), fp(cmp)) //DCP (DCM) 
		op(0xc8, implied, fp(inc), Y) 
		op(0xc9, immediate, fp(cmp), A) 
		op(0xca, implied, fp(dec), X) 
UO		op(0xcb, immediateSbx)  //SBX (AXS, SAX) 
		op(0xcc, absolute, fp(cpy), Y) 
		op(0xcd, absolute, fp(cmp), A) 
		op(0xce, absoluteM, fp(dec)) 	
UO		op(0xcf, absoluteM, fp(dec), fp(cmp)) //DCP (DCM) 
		op(0xd0, branch, Z, false) 
        op(0xd1, indirectIndexed, fp(cmp)) 
UO      op(0xd2, kill) 
UO      op(0xd3, indirectIndexedM, fp(dec), fp(cmp) ) //DCP (DCM) 
UO      op(0xd4, zeroPageIndexed, X) 
        op(0xd5, zeroPageIndexed, X, fp(cmp)) 
        op(0xd6, zeroPageIndexedM, fp(dec)) 
UO      op(0xd7, zeroPageIndexedM, fp(dec), fp(cmp)) //DCP (DCM) 
		op(0xd8, clear, D) 
		op(0xd9, absoluteIndexed, Y, fp(cmp)) 
UO      op(0xda, nop) 
UO		op(0xdb, absoluteIndexedM, Y, fp(dec), fp(cmp)) //DCP (DCM) 
UO      op(0xdc, absoluteIndexed, X) 
		op(0xdd, absoluteIndexed, X, fp(cmp)) 
		op(0xde, absoluteIndexedM, X, fp(dec)) 
UO		op(0xdf, absoluteIndexedM, X, fp(dec), fp(cmp)) //DCP (DCM) 
		op(0xe0, immediate, fp(cpx), X) 
		op(0xe1, indexedIndirect, fp(sbc)) 
UO      op(0xe2, immediate) //nop 
UO      op(0xe3, indexedIndirectM, fp(inc), fp(sbc) ) //ISC (ISB, INS) 
        op(0xe4, zeroPage, fp(cpx), X) 
        op(0xe5, zeroPage, fp(sbc), A) 
        op(0xe6, zeroPageM, fp(inc)) 
UO      op(0xe7, zeroPageM, fp(inc), fp(sbc)) //ISC (ISB, INS) 
		op(0xe8, implied, fp(inc), X) 
		op(0xe9, immediate, fp(sbc), A) 
		op(0xea, nop)  
UO      op(0xeb, immediate, fp(sbc), A) 
		op(0xec, absolute, fp(cpx), X) 
		op(0xed, absolute, fp(sbc)) 
		op(0xee, absoluteM, fp(inc)) 
UO		op(0xef, absoluteM, fp(inc), fp(sbc)) //ISC (ISB, INS) 
		op(0xf0, branch, Z, true) 
        op(0xf1, indirectIndexed, fp(sbc)) 
UO      op(0xf2, kill) 
UO      op(0xf3, indirectIndexedM, fp(inc), fp(sbc) ) //ISC (ISB, INS) 
UO      op(0xf4, zeroPageIndexed, X) 
        op(0xf5, zeroPageIndexed, X, fp(sbc)) 
        op(0xf6, zeroPageIndexedM, fp(inc)) 
UO      op(0xf7, zeroPageIndexedM, fp(inc), fp(sbc)) //ISC (ISB, INS) 
		op(0xf8, set, D) 
		op(0xf9, absoluteIndexed, Y, fp(sbc)) 
UO      op(0xfa, nop) 
UO		op(0xfb, absoluteIndexedM, Y, fp(inc), fp(sbc)) //ISC (ISB, INS) 
UO      op(0xfc, absoluteIndexed, X) 
		op(0xfd, absoluteIndexed, X, fp(sbc)) 
		op(0xfe, absoluteIndexedM, X, fp(inc)) 
UO		op(0xff, absoluteIndexedM, X, fp(inc), fp(sbc)) //ISC (ISB, INS) 				            
	}
}

}

#undef op
#undef fp
#undef UO
#undef UUO
				