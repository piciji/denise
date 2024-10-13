
#pragma once

#include <cstring>
#include <string>
#include <ft2build.h>
#include FT_FREETYPE_H

#define MES_ANIMATION_DURATION 400 // ms
#define MES_ANIMATION_MOVEMENT 0.05f
#define MES_ANIMATION_FADE_FKT(x) powf(0.7f, 0.014f * (x)) // exponential function curve

namespace DRIVER {

#include "../tools/utf8.h"

struct Freetype {
    FT_Library ft = nullptr;
    FT_Face ftFace = nullptr;
    FT_GlyphSlot ftGlyph;
    bool ftLoaded = false;

    unsigned char* ftTextBuffer = nullptr;
    unsigned ftTotalWidth = 0;
    unsigned ftTotalHeight = 0;

    unsigned ftPaddingHorizontal = 10;
    int ftPaddingVertical = 8;
    float ftMarginHorizontal = 0.02f;
    float ftMarginVertical = -1.0;

    std::string ftText;
    bool ftWarn;
    unsigned ftDuration;
    int ftAlignment;
    int ftUpdated;
    unsigned ftTs;
    unsigned ftTsStart;

    struct FtColNorm {
        float r;
        float g;
        float b;
        float a;
    } ftColNorm, ftColBgNorm;

    ScreenTextDescription ftDesc;

    std::mutex ftUpdateMutex;

    float ftPosCoords[4][4] = {
        {0, 0, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},
        {0, 0, 1, 1}
    };

    bool ftInitialized = false;

    virtual ~Freetype() {
        ftUnload();
    }

    Freetype() {
        ftText = "";
        ftUpdated = 0;
        ftWarn = false;
        ftDuration = 0;
        ftAlignment = 0;
        ftTs = 0;
        ftTsStart = 0;
    }

    virtual auto ftSetColor(FtColNorm& _colNorm, FtColNorm& _colBgNorm) -> void = 0;
    virtual auto ftBuildTexture(std::string text, bool keepOldSize = false) -> void = 0;
    virtual auto ftSetCoordsPosition() -> void {}

    auto ftLoadFont(const std::string& fontPath, unsigned fontIndex) -> bool {
        ftUnload();

        if (FT_Init_FreeType( &ft ))
            return false;

        if (FT_New_Face(ft, fontPath.c_str(), (FT_Long)fontIndex, &ftFace)) {
            if (fontIndex > 0) {
                if (FT_New_Face(ft, fontPath.c_str(), 0, &ftFace))
                    return false;
            } else
                return false;
        }

        FT_Set_Pixel_Sizes(ftFace, 0, 1);

        ftGlyph = ftFace->glyph;

        ftLoaded = true;
        return true;
    }

    auto ftUnload() -> void {
        if(ftFace) {
            FT_Done_Face(ftFace);
            ftFace = nullptr;
        }

        if(ft) {
            FT_Done_FreeType(ft);
            ft = nullptr;
        }

        ftDeleteTexture();

        ftLoaded = false;
    }

    auto ftDeleteTexture() -> void {
        if (ftTextBuffer) {
            delete[] ftTextBuffer;
            ftTextBuffer = nullptr;
        }
    }

    auto ftSetFontSize(int value) -> void {
        if (ftFace)
            FT_Set_Pixel_Sizes(ftFace, 0, value);
    }

    auto ftProcessUpdates(Viewport& viewport) -> void {
        ftUpdateMutex.lock();

        if (ftUpdated & 8) {
            ftUpdated = 0;
            ScreenTextDescription _desc = ftDesc;
            std::string _text = ftText;
            bool _warn = ftWarn;
            unsigned _duration = ftDuration;
            ftUpdateMutex.unlock();

            if (!_desc.fontPath.empty())
                ftLoadFont(_desc.fontPath, _desc.fontIndex);

            ftPaddingHorizontal = _desc.paddingHorizontal;
            ftPaddingVertical = _desc.paddingVertical;
            ftMarginHorizontal = (float)_desc.marginHorizontal / 500.0f;
            ftMarginVertical = (float)_desc.marginVertical / 500.0f;

            ftSelectColor(_desc, _warn);
            ftSetFontSize(_desc.fontSize);
            ftSetPosition(_desc.position);
            ftSetDuration(_duration * 1000);
            ftBuildTexture(_text, _duration == 0);
            ftCalcCoords(viewport);
            ftSetColor(ftColNorm, ftColBgNorm);
        } else if (ftUpdated & 4) {
            if (ftUpdated & 16)
                ftSelectColor(ftDesc, ftWarn);
            ftUpdated = 0;
            std::string _text = ftText;
            unsigned _duration = ftDuration;
            ftUpdateMutex.unlock();
            ftSetDuration(_duration * 1000);
            ftBuildTexture(_text, _duration == 0);
            ftCalcCoords(viewport);
            ftSetColor(ftColNorm, ftColBgNorm);
        } else if (ftUpdated & 16) {
            ftUpdated = 0;
            ftSelectColor(ftDesc, ftWarn);
            ftUpdateMutex.unlock();
            ftSetColor(ftColNorm, ftColBgNorm);
        } else if (ftUpdated & 2) {
            ftUpdated = 0;
            unsigned _duration = ftDuration;
            ftUpdateMutex.unlock();
            ftSetDuration(_duration * 1000);
        } else if (ftUpdated & 1) {
            ftUpdated = 0;
            ftUpdateMutex.unlock();
            ftCalcCoords(viewport);
        } else
            ftUpdateMutex.unlock();
    }

    auto ftSelectColor(ScreenTextDescription& _desc, bool _warn) -> void {
        if (_warn) {
            ftTransformCol(_desc.warnColor, ftColNorm);
            ftTransformCol(_desc.warnBackgroundColor, ftColBgNorm);
        } else {
            ftTransformCol(_desc.fontColor, ftColNorm);
            ftTransformCol(_desc.backgroundColor, ftColBgNorm);
        }
    }

    static auto ftTransformCol(unsigned& source, FtColNorm& dest) -> void {
        dest.r = ftNormComponent(source, 16);
        dest.g = ftNormComponent(source, 8);
        dest.b = ftNormComponent(source, 0);
        dest.a = ftNormComponent(source, 24);
    }

    static auto ftNormComponent(const unsigned& value, unsigned shift) -> float {
        uint8_t col = (value >> shift) & 0xff;
        return float(col) / 255.0;
    }

    auto ftSetPosition(ScreenTextDescription::Position& pos) -> void {
        ftAlignment = 0;
        if (pos == ScreenTextDescription::POSITION_BOTTOM_CENTER || pos == ScreenTextDescription::POSITION_TOP_CENTER)
            ftAlignment |= 1;
        else if (pos == ScreenTextDescription::POSITION_BOTTOM_RIGHT || pos == ScreenTextDescription::POSITION_TOP_RIGHT)
            ftAlignment |= 2;
        if (pos == ScreenTextDescription::POSITION_BOTTOM_RIGHT || pos == ScreenTextDescription::POSITION_BOTTOM_CENTER || pos == ScreenTextDescription::POSITION_BOTTOM_LEFT)
            ftAlignment |= 4;
    }

    auto ftSetDuration(unsigned _duration) -> void {
        if (_duration) {
            if (!ftTs) {
                ftTsStart = Chronos::getTimestampInMilliseconds();
                ftTs = ftTsStart + _duration + MES_ANIMATION_DURATION;
            } else {
                auto _temp = Chronos::getTimestampInMilliseconds();
                if (ftTs > _temp) {
                    _temp = ftTs - _temp;
                    if (_duration > _temp)
                        ftTs += _duration - _temp;
                } else
                    ftTs = _temp + _duration;
            }
        } else
            ftTs = 0;
    }

    virtual auto ftCalcCoords(Viewport& viewport, float fade = 0.0) -> void {
        float screenx = 2.0f / (float)viewport.width, screeny = 2.0f / (float)viewport.height;
        float textx = (float)ftTotalWidth * screenx;
        float texty = (float)ftTotalHeight * screeny;

        float x = -1.0f;
        float y = 1.0f;

        if (ftAlignment & 1) { // center
            x = 0.0f - textx / 2.0f - 0.00001f;
        } else if(ftAlignment & 2) { // right
            x = 1.0f - textx - ftMarginHorizontal;
        } else
            x += ftMarginHorizontal;

        float _vert;
        if (ftMarginVertical < 0.0f)
            _vert = (float)viewport.width * (ftMarginHorizontal / (float)viewport.height);
        else
            _vert = ftMarginVertical;

        if (ftAlignment & 4) { // bottom
            y = -1.0f + texty + _vert - fade;
        } else
            y -= _vert - fade;

        ftPosCoords[0][0] = x;
        ftPosCoords[0][1] = y;
        ftPosCoords[1][0] = x + textx;
        ftPosCoords[1][1] = y;
        ftPosCoords[2][0] = x;
        ftPosCoords[2][1] = y - texty;
        ftPosCoords[3][0] = x + textx;
        ftPosCoords[3][1] = y - texty;

        ftSetCoordsPosition();
    }

    auto ftHandleAnimation(Viewport& viewport) -> bool {
        unsigned curTs = Chronos::getTimestampInMilliseconds();
        if (curTs >= ftTs) {
            ftTs = 0;
            ftSetColor(ftColNorm, ftColBgNorm);
            ftUpdateMutex.lock();
            ftText = "";
            ftUpdated |= 4;
            ftDuration = 0;
            ftUpdateMutex.unlock();
            return true;
        }

        if ((ftTs - curTs) < MES_ANIMATION_DURATION) {
            int left = ftTs - curTs;
            float yFadeOutAdjust = -(MES_ANIMATION_MOVEMENT / (float)MES_ANIMATION_DURATION) * (float)(left - MES_ANIMATION_DURATION);
            ftCalcCoords(viewport, yFadeOutAdjust);
            auto _colA = ftColNorm.a;
            auto _colBgA = ftColBgNorm.a;
            float _f = MES_ANIMATION_FADE_FKT((float)(MES_ANIMATION_DURATION - left));
            ftColNorm.a = ftColNorm.a * _f;
            ftColBgNorm.a = ftColBgNorm.a * _f;

            ftSetColor(ftColNorm, ftColBgNorm);
            ftColNorm.a = _colA;
            ftColBgNorm.a = _colBgA;
        } else if ((curTs - ftTsStart) < MES_ANIMATION_DURATION) {
            int left = curTs - ftTsStart;
            float yFadeOutAdjust = -(MES_ANIMATION_MOVEMENT / (float)MES_ANIMATION_DURATION) * (float)(left - MES_ANIMATION_DURATION);
            ftCalcCoords(viewport, yFadeOutAdjust);
            auto _colA = ftColNorm.a;
            auto _colBgA = ftColBgNorm.a;
            float _f = MES_ANIMATION_FADE_FKT((float)left);
            ftColNorm.a = ftColNorm.a * -_f + ftColNorm.a;
            ftColBgNorm.a = ftColBgNorm.a * -_f + ftColBgNorm.a;

            ftSetColor(ftColNorm, ftColBgNorm);
            ftColNorm.a = _colA;
            ftColBgNorm.a = _colBgA;
        } else if (ftTsStart) {
            ftCalcCoords(viewport);
            ftSetColor(ftColNorm, ftColBgNorm);
            ftTsStart = 0;
        }
        return false;
    }

    auto ftBuildText(std::string text, bool& keepOldSize) -> bool {
        if (!ftLoaded || text.empty()) {
            ftTotalWidth = ftTotalHeight = 0;
            ftDeleteTexture();
            return false;
        }

        unsigned index = 0;
        unsigned code;
        auto _totalWidth = ftTotalWidth;
        auto _totalHeight = ftTotalHeight;
        ftTotalWidth = 0;
        ftTotalHeight = 0;
        unsigned upHeight = 0;
        unsigned downHeight = 0;

        while ((code = utf8decode(text, index)) != 0) {

            if (FT_Load_Char(ftFace, code, FT_LOAD_RENDER | FT_LOAD_NO_BITMAP)) {
                continue;
            }

            ftTotalWidth += (ftGlyph->advance.x >> 6 );

            upHeight = std::max<unsigned>(upHeight, (unsigned)(ftGlyph->bitmap.rows ));

            if (ftGlyph->bitmap_top < 0)
                continue;

            if (ftGlyph->bitmap.rows > ftGlyph->bitmap_top)
                downHeight = std::max<unsigned>(downHeight, ftGlyph->bitmap.rows - ftGlyph->bitmap_top);
        }

        ftTotalHeight = upHeight + downHeight;
        ftTotalWidth += ftPaddingHorizontal << 1;
        unsigned _totalHeightPadded;

        if (ftPaddingVertical < 0) {
            ftTotalHeight += ftPaddingHorizontal << 1;
            upHeight += ftPaddingHorizontal;
            _totalHeightPadded = ftTotalHeight - ftPaddingHorizontal;
        } else {
            ftTotalHeight += ftPaddingVertical << 1;
            upHeight += ftPaddingVertical;
            _totalHeightPadded = ftTotalHeight - ftPaddingVertical;
        }

        unsigned _size = ftTotalWidth * ftTotalHeight;

        if (_size == 0) {
            ftTotalWidth = ftTotalHeight = 0;
            ftDeleteTexture();
            return false;
        }

        if (keepOldSize) {
            if (!ftTextBuffer || (_totalWidth < ftTotalWidth) || (_totalHeight < ftTotalHeight)) {
                ftDeleteTexture();
                ftTextBuffer = new unsigned char[ _size ];
                keepOldSize = false;
            } else {
                ftTotalWidth = _totalWidth;
                ftTotalHeight = _totalHeight;
                _size = _totalWidth * _totalHeight;
            }
        } else {
            ftDeleteTexture();
            ftTextBuffer = new unsigned char[ _size ];
        }

        std::memset(ftTextBuffer, 0, _size );

        index = 0;
        unsigned writePos = ftPaddingHorizontal;
        unsigned curPos;

        while ((code = utf8decode(text, index)) != 0) {
            if (FT_Load_Char(ftFace, code, FT_LOAD_RENDER | FT_LOAD_NO_BITMAP)) {
                continue;
            }

            for(unsigned i = 0; i < ftGlyph->bitmap.rows; i++) {

                for(unsigned j = 0; j < ftGlyph->bitmap.width; j++) {

                    if (ftGlyph->bitmap_top < 0) {
                        curPos = (_totalHeightPadded + ftGlyph->bitmap_top + i) * ftTotalWidth + writePos + j + ftGlyph->bitmap_left;

                    } else {
                        curPos = (upHeight - ftGlyph->bitmap_top + i) * ftTotalWidth + writePos + j + ftGlyph->bitmap_left;
                    }

                    if (curPos >= _size)
                        break;

                    *( ftTextBuffer + curPos ) = ftGlyph->bitmap.buffer[i * ftGlyph->bitmap.width + j ];
                }
            }
            writePos += (ftGlyph->advance.x >> 6);
        }

        return true;
    }

    auto ftSetScreenTextDescription(ScreenTextDescription& _desc) -> void {
        if (!ftInitialized)
            return;

        ftUpdateMutex.lock();
        ftDesc = _desc;
        ftUpdated |= 8;
        ftUpdateMutex.unlock();
    }

    auto ftUpdateMessage( const std::string& _text, unsigned _duration, bool _warn) -> void {
        if (!ftInitialized)
            return;

        ftUpdateMutex.lock();
        if (!_duration && ftDuration) {
            ftUpdateMutex.unlock();
            return;
        }

        if (ftText != _text) {
            ftText = _text;
            ftUpdated |= 4;
        }

        if (ftWarn != _warn) {
            ftWarn = _warn;
            ftUpdated |= 16;
        }

        if (_duration) {
            ftDuration = _duration;
            ftUpdated |= 2;
        }

        ftUpdateMutex.unlock();
    }

    auto ftUpdateCoords() -> void {
        if (!ftInitialized)
            return;

        ftUpdateMutex.lock();
        ftUpdated |= 1;
        ftUpdateMutex.unlock();
    }
};

}
