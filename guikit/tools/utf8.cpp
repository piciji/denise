
auto Utf8::encode( unsigned code, std::vector<uint8_t>& out ) -> unsigned {
    
    unsigned len = 0;
    
    if (code >= 0x00 && code <= 0x7f) {
        out.push_back( code );
        len = 1;
        
    } else if (code >= 0x80 && code <= 0x7ff) {
        out.push_back( 0xc0 | (uint8_t) (code >> 6) );
        out.push_back( 0x80 | (uint8_t) (code & 0x3f) );
        len = 2;
        
    } else if (code >= 0x800 && code <= 0xffff) {
        out.push_back( 0xe0 | (uint8_t) (code >> 12) );
        out.push_back( 0x80 | (uint8_t) ((code >> 6) & 0x3f) );
        out.push_back( 0x80 | (uint8_t) (code & 0x3f) );
        len = 3;
        
    } else if (code >= 0x10000 && code <= 0x10ffff) {
        out.push_back( 0xf0 | (uint8_t) (code >> 18) );
        out.push_back( 0x80 | (uint8_t) ((code >> 12) & 0x3f) );
        out.push_back( 0x80 | (uint8_t) ((code >> 6) & 0x3f) );
        out.push_back( 0x80 | (uint8_t) (code & 0x3f) );
        len = 4;
    }

    return len;
}

auto Utf8::decode( std::string text, unsigned& pos ) -> unsigned {

    unsigned size = 0;
    unsigned code = 0;

    while( pos < text.size() ) {            

        char c = text[pos++];

        if (!(c & 0x80)) { //ansi (single byte)
            return c;
        }

        if ((c & 0xc0) == 0xc0) { //init multi byte

            while(c & 0x80) {
                size++;
                c <<= 1;
            }

            if (size > 4) { //error max 4 byte
                return 0;
            }

            code = c >> size;

            continue;
        }

        if (size == 0)
            return 0; //decode error

        if ((c & 0xc0) == 0x80) { //multi byte follow up
            c <<= 2;

            for(unsigned i = 0; i < 6; i++) {
                code <<= 1;
                code |= (c >> 7) & 1;
                c <<= 1;
            }

            if (--size == 1) {
                break; //sequence complete
            }
        }            
    }

    return code;
}