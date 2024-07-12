
#pragma once

#include "../../guikit/api.h"
#include "message.h"

struct View : GUIKIT::Window {
    Message* message;
	GUIKIT::Timer anyloadTimer;
	GUIKIT::Timer displayChangeTimer;
	GUIKIT::Timer fullscreenOnStartUp;
    GUIKIT::Timer cursorHideTimer;
    GUIKIT::StatusBar statusBar;
    GUIKIT::Image placeholder;
	bool requestFullscreenSwitch = false;
    bool customResizeMode = false;
    int dropZone = 0;
	bool grabMouseLeft = false;

    struct ShaderFavourites {
        std::string path;
        GUIKIT::MenuRadioItem* item;
    };

    struct SystemMenu {
        Emulator::Interface* emulator;
        GUIKIT::Menu* system;
        GUIKIT::MenuItem* poweron;
		GUIKIT::MenuItem* poweronAndRemoveExpansions;
        GUIKIT::MenuItem* poweronAndRemoveDisks;
		GUIKIT::MenuItem* reset;
        GUIKIT::MenuItem* freeze;
        GUIKIT::MenuItem* powerLED;
        GUIKIT::MenuItem* menu;
        GUIKIT::MenuItem* firmware;
        GUIKIT::MenuItem* loadSoftware;
        GUIKIT::MenuItem* media;
        GUIKIT::Menu* states;
    		GUIKIT::MenuItem* save;
    		GUIKIT::MenuItem* slotUp;
    		GUIKIT::MenuItem* slotDown;
    		GUIKIT::MenuItem* load;
        GUIKIT::MenuItem* systemManagement;
        GUIKIT::MenuItem* audio;
        GUIKIT::MenuItem* configurations;
        GUIKIT::Menu* shaderMenu;
        GUIKIT::MenuItem* presentation;
        GUIKIT::MenuItem* palette;
        GUIKIT::MenuItem* geometry;
        GUIKIT::MenuItem* misc;

        std::vector<ShaderFavourites> shaderFavourites;
    };

    auto translate() -> void;
    auto getViewportHandle(bool driverChange) -> uintptr_t;

    auto build() -> void;
    auto setConnectors() -> void;
    auto checkInputDevice( Emulator::Interface* emulator, Emulator::Interface::Connector* connector, Emulator::Interface::Device* device ) -> void;
    auto updateDeviceSelection( Emulator::Interface* emulator ) -> void;
	auto removeMenuTree( GUIKIT::Menu* menu = nullptr ) -> void;
	auto showTapeMenu( bool show, Emulator::Interface::TapeMode mode = Emulator::Interface::TapeMode::Unpressed ) -> void;
    auto updateTapeIcons( Emulator::Interface::TapeMode mode = Emulator::Interface::TapeMode::Unpressed ) -> void;
    auto updateTapeStatusIcons( Emulator::Interface::TapeMode mode ) -> void;
    auto updateCartButtons( Emulator::Interface* emulator ) -> void;
	auto setAnyload(Emulator::Interface* emulator) -> void;
	auto prepareCursorHide(unsigned interval, bool withFocus = false) -> void;

    auto buildMenu() -> void;
    auto updateViewport() -> void;
	auto updateShader(Emulator::Interface* emulator) -> void;
    auto buildShader() -> void;
	auto switchFullScreen(bool fullScreen = true, bool forceUnacquire = false) -> void;
    auto updateMenuBar( bool toggle = false ) -> void;
    auto updateStatusBar(bool toggle = false ) -> void;
    auto loadCursor() -> void;
    auto setCursor( Emulator::Interface* emulator ) -> void;
    auto setDragnDrop() -> void;
    auto getSysMenu( Emulator::Interface* emulator ) -> SystemMenu*;
    auto cursorForPlaceholderInUpperTriangle(GUIKIT::Position p) -> int;
    auto cursorForPlaceholderInUpperTriangle() -> int;
    auto loadImages() -> void;
    
    auto loadPlaceholder() -> void;
    auto renderPlaceholder() -> bool;
    auto loadDragnDropOverlay() -> void;
    auto togglePause() -> void;
    auto updatePauseCheck() -> void;
    auto updateFastforwardCheck() -> void;
    auto updateEmuUsage() -> void;
    auto updateDiskMenu() -> void;
	auto updateMouseGrab() -> void;

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
        GUIKIT::MenuItem driversItem;
		GUIKIT::MenuItem settingsItem;

        GUIKIT::MenuCheckItem videoSyncItem;
        GUIKIT::MenuCheckItem vrrItem;
        GUIKIT::MenuCheckItem dynamicRateControl;

        GUIKIT::MenuItem fullscreenItem;
    
        GUIKIT::MenuCheckItem muteItem;
        GUIKIT::Menu statusTextMenu;
            GUIKIT::MenuRadioItem screenStatusEnabledItem;
            GUIKIT::MenuRadioItem screenStatusDisabledItem;
            GUIKIT::MenuRadioItem screenStatusAutoItem;

            GUIKIT::MenuCheckItem fpsItem;
			GUIKIT::MenuCheckItem volumeItem;
            GUIKIT::MenuCheckItem audioBufferItem;

        GUIKIT::MenuItem saveItem;	

		GUIKIT::MenuItem exit; 

	GUIKIT::Menu tapeControlMenu;
        GUIKIT::MenuItem insertTapeItem;
        GUIKIT::MenuItem ejectTapeItem;
        GUIKIT::MenuItem tapePlayItem;
		GUIKIT::MenuItem tapeStopItem;
		GUIKIT::MenuItem tapeForwardItem;
		GUIKIT::MenuItem tapeRewindItem;
		GUIKIT::MenuItem tapeRecordItem;
		GUIKIT::MenuItem tapeResetCounterItem;

    GUIKIT::Menu speedControlMenu;
        GUIKIT::MenuCheckItem fastForwardItem;
        GUIKIT::MenuCheckItem aggressiveFastForwardItem;
        GUIKIT::MenuCheckItem pauseItem;
        std::vector<GUIKIT::MenuRadioItem*> speedItems;
        GUIKIT::MenuRadioItem maximumSpeedItem;
        GUIKIT::MenuItem customizeSpeedItem;

    struct {
        GUIKIT::Menu menu;
        GUIKIT::MenuItem insert;
        GUIKIT::MenuItem eject;
        GUIKIT::MenuItem reset;
        GUIKIT::MenuItem inactive;
    	GUIKIT::MenuItem clearSave;
    } diskControlMenus[4];

    struct {
        GUIKIT::Menu menu;
        GUIKIT::MenuItem power;
        GUIKIT::MenuItem reset;
        std::vector<GUIKIT::MenuRadioItem*> filters;
    } power;
		
    GUIKIT::Image powerImage;
    GUIKIT::Image freezeImage;
    GUIKIT::Image menuImage;
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
	GUIKIT::Image editImage;
    GUIKIT::Image fanImage;
    GUIKIT::Image hideImage;
    GUIKIT::Image fullscreenImage;
	GUIKIT::Image gearsImage;
	GUIKIT::Image infoImage;
    
    GUIKIT::Image playImage;
    GUIKIT::Image playhiImage;
    GUIKIT::Image stopImage;
    GUIKIT::Image stophiImage;
    GUIKIT::Image recordImage;
    GUIKIT::Image recordhiImage;
    GUIKIT::Image forwardImage;
    GUIKIT::Image forwardhiImage;
    GUIKIT::Image rewindImage;
    GUIKIT::Image rewindhiImage;
	GUIKIT::Image counterImage;

    GUIKIT::Image playStatusImage;
    GUIKIT::Image playPauseStatusImage;
    GUIKIT::Image recordStatusImage;
    GUIKIT::Image stopStatusImage;
    GUIKIT::Image forwardStatusImage;
    GUIKIT::Image forwardPauseStatusImage;
    GUIKIT::Image rewindStatusImage;
    GUIKIT::Image rewindPauseStatusImage;
    GUIKIT::Image recordPauseStatusImage;
    GUIKIT::Image ejectImage;
    
    GUIKIT::Image pencilImage;
    GUIKIT::Image crosshairImage;
    
    GUIKIT::Image ledOffImage;
    GUIKIT::Image ledRedImage;
    GUIKIT::Image ledGreenImage;

    GUIKIT::Image ledGreen2Image;
    GUIKIT::Image ledGreen2DimImage;
    GUIKIT::Image ledRed2Image;
    GUIKIT::Image ledYellowImage;

	GUIKIT::Image delImage;
            	
    auto questionToWrite(Emulator::Interface::Media* media) -> bool;
    auto updateSpeedLabels() -> void;
    auto updatePowerMenu() -> void;
    auto getSpeedBySelectedProfile(float& speed, bool& percent) -> unsigned;
    auto getSpeed(unsigned pos, float& speed, bool& percent) -> void;
    auto isMaximumSpeed() -> bool;
    auto isCustomSpeed() -> bool;
	auto activateCustomSpeed() -> void;
	auto threadedRendererWasToggled(bool state) -> void;
    auto updateGeometry(bool withViewport = false) -> void;
    auto adjustToEmu(bool withViewport) -> void;
    
    View();
};

extern View* view;
