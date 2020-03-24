
#pragma once

#include <string>
#include <vector>
#include <functional>

namespace Emulator {
    
/**
 * base emulator interface to inherit
 */		
struct Interface {
    
    // key idents for the emulated keyboards  
    
    enum class Key : unsigned {
        D0, D1, D2, D3, D4, D5, D6, D7, D8, D9,
        A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10,
        NumPad0, NumPad1, NumPad2, NumPad3, NumPad4, NumPad5, NumPad6, NumPad7, NumPad8, NumPad9,
        NumParenthesesLeft, NumParenthesesRight, NumDivide, NumMultiply, NumSubtract, NumAdd, NumEnter, NumComma,        
        NumLock, ScrollLock, ShiftNumLock, ShiftScrollLock,
        CursorRight, CursorLeft, CursorUp, CursorDown,
        Backspace, Return, Plus, Minus, ShiftLeft, ShiftRight, Comma, Period, Colon, Tab, Esc, Del, Help,
        Semicolon, Pound, Asterisk, Space, Home, Equal, RunStop, Commodore, Ctrl,
        AltLeft, AltRight, 
        SystemLeft, SystemRight,
        ArrowUp, ArrowLeft, Slash, BackSlash, ControlLeft, At, ShiftLock, Restore, 
        ExclamationMark, NumberSign, DoubleQuotes, Dollar, Percent, Ampersand,
        Acute, ParenthesesLeft/*round bracket*/, ParenthesesRight,
        OpenSquareBracket, ClosedSquareBracket, QuestionMark, 
        Less, Greater, Pipe,
        // use following keys for emulated systems which use multiple keyboard layouts
        Shared1, Shared2, Shared3, Shared4, Shared5, Shared6, Shared7, Shared8, 
        Shared9, Shared10, Shared11, Shared12, Shared13, 

        ShiftShared1, ShiftShared2, ShiftShared3, ShiftShared4, ShiftShared5,
        ShiftShared6, ShiftShared7, ShiftShared8, ShiftShared9, ShiftShared10,
        ShiftShared11, ShiftShared12, ShiftShared13, 
        
        ShiftAnd0, ShiftAnd1, ShiftAnd2, ShiftAnd3, ShiftAnd4, 
        ShiftAnd5, ShiftAnd6, ShiftAnd7, ShiftAnd8, ShiftAnd9, 
        
        AltAnd2, 
        
        JoyUpRight, JoyDownRight, JoyUpLeft, JoyDownLeft,
    };
    
    enum class CropType { Off = 0, Monitor = 1, Auto = 2, SemiAuto = 3, Free = 4 };
    enum class TapeMode { Stop = 0, Play = 1, Record = 2, Forward = 3, Rewind = 4, ResetCounter = 5, Unpressed = 6 };
    enum class FastForward { NoAudioOut = 1, NoVideoOut = 2, ReduceVideoOutput = 4, NoVideoSequencer = 8 };
    
    std::string ident;
    
	Interface( std::string ident ) {
        this->ident = ident;        
    }
	
    struct Device {
        unsigned id;
        std::string name;
		enum Type : unsigned { None, Joypad, Mouse, Paddles, LightGun, LightPen, Keyboard } type;
        unsigned userData; // free to use, easy way to transfer data for a specific device from external
                
        auto isMouse() -> bool { return type == Type::Mouse; }
        auto isPaddles() -> bool { return type == Type::Paddles; }        
        auto isJoypad() -> bool { return type == Type::Joypad; }
        auto isLightGun() -> bool { return type == Type::LightGun; }
        auto isLightPen() -> bool { return type == Type::LightPen; }
        auto isLightDevice() -> bool { return isLightGun() || isLightPen(); }
        auto isKeyboard() -> bool { return type == Type::Keyboard; }        
        auto isUnplugged() -> bool { return type == Type::None; }

        struct Layout {
            enum Type { Uk, Us, Fr, De } type;
            std::string name;
        };
        
        struct Input {
            unsigned id;
            std::string name;
            Key key;

            // for alternate key names
            std::vector<Layout> layouts;
            
            // layout of host keyboard and emulated keyboard are different for some keys.
            // to input some key combinations on host keyboard you need a print out of emulated keyboard.
            // that would be cumbersome. its easier to overmap some key combinations of emulated system.            
            // you could assign a single host key to input a emulated key combination.
            // i.e. left and up cursor are secondary functions on c64 but host keyboard have all 4 keys.            
            std::vector<unsigned> shadowMap;            
            
            unsigned type; //0 = digital, 1 = analog
            uintptr_t guid; //free to use
            
            auto isDigital() -> bool { return type == 0; }
        };
        
        auto addVirtual( std::string ident, std::vector<unsigned> inputIds, Key key, std::vector<Layout> layouts = {} ) -> void {
                        
            inputs.push_back( { (unsigned)inputs.size(), ident, key, layouts, inputIds } );
        }
        
        std::vector<Input> inputs;
    };
    
    std::vector<Device> devices;

    struct Connector {
        unsigned id;
        std::string name;        
        enum Type : unsigned { Port1, Port2 } type;
        
        auto isPort1() -> bool { return type == Type::Port1; }
        auto isPort2() -> bool { return type == Type::Port2; }
    };

    std::vector<Connector> connectors;    

    struct Memory {
        unsigned id;
        unsigned size;
    };

    struct MemoryType {
        unsigned id;
        std::string name;
        unsigned defaultMemoryId;

        std::vector<Memory> memory;
    };

    std::vector<MemoryType> memoryTypes; 
    
    struct MediaGroup;
    
    // possible extra hardware a medium can have
    struct PCBLayout {
        unsigned id;
        std::string name;
    };
    
    struct Jumper {
        unsigned id;
        std::string name;
    };
    
    struct Expansion {
        unsigned id;
        std::string name;        
        unsigned typeFlags;
        MemoryType* memoryType; // uses RAM
        MediaGroup* mediaGroup; // uses ROM
        std::vector<PCBLayout> pcbs;
        std::vector<Jumper> jumpers;
        enum Type : unsigned { Empty = 0, Game = 1, Ram = 2, Eprom = 4, Flash = 8, TurboCart = 16, Freezer = 32 };
        
        auto isEmpty() const -> bool { return typeFlags == (unsigned)Type::Empty; }
        auto isGame() const -> bool { return typeFlags & Type::Game; }
        auto isRam() const -> bool { return typeFlags & Type::Ram; }
        auto isEprom() const -> bool { return typeFlags & Type::Eprom; }
        auto isFlash() const -> bool { return typeFlags & Type::Flash; }
        auto isTurboCart() const -> bool { return typeFlags & Type::TurboCart; }               
        auto isFreezer() const -> bool { return typeFlags & Type::Freezer; }      
    };
    std::vector<Expansion> expansions;       
    
    struct Media {
        unsigned id;
        std::string name;
        uintptr_t guid; //free to use
        MediaGroup* group;        
        PCBLayout* pcbLayout;
        bool memoryDump;
    };   

    struct MediaGroup {
        unsigned id;
        std::string name;        
		enum class Type : unsigned { Disk, HardDisk, Tape, Expansion, Program } type;
        std::vector<std::string> suffix;
        std::vector<std::string> creatable;
        Media* selected; 
        Expansion* expansion;
        std::vector<Media> media;
        
        auto isDisk() const -> bool { return type == Type::Disk; }
        auto isHardDisk() const -> bool { return type == Type::HardDisk; }
        auto isTape() const -> bool { return type == Type::Tape; }
		auto isExpansion() const -> bool { return type == Type::Expansion; }
		auto isProgram() const -> bool { return type == Type::Program; }
        auto isDrive() const -> bool { return isDisk() || isTape() || isHardDisk(); };
        auto isWritable() const -> bool { return isDrive() || (
                   isExpansion() && (expansion->isFlash() || expansion->isEprom())); }
        // default count of connected drives
        auto defaultUsage() -> unsigned { return type == Type::Disk ? 1 : 0; }       
    };

    std::vector<MediaGroup> mediaGroups;         
    
    struct Cpu {
        unsigned id;
        std::string name;
    };

    std::vector<Cpu> cpus;
	
	struct Firmware {
		unsigned id;
		std::string name;
	};	
	std::vector<Firmware> firmwares;  
	
	// custom features  
	struct Feature {
		unsigned id;
		std::string name;
		enum Type : unsigned { Switch, Range, Hex } type;
		int defaultValue;
        bool runtimeChangeable;
        bool performanceHit;
		std::vector<int> range;

		auto isSwitch() -> bool { return type == Type::Switch; }
        auto isHex() -> bool { return type == Type::Hex; }
	};
	std::vector<Feature> features;
	
	struct Chipset {
		unsigned id;
		std::string name;		
	};
	
	// chipset
	std::vector<Chipset> chipsets;	    
	
	// general purpose emulator output listing
	struct Listing {
		unsigned id;
		std::vector<uint8_t> line; // host is responsible for conversion, e.g. c64 use petscii charset
	};	
    
    enum Region : uint8_t { Pal = 0, Ntsc = 1 };
    
    struct Stats {
        Region region;
        double sampleRate;
        double fps;
        bool stereoSound;
        auto isPal() -> bool { return region == Region::Pal; }
        auto isNtsc() -> bool { return region == Region::Ntsc; }
    };
    
    std::vector<Stats> stats;
    
    struct PaletteColor {
        std::string name;
        unsigned rgb;
        uint8_t r;
        uint8_t g;
        uint8_t b;
        
        auto updateChannels() -> void {
            r = (rgb >> 16) & 0xff;
            g = (rgb >> 8) & 0xff;
            b = (rgb >> 0) & 0xff;
        }
        
        auto set( unsigned rgb ) -> void {
            this->rgb = rgb;
            updateChannels();
        }
    };
    
    struct Palette {
        unsigned id;
        std::string name;
        bool editable;
        std::vector<PaletteColor> paletteColors;
    };
    
    std::vector<Palette> palettes;

    //callbacks
    struct Bind {
        virtual auto inputPoll(uint16_t, uint16_t) -> int16_t { return 0; }
        virtual auto videoRefresh(const uint16_t*, unsigned, unsigned, unsigned) -> void {}
        virtual auto audioSample(int16_t, int16_t) -> void {}
        virtual auto readMedia(Media*, uint8_t*, unsigned, unsigned) -> unsigned { return 0; }
        virtual auto writeMedia(Media*, uint8_t*, unsigned, unsigned) -> unsigned { return 0; }
        virtual auto truncateMedia(Media* ) -> bool { return false; }
        virtual auto updateDriveState(Media*, unsigned, unsigned) -> void {}
        virtual auto log(std::string, bool) -> void {} //for debugging
        virtual auto exit( int code ) -> void {}
        virtual auto finishVBlank() -> void {}
        virtual auto midScreenCallback( ) -> void {}
        virtual auto questionToWrite(Media* media) -> bool { return false; }
    };
    Bind* bind = nullptr;

    auto inputPoll(uint16_t deviceId, uint16_t inputId) -> int16_t {
        return bind->inputPoll(deviceId, inputId);
    }
    
    auto audioSample(int16_t sampleLeft, int16_t sampleRight) -> void {
        bind->audioSample(sampleLeft, sampleRight);
    }
    //return color format is native
    auto videoRefresh(const uint16_t* frame, unsigned width, unsigned height, unsigned linePitch) -> void {
        bind->videoRefresh(frame, width, height, linePitch);
    }
    
    auto finishVBlank() -> void {
        bind->finishVBlank();
    }
    
    auto midScreenCallback() -> void {
        bind->midScreenCallback();
    }

    auto readMedia(Media* media, uint8_t* buffer, unsigned length, unsigned offset) -> unsigned {
        return bind->readMedia(media, buffer, length, offset);
    }

    auto writeMedia(Media* media, uint8_t* buffer, unsigned length, unsigned offset) -> unsigned {
        return bind->writeMedia(media, buffer, length, offset);
    }
    
    auto truncateMedia(Media* media) -> bool {
        return bind->truncateMedia( media );
    }

    auto updateDriveState(Media* media, unsigned mode, unsigned track) -> void {
        bind->updateDriveState(media, mode, track); //mode: 0 - no operation, 1 - read, 2 - write, 3 - list
    }
    
    auto questionToWrite(Media* media) -> bool {
        return bind->questionToWrite(media);
    }

    auto log(const char* data, bool newLine = true) -> void {
        bind->log(data, newLine);
    }
    
    auto exit(int code) -> void {
        bind->exit( code );
    }

    template<typename T> auto log(T data, bool newLine = true, bool asHex = false) -> void {			

        std::string out = std::to_string(data);	

        if (asHex) {
            out = "0x";
            char hex[8];
            sprintf( hex, "%x", data );
            out += (std::string)hex;
        }
        bind->log(out, newLine);
    }   

    // set amount of tape, disk, hard drives or module slots connected to the system
    virtual auto setDrivesConnected(MediaGroup* group, unsigned count) -> void {}
    virtual auto getDrivesConnected(MediaGroup* group) -> unsigned { return 0; }
    virtual auto setDriveSpeed(MediaGroup* group, double rpm, double wobble) -> void {}
    // disk handling
    virtual auto insertDisk(Media* media, uint8_t* data, unsigned size) -> void {}
    virtual auto writeProtectDisk(Media* media, bool state) -> void {}
    virtual auto ejectDisk(Media* media) -> void { } 
	virtual auto getDiskImageSize(unsigned typeId, bool hd) -> unsigned { return 0; } //get size needed for a new disk image
	virtual auto createDiskImage(unsigned typeId, bool hd = false, std::string name = "", bool ffs = false) -> uint8_t* { return nullptr; }        
    virtual auto getDiskListing(Media* media) -> std::vector<Listing> { return {}; }
    virtual auto getDiskPreview(uint8_t* data, unsigned size) -> std::vector<Listing> { return {}; }
    virtual auto selectDiskListing(Media* media, unsigned pos) -> void { }
    
    // hard disk handling
    virtual auto insertHardDisk(Media* media, unsigned size) -> void {} //uses read and write callbacks above because of big data
	virtual auto ejectHardDisk(Media* media) -> void {}
    virtual auto createHardDisk(std::function<void (uint8_t* buffer, unsigned length, unsigned offset)> onCreate, unsigned size, std::string name = "") -> void {}
    // tape handling
    virtual auto insertTape(Media* media, uint8_t* data, unsigned size) -> void {}    
    virtual auto writeProtectTape(Media* media, bool state) -> void {}
    virtual auto ejectTape(Media* media) -> void { } 
    virtual auto controlTape(Media* media, TapeMode mode) -> void {}
    virtual auto getTapeControl(Media* media) -> TapeMode { return TapeMode::Unpressed; }
	virtual auto createTapeImage(unsigned& imageSize) -> uint8_t* { return nullptr; }
    virtual auto selectTapeListing(Media* media, unsigned pos) -> void { }
    // expansion image handling
    virtual auto insertExpansionImage(Media* media, uint8_t* data, unsigned size) -> void {}
    virtual auto ejectExpansionImage(Media* media) -> void {}
    virtual auto writeProtectExpansion(Media* media, bool state) -> void {}
    virtual auto createExpansionImage(MediaGroup* group, unsigned& imageSize) -> uint8_t* { return nullptr; }
    virtual auto getMediaForCustomFileSuffix(std::string suffix) -> Media* { return nullptr; }
    virtual auto isExpansionBootable() -> bool { return false; }
    
	// program 
	virtual auto insertProgram(Media* media, uint8_t* data, unsigned size) -> void {}
	virtual auto ejectProgram(Media* media) -> void {}	
	virtual auto getLoadedProgram(unsigned& size) -> uint8_t* { return nullptr; }
	virtual auto getProgramListing(Media* media) -> std::vector<Listing> { return {}; }
    virtual auto getProgramPreview(uint8_t* data, unsigned size) -> std::vector<Listing> { return {}; }
	virtual auto selectProgramListing(Media* media, unsigned pos) -> bool { return false; }	
    // expansion port
    virtual auto setExpansion(unsigned expansionId) -> void {}
    virtual auto unsetExpansion() -> void {}    
    virtual auto getExpansion() -> Expansion* { return nullptr; }
    virtual auto analyzeExpansion(uint8_t* data, unsigned size) -> Expansion* { return nullptr; }    
    virtual auto setExpansionJumper( Media* media, unsigned jumperId, bool state ) -> void {}
    virtual auto getExpansionJumper( Media* media, unsigned jumperId ) -> bool { return false; }
    // freezer carts
    virtual auto hasFreezerButton() -> bool { return false; }
    virtual auto freeze() -> void {}

    // savestates
    virtual auto checkstate(uint8_t* data, unsigned size) -> bool { return false; }    
    virtual auto loadstate(uint8_t* data, unsigned size) -> bool { return false; }
    virtual auto savestate(unsigned& size) -> uint8_t* { return nullptr; }
    
	// graphic chipset
	virtual auto setChipset(unsigned chipsetId) -> void {}
    virtual auto getChipset() -> unsigned { return 0; }
    // get system specific feature parameter
    virtual auto setFeature(unsigned featureId, int value) -> void {}
    virtual auto getFeature(unsigned featureId) -> int { return 0; }
    
    //controls
    virtual auto connect(unsigned connectorId, unsigned deviceId) -> void {}
    virtual auto connect(Connector* connector, Device* device) -> void {}
    // to avoid non objects for unplugged ports, you should use a placeholder device
    virtual auto getUnplugDevice() -> Device* { return &devices[ 0 ]; }  

    virtual auto getConnectedDevice( Connector* connector ) -> Device* { return getUnplugDevice(); }
    // for light devices you can disable cursor rendering by requesting cursor position in order to
    // draw cursor by yourself. that is usefull if you want to draw the cursor in a higher resolution.
    // of course, the coordinates are in native resoltion of the emulated system. you need to convert them.
    virtual auto getCursorPosition( Device* device, int16_t& x, int16_t& y ) -> bool { return false; }
    
    virtual auto setCpu(unsigned cpuId) -> void {}
    virtual auto getCpu() -> unsigned { return 0; }
    virtual auto setMemory(MemoryType* memoryType, unsigned memoryId) -> void {}    
    virtual auto setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void {}
    virtual auto getCharRom() -> Firmware* { return nullptr; }
    
    virtual auto power() -> void {} //hard reset
	virtual auto reset() -> void {} //soft reset
	virtual auto powerOff() -> void {} //shutdown system
    virtual auto run() -> void {} //emulate one frame
    virtual auto setRegion(Region region) -> void {} 
    virtual auto getRegion() -> Region { return Region::Pal; }
    
    //crop
	virtual auto crop( CropType type, bool aspectCorrect, unsigned left = 0, unsigned right = 0, unsigned top = 0, unsigned bottom = 0 ) -> void {}
    // get native resolution after cropping
    virtual auto cropWidth() -> unsigned { return 0; }
    virtual auto cropHeight() -> unsigned { return 0; }
    virtual auto cropTop() -> unsigned { return 0; }
    virtual auto cropLeft() -> unsigned { return 0; }
    virtual auto cropData() -> uint16_t* { return nullptr; }
    virtual auto cropPitch() -> unsigned { return 0; }
    
    virtual auto runCycles(unsigned cycles) -> void {}
    
    //sets alternative per line callbacks
    virtual auto setLineCallback(bool state, unsigned scanline = 0) -> void {}
    virtual auto setFinishVblankCallback(bool state) -> void {}
    
    virtual auto fastForward(unsigned config) -> void {}
    
    auto getStatsForSelectedRegion() -> Stats& {  
        auto region = getRegion();

        for( auto& stat : stats )
            if (stat.region == region)
                return stat;        
        
        return stats[0];
    }
    
	//shortcuts
	auto insertMedium(Media* media, uint8_t* data, unsigned size) -> void {
		switch(media->group->type) {
			case MediaGroup::Type::Disk: insertDisk(media, data, size); break;
			case MediaGroup::Type::Tape: insertTape(media, data, size); break;
			case MediaGroup::Type::Expansion: insertExpansionImage(media, data, size); break;
			case MediaGroup::Type::Program: insertProgram(media, data, size); break;
			case MediaGroup::Type::HardDisk: insertHardDisk(media, size); break;
		}
	}
	
	auto writeProtect(Media* media, bool state) -> void {
		switch(media->group->type) {
			case MediaGroup::Type::Disk: writeProtectDisk(media, state); break;
			case MediaGroup::Type::Tape: writeProtectTape(media, state); break;
			case MediaGroup::Type::Expansion: writeProtectExpansion(media, state); break;
			case MediaGroup::Type::Program: break;
			case MediaGroup::Type::HardDisk: break;
		}
	}
	
	auto ejectMedium(Media* media) -> void {		
		switch(media->group->type) {
			case MediaGroup::Type::Disk: ejectDisk(media); break;
			case MediaGroup::Type::Tape: ejectTape(media); break;
			case MediaGroup::Type::Program: ejectProgram(media); break;
			case MediaGroup::Type::Expansion: ejectExpansionImage(media); break;
			case MediaGroup::Type::HardDisk: ejectHardDisk(media); break;
		}		
	}
    
    auto getListing(Media* media) -> std::vector<Listing> {
        switch(media->group->type) {
			case MediaGroup::Type::Disk:
                return getDiskListing( media );
			case MediaGroup::Type::Tape: break;
			case MediaGroup::Type::Program:
                return getProgramListing( media );
			case MediaGroup::Type::Expansion: break;
			case MediaGroup::Type::HardDisk: break;
		}	
        return {};
    }
    
    auto selectListing(Media* media, unsigned position) -> bool {
        switch(media->group->type) {
			case MediaGroup::Type::Disk:
                selectDiskListing( media, position );
                return true;
			case MediaGroup::Type::Tape:
                selectTapeListing( media, position );
                return true;
			case MediaGroup::Type::Program:
                return selectProgramListing( media, position );
			case MediaGroup::Type::Expansion: break;
			case MediaGroup::Type::HardDisk: break;
		}	
        return false;
    }
    
    auto getDisk( unsigned mediaId ) -> Media* {        
        for(auto& mediaGroup : mediaGroups)            
            if (mediaGroup.isDisk()) {                
                if (mediaGroup.media.size() > mediaId)
                    return &mediaGroup.media[ mediaId ];
            }
        
        return nullptr;
    }
    
    auto getTape( unsigned mediaId ) -> Media* {        
        for(auto& mediaGroup : mediaGroups)            
            if (mediaGroup.isTape()) {                
                if (mediaGroup.media.size() > mediaId)
                    return &mediaGroup.media[ mediaId ];
            }
        
        return nullptr;
    }    
        
    auto getDevice( unsigned deviceId ) -> Device* {        
        
        if (deviceId >= devices.size())
            return getUnplugDevice();
            
        return &devices[ deviceId ];
    }
    
    auto getConnector( unsigned connectorId ) -> Connector* {
        
        if (connectorId >= connectors.size())
            return nullptr;
            
        return &connectors[ connectorId ];  
    } 
    
    auto getMemoryById( MemoryType& memoryType, unsigned memoryId ) -> Memory* {
        
        for(auto& memory : memoryType.memory) {
            
            if (memory.id == memoryId)
                return &memory;
        }
        return nullptr;
    }
    
    auto getExpansionById( unsigned id ) -> Expansion* {
        
        for(auto& expansion : expansions) {
            if (expansion.id == id)
                return &expansion;
        }
        return nullptr;
    }
    
    auto getMedia( MediaGroup& group, unsigned mediaId ) -> Media* {
        
        for( auto& media : group.media ) {
            
            if (media.id == mediaId)
                return &media;
        }
        
        return nullptr;
    }
    
    auto getPCB( Expansion& expansion, unsigned pcbId ) -> PCBLayout* {
        
        for( auto& pcb : expansion.pcbs ) {
            
            if (pcb.id == pcbId)
                return &pcb;
        }
        
        return nullptr;
    }
};

}
