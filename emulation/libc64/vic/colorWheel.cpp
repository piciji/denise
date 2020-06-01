
/**
 * color spectrum emulation by Pepto (new Colodore version)
 */

#include "base.h"

namespace LIBC64 { 
    
auto VicIIBase::initColorWheel() -> void {
    double sector = 360.0 / 16.0;
    double origin = sector / 2.0;
    
    // 9 luma: most common
    double* mc = &luma[0][0];        
    mc[ 0 ] = 0;               // black
    mc[ 0x6 ] = mc[ 0x9 ] = 8; // Blue,    Brown
    mc[ 0xb ] = mc[ 0x2 ] = 10; // Dk.Grey, Red
    mc[ 0x4 ] = mc[ 0x8 ] = 12; // Purple,  Orange
    mc[ 0xc ] = mc[ 0xe ] = 15; // Md.Grey, Lt.Blue
    mc[ 0x5 ] = mc[ 0xa ] = 16; // Green,   Lt.Red
    mc[ 0xf ] = mc[ 0x3 ] = 20; // Lt.Grey, Cyan
    mc[ 0x7 ] = mc[ 0xd ] = 24; // Yellow,  Lt.Green
    mc[ 0x1 ] = 32; // White

    // 5 luma: first revision
    double* fr = &luma[1][0];
    fr[ 0 ] = 0;                 // black
    fr[ 0x2 ] = fr[ 0x6 ] = fr[ 0x9 ] = fr[ 0xb ] = 8 * 1;
    fr[ 0x4 ] = fr[ 0x5 ] = fr[ 0x8 ] = fr[ 0xa ] = fr[ 0xc ] = fr[ 0xe ] = 8 * 2;
    fr[ 0x3 ] = fr[ 0x7 ] = fr[ 0xd ] = fr[ 0xf ] = 8 * 3;
    fr[ 0x1 ] = 8 * 4;
    
    for(auto& l : luma[0])
        l = 8.0 * l;

    for (auto& l : luma[1])
        l = 8.0 * l;
    
    // no chroma, monochrome
    chroma[0] = chroma[1] = chroma[0xb] = chroma[0xc] = chroma[0xf] = 0;
    // chromas
    chroma[ 0x4 ] = 2; // Purple
    chroma[ 0x2 ] = chroma[ 0xa ] = 4; // Red
    chroma[ 0x8 ] = 5; // Orange
    chroma[ 0x9 ] = 6; // Brown
    chroma[ 0x7 ] = 7; // Yellow
    chroma[ 0x5 ] = chroma[ 0xd ] = 2 + 8; // Green
    chroma[ 0x3 ] = 4 + 8; // Cyan
    chroma[ 0x6 ] = chroma[ 0xe ] = 7 + 8; // Blue
    
    for(auto& c : chroma) {
        if (c)
            c = origin + c * sector;
    }
}

auto VicIIBase::getLuma(uint8_t index, bool newRevision) -> double {
    
    return luma[ newRevision ? 0 : 1 ][ index & 15 ];
}

auto VicIIBase::getChroma(uint8_t index) -> double {
    
    return chroma[ index & 15 ];
}

}
