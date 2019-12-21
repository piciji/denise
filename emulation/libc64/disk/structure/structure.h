
#pragma once

#include <functional>
#include <string>
#include <cstring>
#include <vector>

#include "../../../tools/gcr.h"
#include "../../../interface.h"
#include "../../../tools/buffer.h"

namespace LIBC64 {

#define MAX_TRACKS_1541 42
    
struct Structure1541 {
    
    Structure1541();
    ~Structure1541();

    static const unsigned MAX_TRACKS;    // 42, that's the maximum some drives can access
    static const unsigned TYPICAL_TRACKS; // 35 tracks for standard cbm dos image
    static const unsigned TYPICAL_SIZE;  // for 35 tracks in cbm dos
    static const uint8_t SECTORS_IN_SPEEDZONE[4];
    static const unsigned BYTES_IN_SPEEDZONE[4];
    static const uint8_t GAPS_IN_SPEEDZONE[4];
    
    enum class Type { D64 = 0, G64 = 1, P64 = 2, Unknown = -1 } type; 
    
    std::function<unsigned (uint8_t*, unsigned, unsigned)> write = [](uint8_t* buffer, unsigned length, unsigned offset){ return 0; };    
    
    enum CBM_Error {
        ERR_OK = 1,
        ERR_HEADER = 2,
        ERR_SYNC = 3,
        ERR_NOBLOCK = 4,
        ERR_CHECKSUM = 5,
        ERR_VERIFY_FORMAT = 6,
        ERR_VERIFY = 7,
        ERR_WPROTECT = 8,
        ERR_HEADER_CHECKSUM = 9,
        ERR_WRITE = 0xa,
        ERR_SECTOR_ID = 0xb,
        ERR_DRIVE_NOT_READY = 0xf
    };
    
    struct GcrTrack {
        uint8_t* data = nullptr;
        unsigned size = 0; 
        unsigned bits = 0;
    };
    
    std::vector<Emulator::Interface::Listing> listings;
    std::vector<std::vector<uint8_t>> loader;
   
    auto prepare() -> void;
    auto analyze() -> bool;   
    static auto create( Type newType, std::string diskName ) -> uint8_t*;
    static auto imageSize( Type newType ) -> unsigned;
    
    auto getTrackPtr( uint8_t halfTrack ) -> GcrTrack*;
    auto writeTrack( const GcrTrack* trackPtr, uint8_t halfTrack ) -> void;
    auto attach( uint8_t* data, unsigned size ) -> bool;
    auto detach() -> void;
    auto createListing( ) -> void;
    auto getListing( ) -> std::vector<Emulator::Interface::Listing>&;
    auto selectListing( Emulator::Interface::Media* media, unsigned pos ) -> void;
    auto clearTrackData() -> void;
    auto getLogicalTrack(uint8_t _track, int offset) -> uint8_t;
    
private:    
    uint8_t* rawData;
    uint32_t rawSize;
    uint8_t tracks;
    uint8_t maxHalfTracks;
    unsigned maxTrackLength;
    
    GcrTrack gcrTrack[ MAX_TRACKS_1541 * 2 ];
        
    uint8_t* errorMap;
    uint32_t errorMapSize;
        
    auto analyzeD64() -> bool;
    auto analyzeG64() -> bool;
    
    static auto createD64( std::string diskName ) -> uint8_t*;
    static auto createG64( std::string diskName ) -> uint8_t*;
    static auto cutId( std::string& diskName ) -> std::string;
    
    static auto imageSizeG64() -> unsigned;
    static auto imageSizeD64() -> unsigned;
    
    static auto speedzone( uint8_t track ) -> uint8_t;
    static auto countSectors( uint8_t track ) -> uint8_t;
    static auto countSectors( uint8_t track, uint8_t sector ) -> int;
    static auto countBytes( uint8_t track ) -> unsigned;
    static auto gapSize( uint8_t track ) -> unsigned;
        
    auto prepareG64() -> void;
    auto prepareD64() -> void;
    auto getTrackOffsetG64( uint8_t halfTrack, int& error ) -> uint32_t;
        
    auto writeD64(const GcrTrack* trackPtr, unsigned track) -> bool;
    auto writeG64(const GcrTrack* trackPtr, unsigned halfTrack) -> bool;
    
    static auto writeSector( uint8_t* target, uint8_t* buffer, uint8_t track, uint8_t sector ) -> void;
    static auto readSector( uint8_t* src, uint8_t* buffer, uint8_t track, uint8_t sector ) -> bool;
    static auto createBAM( std::string diskName, uint8_t tracksInImage, uint8_t* buffer ) -> void;

    static auto encodeSector(const uint8_t* src, uint8_t* target, uint8_t track, uint8_t sector, uint8_t id1, uint8_t id2, int errorCode) -> void;    
    auto decodeSector( const GcrTrack* trackPtr, uint8_t* dest, uint8_t sector ) -> int;
    auto findSync( const GcrTrack* trackPtr, unsigned& offset, unsigned size ) -> bool;
    auto decode( const GcrTrack* trackPtr, unsigned offset, uint8_t* buffer, unsigned blockCount ) -> void;    
};

}
