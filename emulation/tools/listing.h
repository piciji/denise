
#pragma once

#include <string>
#include <vector>
#include "petcii.h"

namespace Emulator {   
    
// converts directory listing from petscii to ascii for view in host
struct C64Listing {    
    
    // filename in petscii for injection
    // updates with each call of 'buildListing'
    std::vector<uint8_t> loader;
    
    // default conversion is ascii
    // if host uses a custom c64 font, activate screencode conversion
    bool convertToScreencode;

    PetciiConversion conv;
    
    auto buildHeadline( uint8_t* namePtr, uint8_t* dosTypePtr = nullptr, uint8_t* extPtr = nullptr ) -> std::vector<uint16_t> {
                     
        std::vector<uint16_t> out;
        
        out.push_back( decodeToScreencodeHi( 0x22 ) );
		
		for (unsigned i = 0; i < 16; i++)			
			out.push_back( decodeToScreencodeHi( *(namePtr + i) ) );
		
		out.push_back( decodeToScreencodeHi( 0x22) );
        out.push_back( decodeToScreencodeHi( 0x20) );		
        
		for (unsigned i = 0; i < 3; i++)
            out.push_back( decodeToScreencodeHi( extPtr ? extPtr[i] : 0x20) );        		
        
        for (unsigned i = 0; i < 2; i++)
            out.push_back( decodeToScreencodeHi( dosTypePtr ? dosTypePtr[i] : 0x20) );
		
        prependBlockSize( out, 0, 2 );
        
		return out;
    }
    
    auto buildListing( uint8_t* namePtr, unsigned size, uint8_t type ) -> std::vector<uint16_t> {
        loader.clear();
		std::vector<uint16_t> out;
		bool a0Found = false;
		
        if (namePtr) {           
            out.push_back( decodeToScreencode( 0x22 ) );
            
            for (unsigned i = 0; i < 16; i++) {

                uint8_t code = *(namePtr + i);

                if (code == 0xa0 && ((type & 0x20) == 0) ) {
                    if ( !a0Found )
                        out.push_back( decodeToScreencode( 0x22 ) );
                    else
                        out.push_back( decodeToScreencode( 0x20 ) );

                    a0Found = true;

                } else {
                    out.push_back( decodeToScreencode( code ) );

                    if (!a0Found)
                        loader.push_back( code );
                }
            }

            if (!a0Found)
                out.push_back(decodeToScreencode(0x22));
            else
                out.push_back(decodeToScreencode(0x20));
        }
        
        appendType( out, type );
        
        prependBlockSize( out, size, 5 );
		
		return out;
	}
    
    auto buildFreeLine( unsigned size ) -> std::vector<uint16_t> {
        
		std::vector<uint16_t> out;
        
        std::string str = "BLOCKS FREE.";
        
        for (unsigned i = 0; i < str.size(); i++)            
            out.push_back( decodeToScreencode( str[i] ) ); 
        
        prependBlockSize( out, size, std::to_string( size ).size() + 1 );
        
        return out;
    }
    
    auto prependBlockSize( std::vector<uint16_t>& target, unsigned size, unsigned padding) -> void {
		
		std::string str = std::to_string( size );		

        if (str.size() < padding) {
            for (int i = 0; i < (padding - str.size()); i++)
                target.insert(target.begin(), 0x20);
        }
		
		for(int i = str.size() - 1; i >= 0; i--)
			target.insert( target.begin(), *(str.c_str() + i) );		
	}
    
    auto appendType(std::vector<uint16_t>& target, uint8_t type) -> void {

        if (type & 0x20) { // TAPE
            target.push_back( 0x20 );
            if (type & 0x10) // TURBO TAPE
                target.push_back( decodeToScreencode( 'T' ) );
            else
                target.push_back( 0x20 );
        } else if (type & 0x80)
            target.push_back( 0x20 );
        else
            target.push_back( decodeToScreencode( 42 ) );
        
        std::string str = "";
        
        switch( type & 0x7 ) {
            case 0: str = "DEL"; break;
            case 1: str = "SEQ"; break;
            case 2: str = "PRG"; break;
            case 3: str = "USR"; break;
            case 4: str = "REL"; break;
            case 5: str = "CBM"; break;
            default: str = "DEL"; break;
        }
        
        for (unsigned i = 0; i < str.size(); i++)            
            target.push_back( decodeToScreencode( str[i] ) );

        if (type & 0x20) { // TAPE

        } else if ( type & 0x40 )
            target.push_back( decodeToScreencode( 60 ) );     
    }
    
    auto decodeToScreencode( uint8_t petscii ) -> uint8_t {
        
        return convertToScreencode ? conv.decodeToScreencode( petscii )
                : conv.decode( petscii );
    }
    
    auto decodeToScreencodeHi( uint8_t petscii ) -> uint8_t {
        
        return convertToScreencode ? conv.decodeToScreencodeHi( petscii )
                : conv.decode( petscii );
    }
	
	auto decodeToScreencode( std::vector<uint8_t> line ) -> std::vector<uint16_t> {
		
		std::vector<uint16_t> out;
		
		for ( auto& code : line ) {
			
			out.push_back( decodeToScreencode(code) );
		}
		
		return out;
	}
};

}