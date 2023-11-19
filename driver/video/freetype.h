
#pragma once

#include <cstring>
#include <string>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "../tools/font.h"

namespace DRIVER {

#include "../tools/fonts.c"
#include "../tools/utf8.h"

struct Freetype {
    ~Freetype() {
        term();
    }

    FT_Library ft = nullptr;
    FT_Face face = nullptr;
    FT_GlyphSlot glyph;
    FT_Byte* data = nullptr;
    bool initialized = false;

    unsigned char* textBuffer = nullptr;
    unsigned totalWidth = 0;
    unsigned totalHeight = 0;

    auto hasText() -> bool {
        return textBuffer != nullptr;
    }

    auto init() -> bool {
        term();

        if (FT_Init_FreeType( &ft ))
            return false;

        if (FT_New_Face(ft, getFontFile().c_str(), 0, &face)) {
            data = new FT_Byte[ sizeof (sourceCodePro) ];
            std::memcpy(data, &sourceCodePro, sizeof (sourceCodePro));

            if (FT_New_Memory_Face(ft, data, sizeof(sourceCodePro), 0, &face))
                return false;
        }

        FT_Set_Pixel_Sizes(face, 0, 1);

        glyph = face->glyph;

        initialized = true;
        return true;
    }

    auto term() -> void {
        if(face) {
            FT_Done_Face(face);
            face = nullptr;
        }

        if(ft) {
            FT_Done_FreeType(ft);
            ft = nullptr;
        }

        if (textBuffer) {
            delete[] textBuffer;
            textBuffer = nullptr;
        }

        if (data) {
            delete[] data;
            data = nullptr;
        }
        initialized = false;
    }

    auto setFontSize(int value) -> void {
        if (face)
            FT_Set_Pixel_Sizes(face, 0, value);
    }

    auto buildTexture(std::string& text) -> bool {
        if (textBuffer) {
            delete[] textBuffer;
            textBuffer = nullptr;
        }

        if (!initialized || text.empty())
            return false;

        unsigned index = 0;
        unsigned code;
        totalWidth = 0;
        totalHeight = 0;
        unsigned upHeight = 0;
        unsigned downHeight = 0;

        while (code = utf8decode(text, index)) {

            if (FT_Load_Char(face, code, FT_LOAD_RENDER)) {
                continue;
            }

            totalWidth += (glyph->advance.x >> 6 );

            upHeight = std::max<unsigned>(upHeight, (unsigned)(glyph->bitmap.rows ));

            if (glyph->bitmap_top < 0)
                continue;

            if (glyph->bitmap.rows > glyph->bitmap_top)
                downHeight = std::max<unsigned>(downHeight, glyph->bitmap.rows - glyph->bitmap_top);
        }

        upHeight += 1;
        downHeight += 1;
        totalHeight = upHeight + downHeight;

        unsigned _size = totalWidth * totalHeight;

        if (_size == 0)
            return false;

        textBuffer = new unsigned char[ _size ];
        std::memset(textBuffer, 0, _size );

        index = 0;
        unsigned writePos = 0;
        unsigned curPos;

        while (code = utf8decode(text, index)) {
            if (FT_Load_Char(face, code, FT_LOAD_RENDER)) {
                continue;
            }

            for(unsigned i = 0; i < glyph->bitmap.rows; i++) {

                for(unsigned j = 0; j < glyph->bitmap.width; j++) {

                    if (glyph->bitmap_top < 0) {
                        curPos = (totalHeight + glyph->bitmap_top + i) * totalWidth + writePos + j + glyph->bitmap_left;

                    } else {
                        curPos = (upHeight - glyph->bitmap_top + i) * totalWidth + writePos + j + glyph->bitmap_left;
                    }

                    if (curPos >= _size)
                        break;

                    *( textBuffer + curPos ) = glyph->bitmap.buffer[i * glyph->bitmap.width + j ];
                }
            }
            writePos += (glyph->advance.x >> 6);
        }

        return true;
    }

};

}
