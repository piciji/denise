
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <optional>
#include <mutex>

namespace Emulator {
    
/**
 * base emulator interface to inherit
 */		
struct Interface {

    virtual ~Interface() {}
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
        Button, Direction, Autofire, AutofireDirection, ToggleAutofire,

        AltAndShift2, AltAndShift3,
    };
    
    enum class CropType { Off = 0, Monitor = 1, AutoRatio = 2, Auto = 3, AllSidesRatio = 4, AllSides = 5, Free = 6 };
    enum class TapeMode { Stop = 0, Play = 1, Record = 2, Forward = 3, Rewind = 4, ResetCounter = 5, Unpressed = 6 };
    enum class WarpMode { NoAudioOut = 1, NoVideoOut = 2, ReduceVideoOutputEach16th = 4, NoVideoSequencer = 8, SlowSpeed = 16,
        ReduceVideoOutputEach4th = 32, ReduceVideoOutputEach8th = 64 };
    enum class DriveSound { FloppyInsert = 1, FloppyEject = 2, FloppySpinUp = 3, FloppySpinDown = 4,
                            FloppySpin = 5, FloppyHeadBang = 6, FloppyStep = 7, FloppySnatch = 9,
                            TapeInsert = 10, TapeEject = 11, TapeAnyButton = 12, TapeStopButton = 13,
                            TapePlaySpinUp = 14, TapePlaySpin = 15, TapeSpinDown = 16, TapeForwardSpin = 17, TapeRewindSpin = 18,
    };
    enum class ThreadPriority { Normal = 0, High = 1, Realtime = 2 };
    enum class LedId { Power, CapsLock, MHz2 };

    std::string ident;
    
	Interface( std::string ident ) {
        this->ident = ident;        
    }

    struct Crop {
        unsigned left;
        unsigned right;
        unsigned top;
        unsigned bottom;
    };
	
    struct Device {
        unsigned id;
        std::string name;
		enum Type : unsigned { None, Joypad, Mouse, Paddles, LightGun, LightPen, Keyboard, MultiPlayerAdapter } type;
        unsigned userData; // free to use, easy way to transfer data for a specific device from external
                
        auto isMouse() const -> bool { return type == Type::Mouse; }
        auto isPaddles() const -> bool { return type == Type::Paddles; }
        auto isJoypad() const -> bool { return type == Type::Joypad; }
        auto isMultiPlayerAdapter() const -> bool { return type == Type::MultiPlayerAdapter; }
        auto isJoypadOrMultiAdapter() const -> bool { return isJoypad() || isMultiPlayerAdapter(); }
        auto isLightGun() const -> bool { return type == Type::LightGun; }
        auto isLightPen() const -> bool { return type == Type::LightPen; }
        auto isLightDevice() const -> bool { return isLightGun() || isLightPen(); }
        auto isKeyboard() const -> bool { return type == Type::Keyboard; }
        auto isUnplugged() const -> bool { return type == Type::None; }

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
            // that would be cumbersome. it's easier to over map some key combinations of emulated system.
            // you could assign a single host key to input an emulated key combination.
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

    struct MediaGroup;
    
    // possible extra hardware a medium can have
    struct PCBLayout {
        unsigned id;
        std::string name;
    };
    
    struct Jumper {
        unsigned id;
        std::string name;
        bool defaultState;
    };
    
    struct Expansion {
        unsigned id;
        std::string name;        
        unsigned typeFlags;
        MediaGroup* mediaGroup; // ROM, RAM dumps
		MediaGroup* mediaGroupExpanded; // expanded expansion
        std::vector<PCBLayout> pcbs;
        std::vector<Jumper> jumpers;
		std::vector<std::string> creationIdents; 
        enum Type : unsigned { Empty = 0, Standard = 1, Ram = 2, Eprom = 4, Flash = 8, TurboCart = 16,
            Freezer = 32, Battery = 64, RS232 = 128, Fastloader = 256, HDController = 512 };
        
        auto isEmpty() const -> bool { return typeFlags == (unsigned)Type::Empty; }
        auto isStandard() const -> bool { return typeFlags & Type::Standard; }
        auto isRam() const -> bool { return typeFlags & Type::Ram; }
        auto isEprom() const -> bool { return typeFlags & Type::Eprom; }
		auto isBattery() const -> bool { return typeFlags & Type::Battery; }
        auto isFlash() const -> bool { return typeFlags & Type::Flash; }
        auto isTurboCart() const -> bool { return typeFlags & Type::TurboCart; }
        auto isFreezer() const -> bool { return typeFlags & Type::Freezer; }
        auto isRS232() const -> bool { return typeFlags & Type::RS232; }
        auto isFastloader() const -> bool { return typeFlags & Type::Fastloader; }
        auto isHDController() const -> bool { return typeFlags & Type::HDController; }
    };
    std::vector<Expansion> expansions;
    
    struct Media {
        unsigned id;
        std::string name;
        uintptr_t guid; //free to use
        MediaGroup* group;        
        PCBLayout* pcbLayout;
        bool secondary; // secondary memory on cartridges, like Eeprom besides Flash on Gmod2 or REU ROM besides RAM dump
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
                   isExpansion() && (expansion->isFlash() || expansion->isEprom() || expansion->isBattery())); }
    };

    std::vector<MediaGroup> mediaGroups;         
    	
	struct Firmware {
		unsigned id;
		std::string name;
	};	
	std::vector<Firmware> firmwares;  
	
	// emulated model  
	struct Model {
		unsigned id;
		std::string name;		
		enum Type : unsigned { Switch, Range, Hex, Radio, Combo, Slider } type;
		enum Purpose : unsigned { Cpu, GraphicChip, SoundChip, Cia, AudioSettings, AudioResampler, Misc, DriveSettings, Performance, Memory, SubModels, DriveMechanics, AudioExtern } purpose;
		int defaultValue;
		std::vector<int> range;
		std::vector<std::string> options;
        unsigned steps;
        float scaler;

		auto isRadio() const -> bool { return type == Type::Radio; }
		auto isCombo() const -> bool { return type == Type::Combo; }
		auto isSwitch() const -> bool { return type == Type::Switch; }
        auto isHex() const -> bool { return type == Type::Hex; }
		auto isRange() const -> bool { return type == Type::Range; }
        auto isSlider() const -> bool { return type == Type::Slider; }
		
		auto isGraphicChip() const -> bool { return purpose == Purpose::GraphicChip; }
        auto isSoundChip() const -> bool { return purpose == Purpose::SoundChip; }
        auto isAudioResampler() const -> bool { return purpose == Purpose::AudioResampler; }
        auto isAudioSettings() const -> bool { return purpose == Purpose::AudioSettings; }
        auto isDriveSettings() const -> bool { return purpose == Purpose::DriveSettings; }
	    auto isDriveMechanics() const -> bool { return purpose == Purpose::DriveMechanics; }
        auto isPerformance() const -> bool { return purpose == Purpose::Performance; }
	};
	std::vector<Model> models;	
    	
	// general purpose emulator output listing
	struct Listing {
		std::vector<uint16_t> line; // host is responsible for conversion, e.g. c64 use petscii charset
		std::vector<uint16_t> loadCommand;
	};	
    
    enum Region : uint8_t { Pal = 0, Ntsc = 1 };
	enum SubRegion : uint8_t { Pal_B = 0, Ntsc_M = 1, Pal_N = 2, Pal_M = 3 };
    
    struct Stats {
        Region region;
        double sampleRate;
        uint8_t sampleIntervall;
        double fps;
        bool stereoSound;
        auto isPal() -> bool { return region == Region::Pal; }
        auto isNtsc() -> bool { return region == Region::Ntsc; }
    };
    
    Stats stats;
    
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

    struct Data {
        uint8_t* ptr;
        unsigned size;
    };

    struct Item {
        unsigned id;
        std::string name;
        Data data;
        bool isGroup = false;
        Item* parent = nullptr;
        bool primary = false;
        std::vector<Item*> childs;
    };

    struct MemoryPattern {
        uint8_t value = 255;
        unsigned invertEvery = 64;
        unsigned offset = 0;
        uint8_t secondValue = 0;
        unsigned secondInvertEvery = 0;
        unsigned randomPatternLength = 1;
        unsigned repeatRandomPattern = 256;
        unsigned randomChance = 0;
    };

    struct DebuggerIdent {
        uint32_t vector;
        const char* ident;
    };

    struct DebuggerDma {
        enum class Hilight { Default = 0, Write = 1, Opcode = 2 };
        // DMA usage
        uint8_t usage;
        uint32_t address;
        uint16_t data;

        // CPU usage or DMA
        uint8_t usageCpu;
        uint32_t addrCpu;
        uint16_t dataCpu;
        Hilight hilight;
        const char* mnemonic;

        // free assignable
        uint16_t watcher[4];
    };

    enum class DebuggerTheme : unsigned { Unspecified = 0,
        CPU = 1, SCPU = 2, Drive8CPU = 4, Drive9CPU = 8, Drive10CPU = 0x10, Drive11CPU = 0x20,
        Memory = 0x40, MemorySCPU = 0x80, Drive8Memory = 0x100, Drive9Memory = 0x200, Drive10Memory = 0x400, Drive11Memory = 0x800,
        CIA = 0x1000, Video = 0x2000, DMA = 0x4000, SID = 0x8000, Copper = 0x10000, Blitter = 0x20000,
        Agnus = 0x40000, Paula = 0x80000, Drive8VIA = 0x100000, Drive9VIA = 0x200000, Drive10VIA = 0x400000, Drive11VIA = 0x800000,
        Serial = 0x1000000
    };

    enum class DebuggerAction { None, Breakpoint, Watchpoint, WatchpointWrite, ExceptionPoint, Softstop, ModifiedCode, History, Line, Frame,
        DmaView, DmaLog, DmaWatch, AutoUpdate, UIRequestedStop, HaltCPU, MemROM, MemIO, MemCART };

    struct DebuggerSnapshot {
        unsigned themes = 0; // multiple themes
        DebuggerAction callbackAction;
        DebuggerTheme callbackTheme;
        uint32_t callbackAddress;
        bool codeMaybeModified = false;
        std::mutex mutex;
    };

    //callbacks
    struct Bind {
        virtual auto jitPoll(int) -> bool { return false; }
        virtual auto inputPoll(uint16_t, uint16_t) -> int16_t { return 0; }
		virtual auto videoRefresh8(const uint8_t*, unsigned, unsigned, unsigned) -> void {}
        virtual auto videoRefresh(const uint16_t*, unsigned, unsigned, unsigned, uint8_t) -> void {}
        virtual auto audioSample(int16_t, int16_t) -> void {}
        virtual auto audioFlush() -> void {}
        virtual auto readMedia(Media*, uint8_t*, unsigned, uint64_t) -> unsigned { return 0; }
        virtual auto writeMedia(Media*, uint8_t*, unsigned, uint64_t) -> unsigned { return 0; }
        virtual auto readAssignedMedia(Media*, uint8_t*&, bool) -> unsigned { return 0; }
        virtual auto writeAssignedMedia(Media*, uint8_t*, unsigned) -> unsigned { return 0; }
		virtual auto getFileNameFromMedia(Media*) -> std::string { return ""; }
        virtual auto unloadMedia(Media*) -> void {}
        virtual auto truncateMedia(Media* ) -> bool { return false; }
        virtual auto updateDeviceState(Media*, bool, unsigned, uint8_t, bool ) -> void {}
        virtual auto updateLedState(Emulator::Interface::LedId, uint8_t) -> void {}
        virtual auto log(std::string, bool) -> void {} //for debugging
        virtual auto exit( int code ) -> void {}
        virtual auto questionToWrite(Media*) -> bool { return false; }
        virtual auto hintAutoWarp(uint8_t) -> void {}
        virtual auto autoStartFinish(bool) -> void {}
        virtual auto mixDriveSound( Media*, DriveSound, bool, uint8_t ) -> void {}
        virtual auto jam(Media*) -> void {}
        virtual auto setThreadPriority(ThreadPriority, float, float) -> bool { return false; }
        virtual auto fpsChanged() -> void {}
        virtual auto trapsResult(Media*, bool error) -> void {}
        virtual auto libraryMissing(std::string) -> void {}
        virtual auto debugger(DebuggerSnapshot* snapshot) -> void {}
    };
    Bind* bind = nullptr;

    auto jitPoll(int delay) -> bool {
        return bind->jitPoll(delay);
    }
    
    auto inputPoll(uint16_t deviceId, uint16_t inputId) -> int16_t {
        return bind->inputPoll(deviceId, inputId);
    }
    
    auto audioSample(int16_t sampleLeft, int16_t sampleRight) -> void {
        bind->audioSample(sampleLeft, sampleRight);
    }

    auto audioFlush() -> void {
        bind->audioFlush();
    }

    auto videoRefresh(const uint16_t* frame, unsigned width, unsigned height, unsigned linePitch, uint8_t options) -> void {
        bind->videoRefresh(frame, width, height, linePitch, options);
    }
	
	auto videoRefresh8(const uint8_t* frame, unsigned width, unsigned height, unsigned linePitch) -> void {
        bind->videoRefresh8(frame, width, height, linePitch);
    }

    auto readMedia(Media* media, uint8_t* buffer, unsigned length, uint64_t offset) -> unsigned {
        return bind->readMedia(media, buffer, length, offset);
    }

    auto writeMedia(Media* media, uint8_t* buffer, unsigned length, uint64_t offset) -> unsigned {
        return bind->writeMedia(media, buffer, length, offset);
    }

    auto readAssignedMedia(Media* media, uint8_t*& buffer, bool preview) -> unsigned {
        return bind->readAssignedMedia(media, buffer, preview);
    }

    auto writeAssignedMedia(Media* media, uint8_t* buffer, unsigned length) -> unsigned {
        return bind->writeAssignedMedia(media, buffer, length);
    }
	
	auto getFileNameFromMedia(Media* media) -> std::string {
		return bind->getFileNameFromMedia(media);
	}

    auto unloadMedia(Media* media) -> void {
        bind->unloadMedia(media);
    }
    
    auto truncateMedia(Media* media) -> bool {
        return bind->truncateMedia( media );
    }
    
    auto updateDeviceState(Media* media, bool write, unsigned position, uint8_t LED, bool motorOff ) -> void {
        bind->updateDeviceState(media, write, position, LED, motorOff);
    }

    auto updateLedState(Emulator::Interface::LedId ledId, uint8_t state) -> void {
        bind->updateLedState(ledId, state);
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

    auto hintAutoWarp(uint8_t state) -> void {
        bind->hintAutoWarp( state );
    }

    auto autoStartFinish(bool soft) -> void {
        bind->autoStartFinish(soft);
    }

    auto mixDriveSound( Media* media, DriveSound driveSound, bool alternate = false, uint8_t data = 0 ) -> void {
        bind->mixDriveSound( media, driveSound, alternate, data );
    }

    auto jam( Media* media = nullptr ) -> void {
        bind->jam( media );
    }

    auto setThreadPriority(ThreadPriority priority, float minProcessingTimeInMilliSeconds = 0.0, float maxProcessingTimeInMilliSeconds = 0.0) -> bool {
        return bind->setThreadPriority( priority, minProcessingTimeInMilliSeconds, maxProcessingTimeInMilliSeconds);
    }

    auto trapsResult(Media* media, bool error) -> void {
        bind->trapsResult(media, error);
    }

    auto fpsChanged() -> void {
        bind->fpsChanged();
    }

    auto log(std::string data, bool newLine = true) -> void {
        bind->log(data, newLine);
    }

    auto libraryMissing(std::string ident) -> void {
        bind->libraryMissing(ident);
    }

    auto debugger(DebuggerSnapshot* snapshot) -> void {
        bind->debugger(snapshot);
    }

    template<typename T> auto log(T data, bool newLine = true, bool asHex = false) -> void {

        std::string out = std::to_string(data);	

        if (asHex) {
            out = "0x";
            char hex[8];

            snprintf( hex, 8, "%x", data );
            
            out += (std::string)hex;
        }
        bind->log(out, newLine);
    }

    // disk handling
    virtual auto insertDisk(Media* media, uint8_t* data, unsigned size) -> void {}
    virtual auto writeProtectDisk(Media* media, bool state) -> void {}
    virtual auto isWriteProtectedDisk(Media* media) -> bool { return false; }
    virtual auto ejectDisk(Media* media) -> void { }
	virtual auto createDiskImage(unsigned typeId, std::string name = "", bool hd = false, bool ffs = false, bool bootable = false) -> Data { return {nullptr, 0}; }
    virtual auto getDiskListing(Media* media) -> std::vector<Listing> { return {}; }
    virtual auto getDiskPreview(uint8_t* data, unsigned size, Media* media = nullptr) -> std::vector<Listing> { return {}; }
    virtual auto selectDiskListing(Media* media, unsigned pos, uint8_t options = 0) -> void { }
    virtual auto selectDiskListing(Media* media, std::string fileName, uint8_t options = 0) -> void { }
    virtual auto resetDrive(Media* media) -> void { }
    virtual auto hideDrive(Media* media) -> void { }
    
    // hard disk handling
    virtual auto insertHardDisk(Media* media, uint8_t* data, uint64_t size) -> void {} //uses read and write callbacks above because of big data
    virtual auto writeProtectHardDisk(Media* media, bool state) -> void {}
    virtual auto isWriteProtectedHardDisk(Media* media) -> bool { return false; }    
	virtual auto ejectHardDisk(Media* media) -> void {}
    virtual auto getHardDiskListing(Media* media) -> std::vector<Listing> { return {}; }
    virtual auto getHardDiskPreview(uint8_t* data, uint64_t size, Media* media) -> std::vector<Listing> { return {}; }
    virtual auto createHardDiskImage(uint64_t _size, bool vhd) -> Data { return { nullptr, 0 }; }
    // tape handling
    virtual auto insertTape(Media* media, uint8_t* data, unsigned size) -> void {}    
    virtual auto writeProtectTape(Media* media, bool state) -> void {}
    virtual auto isWriteProtectedTape(Media* media) -> bool { return false; }
    virtual auto ejectTape(Media* media) -> void { } 
    virtual auto controlTape(Media* media, TapeMode mode) -> void {}
    virtual auto getTapeControl(Media* media) -> TapeMode { return TapeMode::Unpressed; }
	virtual auto createTapeImage(unsigned& imageSize) -> uint8_t* { return nullptr; }
    virtual auto getTapeListing(Media* media) -> std::vector<Listing> { return {}; }
    virtual auto getTapePreview(uint8_t* data, unsigned size, Media* media = nullptr) -> std::vector<Listing> { return {}; }
    virtual auto selectTapeListing(Media* media, unsigned pos, uint8_t options = 0) -> void { }
    // expansion handling
    virtual auto insertExpansionImage(Media* media, uint8_t* data, unsigned size) -> void {}
    virtual auto ejectExpansionImage(Media* media) -> void {}
    virtual auto writeProtectExpansion(Media* media, bool state) -> void {}
    virtual auto isWriteProtectedExpansion(Media* media) -> bool { return false; }
    virtual auto createExpansionImage(MediaGroup* group, unsigned& imageSize, uint8_t id = 0) -> uint8_t* { return nullptr; }
    virtual auto isExpansionBootable() -> bool { return false; }
	virtual auto hasExpansionSecondaryRom() -> bool { return false; }
    virtual auto isExpansionUnsupported() -> bool { return false; }
    
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
    virtual auto analyzeExpansion(uint8_t* data, unsigned size, std::string suffix = "") -> Expansion* { return nullptr; }    
    virtual auto setExpansionJumper( Media* media, unsigned jumperId, bool state ) -> void {}
    virtual auto getExpansionJumper( Media* media, unsigned jumperId ) -> bool { return false; }
    virtual auto getExpansionPreview(uint8_t* data, unsigned size) -> std::vector<Listing> { return {}; }
    // cart buttons
    virtual auto hasFreezeButton() -> bool { return false; }
    virtual auto freezeButton() -> void {}
    virtual auto hasCustomCartridgeButton() -> bool { return false; }
    virtual auto customCartridgeButton() -> void {}

    // savestates
    virtual auto checkstate(uint8_t* data, unsigned size) -> bool { return false; }    
    virtual auto loadstate(uint8_t* data, unsigned size) -> bool { return false; }
    virtual auto savestate(unsigned& size) -> uint8_t* { return nullptr; }
    
    // model
    virtual auto setModelValue(unsigned modelId, int value) -> void {}
    virtual auto getModelValue(unsigned modelId) -> int { return 0; }
    virtual auto getModelIdOfEnabledDrives(MediaGroup* group) -> unsigned { return ~0; }
    virtual auto getModelIdOfCycleRenderer() -> unsigned { return ~0; }
    
    //controls
    virtual auto connect(unsigned connectorId, unsigned deviceId) -> void {}
    virtual auto connect(Connector* connector, Device* device) -> void {}
    // to avoid non objects for unplugged ports, you should use a placeholder device
    virtual auto getUnplugDevice() -> Device* { return &devices[ 0 ]; }  

    virtual auto getConnectedDevice( Connector* connector ) -> Device* { return getUnplugDevice(); }
    // for light devices you can disable cursor rendering by requesting cursor position in order to
    // draw cursor by yourself. that is usefully if you want to draw the cursor in a higher resolution.
    // of course, the coordinates are in native resolution of the emulated system. you need to convert them.
    virtual auto getCursorPosition( Device* device, int16_t& x, int16_t& y ) -> bool { return false; }
    // As a rule, retro systems query the status of the connected input devices. Some devices, such as the Amiga keyboard, take the initiative and send input to the computer.
    virtual auto needExternalKeyUpdates() -> bool { return false; }
    virtual auto informAboutKeyUpdate() -> void {}
    virtual auto sendKeyChange(bool pressed, Device::Input* input) -> void {}

    virtual auto setMemoryInitParams(MemoryPattern& pattern) -> void {}
    virtual auto getMemoryInitPattern( uint8_t* pattern ) -> void {}
    virtual auto getMemorySize() -> unsigned { return 0; }
    virtual auto setFirmware(unsigned typeId, uint8_t* data, unsigned size, bool allowPatching) -> void {}
    
    virtual auto power() -> void {} //hard reset
	virtual auto reset() -> void {} //soft reset
	virtual auto powerOff() -> void {} //shutdown system
    virtual auto run() -> void {} //emulate one frame
    virtual auto runAhead(unsigned frames) -> void {}
    virtual auto runAheadPerformance(bool state) -> void {}
	virtual auto runAheadPreventJit(bool state) -> void {}
    virtual auto getRegionEncoding() -> Region { return Region::Pal; }
	virtual auto getRegionGeometry() -> Region { return Region::Pal; }
	virtual auto getSubRegion() -> SubRegion { return SubRegion::Pal_B; }
    
    //crop
	virtual auto cropFrame( CropType type, Crop crop ) -> void {}
    virtual auto cropWidth() -> unsigned { return 0; }
    virtual auto cropHeight() -> unsigned { return 0; }
    virtual auto cropTop() -> unsigned { return 0; }
    virtual auto cropLeft() -> unsigned { return 0; }
    virtual auto cropCoordUpdated(unsigned& top, unsigned& left) -> bool { return false; }
    virtual auto cropData() -> uint8_t* { return nullptr; }
    virtual auto cropData16() -> uint16_t* { return nullptr; }
    virtual auto cropPitch() -> unsigned { return 0; }
    virtual auto cropOptions() -> uint8_t { return 0; }
    // for taking screenshots with different crop than the visible one
    virtual auto cropAlternatively(unsigned& width, unsigned& height, unsigned& pitch) -> uint8_t* { return nullptr; }
    
    virtual auto videoAddMeta(bool state) -> void {}

    virtual auto setWarpMode(unsigned config) -> void {}

	// copy/paste
	virtual auto pasteText( std::string buffer ) -> void {}
	virtual auto copyText() -> std::string { return ""; }

    // jit
    virtual auto setInputSampling(uint8_t mode) -> void {}

    // drive sounds
    virtual auto enableFloppySounds(bool state) -> void {}
    virtual auto enableTapeSounds(bool state) -> void {}
    virtual auto setTapeLoadingNoise(unsigned volume) -> void {}

    // rewind
    virtual auto configRewind(unsigned steps, unsigned maxSizeInMb) -> void {}
    virtual auto setRewind(bool state) -> void {}

    virtual auto autoStartedByMediaGroup() -> MediaGroup* { return nullptr; }
    
    auto getStatsForSelectedRegion() -> Stats& {  
        return stats;
    }

    virtual auto requestImmediateReturn() -> void {}

    // sockets
    virtual auto prepareSocket( Media* media, std::string address, std::string port ) -> void {}
    
    // a ratio of 1.0 means monitor refresh rate is equal to emulated system original speed
    virtual auto setMonitorFpsRatio(double ratio) -> void {}

    // debugger
    virtual auto debuggerAdd(DebuggerTheme theme, DebuggerAction action, unsigned addr, unsigned addrTo = 0) -> void {}
    virtual auto debuggerRemove(DebuggerTheme theme, DebuggerAction action, std::optional<unsigned> addr = std::nullopt) -> void {}
    virtual auto setWatchpointCondition(DebuggerTheme theme, DebuggerAction action, unsigned addr, unsigned hitCount, unsigned hitCountMode, const std::string& expression, unsigned expressionMode) -> bool { return true; }

    virtual auto debuggerStepOver(DebuggerTheme theme, bool subroutineOnly) -> void {}
    virtual auto debuggerStepInto(DebuggerTheme theme) -> void {}
    virtual auto debuggerStepOut(DebuggerTheme theme) -> bool { return false; }

    virtual auto getMemoryDumpBank(uint8_t bank, uint16_t* dump) -> void {}
    virtual auto getMemoryDumpBank(uint8_t bank, uint8_t* dump) -> void {}
    virtual auto getMemoryDumpPage(DebuggerTheme theme, uint8_t page, uint8_t* dump) -> void {}
    virtual auto getMemory(DebuggerTheme theme, DebuggerAction action, unsigned startAddr, unsigned endAddr, uint8_t* dump) -> void {}
    virtual auto getDmaDump() -> uint8_t* { return nullptr; }

    virtual auto editMemory(DebuggerTheme theme, uint32_t addr, std::vector<uint16_t> values) -> void {}

    // disassembler
    virtual auto disassemble(DebuggerTheme theme, unsigned addr, unsigned& bytes) -> std::string { return ""; }
    virtual auto disassembleData(DebuggerTheme theme, unsigned addr, unsigned bytes) -> std::string { return ""; }
    virtual auto disassembleTrace(DebuggerTheme theme, unsigned i, uint16_t& flags) -> std::string { return ""; }
    
	//shortcuts
	auto insertMedium(Media* media, uint8_t* data, uint64_t size) -> void {
		switch(media->group->type) {
			case MediaGroup::Type::Disk: insertDisk(media, data, (unsigned)size); break;
			case MediaGroup::Type::Tape: insertTape(media, data, (unsigned)size); break;
			case MediaGroup::Type::Expansion: insertExpansionImage(media, data, (unsigned)size); break;
			case MediaGroup::Type::Program: insertProgram(media, data, (unsigned)size); break;
			case MediaGroup::Type::HardDisk: insertHardDisk(media, data, size); break;
		}
	}
	
	auto writeProtect(Media* media, bool state) -> void {
		switch(media->group->type) {
			case MediaGroup::Type::Disk: writeProtectDisk(media, state); break;
			case MediaGroup::Type::Tape: writeProtectTape(media, state); break;
			case MediaGroup::Type::Expansion: writeProtectExpansion(media, state); break;
			case MediaGroup::Type::Program: break;
			case MediaGroup::Type::HardDisk: writeProtectHardDisk(media, state); break;
		}
	}

	auto isWriteProtected(Media* media) -> bool {
		switch(media->group->type) {
			case MediaGroup::Type::Disk: return isWriteProtectedDisk(media);
			case MediaGroup::Type::Tape: return isWriteProtectedTape(media);
			case MediaGroup::Type::Expansion: return isWriteProtectedExpansion(media);
			case MediaGroup::Type::Program: break;
			case MediaGroup::Type::HardDisk: return isWriteProtectedHardDisk(media);
		}
        return false;
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
			case MediaGroup::Type::Tape:
                return getTapeListing( media );
			case MediaGroup::Type::Program:
                return getProgramListing( media );
			case MediaGroup::Type::Expansion: break;
			case MediaGroup::Type::HardDisk:
                return getHardDiskListing( media );
		}	
        return {};
    }
    
    auto selectListing(Media* media, unsigned position, std::string fileName = "", uint8_t options = 0) -> bool {
        switch(media->group->type) {
			case MediaGroup::Type::Disk:
			    if (!fileName.empty() && (position == 0) )
                    selectDiskListing( media, fileName, options );
			    else
                    selectDiskListing( media, position, options );
                return true;
			case MediaGroup::Type::Tape:
                selectTapeListing( media, position, options );
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

    auto getEnabledDisk( unsigned mediaId ) -> Media* {
        for(auto& mediaGroup : mediaGroups) {
            if (mediaGroup.isDisk()) {
                unsigned enabledCount = getModelValue(getModelIdOfEnabledDrives(&mediaGroup));
                if (enabledCount > mediaGroup.media.size())
                    enabledCount = mediaGroup.media.size();

                if (enabledCount > mediaId)
                    return &mediaGroup.media[mediaId];
                return &mediaGroup.media[0];
            }
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

    auto getPRG( unsigned mediaId ) -> Media* {
        for(auto& mediaGroup : mediaGroups)
            if (mediaGroup.isProgram()) {
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

    auto getDriveMediaGroups() -> std::vector<MediaGroup*> {
        std::vector<MediaGroup*> groups;

        for (auto& group : mediaGroups) {
            if (group.isDrive())
                groups.push_back(&group);
        }
        return groups;
    }

	auto getDiskMediaGroup() -> MediaGroup* {
		for (auto& group : mediaGroups) {
			
			if (group.isDisk())
				return &group;
		}
		
		return nullptr;
	}

    auto getHardDiskMediaGroup() -> MediaGroup* {
        for (auto& group : mediaGroups) {

            if (group.isHardDisk())
                return &group;
        }

        return nullptr;
    }

    auto getTapeMediaGroup() -> MediaGroup* {
        for (auto& group : mediaGroups) {

            if (group.isTape())
                return &group;
        }

        return nullptr;
    }

    auto getPRGMediaGroup() -> MediaGroup* {
        for (auto& group : mediaGroups) {

            if (group.isProgram())
                return &group;
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

    auto getModel( unsigned modelId ) -> Model* {
        for(auto& model : models) {
            if (model.id == modelId)
                return &model;
        }
        return nullptr;
    }
};

}
