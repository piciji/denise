
#pragma once

#include <functional>
#include <string>
#include <cstring>
#include <vector>

#include "../../../tools/gcr.h"
#include "../../../interface.h"
#include "../../../tools/buffer.h"
#include "../../../tools/serializer.h"
#include "../../../tools/fpaq0.h"

namespace LIBC64 {

#define MAX_TRACKS_1541 42

struct VirtualDrive;
struct Drive;

struct DiskStructure {
    
    DiskStructure(Drive* drive = nullptr);
    ~DiskStructure();

    static const unsigned MAX_TRACKS;    // 42, that's the maximum some drives can access
    static const unsigned TYPICAL_TRACKS; // 35 tracks for standard cbm dos image
    static const unsigned TYPICAL_SIZE;  // for 35 tracks in cbm dos
    static const uint8_t SECTORS_IN_SPEEDZONE[4];
    static const unsigned BYTES_IN_SPEEDZONE[4];
    static const uint8_t GAPS_IN_SPEEDZONE[4];
    
    enum class Type { D64, G64, P64, D71, G71, P71, Unknown = -1 } type;
	uint8_t number;
	Emulator::Interface::Media* media = nullptr;
	bool autoStarted = false;
    unsigned serializationSize = 0;
    
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
    
    struct Pulse {
        uint32_t position;
        uint32_t strength;
        int32_t previous;
        int32_t next;
    };

    struct GcrTrack {
        uint8_t* data = nullptr;
        unsigned size = 0;
        unsigned bits = 1;
        uint8_t written = 0;

        // not needed for flux based MFM
        uint8_t* mfmSync = nullptr;

        int32_t firstPulse = -1;
        int32_t lastPulse = -1;
        int32_t currentPulse = -1;
        std::vector<Pulse> pulses;
    };

    struct {
        uint8_t* ptr = nullptr;
        unsigned offset = 0;
        bool inUse[MAX_TRACKS_1541 * 2 * 2] = { 0 };
        uint8_t status = 0;

        auto reset() -> void {
            ptr = nullptr;
            offset = 0;
            status = 0;
        }
    } encodingGraceful;

    std::vector<Emulator::Interface::Listing> listings;
    std::vector<std::vector<uint8_t>> loader;
    VirtualDrive* virtualDrive = nullptr;
    Drive* drive = nullptr;

    auto hasSecondSide() -> bool {
        return sides == 2;
    }

    auto prepare() -> void;
    auto analyze() -> bool;   
    static auto create( Type newType, std::string diskName ) -> Emulator::Interface::Data;
    
    auto getTrackPtr( uint8_t side, uint8_t halfTrack ) -> GcrTrack*;
    auto attach( uint8_t* data, unsigned size, bool loadGracefully = false ) -> bool;
    auto detach() -> void;
    auto createListing() -> void;
    auto getListing() -> std::vector<Emulator::Interface::Listing>&;
    auto selectListing( unsigned pos, bool useTraps = false ) -> void;
    auto selectListing( std::string fileName, bool useTraps = false ) -> void;
    auto prepareKeyBufferActions( std::vector<uint8_t>& path, bool useTraps = false ) -> void;
	auto buildLoadCommand( std::vector<uint8_t> loadPath, bool forShow = false ) -> std::vector<uint8_t>;
    auto clearTrackData() -> void;
    auto getLogicalTrack(uint8_t _track, int offset) -> uint8_t;
    auto storeWrittenTracks() -> void;
    auto getStateImageSize() -> unsigned;
    auto serialize(Emulator::Serializer& s, bool written) -> void; 
	
	static auto createD64FromPRG( std::string name, uint8_t* prgData, unsigned prgSize ) -> uint8_t*;
	static auto getBamTrackEntry( uint8_t* bamPtr, uint8_t track ) -> uint8_t*;
	static auto clrBam(uint8_t* bamTrackPtr, unsigned sector) -> void;
	static auto setBam(uint8_t* bamTrackPtr, unsigned sector) -> void;
	static auto issetBam(uint8_t* bamTrackPtr, unsigned sector) -> bool;
	static auto freeSector(uint8_t* bamPtr, uint8_t track, uint8_t sector) -> bool;
	static auto allocateSector(uint8_t* bamPtr, uint8_t track, uint8_t sector) -> bool;
	static auto allocateFreeSector(uint8_t* bamPtr, uint8_t& track, uint8_t& sector) -> bool;
	static auto allocateNextFreeSector(uint8_t* bamPtr, uint8_t& track, uint8_t& sector) -> bool;
	static auto allocateDown(uint8_t* bamPtr, uint8_t& track, uint8_t& sector) -> bool;
	static auto allocateUp(uint8_t* bamPtr, uint8_t& track, uint8_t& sector) -> bool;

    static auto speedzone( uint8_t track ) -> uint8_t;
    static auto countSectors( uint8_t track ) -> uint8_t;
    static auto countSectors( uint8_t track, uint8_t sector ) -> int;
    static auto countBytes( uint8_t track ) -> unsigned;
    static auto gapSize( uint8_t track ) -> unsigned;

    static auto addPulse( GcrTrack* gcrTrack, uint32_t position, uint32_t strength ) -> void;
    static auto freePulse( GcrTrack* gcrTrack, int32_t index ) -> void;

    auto updateSerializationSize() -> void;
    auto prepareP64Graceful() -> void;

    auto readSector( uint8_t* buffer, uint8_t track, uint8_t sector ) -> bool;
    
private:    
    uint8_t* rawData;
    uint32_t rawSize;
	uint8_t* created = nullptr;
    uint8_t sides;
    GcrTrack gcrTracks[2][ MAX_TRACKS_1541 * 2 ];

    // G64 / G71
    unsigned maxTrackLength;

    // D64 / D71
    uint8_t tracksInDxx;
    uint8_t* errorMap;
    uint32_t errorMapSize;

    auto analyzeD64() -> bool;
    auto analyzeD71() -> bool;
    auto analyzeG64() -> bool;
    auto analyzeG71() -> bool;
    auto analyzeP64() -> bool;
    auto analyzeP71() -> bool;
    
    static auto createDxx( std::string diskName, uint8_t sides = 1 ) -> uint8_t*;
    static auto createGxx( std::string diskName, uint8_t sides = 1 ) -> uint8_t*;
    static auto createPxx( std::string diskName, uint8_t sides = 1 ) -> Emulator::Interface::Data;
    static auto cutId( std::string& diskName ) -> std::string;
    
    static auto imageSizeG64() -> unsigned;
    static auto imageSizeG71() -> unsigned;
    static auto imageSizeD64() -> unsigned;
    static auto imageSizeD71() -> unsigned;
        
    auto prepareGxx() -> void;
    auto prepareDxx() -> void;
    auto preparePxx() -> void;
    auto getTrackOffsetGxx( uint8_t halfTrack, int& error ) -> uint32_t;
    auto handleAppendedTracksInDxx() -> bool;
    auto addMfmByte(uint8_t*& dest, uint8_t data, uint16_t& crc) -> void;
    auto parseMfm(GcrTrack* trackPtr, unsigned offset) -> void;
    auto writeMfm(const GcrTrack* trackPtr, unsigned offset) -> bool;
        
    auto writeDxx(const GcrTrack* trackPtr, uint8_t side, unsigned track, bool& errorMapChanged) -> bool;
    auto writeGxx(const GcrTrack* trackPtr, uint8_t side, unsigned halfTrack) -> bool;
    auto writeP64ToMem(unsigned& memSize) -> uint8_t*;
    auto writePxx() -> bool;
    
    static auto writeSector( uint8_t* target, uint8_t* buffer, uint8_t track, uint8_t sector, unsigned offset = 0) -> void;
    static auto readSector( uint8_t* src, uint8_t* buffer, uint8_t track, uint8_t sector, unsigned offset = 0 ) -> bool;
    static auto createBAM( std::string diskName, uint8_t* buffer, uint8_t* bufferSecondSide = nullptr ) -> void;

    static auto encodeSector(const uint8_t* src, uint8_t* target, uint8_t track, uint8_t sector, uint8_t id1, uint8_t id2, int errorCode) -> void;    
    auto decodeSector( const GcrTrack* trackPtr, uint8_t* dest, uint8_t sector ) -> int;
    auto findSync( const GcrTrack* trackPtr, unsigned& offset, unsigned size ) -> bool;
    auto decode( const GcrTrack* trackPtr, unsigned offset, uint8_t* buffer, unsigned blockCount ) -> void;

	inline auto decodeP64( Emulator::Fpaq0& fpaq0, std::vector<Emulator::PredictorEightBitWithPrefix*>& predictors ) -> unsigned;
    inline auto encodeP64( Emulator::Fpaq0& fpaq0, std::vector<Emulator::PredictorEightBitWithPrefix*>& predictors, unsigned value ) -> void;
    auto decodeJob( std::vector<uint8_t*>* workLoad, bool* usePtr ) -> bool;
    auto encodeGCR(GcrTrack* gcrTrack, uint8_t halfTrack) -> void;
    auto prepareTracksNotInUse(bool* inUse) -> void;
    auto createPulsesFromGCR(GcrTrack* gcrTrack) -> void;
    static auto allocatePulse( std::vector<Pulse>& pulses ) -> unsigned;
};

}
