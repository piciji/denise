
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
    };
    
    enum class CropType { Off = 0, Monitor = 1, Auto = 2, SemiAuto = 3, Free = 4 };
    enum class TapeMode { Stop = 0, Play = 1, Record = 2, Forward = 3, Rewind = 4, ResetCounter = 5, Unpressed = 6 };
    
    std::string ident;
    
	Interface( std::string ident ) {
        this->ident = ident;        
    }

    // Note: all id fields in the following structs have to match the vector position of the struct container.
    // sure these id fields are redundant. it's one and only purpose is to access the container vector fast 
    // instead of iterating the whole vector to look for a specific element
	
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
    
    struct Expansion {
        unsigned id;
        std::string name;
        unsigned romSlots; // individual images, not ROM chips
        MemoryType* memoryType;            
    };
    std::vector<Expansion> expansions;
    
    struct DriveGroup;
    
    struct Drive {
        unsigned id;
        std::string name;
        uintptr_t guid; //free to use
        DriveGroup* group;
    };

    struct DriveGroup {
        unsigned id;
        std::string name;        
		enum Type : unsigned { DiskDrive, HardDrive, TapeDrive, ModuleSlot, Memory } type;
        std::vector<std::string> suffix;
        std::vector<std::string> creatable;
        
        auto isDiskDrive() const -> bool { return type == Type::DiskDrive; }
        auto isHardDrive() const -> bool { return type == Type::HardDrive; }
        auto isTapeDrive() const -> bool { return type == Type::TapeDrive; }
		auto isModuleSlot() const -> bool { return type == Type::ModuleSlot; }
		auto isMemory() const -> bool { return type == Type::Memory; }
        //default count of connected drives
        auto defaultUsage() -> unsigned { return type == Type::DiskDrive ? 1 : 0; }

        std::vector<Drive> drives;
    };

    std::vector<DriveGroup> driveGroups;         
    
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
        virtual auto readDrive(unsigned, unsigned, uint8_t*, unsigned, unsigned) -> unsigned { return 0; }
        virtual auto writeDrive(unsigned, unsigned, uint8_t*, unsigned, unsigned) -> unsigned { return 0; }
        virtual auto updateDriveState(unsigned, unsigned, unsigned, unsigned) -> void {}
        virtual auto log(std::string, bool) -> void {} //for debugging
        virtual auto exit( int code ) -> void {}
        virtual auto finishVBlank() -> void {}
        virtual auto midScreenCallback( ) -> void {}
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

    auto readDrive(unsigned groupId, unsigned driveId, uint8_t* buffer, unsigned length, unsigned offset) -> unsigned {
        return bind->readDrive(groupId, driveId, buffer, length, offset);
    }

    auto writeDrive(unsigned groupId, unsigned driveId, uint8_t* buffer, unsigned length, unsigned offset) -> unsigned {
        return bind->writeDrive(groupId, driveId, buffer, length, offset);
    }

    auto updateDriveState(unsigned groupId, unsigned driveId, unsigned mode, unsigned track) -> void {
        bind->updateDriveState(groupId, driveId, mode, track); //mode: 0 - no operation, 1 - read, 2 - write, 3 - list
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
    virtual auto setDrivesConnected(unsigned groupId, unsigned count) -> void {}
    virtual auto getDrivesConnected(unsigned groupId) -> unsigned { return 0; }
    // disk handling
    virtual auto insertDisk(unsigned driveId, uint8_t* data, unsigned size) -> void {}
    virtual auto writeProtectDisk(unsigned driveId, bool state) -> void {}
    virtual auto ejectDisk(unsigned driveId) -> void { } 
	virtual auto getDiskImageSize(unsigned typeId, bool hd) -> unsigned { return 0; } //get size needed for a new disk image
	virtual auto createDiskImage(unsigned typeId, bool hd = false, std::string name = "", bool ffs = false) -> uint8_t* { return nullptr; }        
    virtual auto getDiskListing(unsigned driveId) -> std::vector<Listing> { return {}; }
    virtual auto selectDiskListing(unsigned driveId, unsigned pos) -> void { }
    // hard disk handling
    virtual auto setHardDrive(unsigned driveId, unsigned size) -> void {} //uses read and write callbacks above because of big data
	virtual auto ejectHardDrive(unsigned driveId) -> void {}
    virtual auto createHardDrive(std::function<void (uint8_t* buffer, unsigned length, unsigned offset)> onCreate, unsigned size, std::string name = "") -> void {}
    // tape handling
    virtual auto insertTape(unsigned driveId, uint8_t* data, unsigned size) -> void {}    
    virtual auto writeProtectTape(unsigned driveId, bool state) -> void {}
    virtual auto ejectTape(unsigned driveId) -> void { } 
    virtual auto controlTape(unsigned driveId, TapeMode mode) -> void {}
    virtual auto getTapeControl(unsigned driveId) -> TapeMode { return TapeMode::Unpressed; }
	virtual auto createTapeImage(unsigned& imageSize) -> uint8_t* { return nullptr; }
    virtual auto selectTapeListing(unsigned driveId, unsigned pos) -> void { }
    // module handling
    virtual auto insertModule(uint8_t* data, unsigned size) -> void {}
    virtual auto ejectModule() -> void {}
	// memory 
	virtual auto insertMemory(unsigned driveId, uint8_t* data, unsigned size) -> void {}
	virtual auto ejectMemory(unsigned driveId) -> void {}	
	virtual auto getLoadedMemory(unsigned& size) -> uint8_t* { return nullptr; }
	virtual auto getMemoryListing() -> std::vector<Listing> { return {}; }
	virtual auto selectMemoryListing(unsigned pos) -> bool { return false; }	
    // expansion port
    virtual auto setExpansionPort(unsigned expansionId) -> void {}

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
    virtual auto setMemory(unsigned typeId, unsigned memoryId) -> void {}
    virtual auto getMemory(unsigned typeId) -> Memory* { return nullptr; }
    // firmware will copied internally, so you can close the relevant file afterwards
    virtual auto setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void {}
    
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
    
    virtual auto runCycles(unsigned cycles) -> void {}
    
    //sets alternative per line callbacks
    virtual auto setLineCallback(bool state, unsigned scanline = 0) -> void {}
    virtual auto setFinishVblankCallback(bool state) -> void {}
    
    auto getStatsForSelectedRegion() -> Stats& {  
        auto region = getRegion();

        for( auto& stat : stats )
            if (stat.region == region)
                return stat;        
        
        return stats[0];
    }
    
	//shortcuts
	auto insertMedium(unsigned type, unsigned driveId, uint8_t* data, unsigned size) -> void {
		switch(type) {
			case DriveGroup::Type::DiskDrive: insertDisk(driveId, data, size); break;
			case DriveGroup::Type::TapeDrive: insertTape(driveId, data, size); break;
			case DriveGroup::Type::ModuleSlot: insertModule(data, size); break;
			case DriveGroup::Type::Memory: insertMemory(driveId, data, size); break;
			case DriveGroup::Type::HardDrive: break; //works differently, dont call it this way
		}
	}
	
	auto writeProtect(unsigned type, unsigned driveId, bool state) -> void {
		switch(type) {
			case DriveGroup::Type::DiskDrive: writeProtectDisk(driveId, state); break;
			case DriveGroup::Type::TapeDrive: writeProtectTape(driveId, state); break;
			case DriveGroup::Type::ModuleSlot: break; //roms cant be written anyway
			case DriveGroup::Type::Memory: break; //work mem is controlled by software
			case DriveGroup::Type::HardDrive: break; //harddrives cant be set as writeprotected
		}
	}
	
	auto ejectMedium(unsigned type, unsigned driveId) -> void {		
		switch(type) {
			case DriveGroup::Type::DiskDrive: ejectDisk(driveId); break;
			case DriveGroup::Type::TapeDrive: ejectTape(driveId); break;
			case DriveGroup::Type::Memory: ejectMemory(driveId); break;
			case DriveGroup::Type::ModuleSlot: ejectModule(); break;
			case DriveGroup::Type::HardDrive: ejectHardDrive(driveId); break;
		}		
	}
    
    auto getListing(unsigned type, unsigned driveId) -> std::vector<Listing> {
        switch(type) {
			case DriveGroup::Type::DiskDrive:
                return getDiskListing( driveId );
			case DriveGroup::Type::TapeDrive: break;
			case DriveGroup::Type::Memory:
                return getMemoryListing( );
			case DriveGroup::Type::ModuleSlot: break;
			case DriveGroup::Type::HardDrive: break;
		}	
        return {};
    }
    
    auto selectListing(unsigned type, unsigned driveId, unsigned position) -> bool {
        switch(type) {
			case DriveGroup::Type::DiskDrive:
                selectDiskListing( driveId, position );
                return true;
			case DriveGroup::Type::TapeDrive:
                selectTapeListing( driveId, position );
                return true;
			case DriveGroup::Type::Memory:
                return selectMemoryListing( position );
			case DriveGroup::Type::ModuleSlot: break;
			case DriveGroup::Type::HardDrive: break;
		}	
        return false;
    }
    
    auto getDiskDrive( unsigned driveId ) -> Drive* {        
        for(auto& driveGroup : driveGroups)            
            if (driveGroup.isDiskDrive()) {                
                if (driveGroup.drives.size() > driveId)
                    return &driveGroup.drives[ driveId ];
            }
        
        return nullptr;
    }
    
    auto getTapeDrive( unsigned driveId ) -> Drive* {        
        for(auto& driveGroup : driveGroups)            
            if (driveGroup.isTapeDrive()) {                
                if (driveGroup.drives.size() > driveId)
                    return &driveGroup.drives[ driveId ];
            }
        
        return nullptr;
    } 
    
    auto getModuleSlot( unsigned driveId ) -> Drive* {        
        for(auto& driveGroup : driveGroups)            
            if (driveGroup.isModuleSlot()) {                
                if (driveGroup.drives.size() > driveId)
                    return &driveGroup.drives[ driveId ];
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
    
    auto getExpansion( unsigned id ) -> Expansion* {
        
        for(auto& expansion : expansions) {
            if (expansion.id == id)
                return &expansion;
        }
        return nullptr;
    }
};

}
