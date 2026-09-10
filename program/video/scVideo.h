
#pragma once

#include <cstdint>

#include "manager.h"

class SCVideo {

public:
    SCVideo(VideoManager* manager);
    ~SCVideo();

    auto update() -> void;
    template<typename T> auto renderCrt(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch, unsigned cropTop, unsigned options ) -> void;

private:
    VideoManager* manager;

    uint8_t preCalcGamma[256 * 3];
    uint8_t preCalcScanline[512 * 3];

    int32_t preCalcLumaCenter[0xffff + 1];
    int32_t preCalcLumaNeighbour[0xffff + 1];

    ColorLumaChroma delayLine[ 1024 ];
    ColorRgb lineBefore[ 1024 ];

    ColorLumaChroma* evenTable = nullptr;
    ColorLumaChroma* oddTable = nullptr;
    uint32_t* tempDest = nullptr;

    struct Render {
        unsigned width;
        unsigned height;
        const uint8_t* src;
        unsigned srcPitch;
        unsigned* dest;
        unsigned destPitch;
        unsigned* scanlineDest;
        unsigned* fieldDest;
        uint8_t oddLine;
        unsigned options = 0;
    } render;

    auto calculateGamma() -> void;
    auto calculateLumaDelay() -> void;
    auto injectPhaseTransferError() -> void;
    auto convertLumaChromaToInteger() -> void;

    template<uint8_t options, typename T> auto renderPalCrt() -> void;
    template<uint8_t options, typename T> auto renderNtscCrt() -> void;

};