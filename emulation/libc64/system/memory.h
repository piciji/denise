
#pragma once

#include <functional>

namespace LIBC64 {

struct Memory {    
    
    enum Mode { Direct, Linear };
    
    using Read = std::function<auto (uint16_t) -> uint8_t>;
    using Write = std::function<auto (uint16_t, uint8_t) -> void>;

    Read* reads[256] = {0};
    Write* writes[256] = {0};
    unsigned offsets[256] = {0};
	unsigned offsetsW[256] = {0};    
	
	// use 'size' for mirroring memory
    auto map( Read* read, Write* write, uint8_t pageLo, uint8_t pageHi, Mode mode = Mode::Linear, unsigned pageOffset = 0, unsigned size = 0 ) -> void {
        map(read, pageLo, pageHi, mode, pageOffset, size );
        map(write, pageLo, pageHi, mode, pageOffset, size );
    }
    
    auto map( Read* read, uint8_t pageLo, uint8_t pageHi, Mode mode = Mode::Linear, unsigned pageOffset = 0, unsigned size = 0 ) -> void {
        
        if (reads[ pageLo ] == read)
            return;
        
		unsigned offset = mode == Mode::Direct ? pageLo : 0;
        
        for ( unsigned page = pageLo; page <= pageHi; page++ ) {
            
            reads[ page ] = read;
            
            if ( size )
                offset %= size;
            
            offsets[ page ] = pageOffset + offset++;
        }
    }
    
    auto map( Write* write, uint8_t pageLo, uint8_t pageHi, Mode mode = Mode::Linear, unsigned pageOffset = 0, unsigned size = 0 ) -> void {
        
        if ( writes[ pageLo ] == write )
            return;
        
        unsigned offset = mode == Mode::Direct ? pageLo : 0;
        
        for ( unsigned page = pageLo; page <= pageHi; page++ ) {
            
            writes[ page ] = write;
            
            if ( size )
                offset %= size;
            
            offsetsW[ page ] = pageOffset + offset++;
        }
    }
	
	auto unmap( uint8_t pageLo, uint8_t pageHi ) -> void {
		unmapRead(pageLo, pageHi);
		unmapWrite(pageLo, pageHi);
	}
	
	auto unmapRead( uint8_t pageLo, uint8_t pageHi ) -> void {
		for ( unsigned page = pageLo; page <= pageHi; page++ ) {
			reads[ page ] = 0;
		}
	}
	
	auto unmapWrite( uint8_t pageLo, uint8_t pageHi ) -> void {
		for ( unsigned page = pageLo; page <= pageHi; page++ ) {
			writes[ page ] = 0;
		}
	}
    
    inline auto read( uint16_t addr ) -> uint8_t {

        return (*reads[ addr >> 8 ])( (offsets[ addr >> 8 ] << 8) | (addr & 0xff) );
    }

    inline auto write( uint16_t addr, uint8_t data ) -> void {    

        (*writes[ addr >> 8 ])( (offsetsW[ addr >> 8 ] << 8) | (addr & 0xff), data );
    }
    
    auto isLocation( uint8_t page, Read* read ) -> bool {
        
        return reads[page] == read;
    }
    
};

}
