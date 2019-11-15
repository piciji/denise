
#pragma once

static auto utf8decode( std::string text, unsigned& pos ) -> unsigned {

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
