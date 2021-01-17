
#pragma once

#include "../../guikit/api.h"
#include "message.h"

struct View : public GUIKIT::Window {
    Message* message;
	GUIKIT::Timer placeholderTimer;
	GUIKIT::Timer anyloadTimer;
    GUIKIT::StatusBar statusBar;
    GUIKIT::Image placeholder;    
        
    struct SystemMenu {
        Emulator::Interface* emulator;
        GUIKIT::Menu* system;
        GUIKIT::MenuItem* poweron;
		GUIKIT::MenuItem* reset;
        GUIKIT::MenuItem* freeze;
        GUIKIT::MenuItem* firmware;
        GUIKIT::MenuItem* loadSoftware;
        GUIKIT::MenuItem* media;
        GUIKIT::MenuItem* systemManagement;
        GUIKIT::MenuItem* audio;
        GUIKIT::MenuItem* configurations;
        GUIKIT::Menu* shaderMenu;
        GUIKIT::MenuItem* presentation;
        GUIKIT::MenuItem* palette;
        GUIKIT::MenuItem* border;		       
    };

    auto translate() -> void;
    auto show() -> void ;
    auto getViewportHandle() -> uintptr_t;

    auto build() -> void;
	auto update() -> void;
    auto setConnectors() -> void;
    auto checkInputDevice( Emulator::Interface* emulator, Emulator::Interface::Connector* connector, Emulator::Interface::Device* device ) -> void;
    auto updateDeviceSelection( Emulator::Interface* emulator ) -> void;
	auto removeMenuTree( GUIKIT::Menu* menu = nullptr ) -> void;
	auto showTapeMenu( bool show, Emulator::Interface::TapeMode mode = Emulator::Interface::TapeMode::Unpressed ) -> void;
    auto updateTapeIcons( Emulator::Interface::TapeMode mode = Emulator::Interface::TapeMode::Unpressed ) -> void;
    auto updateTapeStatusIcons( Emulator::Interface::TapeMode mode ) -> void;
    auto updateFreeze( Emulator::Interface* emulator ) -> void;
	auto setAnyload(Emulator::Interface* emulator) -> void;

    auto buildMenu() -> void;
    auto updateViewport() -> void;
	auto updateShader() -> void;
	auto setFullScreen(bool fullScreen = true) -> void;
	auto exclusiveFullscreen() -> bool;
    auto updateMenuBar( bool toggle = false ) -> void;
    auto updateStatusBar(bool toggle = false ) -> void;
    auto loadCursor() -> void;
    auto setCursor( Emulator::Interface* emulator ) -> void;
    auto setDragnDrop() -> void;
    auto getSysMenu( Emulator::Interface* emulator ) -> SystemMenu*;
    auto cursorForPlaceholderInUpperTriangle(GUIKIT::Position& p) -> bool;
    auto cursorForPlaceholderInUpperTriangle() -> bool;
    auto loadImages() -> void;
    
    auto loadPlaceholder() -> void;
    auto renderPlaceholder(bool blackScreen = false) -> void;
           
    GUIKIT::Viewport viewport;    
    
    std::vector<SystemMenu> sysMenus;
    
    struct InputDevice {
        Emulator::Interface::Connector* connector;
        Emulator::Interface::Device* device;
        GUIKIT::MenuRadioItem* item;
    };
    
    struct InputMenu {
        Emulator::Interface* emulator;
        
        std::vector<InputDevice> inputDevices;
    };

    std::vector<InputMenu> inputMenus;

    GUIKIT::Menu editMenu;
        GUIKIT::MenuItem copyItem;
        GUIKIT::MenuItem pasteItem;

    GUIKIT::Menu controlMenu;
    GUIKIT::Menu optionsMenu;
			
		GUIKIT::MenuItem globalAudioItem;
        GUIKIT::MenuItem globalVideoItem;
        GUIKIT::MenuItem globalInputItem;

        GUIKIT::MenuCheckItem videoSyncItem;
        GUIKIT::MenuCheckItem audioSyncItem;        
        GUIKIT::MenuCheckItem fpsLimitItem;      
        GUIKIT::MenuCheckItem dynamicRateControl;

        GUIKIT::MenuItem fullscreenItem;
    
        GUIKIT::MenuCheckItem muteItem;
        GUIKIT::MenuCheckItem fpsItem;
        GUIKIT::MenuCheckItem audioBufferItem;

        GUIKIT::MenuItem settingsItem;		
        GUIKIT::MenuItem saveItem;	
		
		GUIKIT::MenuItem poweroff;
		GUIKIT::MenuItem exit; 

	GUIKIT::Menu tapeControlMenu;
		GUIKIT::MenuItem tapePlayItem;
		GUIKIT::MenuItem tapeStopItem;
		GUIKIT::MenuItem tapeForwardItem;
		GUIKIT::MenuItem tapeRewindItem;
		GUIKIT::MenuItem tapeRecordItem;
		GUIKIT::MenuItem tapeResetCounterItem;
		
    GUIKIT::Image regionImage;
    GUIKIT::Image powerImage;
    GUIKIT::Image poweroffImage;
    GUIKIT::Image freezeImage;
    GUIKIT::Image firmwareImage;
    GUIKIT::Image driveImage;
    GUIKIT::Image swapperImage;
    GUIKIT::Image systemImage;
    GUIKIT::Image scriptImage;
    GUIKIT::Image joystickImage;
    GUIKIT::Image volumeImage;
    GUIKIT::Image plugImage;
    GUIKIT::Image displayImage;
    GUIKIT::Image toolsImage;
	GUIKIT::Image quitImage;
	GUIKIT::Image keyboardImage;
	GUIKIT::Image colorImage;
    GUIKIT::Image paletteImage;
    GUIKIT::Image cropImage;
	GUIKIT::Image tapeImage;
    GUIKIT::Image diskImage;
    
    GUIKIT::Image playImage;
    GUIKIT::Image playhiImage;
    GUIKIT::Image playhiPauseImage;
    GUIKIT::Image stopImage;
    GUIKIT::Image stophiImage;
    GUIKIT::Image recordImage;
    GUIKIT::Image recordhiImage;
    GUIKIT::Image forwardImage;
    GUIKIT::Image forwardhiImage;
    GUIKIT::Image rewindImage;
    GUIKIT::Image rewindhiImage;
	GUIKIT::Image counterImage;
    
    GUIKIT::Image pencilImage;
    GUIKIT::Image crosshairImage;
    
    GUIKIT::Image ledOffImage;
    GUIKIT::Image ledRedImage;
    GUIKIT::Image ledGreenImage;
            	
    auto questionToWrite(Emulator::Interface::Media* media) -> bool;
    
    View();
};

extern View* view;
