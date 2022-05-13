
#pragma once

#include <functional>
#include <cstdint>

namespace LIBC64 {

struct Memory {    
    
    using Read = std::function<auto (uint16_t) -> uint8_t>;
    using Write = std::function<auto (uint16_t, uint8_t) -> void>;

    Read* reads[256] = {0};
    Write* writes[256] = {0};

    auto map( Read* read, Write* write, uint8_t pageLo, uint8_t pageHi ) -> void {
        map(read, pageLo, pageHi );
        map(write, pageLo, pageHi );
    }

    auto mapFast( Read* read, Write* write, uint8_t pageLo, uint8_t pageHi ) -> void {

        if (reads[ pageLo ] == read)
            return;

        for ( unsigned page = pageLo; page <= pageHi; page++ ) {
            reads[page] = read;
            writes[ page ] = write;
        }
    }

    auto map( Read* read, uint8_t pageLo, uint8_t pageHi ) -> void {
        
        if (reads[ pageLo ] == read)
            return;
        
        for ( unsigned page = pageLo; page <= pageHi; page++ )            
            reads[ page ] = read;        
    }
    
    auto map( Write* write, uint8_t pageLo, uint8_t pageHi ) -> void {
        
        if ( writes[ pageLo ] == write )
            return;
        
        for ( unsigned page = pageLo; page <= pageHi; page++ )            
            writes[ page ] = write;       
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
        
        return (*reads[ addr >> 8 ])( addr );
    }

    inline auto write( uint16_t addr, uint8_t data ) -> void {    
        
        (*writes[ addr >> 8 ])( addr, data );
    }
    
    auto isLocation( uint8_t page, Read* read ) -> bool {
        
        return reads[page] == read;
    }
    
};

}
