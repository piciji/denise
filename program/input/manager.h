
#pragma once

#include "../../driver/driver.h"
#include "../../guikit/api.h"
#include "../../emulation/interface.h"
#include "../states/states.h"
#include "../media/media.h"

//#define DEBUG_INPUT_CHANGE

struct InputManager;

struct InputMapping {
    enum Type : unsigned { Digital = 0, Analog = 1, Switch = 2 } type;
	enum Qualifier : unsigned { None, Lo, Hi };		
    std::vector<InputMapping*> shadowMap;
    uint8_t analogTimer = 0;
	
	struct Assign {		
		Hid::Device* device;
		Hid::Group* group;
		Hid::Input* input;	
		unsigned qualifier;
        bool disable;
	};
	
    InputMapping* alternate = nullptr;
    InputMapping* parent = nullptr;
    InputMapping* sortedNext = nullptr;
    
    Emulator::Interface::Device* emuDevice = nullptr;
	unsigned hotkeyId;
	
    InputManager* inputManager = nullptr;
	std::vector<Assign> hids; // multiple mappings
	bool anded = false; // mappings linked together as and/or 	
	GUIKIT::Setting* setting;
    int16_t state;
    bool hasUnknownAssignment = false;
    int analogSensitivity = 16384;
	
	auto isAnalog() const -> bool { return type == Analog; }
    auto isSwitch() const -> bool { return type == Switch; }
	auto checkSanity(Hid::Device* device, unsigned groupId, unsigned inputId) -> unsigned;
	auto updateSetting() -> void;
    template<bool useOldValue> auto adjustDigitalValue( Assign& hid ) -> int16_t;
    template<bool lightMode = false> auto adjustAnalogValue( Assign& hid ) -> int16_t;
	auto informChange(Assign& hid) -> void;
	auto getDescription() -> std::string;
	auto swapLinker() -> void;
	auto init() -> void;
    auto applyMouseSensitivity( int16_t value ) -> int16_t;
    auto applyAxisSensitivity( int16_t value ) -> int16_t;
    auto sortHidsByValue() -> void;
    auto generateAlternate(GUIKIT::Settings* settingContainer) -> void;
};

struct Hotkey {
    enum Id : unsigned { Pause, Fullscreen, CaptureMouse, DiskSwapper, Software, States,
		Savestate, Loadstate, IncSlot, DecSlot, ToggleMenu, ToggleStatus, 
		ToggleSidFilter, SwapSid, DigiBoost, AdjustBiasUp, AdjustBiasDown,
		PlayTape, RecordTape, StopTape, ForwardTape, RewindTape, ResetTapeCounter,
		FloppyAccess,
		DiskSwap0, DiskSwap1, DiskSwap2, DiskSwap3, DiskSwap4, DiskSwap5, DiskSwap6,
        DiskSwap7, DiskSwap8, DiskSwap9, DiskSwap10, DiskSwap11,
        DiskSwap12, DiskSwap13, DiskSwap14,
        ToggleFastForward, ToggleFastForwardAggressive, Presentation, Palette, Border, System, Firmware, Control,
		SwapInputDevices, Power, SoftReset, AnyLoad,
        RunAheadUp, RunAheadDown, RunAheadToggleMode, AudioRecord, ToggleRenderer
    } id;
    std::string name;
	bool share;
    uintptr_t guid;
};

struct KeyboardLayout {
    enum Type { Uk, Us, Fr, De } type;
    
    std::string language;
    std::string code;
};

struct InputManager {
        
    static std::vector<KeyboardLayout> keyboardLayouts;
    
    InputManager(Emulator::Interface* emulator = nullptr);
        
	static const unsigned MaxMappings = 4;
	static std::vector<Hotkey> hotkeys;    
    static InputMapping* captureObject;
    static unsigned retry;
	static std::vector<Hid::Device*> hidDevices;
    static bool urgentUpdate;
    
    struct JIT { // Just In Time Polling
        uint64_t lastTimestamp = 0;
        bool enable = false;
    }; 
    
    static JIT jit;
    
    struct DeviceRemap {
        Hid::Device* remember;
        Hid::Device* current;
    };
    
    static std::vector<DeviceRemap> remapDevices;
        
    struct {
        bool updated = false;
        GUIKIT::Position pos;
    } uiMouse;
    
	std::vector<Hotkey> customHotkeys;
	Emulator::Interface* emulator = nullptr;
	std::vector<InputMapping*> mappings;
    std::vector<InputMapping*> mappingsInUse;
    std::vector<InputMapping*> andTriggers;

	static std::vector<InputMapping*> hotkeyTriggers;
    static bool driverChange;
	
	static auto getManager( Emulator::Interface* emulator ) -> InputManager*;
	static auto build() -> void;
	static auto init() -> void;
	static auto setHotkeys() -> void;
	static auto setMappings() -> void;    
	static auto autoAssignHotkeys() -> void;
    static auto automap( KeyboardLayout::Type type, Emulator::Interface::Key key ) -> std::vector<std::vector<Hid::Key>>;
	static auto bindHids() -> void;
	static auto capture(InputMapping* _captureObject) -> void;
    static auto capture( bool overwriteExisting = false ) -> bool;
	static auto fetch() -> void;
	static auto poll() -> void;
	static auto pollHotkeys() -> void;
	static auto activateHotkey(Hotkey::Id id, Emulator::Interface* emulator = nullptr) -> void;
    static auto fireHotkey(Emulator::Interface* emulator, Hotkey::Id id) -> void;
	static auto unmapHotkeys() -> void;
    static auto assumeLayoutType() -> KeyboardLayout::Type;
    static auto rememberLastDeviceState() -> void;
    static auto clearLastDeviceState() -> void;
    static auto assignChangedDeviceState() -> void;
    static auto getDeviceFromIdent( unsigned id ) -> Hid::Device*;
    static auto openMenu( Emulator::Interface* emulator, Hotkey::Id id ) -> void;
	static auto updateAllMappingsInUse( bool emuOnly = false ) -> void;
    static auto jitPoll() -> bool;
    static auto resetJit() -> void;    
	
    auto autoAssign( KeyboardLayout::Type type, bool keyboardOnly = true ) -> void;
	auto addMapping(InputMapping* mapping) -> void;
    auto addMappingInUse(InputMapping* mapping) -> void;
    auto update() -> void;
    auto unmapDevice(unsigned deviceId) -> void;    
    auto initMapping(InputMapping* mapping) -> void;
	auto sort() -> void;				
    auto updateMappingsInUse() -> void;
    auto matchButtons( Emulator::Interface::Device::Input* emuInput, Hid::Input* hidInput ) -> bool;
    auto priorizeConnectedDevicesOverKeyboard() -> void;
    auto alternateSort() -> void;
    auto updateAnalogSensitivity(Emulator::Interface::Device* updateDevice = nullptr) -> void;
	auto setCustomHotkeys() -> void;
	auto unmapCustomHotkeys() -> void;        
    
    inline auto updateAndTrigger() -> void;
    inline auto addAndTrigger(InputMapping* newTrigger) -> void;
    inline auto blockedByAndTrigger(InputMapping::Assign& hid) -> bool;
};

extern std::vector<InputManager*> inputManagers;
