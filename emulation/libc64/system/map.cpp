
#define IO_MAPPING  \
    memoryCpu.map( &readVicReg, &writeVicReg, 0xd0, 0xd3);      \
                                                                \
    if (!debugCart.enable)                                      \
        memoryCpu.map( &readSidReg, &writeSidReg, 0xd4, 0xd7);  \
    else {                                                      \
        memoryCpu.map( &readSidReg, &writeSidReg, 0xd4, 0xd6);  \
        memoryCpu.map( &readSidReg, &writeDebugReg, 0xd7, 0xd7);\
    }                                                           \
    memoryCpu.map( &readColorRam, &writeColorRam, 0xd8, 0xdb);  \
    memoryCpu.map( &readCia1Reg, &writeCia1Reg, 0xdc, 0xdc);    \
    memoryCpu.map( &readCia2Reg, &writeCia2Reg, 0xdd, 0xdd);    \
    memoryCpu.map( &readIo1Reg, &writeIo1Reg, 0xde, 0xde);      \
    memoryCpu.map( &readIo2Reg, &writeIo2Reg, 0xdf, 0xdf);

namespace LIBC64 {

auto System::remapCpu( ) -> void {

    if (expansionPort == noExpansion) {
        
        memoryCpu.map( &readRam, &writeRam, 0x0, 0x9f );

        memoryCpu.map( ((mode & 3) == 3) ? &readBasicRom : &readRam, &writeRam, 0xa0, 0xbf );

        memoryCpu.map( &readRam, &writeRam, 0xc0, 0xcf );

        if ((mode & 3) == 0)
            memoryCpu.map( &readRam, &writeRam, 0xd0, 0xdf );
        else if ((mode & 4) == 0)
            memoryCpu.map( &readCharRom, &writeRam, 0xd0, 0xdf );
        else {
            IO_MAPPING
        }

        memoryCpu.map( (mode & 2) ? &readKernalRom : &readRam, &writeRam, 0xe0, 0xff );

        return;
    } 
    
    // full mapping for 8k, 16k, ultimax, no cart
    uint8_t subMode = mode & 7;
    uint8_t ramMode = mode & 3;
    uint8_t cartMode = (mode >> 3) & 3;
    bool ultimax = isUltimax();

    // 00 - 0f -> always ram
    memoryCpu.map( &readRam, &writeRam, 0x0, 0x0f );

    // 10 - 7f
    if ( ultimax )
        memoryCpu.map( &readUnmapped, &writeUnmapped, 0x10, 0x7f );
    else
        memoryCpu.map( &readRam, &writeRam, 0x10, 0x7f );

    // 80 - 9f
    if ( ultimax ) {
        memoryCpu.map( &readRomL, 0x80, 0x9f);
        memoryCpu.map( &writeUltimaxRomL, 0x80, 0x9f );

    } else if ( (cartMode == 0 || cartMode == 1) && ramMode == 3 ) {
        memoryCpu.map( &readRomL, 0x80, 0x9f);
        memoryCpu.map( &writeRomL, 0x80, 0x9f );
    } else
        memoryCpu.map( &readRam, &writeRamAt80To9F, 0x80, 0x9f );

    // a0 - bf
    if ( ultimax )
        memoryCpu.map( &readUltimaxA0, &writeUltimaxA0, 0xa0, 0xbf );

    else if ( (cartMode == 1 || cartMode == 3) && ramMode == 3 ) {
        memoryCpu.map( &readBasicRom, 0xa0, 0xbf );
        memoryCpu.map( &writeRamAtA0ToBF, 0xa0, 0xbf );

    } else if (cartMode == 0 && (ramMode == 2 || ramMode == 3) ) {

        memoryCpu.map( &readRomH, 0xa0, 0xbf );
        memoryCpu.map( &writeRomH, 0xa0, 0xbf );
    } else
        memoryCpu.map( &readRam, &writeRamAtA0ToBF, 0xa0, 0xbf );

    // c0 - cf
    if ( ultimax )
        memoryCpu.map( &readUnmapped, &writeUnmapped, 0xc0, 0xcf );
    else
        memoryCpu.map( &readRam, &writeRam, 0xc0, 0xcf );

    // d0 - df
    if ( ultimax || subMode == 5 || subMode == 6 || subMode == 7 ) {
        IO_MAPPING
    } else if ( (subMode == 1 || subMode == 2 || subMode == 3) && (mode != 1)  ) {

        memoryCpu.map( &readCharRom, 0xd0, 0xdf );
        memoryCpu.map( &writeRam, 0xd0, 0xdf );
    } else
        memoryCpu.map( &readRam, &writeRam, 0xd0, 0xdf );

    // e0 - ff
    if ( ultimax ) {
        memoryCpu.map( &readRomH, 0xe0, 0xff);
        memoryCpu.map( &writeUltimaxRomH, 0xe0, 0xff );

    } else if (ramMode == 2 || ramMode == 3) {
        memoryCpu.map( &readKernalRom, 0xe0, 0xff );
        memoryCpu.map( &writeRam, 0xe0, 0xff );

    } else
        memoryCpu.map( &readRam, &writeRam, 0xe0, 0xff );
    
    expansionPort->memoryMapUpdated();
}

}