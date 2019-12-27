
#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include "../../driver/driver.h"
#include "../../guikit/api.h"
#include "../../emulation/interface.h"
#include "../states/states.h"

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
    InputManager* inputManager = nullptr;
	std::vector<Assign> hids; // multiple mappings
	bool anded = false; // mappings linked together as and/or 	
	GUIKIT::Setting* setting;
    int16_t state;
	
	auto isAnalog() -> bool { return type == Analog; }
    auto isSwitch() -> bool { return type == Switch; }
	auto checkSanity(Hid::Device* device, unsigned groupId, unsigned inputId) -> unsigned;
	auto updateSetting() -> void;
    template<bool useOldValue> auto adjustDigitalValue( Assign& hid ) -> int16_t;
    template<bool lightMode = false> auto adjustAnalogValue( Assign& hid ) -> int16_t;
	auto informChange(Assign& hid) -> void;
	auto getDescription() -> std::string;
	auto swapLinker() -> void;
	auto init() -> void;
    auto applyMouseSense( int16_t value ) -> int16_t;
    auto applyAnalogSense( int16_t value ) -> int16_t;
    auto sortHidsByValue() -> void;
    auto generateAlternate() -> void;
};

struct Hotkey {
    enum Id : unsigned { Pause, Fullscreen, CaptureMouse, DiskSwapper, Drives, States,
		Savestate, Loadstate, IncSlot, DecSlot, ToggleMenu, ToggleStatus, 
		ActivateFilter, SwapSid, DigiBoost, AdjustBiasUp, AdjustBiasDown,
		PlayTape, RecordTape, StopTape, ForwardTape, RewindTape, ResetTapeCounter,
		FloppyAccess,
		DiskSwap0, DiskSwap1, DiskSwap2, DiskSwap3, DiskSwap4, DiskSwap5, DiskSwap6,
        DiskSwap7, DiskSwap8, DiskSwap9, DiskSwap10, DiskSwap11,
        DiskSwap12, DiskSwap13, DiskSwap14,
        ToggleFastForward, ToggleFastForwardAggressive, Video, Palette, Border, System, Firmware, Input
    } id;
    std::string name;
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
    
    struct DeviceRemap {
        Hid::Device* remember;
        Hid::Device* current;
    };
    
    static std::vector<DeviceRemap> remapDevices;
        
    struct {
        bool updated = false;
        GUIKIT::Position pos;
    } uiMouse;
    
	Emulator::Interface* emulator = nullptr;
	std::vector<InputMapping*> mappings;
    std::vector<InputMapping*> mappingsInUse;
    InputMapping* andTrigger = nullptr;
    static std::vector<Hotkey::Id> hotkeyTriggers;
	
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
	static auto activateHotkey(Hotkey::Id id) -> void;
    static auto fireHotkey(Hotkey::Id id) -> void;
	static auto unmapHotkeys() -> void;
    static auto assumeLayoutType() -> KeyboardLayout::Type;
    static auto rememberLastDeviceState() -> void;
    static auto assignChangedDeviceState() -> void;
    static auto getDeviceFromIdent( unsigned id ) -> Hid::Device*;
    static auto openMenu( Hotkey::Id id ) -> void;
	
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
};

extern std::vector<InputManager*> inputManagers;

#endif
