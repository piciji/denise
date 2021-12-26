
#include "view.h"
#include "../program.h"
#include "../config/config.h"
#include "../emuconfig/config.h"
#include "../input/manager.h"
#include "../config/archiveViewer.h"
#include "../audio/manager.h"
#include "../cmd/cmd.h"
#include "status.h"
#include "../media/autoloader.h"
#include "../media/fileloader.h"
#include "placeholder.cpp"
#include "../../data/icons.h"

View* view = nullptr;

View::View() {
    message = new Message(this);
}

auto View::build() -> void {
    setTitle( APP_NAME " " VERSION );
    setBackgroundColor(0);
    cocoa.setDisableIconsInTopMenu(true);
    
    GUIKIT::Geometry defaultGeometry = {100, 100, 600, 400};
    
    GUIKIT::Geometry geometry = {globalSettings->get<int>("screen_x", defaultGeometry.x)
        ,globalSettings->get<int>("screen_y", defaultGeometry.y)
        ,globalSettings->get<unsigned>("screen_width", defaultGeometry.width)
        ,globalSettings->get<unsigned>("screen_height", defaultGeometry.height)
    };
    
    setGeometry( geometry );
    
    if (isOffscreen())        
        setGeometry( defaultGeometry );    
    
    append(viewport);
    
    loadImages();

    statusBar.setFont( GUIKIT::Font::system() );
        
	statusHandler->init(&statusBar);

    append(statusBar);
		
    if (!cmd->noGui) {                
        
        buildMenu();    
        updateShader();
        translate();

        updateMenuBar();
        updateStatusBar();
        loadCursor();
    }
    
    onClose = []() {
        archiveViewer->setVisible(false);
        if (configView)
            configView->setVisible(false);
        
		for(auto emuView : emuConfigViews)
            emuView->setVisible(false);
			        
        program->quit();
        GUIKIT::Application::quit();
    };
    
    onMove = [this]() { 
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        globalSettings->set<int>("screen_x", geometry.x);
        globalSettings->set<int>("screen_y", geometry.y);
		audioDriver->clear();
    };
    
    onSize = [this]() {
        if (fullScreen()) {
			if (view->exclusiveFullscreen())
				setStatusVisible( false );
			else
				updateStatusBar();
			
            setMenuVisible(false);
        } else {
            updateMenuBar();
            updateStatusBar();
            
            GUIKIT::Geometry geometry = this->geometry();
            globalSettings->set<int>("screen_width", geometry.width);
            globalSettings->set<int>("screen_height", geometry.height);
        }
        updateViewport();
		audioDriver->clear();
    };
	
	onContext = [this]() {
        if ( program->couldDeviceBlockSecondMouseButton( ) )
            return false;
                        
		bool allow = !inputDriver->mIsAcquired();
		                
		if (allow && exclusiveFullscreen()) {
			InputManager::activateHotkey(Hotkey::Id::Fullscreen);
			allow = false;
		}		
		return allow;
	};
    
    onUnminimize = [this]() {
        this->updateViewport();
        statusHandler->resetFrameCounter();
    };
	
	winapi.onMenu = []() {
	//	audioDriver->clear();
	};
	
	GUIKIT::BrowserWindow::onCall = []() {
		audioDriver->clear();
	};

    GUIKIT::Application::Cocoa::onOpenFile = [] (std::string fileName) {
        
        autoloader->init( {fileName}, false, Autoloader::Mode::AutoStart );
        
        autoloader->loadFiles();
        
        if (!cmd->debug && !cmd->noDriver && !cmd->noGui && globalSettings->get<bool>("open_fullscreen", false)) {
            view->setFullScreen(true);
        }
    };
    
    //osx extra menu points
    GUIKIT::Application::Cocoa::onQuit = [this] {
        onClose();
    };
    
    GUIKIT::Application::Cocoa::onAbout = [this] {
        message->information(APP_NAME " " VERSION "\n"
            + trans->get("author", {}, true) + AUTHOR "\n"
            + trans->get("license", {}, true) + LICENSE
        ,trans->get("about", {{"%app%", APP_NAME}}));
    };
    
    GUIKIT::Application::Cocoa::onPreferences = [] {
        ConfigView::TabWindow::open(ConfigView::TabWindow::Layout::Settings);
    };
    	
	GUIKIT::Application::Cocoa::onCustom1 = []() {
		program->saveSettings();
	};
	
    GUIKIT::Application::Cocoa::onDock = [] {
        view->setFocused();
    };
	
	GUIKIT::Application::onClipboardRequest = [](std::string text) {
		if (activeEmulator && !text.empty())
			activeEmulator->pasteText(text);
	};

    GUIKIT::Application::onDisplayChange = [this]() {
        displayChangeTimer.setEnabled();
    };
        
	placeholderTimer.setInterval(40);
	placeholderTimer.onFinished = [this]() {
		placeholderTimer.setEnabled(false);		
		renderPlaceholder(false);
        renderPlaceholder(false);
	};
	
	anyloadTimer.setInterval(40);
    displayChangeTimer.setInterval(300);
    displayChangeTimer.onFinished = [this]() {
        displayChangeTimer.setEnabled(false);
        statusHandler->setMessage( std::to_string(GUIKIT::Monitor::getCurrentRefreshRate()) );
        VideoManager::setSynchronize();
    };
	
	viewport.onMousePress = [this](GUIKIT::Mouse::Button button) {
		
		if (program->isRunning || (button != GUIKIT::Mouse::Button::Left))
			return;	
		
		if (cursorForPlaceholderInUpperTriangle()) {
			program->power( program->getEmulator("C64") );
		} else {
			
		}
	};
    
    viewport.onMouseMove = [this](GUIKIT::Position& pos) {
        
        if (program->isRunning)
            return;
                        
        if (cursorForPlaceholderInUpperTriangle(pos)) {
            view->setPointerCursor();
		} else {
            view->setDefaultCursor();
		}
    };
	
	viewport.onMouseLeave = []() {
		if (!program->isRunning)
			view->setDefaultCursor();
	};
	
    setDragnDrop();        
}

auto View::setAnyload( Emulator::Interface* emulator ) -> void {

	bool mIsAcquiredBefore = inputDriver->mIsAcquired();
	if (mIsAcquiredBefore)
		inputDriver->mUnacquire();
	
	anyloadTimer.onFinished = [this, emulator, mIsAcquiredBefore]() {
		anyloadTimer.setEnabled(false);		
		fileloader->anyLoad( emulator, mIsAcquiredBefore );
	};
	
	anyloadTimer.setEnabled();
}

auto View::setDragnDrop() -> void {
    
    viewport.setDroppable();
    
    setDroppable();
    
    // aspect correct viewport doesn't fill up the complete window.
    // thats why, we have to set drop event on whole window too.
    // but viewport is on top of window, so we simply set drop event on both.
    onDrop = [this]( std::vector<std::string> files ) {
        viewport.onDrop( files );
    };
    
    viewport.onDrop = []( std::vector<std::string> files ) {

        autoloader->init( files, false, Autoloader::Mode::DragnDrop );
        
        autoloader->loadFiles();            
    };        
}

auto View::show() -> void {
    program->setVideoManagerGlobals();
    setVisible();
    updateViewport();
}

auto View::update() -> void {
	if ( exclusiveFullscreen() ) {
		setStatusVisible(false);
		updateViewport();
	}
	program->hintExclusiveFullscreen();
}

auto View::setFullScreen(bool fullScreen) -> void {
	if(fullScreen && program->isRunning) inputDriver->mAcquire();
	else inputDriver->mUnacquire();	
    
    GUIKIT::Window::setFullScreen(fullScreen);
    displayChangeTimer.setEnabled();
}

auto View::exclusiveFullscreen() -> bool {
	static auto exclusiveFullscreen = globalSettings->getOrInit("exclusive_fullscreen", false);	
	return *exclusiveFullscreen && fullScreen() && program->isRunning;
}

auto View::updateMenuBar( bool toggle ) -> void {
    
    bool state = globalSettings->get("menubar", true);
    
    if(toggle) {
        state ^= 1;
        globalSettings->set("menubar", state);
    }

    if (menuVisible() == state)
        return;
    
    setMenuVisible( state );
    
    if(toggle)
        updateViewport();
}

auto View::updateStatusBar(bool toggle) -> void {

    bool state = globalSettings->get( !fullScreen() ? "statusbar" : "statusbar_fullscreen", !fullScreen());		

    if (toggle) {
        state ^= 1;
        globalSettings->set( !fullScreen() ? "statusbar" : "statusbar_fullscreen", state);
    }

    if (statusVisible() == state)
        return;

    setStatusVisible( state );
    
    if(toggle)
        updateViewport();
}

auto View::updateViewport() -> void {
	unsigned currentHeight = 0;
	bool integerScaling = false;
	int _height;
    GUIKIT::Geometry geometry = this->geometry();
    geometry.x = geometry.y = 0;
    
    if (activeVideoManager) {
        integerScaling = VideoManager::integerScaling;
        currentHeight = activeVideoManager->currentHeight;
        
        if ((currentHeight == 0) || (geometry.height < currentHeight))
            integerScaling = false;
		
		_height = currentHeight;
    }	

	if (integerScaling) {
		while (geometry.height > _height)
			_height += currentHeight;

		while (_height > geometry.height)
			_height -= currentHeight;

		geometry.y = (geometry.height - _height) / 2;
		geometry.height = _height;
	}
	
	if (VideoManager::aspectCorrect) {
		
		while(1) {
			_height = geometry.height;
			int _width = (unsigned)(((double(_height) / 3.0) * 4.0) + 0.5);

			if (_width > geometry.width) {
				if (integerScaling) {
					_height = geometry.height - currentHeight;
					
					if (_height >= currentHeight) {
						geometry.y += (geometry.height - _height) / 2;
						geometry.height = _height;
						continue;						
					}					
				}
				                
				_height = (unsigned)(((double(geometry.width) / 4.0) * 3.0) + 0.5);
				geometry.x = 0;
				geometry.y += (geometry.height - _height) / 2;
				geometry.height = _height;				
				
			} else {
				geometry.x = (geometry.width - _width) / 2;
				geometry.width = _width;
			}
			
			break;
		}
	}
	viewport.setGeometry( geometry );
	placeholderTimer.setEnabled(true);
}

auto View::checkInputDevice( Emulator::Interface* emulator, Emulator::Interface::Connector* connector, Emulator::Interface::Device* device ) -> void {
    
    for(auto& iM : inputMenus) {
        
        if ( iM.emulator != emulator)
            continue;
        
        for ( auto& inputDevice : iM.inputDevices ) {

            if ( inputDevice.connector == connector && inputDevice.device == device ) {

                if (!inputDevice.item->checked())
                    inputDevice.item->setChecked();

                return;
            }
        }
    }
}

auto View::updateDeviceSelection( Emulator::Interface* emulator ) -> void {
    
    for (auto& connector : emulator->connectors) {

        auto selectedDevice = emulator->getConnectedDevice(&connector);

        checkInputDevice(emulator, &connector, selectedDevice);
    }
}

auto View::removeMenuTree( GUIKIT::Menu* menu ) -> void {		
		
	auto childs = menu->childs;
			
	for(auto child : menu->childs) {
		
		if (dynamic_cast<GUIKIT::Menu*>(child) )
			removeMenuTree( (GUIKIT::Menu*)child );		
	}		
			
	menu->reset();
	
	for(auto child : childs)
		delete child;
}

auto View::setConnectors() -> void {
    std::vector<GUIKIT::MenuRadioItem*> connectorItems;
    GUIKIT::Menu* connectorMenu;
    GUIKIT::MenuItem* inputItem;
    
    removeMenuTree( &controlMenu );
    
    for(auto& iM : inputMenus) {
        
        iM.inputDevices.clear();
        
        auto emulator = iM.emulator;
        
        auto settings = program->getSettings( emulator );
        
        for(auto& connector : emulator->connectors) {

            connectorMenu = new GUIKIT::Menu;
            connectorMenu->setText(emulator->ident + " " + trans->get( connector.name ) );
            connectorMenu->setIcon(plugImage);
            connectorMenu->setVisible();

            auto selectedDevice = emulator->getConnectedDevice(&connector);
            connectorItems.clear();
            GUIKIT::MenuRadioItem* checkItem = nullptr;

            for (auto& device : emulator->devices) {
                if (device.isKeyboard())
                    continue;

                auto item = new GUIKIT::MenuRadioItem;
                item->setText( trans->get(device.name));

                item->onActivate = [emulator, connector, device, settings]() {
                    settings->set<unsigned>( _underscore(connector.name), device.id);
                    emulator->connect(connector.id, device.id);
                    InputManager::getManager(emulator)->updateMappingsInUse();
                    auto emuView = EmuConfigView::TabWindow::getView(emulator);
                    if (emuView)
                        emuView->inputLayout->updateConnectorButtons();
                    view->setCursor(emulator);
                };

                connectorMenu->append(*item);
                if (selectedDevice == &device)
                    checkItem = item;

                connectorItems.push_back(item);

                iM.inputDevices.push_back({&connector, &device, item});
            }
            GUIKIT::MenuRadioItem::setGroup(connectorItems);
            if (checkItem)
                checkItem->setChecked();

            controlMenu.append(*connectorMenu);
            
            if ( !dynamic_cast<LIBC64::Interface*>(emulator))
                connectorMenu->setEnabled(false);
        }
        
        inputItem = new GUIKIT::MenuItem;
		inputItem->setText(emulator->ident + " " + trans->get("swap_ports") );
        
        inputItem->onActivate = [emulator, settings]() {
            
            auto connector1 = emulator->getConnector( 0 );
            auto connectedDevice1 = emulator->getConnectedDevice( connector1 );
            
            auto connector2 = emulator->getConnector( 1 );
            auto connectedDevice2 = emulator->getConnectedDevice( connector2 );
            
            emulator->connect( connector1, connectedDevice2 );
            emulator->connect( connector2, connectedDevice1 );
            
            settings->set<unsigned>( _underscore(connector1->name), connectedDevice2->id);
            settings->set<unsigned>( _underscore(connector2->name), connectedDevice1->id);

            auto emuView = EmuConfigView::TabWindow::getView(emulator);
            if (emuView)
                emuView->inputLayout->updateConnectorButtons();
            
            view->checkInputDevice(emulator, connector1, connectedDevice2);
            view->checkInputDevice(emulator, connector2, connectedDevice1);            
        };
        
        inputItem->setIcon(swapperImage);
        controlMenu.append(*inputItem);
        if ( !dynamic_cast<LIBC64::Interface*>(emulator))
            inputItem->setEnabled(false);
        
        inputItem = new GUIKIT::MenuItem;
        inputItem->setText(emulator->ident + " " + trans->get("config") );

        inputItem->onActivate = [emulator]() {
            auto emuView = EmuConfigView::TabWindow::getView( emulator, true );
            emuView->show(EmuConfigView::TabWindow::Layout::Control);
        };
        inputItem->setIcon(toolsImage);
        controlMenu.append(*inputItem);
        if ( !dynamic_cast<LIBC64::Interface*>(emulator))
            inputItem->setEnabled(false);

        controlMenu.append( *new GUIKIT::MenuSeparator );
    }
}

auto View::updateShader() -> void {
    
	std::vector<GUIKIT::File::Info> shaderList;
	auto folder = globalSettings->get<std::string>("shader_folder", "");
    
    if (folder.empty())
        folder = program->shaderFolder();
    
    if (!folder.empty())
		shaderList = GUIKIT::File::getFolderList(folder);	
    
    for(auto& sM : sysMenus) {
        
        removeMenuTree( sM.shaderMenu );

        sM.shaderMenu->setEnabled( shaderList.size() != 0 );
    }
    
    if (shaderList.size() == 0)
        return;
    
    for(auto& sM : sysMenus) {
        auto emulator = sM.emulator;
        
		auto vManager = VideoManager::getInstance( emulator );
		
        auto activeShaders = vManager->shader.getActiveShaders();

        for (auto& shaderInfo : shaderList) {
            GUIKIT::MenuCheckItem* item = new GUIKIT::MenuCheckItem;            
            item->setText(shaderInfo.name);
            
            for (auto& activeShader : activeShaders) {
                if (activeShader == shaderInfo.name) {
                    item->setChecked();
                    break;
                }
            }
            item->onToggle = [item, vManager]() {
                if (item->checked()) {
					vManager->shader.addActiveShader(item->text());
                } else {
                    vManager->shader.removeActiveShader(item->text());
                }
            };
            sM.shaderMenu->append(*item);
        }
    }
}

auto View::updatePauseCheck() -> void {

    if (program->isPause != pauseItem.checked() )
        pauseItem.setChecked(program->isPause);
}

auto View::updateFastforwardCheck() -> void {
    bool ff = program->warp.active && !program->warp.aggressive;
    bool ffa = program->warp.active && program->warp.aggressive;

    if (ff != fastForwardItem.checked())
        fastForwardItem.setChecked(ff);

    if (ffa != aggressiveFastForwardItem.checked())
        aggressiveFastForwardItem.setChecked(ffa);
}

auto View::togglePause() -> void {
    if (!activeEmulator)
        return;
    program->isPause ^= 1;
    audioDriver->clear();
    if (!program->isPause)
        statusHandler->resetFrameCounter();
}

auto View::loadImages() -> void {
    #include "../../data/resource.h" // for win xp only 
    regionImage.loadPng((uint8_t*)Icons::globe, sizeof(Icons::globe));
    powerImage.loadPng((uint8_t*)Icons::power, sizeof(Icons::power));
    freezeImage.loadPng((uint8_t*)Icons::freeze, sizeof(Icons::freeze));
    menuImage.loadPng((uint8_t*)Icons::menu, sizeof(Icons::menu));
	poweroffImage.loadPng((uint8_t*)Icons::shutdown, sizeof(Icons::shutdown));
    firmwareImage.loadPng((uint8_t*)Icons::memory, sizeof(Icons::memory));
    driveImage.loadPng((uint8_t*)Icons::drive, sizeof(Icons::drive));
    swapperImage.loadPng((uint8_t*)Icons::swapper, sizeof(Icons::swapper));
    scriptImage.loadPng((uint8_t*)Icons::script, sizeof(Icons::script));
    systemImage.loadPng((uint8_t*)Icons::system, sizeof(Icons::system));
    joystickImage.loadPng((uint8_t*)Icons::joystick, sizeof(Icons::joystick));
    volumeImage.loadPng((uint8_t*)Icons::volume, sizeof(Icons::volume));
    plugImage.loadPng((uint8_t*)Icons::plug, sizeof(Icons::plug));
    displayImage.loadPng((uint8_t*)Icons::display, sizeof(Icons::display));
    toolsImage.loadPng((uint8_t*)Icons::tools, sizeof(Icons::tools));
	quitImage.loadPng((uint8_t*)Icons::quit, sizeof(Icons::quit));
	keyboardImage.loadPng((uint8_t*)Icons::keyboard, sizeof(Icons::keyboard));
	colorImage.loadPng((uint8_t*)Icons::color, sizeof(Icons::color));
	tapeImage.loadPng((uint8_t*)Icons::tape, sizeof(Icons::tape));
    paletteImage.loadPng((uint8_t*)Icons::palette, sizeof(Icons::palette));
    cropImage.loadPng((uint8_t*)Icons::crop, sizeof(Icons::crop));
    playImage.loadPng((uint8_t*)Icons::play, sizeof(Icons::play));
    playhiImage.loadPng((uint8_t*)Icons::playHi, sizeof(Icons::playHi));
    stopImage.loadPng((uint8_t*)Icons::stop, sizeof(Icons::stop));
    stophiImage.loadPng((uint8_t*)Icons::stopHi, sizeof(Icons::stopHi));
    recordImage.loadPng((uint8_t*)Icons::record, sizeof(Icons::record));
    recordhiImage.loadPng((uint8_t*)Icons::recordHi, sizeof(Icons::recordHi));
    forwardImage.loadPng((uint8_t*)Icons::forward, sizeof(Icons::forward));
    forwardhiImage.loadPng((uint8_t*)Icons::forwardHi, sizeof(Icons::forwardHi));
    rewindImage.loadPng((uint8_t*)Icons::rewind, sizeof(Icons::rewind));
    rewindhiImage.loadPng((uint8_t*)Icons::rewindHi, sizeof(Icons::rewindHi));
	counterImage.loadPng((uint8_t*)Icons::counter, sizeof(Icons::counter));
    diskImage.loadPng((uint8_t*) Icons::disk, sizeof (Icons::disk));
	editImage.loadPng((uint8_t*)Icons::edit, sizeof(Icons::edit));
    ejectImage.loadPng((uint8_t*)Icons::eject, sizeof(Icons::eject));

    playPauseStatusImage.loadPng((uint8_t*)Icons::playPauseStatus, sizeof(Icons::playPauseStatus));
    forwardPauseStatusImage.loadPng((uint8_t*)Icons::forwardPauseStatus, sizeof(Icons::forwardPauseStatus));
    rewindPauseStatusImage.loadPng((uint8_t*)Icons::rewindPauseStatus, sizeof(Icons::rewindPauseStatus));
    recordPauseStatusImage.loadPng((uint8_t*)Icons::recordPauseStatus, sizeof(Icons::recordPauseStatus));
    //forwardStatusImage.loadPng((uint8_t*)Icons::forwardStatus, sizeof(Icons::forwardStatus));

    playStatusImage = playhiImage;
    stopStatusImage = stopImage;
    recordStatusImage = recordhiImage;
    rewindStatusImage = rewindhiImage;
    forwardStatusImage = forwardhiImage;

    ledOffImage.loadPng((uint8_t*) Icons::ledOff, sizeof (Icons::ledOff));
    ledRedImage.loadPng((uint8_t*) Icons::ledRed, sizeof (Icons::ledRed));
    ledGreenImage.loadPng((uint8_t*) Icons::ledGreen, sizeof (Icons::ledGreen));
}

auto View::buildMenu() -> void {
		
    for(auto emulator : emulators) {
        SystemMenu sM;
        
        sM.emulator = emulator;
        sM.system = new GUIKIT::Menu;
        sM.system->setIcon( systemImage );       
        sM.poweron = new GUIKIT::MenuItem;
        sM.poweron->setIcon( powerImage );
        sM.poweron->onActivate = [emulator]() {
		    program->power(emulator);
	    };	
        sM.system->append( *sM.poweron );
		
		sM.poweronAndRemoveExpansions = new GUIKIT::MenuItem;
        sM.poweronAndRemoveExpansions->setIcon( powerImage );
        sM.poweronAndRemoveExpansions->onActivate = [emulator]() {
		    program->power(emulator);
            program->removeExpansion( false );
	    };	
        sM.system->append( *sM.poweronAndRemoveExpansions );

		        
        sM.reset = new GUIKIT::MenuItem;
        sM.reset->onActivate = [emulator]() {
		    program->reset(emulator);
	    };	
        sM.reset->setIcon( powerImage );
        sM.system->append( *sM.reset );
        sM.freeze = new GUIKIT::MenuItem;
        sM.freeze->setIcon( freezeImage );
        sM.freeze->onActivate = [emulator]() {
		    emulator->freezeButton();
	    };	
        sM.freeze->setEnabled(false);
        sM.system->append( *sM.freeze );

        if ( dynamic_cast<LIBC64::Interface*>(emulator)) {
            sM.menu = new GUIKIT::MenuItem;
            sM.menu->setIcon( menuImage );
            sM.menu->onActivate = [emulator]() {
                emulator->customCartridgeButton();
            };
            sM.menu->setEnabled(false);
            sM.system->append(*sM.menu);
        } else
            sM.menu = nullptr;

        sM.system->append(*GUIKIT::MenuSeparator::getInstance());
		
		sM.loadSoftware = new GUIKIT::MenuItem;
        sM.loadSoftware->setIcon( driveImage );
        sM.loadSoftware->onActivate = [this, emulator]() {			
            setAnyload( emulator );
	    };
        sM.system->append( *sM.loadSoftware );
        
        sM.media = new GUIKIT::MenuItem;
        sM.media->setIcon( driveImage );
        sM.media->onActivate = [emulator]() {
            auto emuView = EmuConfigView::TabWindow::getView( emulator, true );
            emuView->mediaLayout->show();
            emuView->show(EmuConfigView::TabWindow::Layout::Media);
	    };
        sM.system->append( *sM.media );
		
		sM.system->append(*GUIKIT::MenuSeparator::getInstance());
        
		sM.systemManagement = new GUIKIT::MenuItem;
        sM.systemManagement->setIcon( systemImage );
        sM.systemManagement->onActivate = [emulator]() {
            auto emuView = EmuConfigView::TabWindow::getView( emulator, true );
            emuView->show(EmuConfigView::TabWindow::Layout::System);
	    };
        sM.system->append( *sM.systemManagement );
            
        sM.configurations = new GUIKIT::MenuItem;
        sM.configurations->setIcon( scriptImage );
        sM.configurations->onActivate = [emulator]() {
            auto emuView = EmuConfigView::TabWindow::getView( emulator, true );
            emuView->show(EmuConfigView::TabWindow::Layout::Configurations);
        };
        sM.system->append( *sM.configurations );

        sM.presentation = new GUIKIT::MenuItem;
        sM.presentation->setIcon( displayImage );
        sM.presentation->onActivate = [emulator]() {
            auto emuView = EmuConfigView::TabWindow::getView( emulator, true );
            emuView->show(EmuConfigView::TabWindow::Layout::Presentation);
        };
        sM.system->append( *sM.presentation );

        if ( dynamic_cast<LIBC64::Interface*>(emulator)) {
            sM.palette = new GUIKIT::MenuItem;
            sM.palette->setIcon(paletteImage);
            sM.palette->onActivate = [emulator]() {
                auto emuView = EmuConfigView::TabWindow::getView( emulator, true );
                emuView->show(EmuConfigView::TabWindow::Layout::Palette);
            };
            sM.system->append(*sM.palette);				
        }

        sM.audio = new GUIKIT::MenuItem;
        sM.audio->setIcon( volumeImage );
        sM.audio->onActivate = [emulator]() {
            auto emuView = EmuConfigView::TabWindow::getView( emulator, true );
            emuView->show(EmuConfigView::TabWindow::Layout::Audio);
        };
        sM.system->append( *sM.audio );

        sM.firmware = new GUIKIT::MenuItem;
        sM.firmware->setIcon( firmwareImage );
        sM.firmware->onActivate = [emulator]() {
            auto emuView = EmuConfigView::TabWindow::getView( emulator, true );
            emuView->show(EmuConfigView::TabWindow::Layout::Firmware);
        };
        sM.system->append( *sM.firmware );

        sM.border = new GUIKIT::MenuItem;
        sM.border->setIcon( cropImage );
        sM.border->onActivate = [emulator]() {
            auto emuView = EmuConfigView::TabWindow::getView( emulator, true );
            emuView->show(EmuConfigView::TabWindow::Layout::Border);
        };
        sM.system->append( *sM.border );

        sM.misc = new GUIKIT::MenuItem;
        sM.misc->setIcon( toolsImage );
        sM.misc->onActivate = [emulator]() {
            auto emuView = EmuConfigView::TabWindow::getView( emulator, true );
            emuView->show(EmuConfigView::TabWindow::Layout::Misc);
        };
        sM.system->append( *sM.misc );
		
		sM.system->append(*GUIKIT::MenuSeparator::getInstance());						
                	
		sM.shaderMenu = new GUIKIT::Menu;
        sM.shaderMenu->setIcon( colorImage );
        sM.system->append( *sM.shaderMenu );		
        
        sysMenus.push_back( sM );
        
        if ( dynamic_cast<LIBAMI::Interface*>(emulator))        
            sM.system->setEnabled( false );
        
        append( *sM.system );
            
        InputMenu iM;
        iM.emulator = emulator;
        inputMenus.push_back(iM);
    }

    controlMenu.setIcon(joystickImage);
    append(controlMenu);
	
    editMenu.append( copyItem );

    pasteItem.onActivate = []() {
        GUIKIT::Application::requestClipboardText();
    };

    copyItem.onActivate = []() {
        if (!activeEmulator)
            return;

        std::string text = activeEmulator->copyText( );

        GUIKIT::Application::setClipboardText( text );
    };

    editMenu.append( pasteItem );

	editMenu.setIcon(editImage);
    append( editMenu );

    optionsMenu.setIcon(toolsImage);
    append(optionsMenu);

    globalVideoItem.setIcon( displayImage );
    globalVideoItem.onActivate = []() {
        ConfigView::TabWindow::open(ConfigView::TabWindow::Layout::Video);
    };

    optionsMenu.append(globalVideoItem);

    globalAudioItem.onActivate = []() {
        ConfigView::TabWindow::open(ConfigView::TabWindow::Layout::Audio);
    };
    globalAudioItem.setIcon( volumeImage );
    optionsMenu.append(globalAudioItem);

    globalInputItem.onActivate = []() {
        ConfigView::TabWindow::open(ConfigView::TabWindow::Layout::Input);
    };
    globalInputItem.setIcon( keyboardImage );
    optionsMenu.append(globalInputItem);

    settingsItem.onActivate = []() {
        ConfigView::TabWindow::open(ConfigView::TabWindow::Layout::Settings);
    };
    settingsItem.setIcon(toolsImage);
    optionsMenu.append(settingsItem);

    optionsMenu.append(*GUIKIT::MenuSeparator::getInstance());

    videoSyncItem.onToggle = [&]() {
        globalSettings->set<bool>("video_sync", videoSyncItem.checked() );
        program->fastForward( false );
        VideoManager::setSynchronize();
        statusHandler->resetFrameCounter();
        bool threadedRenderer = globalSettings->get("threaded_renderer", false);
        adaptiveSyncItem.setEnabled( videoSyncItem.checked() && !threadedRenderer );
        dynamicRateControl.setEnabled( videoSyncItem.checked() && !threadedRenderer );
    };
    bool threadedRenderer = globalSettings->get("threaded_renderer", false);
    bool vsync = globalSettings->get<bool>("video_sync", true);

    if (vsync)
        videoSyncItem.setChecked();

    adaptiveSyncItem.setEnabled( !threadedRenderer && vsync );

    optionsMenu.append(videoSyncItem);

    adaptiveSyncItem.onToggle = [&]() {
        globalSettings->set<bool>("adaptive_sync", adaptiveSyncItem.checked() );
        program->fastForward( false );
        VideoManager::setSynchronize();
        //statusHandler->resetFrameCounter();
    };
    if ( globalSettings->get<bool>("adaptive_sync", true) )
        adaptiveSyncItem.setChecked();

    optionsMenu.append(adaptiveSyncItem);
    
//    fpsLimitItem.onToggle = [&]() {
//        globalSettings->set<bool>("fps_limit", fpsLimitItem.checked() );
//        program->setFpsLimit();
//
//        if (fpsLimitItem.checked()) {
//            speedItems[0]->setChecked();
//            speedItems[0]->onActivate();
//        }
//    };
//    if ( globalSettings->get<bool>("fps_limit", false) ) fpsLimitItem.setChecked();
//    optionsMenu.append(fpsLimitItem);

    dynamicRateControl.onToggle = [&]() {
        globalSettings->set<bool>("dynamic_rate_control", dynamicRateControl.checked() );
        VideoManager::setSynchronize();
        //audioManager->setRateControl();
    };
    if ( globalSettings->get<bool>("dynamic_rate_control", false) )
        dynamicRateControl.setChecked();

    dynamicRateControl.setEnabled( !threadedRenderer && vsync );

    optionsMenu.append(dynamicRateControl);
        
    optionsMenu.append(*GUIKIT::MenuSeparator::getInstance());
        
    fullscreenItem.onActivate = [this]() {
        inputDriver->mUnacquire();
        GUIKIT::Window::setFullScreen(!fullScreen());
    };
        
    optionsMenu.append(fullscreenItem);
        
    optionsMenu.append(*GUIKIT::MenuSeparator::getInstance());
            
    muteItem.onToggle = [&]() {
        globalSettings->set<bool>("audio_mute", muteItem.checked() );
        audioManager->setVolume();
    };
    if ( globalSettings->get<bool>("audio_mute", false) ) muteItem.setChecked();
    optionsMenu.append(muteItem);
    fpsItem.onToggle = [&]() {
        globalSettings->set<bool>("fps", fpsItem.checked() );
        statusHandler->updateFPS( fpsItem.checked() );
    };
    if ( globalSettings->get<bool>("fps", false) ) fpsItem.setChecked();
    optionsMenu.append(fpsItem);
    
    audioBufferItem.onToggle = [&]() {
        globalSettings->set<bool>("show_audio_buffer", audioBufferItem.checked() );
		audioManager->setStatistics();
        statusHandler->updateDRC( audioBufferItem.checked() );
    };
    if ( globalSettings->get<bool>("show_audio_buffer", false) ) audioBufferItem.setChecked();
    optionsMenu.append(audioBufferItem);

    optionsMenu.append(*GUIKIT::MenuSeparator::getInstance());

    saveItem.onActivate = []() {
        program->saveSettings();
    };
    saveItem.setIcon(diskImage);
    optionsMenu.append(saveItem);
		
	optionsMenu.append(*GUIKIT::MenuSeparator::getInstance());	

	poweroff.setIcon( poweroffImage );
	poweroff.onActivate = [this]() {
		program->powerOff();
		videoDriver->setFilter( DRIVER::Video::Filter::Linear );
		this->updateViewport();

		if (cursorForPlaceholderInUpperTriangle()) {
			view->setPointerCursor();
		} else {
			view->setDefaultCursor();
		}
	};	
	optionsMenu.append( poweroff );   
	
	
	if(!GUIKIT::Application::isCocoa()) {

		exit.setIcon( quitImage );
		exit.onActivate = [this]() {
			onClose();
		};	
		optionsMenu.append( exit );
	}
	
	// prepare Tape Control	
	tapeControlMenu.setIcon( tapeImage );

    insertTapeItem.setIcon( tapeImage );
    
    insertTapeItem.onActivate = []() {
        auto emulator = activeEmulator;

         if (!activeEmulator)
             emulator = program->getLastUsedEmu();

        fileloader->load( emulator, emulator->getTape(0) );
    };
    
    tapeControlMenu.append( insertTapeItem );
    
    ejectTapeItem.setIcon( ejectImage );
    
    ejectTapeItem.onActivate = []() {
        auto emulator = activeEmulator;

         if (!activeEmulator)
             emulator = program->getLastUsedEmu();

        fileloader->eject( emulator, emulator->getTape(0) );
    };    
    
    tapeControlMenu.append( ejectTapeItem );
    
    tapeControlMenu.append( *GUIKIT::MenuSeparator::getInstance() );

	tapePlayItem.onActivate = []() {
		InputManager::activateHotkey(Hotkey::Id::PlayTape);
	};
	tapeControlMenu.append( tapePlayItem );
	
	tapeStopItem.onActivate = []() {
		InputManager::activateHotkey(Hotkey::Id::StopTape);
	};
	tapeControlMenu.append( tapeStopItem );
	
	tapeForwardItem.onActivate = []() {
		InputManager::activateHotkey(Hotkey::Id::ForwardTape);
	};
	tapeControlMenu.append( tapeForwardItem );
	
	tapeRewindItem.onActivate = []() {
		InputManager::activateHotkey(Hotkey::Id::RewindTape);
	};
	tapeControlMenu.append( tapeRewindItem );
	
	tapeRecordItem.onActivate = []() {
		InputManager::activateHotkey(Hotkey::Id::RecordTape);
	};
	tapeControlMenu.append( tapeRecordItem );
	
    tapeControlMenu.append(*GUIKIT::MenuSeparator::getInstance());	
    
	tapeResetCounterItem.onActivate = []() {
		InputManager::activateHotkey(Hotkey::Id::ResetTapeCounter);
	};
	tapeControlMenu.append( tapeResetCounterItem );  

    // speed menu
    fastForwardItem.onToggle = []() {
        program->toggleFastForward( false );
    };
    speedControlMenu.append( fastForwardItem );

    aggressiveFastForwardItem.onToggle = []() {
        program->toggleFastForward( true );
    };
    speedControlMenu.append( aggressiveFastForwardItem );

    pauseItem.onToggle = [this]() {
        this->togglePause();
    };
    speedControlMenu.append( pauseItem );

    speedControlMenu.append( *GUIKIT::MenuSeparator::getInstance() );

    GUIKIT::MenuRadioItem* speedItem;

    unsigned steps = 12;

    for(unsigned i = 0; i <= steps; i++) {
        if (i == (steps - 1))
            speedItem = &maximumSpeedItem;
        else
            speedItem = new GUIKIT::MenuRadioItem;

        speedItem->onActivate = [this, i]() {
            auto settings = program->getSettings( activeEmulator );
            settings->set<unsigned>("speed_profile", i);
            audioManager->setSynchronize();
            audioManager->setResampler();
            statusHandler->resetFrameCounter();
        };
        speedControlMenu.append( *speedItem );

        if (i == 1 || i == (steps - 1))
            speedControlMenu.append( *GUIKIT::MenuSeparator::getInstance() );

        if (i == (steps - 1)) {
            customizeSpeedItem.onActivate = []() {
                auto emuView = EmuConfigView::TabWindow::getView(activeEmulator, true);
                if (emuView)
                    emuView->show(EmuConfigView::TabWindow::Layout::Misc);
            };
            speedControlMenu.append( customizeSpeedItem );
        }

        speedItems.push_back( speedItem );
    }

    GUIKIT::MenuRadioItem::setGroup( speedItems );

    // prepare Disk Control
    unsigned i = 0;
    for (auto& diskControlMenu : diskControlMenus) {
        
        diskControlMenu.insert.setIcon( diskImage );
        
        diskControlMenu.insert.onActivate = [i]() {
            auto emulator = activeEmulator;

            if (!activeEmulator)
                emulator = program->getLastUsedEmu();

            fileloader->load( emulator, emulator->getDisk(i) );
        };    
        diskControlMenu.menu.append( diskControlMenu.insert );

        diskControlMenu.eject.setIcon( ejectImage );
        
        diskControlMenu.eject.onActivate = [i]() {
            auto emulator = activeEmulator;

            if (!activeEmulator)
                emulator = program->getLastUsedEmu();

            fileloader->eject( emulator, emulator->getDisk(i) );
        };
        diskControlMenu.menu.append( diskControlMenu.eject );
        
        i++;
    }   
}

auto View::updateSpeedLabels(bool force) -> void {
    static int _mode = -1;
    static Emulator::Interface* _emulator = nullptr;
    std::string label;

    if (force) {
        _mode = -1;
        _emulator = nullptr;
    }

    if (!activeEmulator)
        return;

    auto stat = activeEmulator->getStatsForSelectedRegion();

    if ( (_mode == -1) || ((int)stat.isPal() != _mode) || (_emulator != activeEmulator)) {

        _mode = (int)stat.isPal();
        _emulator = activeEmulator;

        speedItems[0]->setText( GUIKIT::String::formatFloatingPoint( stat.fps, 3) + " FPS ( 100 % )");

        unsigned _size = speedItems.size();

        for(unsigned i = 1; i < _size; i++) {
            if (i == (_size - 2) ) // maximum
                continue;

            float value;
            bool percent;
            view->getSpeed(i, value, percent);

            if (percent) {
                label = GUIKIT::String::formatFloatingPoint(value, 3, true) + " %";
            } else {
                label = GUIKIT::String::formatFloatingPoint(value, 3, true) + " FPS";
            }

            if (i == 1) {
                float otherValue = (100.0 * value) / (float)stat.fps;
                label += " ( " + GUIKIT::String::formatFloatingPoint(otherValue, 2, true) + " % )";
            }

            speedItems[i]->setText( label );
        }

        auto settings = program->getSettings( activeEmulator );
        unsigned speedProfile = settings->get<unsigned>("speed_profile", 1, {0, (unsigned)speedItems.size() - 1});
        if (!speedItems[speedProfile]->checked())
            speedItems[speedProfile]->setChecked();
    }
}

auto View::showTapeMenu( bool show, Emulator::Interface::TapeMode mode ) -> void {
        
    if (show)
        updateTapeIcons(mode);
	else
		statusHandler->hideTape();

    if (show == isApended(tapeControlMenu))
        return;
    
	show ? append( tapeControlMenu ) : remove( tapeControlMenu );
}

auto View::updateTapeIcons( Emulator::Interface::TapeMode mode ) -> void {
    typedef Emulator::Interface::TapeMode TapeMode;
    
    tapeStopItem.setIcon( mode == TapeMode::Stop ? stophiImage : stopImage );
    tapePlayItem.setIcon( mode == TapeMode::Play ? playhiImage : playImage );    
    tapeRecordItem.setIcon( mode == TapeMode::Record ? recordhiImage : recordImage );
    tapeForwardItem.setIcon( mode == TapeMode::Forward ? forwardhiImage : forwardImage );
    tapeRewindItem.setIcon( mode == TapeMode::Rewind ? rewindhiImage : rewindImage );
	tapeResetCounterItem.setIcon( counterImage );
    
    updateTapeStatusIcons( mode );
}

auto View::updateTapeStatusIcons( Emulator::Interface::TapeMode mode ) -> void {
    
    GUIKIT::Image* image = &stopStatusImage; // Unpressed
    
    typedef Emulator::Interface::TapeMode TapeMode;
    
    switch( mode ) {
        case TapeMode::Play:        image = &playStatusImage; break;
        case TapeMode::Record:      image = &recordStatusImage; break;
        case TapeMode::Forward:     image = &forwardStatusImage; break;
        case TapeMode::Rewind:      image = &rewindStatusImage; break;
        default: break;
    //    case TapeMode::Stop:        image = &stophiImage; break;
    }
    
    statusHandler->updateTapeImage( image );
        
    statusBar.update();
}

auto View::translate() -> void {
	setConnectors();
	
    for(auto& sysMenu : sysMenus) {
        sysMenu.system->setText(sysMenu.emulator->ident);
        sysMenu.poweron->setText(trans->get("Hard Reset"));
		sysMenu.poweronAndRemoveExpansions->setText(trans->get("Hard Reset + Unplug Cart"));
        sysMenu.reset->setText(trans->get("Soft Reset"));        
        sysMenu.freeze->setText(trans->get("Freeze"));
        if (sysMenu.menu)
            sysMenu.menu->setText(trans->get("cartridge button"));
        sysMenu.loadSoftware->setText(trans->get("load software"));
        sysMenu.media->setText(trans->get("Software"));
        sysMenu.systemManagement->setText(trans->get("system_management"));

        sysMenu.audio->setText(trans->get("Audio"));
        sysMenu.firmware->setText(trans->get("Firmware"));
        sysMenu.configurations->setText(trans->get("Configurations"));
        sysMenu.presentation->setText(trans->get("Presentation"));
        sysMenu.palette->setText(trans->get("Palette"));
        sysMenu.border->setText(trans->get("Border"));
        sysMenu.misc->setText(trans->get("miscellaneous"));

        sysMenu.shaderMenu->setText(trans->get("Shader"));            
    }    

    editMenu.setText( trans->get("Edit") );
    pasteItem.setText( trans->get("Paste") );
    copyItem.setText( trans->get("Copy") );

    controlMenu.setText( trans->get("control") );
    
    optionsMenu.setText( trans->get("options"));

    globalVideoItem.setText( trans->get("video") );
    globalAudioItem.setText( trans->get("audio") );
    globalInputItem.setText( trans->get("input") + " / " + trans->get("hotkeys") );
    settingsItem.setText( trans->get("settings"));

    videoSyncItem.setText( trans->get("Video Sync"));
    adaptiveSyncItem.setText( trans->get("Adaptive Sync"));
    fpsLimitItem.setText( trans->get("Fps Limit"));
    dynamicRateControl.setText( trans->get("dynamic_rate_control"));

    fullscreenItem.setText( trans->get("fullscreen"));
    
    muteItem.setText( trans->get("mute_audio"));
    fpsItem.setText( trans->get("show_fps"));
    audioBufferItem.setText( trans->get("show_audio_buffer"));
    
    saveItem.setText( trans->get("save_preferences"));
    exit.setText(trans->get("Exit"));
	
	poweroff.setText(trans->get("power_off"));
	
	tapeControlMenu.setText( trans->get("Datasette") );
    insertTapeItem.setText( trans->get("insert") );
    ejectTapeItem.setText( trans->get("eject") );
    
    for (auto& diskControlMenu : diskControlMenus) {
        diskControlMenu.insert.setText( trans->get("insert") );
        diskControlMenu.eject.setText( trans->get("eject") );
    }
    
	tapePlayItem.setText( trans->get("tape_play_key") );
	tapeStopItem.setText( trans->get("tape_stop_key") );
	tapeRecordItem.setText( trans->get("tape_record_key") );
	tapeForwardItem.setText( trans->get("tape_forward_key") );
	tapeRewindItem.setText( trans->get("tape_rewind_key") );
	tapeResetCounterItem.setText( trans->get("tape_counter_reset_key") );

    GUIKIT::MessageWindow::translateYes( trans->get("yes") );
    GUIKIT::MessageWindow::translateNo( trans->get("no") );
    //osx extra menu
    cocoa.setTitleForAppMenuItem(GUIKIT::Window::Cocoa::AppMenuItem::About, trans->get("about", {{"%app%", APP_NAME}}));
    cocoa.setTitleForAppMenuItem(GUIKIT::Window::Cocoa::AppMenuItem::Preferences, trans->get("preferences"));
	cocoa.setTitleForAppMenuItem(GUIKIT::Window::Cocoa::AppMenuItem::Custom1, trans->get("save_preferences"));
    cocoa.setTitleForAppMenuItem(GUIKIT::Window::Cocoa::AppMenuItem::Hide, trans->get("hide_app", {{"%app%", APP_NAME}}));
    cocoa.setTitleForAppMenuItem(GUIKIT::Window::Cocoa::AppMenuItem::HideOthers, trans->get("hide_others"));
    cocoa.setTitleForAppMenuItem(GUIKIT::Window::Cocoa::AppMenuItem::ShowAll, trans->get("show_all"));
    cocoa.setTitleForAppMenuItem(GUIKIT::Window::Cocoa::AppMenuItem::Quit, trans->get("quit", {{"%app%", APP_NAME}}));
    
    //cocoa.setHiddenForAppMenuItem(GUIKIT::Window::Cocoa::AppMenuItem::Custom1, true);

	statusBar.updateTooltip(12, trans->get("cartridges") );
	statusBar.updateTooltip(15, trans->get("FPS") );
    pauseItem.setText( trans->get("Pause") );
    fastForwardItem.setText( trans->get("Toggle_fastforward") );
    aggressiveFastForwardItem.setText( trans->get("Toggle_fastforward_aggressive") );

    maximumSpeedItem.setText( trans->get("maximum speed") );
    customizeSpeedItem.setText( trans->get("customize speed") );
}

auto View::getViewportHandle() -> uintptr_t {
    return viewport.handle();
}

auto View::loadCursor() -> void {
    
    #include "../../data/img/cursor.data"

    pencilImage.loadPng((uint8_t*)pencil, sizeof(pencil));  
    
    crosshairImage.loadPng((uint8_t*)crosshair, sizeof(crosshair));
}

auto View::setCursor( Emulator::Interface* emulator ) -> void {

    if (!activeEmulator || !emulator)
        return;
    
    if ( activeEmulator != emulator )
        return;   
    
    for( auto& connector : emulator->connectors ) {
        
        auto device = emulator->getConnectedDevice( &connector );
        
        if (device->isLightPen()) {
            changeCursor( pencilImage, 0, pencilImage.height - 1 );
            return;
            
        } else if (device->isLightGun()) {
            changeCursor( crosshairImage, crosshairImage.width / 2, crosshairImage.height / 2 );
            return;
        }
    }
    
    setDefaultCursor();
}

auto View::getSysMenu( Emulator::Interface* emulator ) -> SystemMenu* {
    
    for (auto& sM : sysMenus) {
        if (sM.emulator == emulator)
            return &sM;
    }
    
    return nullptr;
}

auto View::updateCartButtons( Emulator::Interface* emulator ) -> void {
    bool state;
    
    for (auto& sM : sysMenus) {
         state = (sM.emulator == emulator) && emulator->hasFreezeButton();

        if (sM.freeze->enabled() != state)
            sM.freeze->setEnabled( state );

        if (!sM.menu)
            continue;
         
        state = (sM.emulator == emulator) && emulator->hasCustomCartridgeButton();
        
        if (sM.menu->enabled() != state)
            sM.menu->setEnabled( state );
    }
}

auto View::questionToWrite(Emulator::Interface::Media* media) -> bool {
    
    auto file = (GUIKIT::File*)media->guid;
    
    if (cmd->debug || cmd->noGui || !file || file->isArchived() || file->isReadOnly())
        // archive, removing of write protection is not supported
        return false;
    
    if (exclusiveFullscreen())
        setFullScreen( false );
    
    bool state = !globalSettings->get<bool>("question_media_write", true);
    
    if (!state)
        state = message->question( trans->get("question permanent write", {{"%media%", media->name}}) );
    
    return state;
}

auto View::getSpeedBySelectedProfile(float& speed, bool& percent) -> unsigned {
    auto settings = program->getSettings( activeEmulator );
    unsigned speedProfile = settings->get<unsigned>("speed_profile", 1, {0, (unsigned)speedItems.size() - 1});
    getSpeed(speedProfile, speed, percent);
    return speedProfile;
}

auto View::getSpeed(unsigned pos, float& speed, bool& percent) -> void {
    percent = false;
    speed = 50.0;

    auto stats = activeEmulator->stats;

    switch (pos) {
        case 0: speed = stats.fps; break;
        case 1: speed = stats.isPal() ? 50.0 : 60.0; break;
        case 2: speed = 5.0; break;
        case 3: speed = 25.0; break;
        case 4: speed = stats.isPal() ? 60.0 : 50.0; break;
        case 5: speed = 70.0; break;
        case 6: speed = 75.0; break;
        case 7: speed = 80.0; break;
        case 8: speed = 90.0; break;
        case 9: speed = 100.0; break;
        case 10: speed = 120.0; break;
        case 11: speed = 250.0; break; // maximum
        case 12:
            auto settings = program->getSettings( activeEmulator );
            speed = settings->get<float>("custom_speed", 123.456);
            percent = settings->get<bool>("custom_speed_percent", false);
            break;
    }
}

auto View::activateCustomSpeed() -> void {
    speedItems[speedItems.size() - 1]->activate();
}

auto View::isCustomSpeed() -> bool {
    if (!activeEmulator)
        return false;

    auto settings = program->getSettings( activeEmulator );
    unsigned speedProfile = settings->get<unsigned>("speed_profile", 1, {0, (unsigned)speedItems.size() - 1});

    return speedProfile == (speedItems.size() - 1);
}

auto View::isMaximumSpeed() -> bool {
    if (!activeEmulator)
        return false;

    auto settings = program->getSettings( activeEmulator );
    unsigned speedProfile = settings->get<unsigned>("speed_profile", 1, {0, (unsigned)speedItems.size() - 1});

    return speedProfile == (speedItems.size() - 2);
}
