
#pragma once
#include <vector>

namespace Emulator {

struct PetciiConversion {
    
    const uint8_t PETSCII_UNMAPPED = 0x3f;
    const uint8_t ASCII_UNMAPPED = '.';

    auto encodeWithLineEnding( std::string ascii, std::vector<uint8_t>& in ) -> void {

        bool checkTwoCharLineEnding = false;

        for ( auto& c : ascii ) {
            if (c == '\r') {
                checkTwoCharLineEnding = true;
                continue;
            }

            if (checkTwoCharLineEnding) {
                if (c != '\n')
                    in.push_back( 0x0d );

                checkTwoCharLineEnding = false;
            }

            in.push_back( encode(c) );
        }
    }

    auto encode( std::string ascii, std::vector<uint8_t>& petcii ) -> void {

        for ( auto& c : ascii )
            petcii.push_back( encode( c ) );
    }

    auto encode( std::string ascii ) -> std::string {
        
        std::string petcii = "";
        
        for ( auto& c : ascii )
            petcii += encode( c );       
        
        return petcii;
    }    
    
    auto encode( uint8_t ascii ) -> uint8_t {

        if (ascii == '\n')
            return 0x0d;
        
        if (ascii == '\r')
            return 0x0a;
                
        if (ascii <= 0x1f)
            return PETSCII_UNMAPPED;
        
        if (ascii == '`')
            return 0x27; // '
                
        if ((ascii >= 'a') && (ascii <= 'z'))
            return (uint8_t) ((ascii - 'a') + 0x41);
        
        if ((ascii >= 'A') && (ascii <= 'Z'))
            return (uint8_t) ((ascii - 'A') + 0xc1);
        
        if (ascii >= 0x7b)
            return PETSCII_UNMAPPED;

        return _fix( ascii );
    }
    
    auto decode( uint8_t petcii ) -> uint8_t {

        petcii = _fix( petcii );

        if (petcii == 0x0d)
            return '\n';
        
        if (petcii == 0x0a)
            return '\r';

        if (petcii == 0)
            return 0x20;
        
        if (petcii <= 0x1f)
            return ASCII_UNMAPPED;
        
        if (petcii == 0xa0)
            return ' ';
        
        if ((petcii >= 0xc1) && (petcii <= 0xda))
            return (uint8_t)((petcii - 0xc1) + 'A');
            
        if ((petcii >= 0x41) && (petcii <= 0x5a))
            return (uint8_t)((petcii - 0x41) + 'a');

        return isprint( petcii ) ? petcii : ASCII_UNMAPPED;
    }
    
    auto decodeToScreencodeHi( uint8_t petcii ) -> uint8_t {    
        
        return decodeToScreencode( petcii ) + 0x80;
    }
    
    auto decodeToScreencode( uint8_t petcii ) -> uint8_t {

        if (petcii <= 0x1f) {
            return petcii | 0x80;
        } else if (petcii >= 0x40 && petcii <= 0x5f) {
			return (uint8_t) (petcii - 0x40);
		} else if (petcii >= 0x60 && petcii <= 0x7f) {
			return (uint8_t) (petcii - 0x20);
		} else if (petcii >= 0x80 && petcii <= 0x9f) {
			return (uint8_t) (petcii + 0x40);
		} else if (petcii >= 0xa0 && petcii <= 0xbf) {
			return (uint8_t) (petcii - 0x40);
		} else if (petcii >= 0xc0 && petcii <= 0xfe) {
			return (uint8_t) (petcii - 0x80);
		} else if (petcii == 0xff) {
			return 0x5e;
		} else if (petcii == 0) {
			return 0x20;
		}
		return petcii;
	}

    auto encodeScreencode(uint8_t code) -> uint8_t {
        code &= 0x7f;

        if (code <= 0x1f)
            return (uint8_t)(code + 0x40);

        if (code >= 0x40 && code <= 0x5f)
            return (uint8_t)(code + 0x20);

        return code;
    }
    
    auto _fix( uint8_t c ) -> uint8_t {
        
        if ((c >= 0x60) && (c <= 0x7f))
            return ((c - 0x60) + 0xc0);
        
        if (c >= 0xe0)
            return ((c - 0xe0) + 0xa0);
        
        return c;
    }
};

}
