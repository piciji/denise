
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
#include "../../data/icons.h"
#include "../thread/emuThread.h"
#include "placeholder.cpp"

View* view = nullptr;

View::View() : GUIKIT::Window(GUIKIT::Window::Hints::Video) {
    message = new Message(this);
}

auto View::build() -> void {
    setTitle( APP_NAME " " VERSION );
    setBackgroundColor(0);
    cocoa.setDisableIconsInTopMenu(true);

    if (globalSettings->get<bool>("aspect_correct_resizing", false))
        setAspectRatio( {4,3} );
    else
        setAspectRatio( {0,0} );

    updateGeometry();
    
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
        applyMaximizeCorrection(geometry);

        globalSettings->set<int>("screen_x", geometry.x);
        globalSettings->set<int>("screen_y", geometry.y);
        if (!emuThread->enabled)
		    audioDriver->clear();
    };
    
    onSize = [this](GUIKIT::Window::SIZE_MODE sizeMode ) {
        if (fullScreen()) {
			if (program->canExclusiveFullscreen())
				setStatusVisible( false );
			else
				updateStatusBar();
			
            setMenuVisible(false);
        } else {
            updateMenuBar();
            updateStatusBar();

            GUIKIT::Geometry geometry = this->geometry();

            if (sizeMode == GUIKIT::Window::SIZE_MODE::Maximized)
                applyMaximizeCorrection(geometry);

            globalSettings->set<int>("screen_width", geometry.width);
            globalSettings->set<int>("screen_height", geometry.height);
        }

        if (fullScreen() || requestFullscreenSwitch || (sizeMode != GUIKIT::Window::SIZE_MODE::Default)) {
            updateViewport();

        } else {
            if (activeVideoManager && emuThread->enabled && !program->isPause) {
                videoDriver->lockResize();
                updateViewport();
                videoDriver->unlockResize();
            } else
                updateViewport();
      
			if (activeVideoManager) {
                if (!emuThread->enabled || program->isPause) {
                    activeVideoManager->waitForCrtRenderer();
                    emuThread->lock();
                    videoDriver->redraw();
                    videoDriver->freeContext();
                    emuThread->unlock();
                }
			} else {
				videoDriver->redraw(true);
                videoDriver->freeContext();
            }
        }

        if (!emuThread->enabled)
		    audioDriver->clear();
    };

    onResizeStart = [this]() {
        videoDriver->hintResizing(true);

        if (activeVideoManager /*&& !fullScreen() && !requestFullscreenSwitch*/) {
            if (videoDriver->needResizingPreparations(emuThread->enabled)) {
                emuThread->lock();
                videoDriver->prepareResizing();
                emuThread->unlock();
                customResizeMode = true;
            }
        }
    };

    onResizeEnd = [this]() {

        if (customResizeMode) {
            emuThread->lock();
            videoDriver->endResizing();
            emuThread->unlock();
            customResizeMode = false;
        }
        
        videoDriver->hintResizing(false);
    };
	
	onContext = [this]() {
        emuThread->lock();
        if ( program->couldDeviceBlockSecondMouseButton( ) ) {
            emuThread->unlock();
            return false;
        }
                        
		bool allow = !inputDriver->mIsAcquired();
		                
		if (allow && videoDriver && videoDriver->hasExclusiveFullscreen() ) {
			InputManager::activateHotkey(Hotkey::Id::Fullscreen);
			allow = false;
		}

        emuThread->unlock();
		return allow;
	};
    
    onUnminimize = [this]() {
        this->updateViewport();
        statusHandler->resetFrameCounter();
    };

    onRealize = [this]() {
		updateViewport();
        program->finishStartup();
    };
	
	winapi.onMenu = []() {
	//	audioDriver->clear();
	};
	
	GUIKIT::BrowserWindow::onCall = []() {
        if (!globalSettings->get<bool>("threaded_emu", false) || !globalSettings->get("threaded_renderer", true))
		    audioDriver->clear();
	};

    GUIKIT::Application::Cocoa::onOpenFile = [this] (std::string fileName) {

        emuThread->lock();
        autoloader->init( {fileName}, false, Autoloader::Mode::AutoStart );
        
        autoloader->loadFiles();
        
        if (!cmd->debug && !cmd->noDriver && !cmd->noGui && globalSettings->get<bool>("open_fullscreen", false)) {
            fullscreenOnStartUp.setEnabled();
        }
        emuThread->unlock();
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
        emuThread->lock();
        ConfigView::TabWindow::open(ConfigView::TabWindow::Layout::Settings);
        emuThread->unlock();
    };
    	
	GUIKIT::Application::Cocoa::onCustom1 = []() {
        emuThread->lock();
		program->saveSettings();
        emuThread->unlock();
	};
	
    GUIKIT::Application::Cocoa::onDock = [] {
        view->setFocused();
    };
	
	GUIKIT::Application::onClipboardRequest = [](std::string text) {
		if (activeEmulator && !text.empty()) {
            emuThread->lock();
            activeEmulator->pasteText(text);
            emuThread->unlock();
        }
	};

    GUIKIT::Application::onDisplayChange = [this]() {
		
		if (!displayChangeTimer.enabled()) {
			displayChangeTimer.setEnabled();
		}
    };
	
	anyloadTimer.setInterval(40);
    displayChangeTimer.setInterval(500);
    displayChangeTimer.onFinished = [this]() {
        displayChangeTimer.setEnabled(false);
        //statusHandler->setMessage( std::to_string(GUIKIT::Monitor::getCurrentRefreshRate()) );
        emuThread->lock();
        
		if (videoDriver && fullscreenSetting.inUse
			&& globalSettings->get<bool>("threaded_emu", false)
			&& globalSettings->get<bool>("threaded_renderer", true))
            videoDriver->forceResize();
		
		else if (!requestFullscreenSwitch && !fullScreen()) {
			videoDriver->forceResize();
		}

        VideoManager::setSynchronize();
		requestFullscreenSwitch = false;
        emuThread->unlock();
    };
	
    priorityTimer.setInterval(1500);
    priorityTimer.onFinished = [this]() {
        priorityTimer.setEnabled(false);
        emuThread->lock();
        videoDriver->changeThreadPriorityToRealtime(true);
        emuThread->unlock();
    };
    
	fullscreenOnStartUp.setInterval(500);
	fullscreenOnStartUp.onFinished = [this]() {
		fullscreenOnStartUp.setEnabled(false);
		emuThread->lock();
        switchFullScreen(true);
        emuThread->unlock();
	};
    
    cursorHideTimer.setInterval(1000);
    cursorHideTimer.onFinished = [this]() {
        cursorHideTimer.setEnabled(false);
        inputDriver->mAcquire();
    };
	
	viewport.onMousePress = [this](GUIKIT::Mouse::Button button) {
        if ((button == GUIKIT::Mouse::Button::Left) && VideoManager::placeHolderFrames) {
            emuThread->lock();

            int result = cursorForPlaceholderInUpperTriangle();
            if (result != -1)
                VideoManager::hidePlaceHolder();

            if (result == 1)
                program->power(program->getEmulator("C64"));
            else if (result == 0)
                program->power(program->getEmulator("Amiga"));

            emuThread->unlock();
        }
	};
    
    viewport.onMouseMove = [this](GUIKIT::Position& pos) {
        if (!VideoManager::placeHolderFrames)
            return;

        int result = cursorForPlaceholderInUpperTriangle(pos);

        if (result == -1)
            setDefaultCursor();
        else
            setPointerCursor();
    };
	
	viewport.onMouseLeave = []() {

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

        emuThread->lock();
        autoloader->init( files, false, Autoloader::Mode::DragnDrop );
        autoloader->loadFiles();
        emuThread->unlock();
    };        
}

auto View::switchFullScreen(bool fullScreen, bool forceUnacquire) -> void {
	requestFullscreenSwitch = true;
    if(!forceUnacquire && fullScreen) {
        if (GUIKIT::Application::isCocoa()) {
            cursorHideTimer.setEnabled();
        } else
            inputDriver->mAcquire();
    } else {
        cursorHideTimer.setEnabled(false);
        inputDriver->mUnacquire();
    }

    if (!fullScreen && videoDriver) {
        videoDriver->disableExclusiveFullscreen();
        if (GUIKIT::Application::isCocoa()) {
            videoDriver->changeThreadPriorityToRealtime(false);
            priorityTimer.setEnabled();
        }
    }

    GUIKIT::Window::setFullScreen(fullScreen);
    displayChangeTimer.setEnabled();
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
    GUIKIT::Geometry geometry = this->geometry();
    geometry.x = geometry.y = 0;

	viewport.setGeometry( geometry );
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
    bool firstLoop = true;
    
    for(auto& iM : inputMenus) {

        iM.inputDevices.clear();
        bool useCore = globalSettings->get<bool>("core_" + iM.emulator->ident, true);
        if (!useCore)
            continue;

        if (!firstLoop)
            controlMenu.append( *new GUIKIT::MenuSeparator );
        else
            firstLoop = false;

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
                    emuThread->lock();
                    settings->set<unsigned>( _underscore(connector.name), device.id);
                    emulator->connect(connector.id, device.id);
                    auto manager = InputManager::getManager(emulator);
                    manager->updateMappingsInUse();

                    auto emuView = EmuConfigView::TabWindow::getView(emulator);
                    if (emuView && emuView->inputLayout)
                        emuView->inputLayout->updateConnectorButtons();
                    view->setCursor(emulator);
                    emuThread->unlock();
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
        }
        
        inputItem = new GUIKIT::MenuItem;
		inputItem->setText(emulator->ident + " " + trans->get("swap_ports") );
        
        inputItem->onActivate = [emulator, settings]() {
            emuThread->lock();
            auto connector1 = emulator->getConnector( 0 );
            auto connectedDevice1 = emulator->getConnectedDevice( connector1 );
            
            auto connector2 = emulator->getConnector( 1 );
            auto connectedDevice2 = emulator->getConnectedDevice( connector2 );
            
            emulator->connect( connector1, connectedDevice2 );
            emulator->connect( connector2, connectedDevice1 );

            emuThread->unlock();

            settings->set<unsigned>( _underscore(connector1->name), connectedDevice2->id);
            settings->set<unsigned>( _underscore(connector2->name), connectedDevice1->id);

            auto emuView = EmuConfigView::TabWindow::getView(emulator);
            if (emuView && emuView->inputLayout)
                emuView->inputLayout->updateConnectorButtons();
            
            view->checkInputDevice(emulator, connector1, connectedDevice2);
            view->checkInputDevice(emulator, connector2, connectedDevice1);            
        };
        
        inputItem->setIcon(swapperImage);
        controlMenu.append(*inputItem);

        inputItem = new GUIKIT::MenuItem;
        inputItem->setText(emulator->ident + " " + trans->get("config") );

        inputItem->onActivate = [emulator]() {
            auto emuView = EmuConfigView::TabWindow::getView( emulator, true );
            emuView->show(EmuConfigView::TabWindow::Layout::Control);
        };
        inputItem->setIcon(toolsImage);
        controlMenu.append(*inputItem);
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
                emuThread->lock();
                if (item->checked()) {
					vManager->shader.addActiveShader(item->text());
                } else {
                    vManager->shader.removeActiveShader(item->text());
                }
                emuThread->unlock();
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
    	
    powerImage.loadPng((uint8_t*)Icons::power, sizeof(Icons::power));
	powerImage.setResourceId( ID_POWER );
    freezeImage.loadPng((uint8_t*)Icons::freeze, sizeof(Icons::freeze));
	freezeImage.setResourceId( ID_FREEZE );
    menuImage.loadPng((uint8_t*)Icons::menu, sizeof(Icons::menu));
	menuImage.setResourceId( ID_MENU );
    firmwareImage.loadPng((uint8_t*)Icons::memory, sizeof(Icons::memory));
	firmwareImage.setResourceId( ID_MEMORY );
    driveImage.loadPng((uint8_t*)Icons::drive, sizeof(Icons::drive));
	driveImage.setResourceId( ID_DRIVE );
    swapperImage.loadPng((uint8_t*)Icons::swapper, sizeof(Icons::swapper));
	swapperImage.setResourceId( ID_SWAPPER );
    scriptImage.loadPng((uint8_t*)Icons::script, sizeof(Icons::script));
	scriptImage.setResourceId( ID_SCRIPT );
    systemImage.loadPng((uint8_t*)Icons::system, sizeof(Icons::system));
	systemImage.setResourceId( ID_SYSTEM );
    joystickImage.loadPng((uint8_t*)Icons::joystick, sizeof(Icons::joystick));
	joystickImage.setResourceId( ID_JOYSTICK );
    volumeImage.loadPng((uint8_t*)Icons::volume, sizeof(Icons::volume));
	volumeImage.setResourceId( ID_VOLUME );
    plugImage.loadPng((uint8_t*)Icons::plug, sizeof(Icons::plug));
	plugImage.setResourceId( ID_PLUG );
    displayImage.loadPng((uint8_t*)Icons::display, sizeof(Icons::display));
	displayImage.setResourceId( ID_DISPLAY );
    toolsImage.loadPng((uint8_t*)Icons::tools, sizeof(Icons::tools));
	toolsImage.setResourceId( ID_TOOLS );
	quitImage.loadPng((uint8_t*)Icons::quit, sizeof(Icons::quit));
	quitImage.setResourceId( ID_QUIT );
	keyboardImage.loadPng((uint8_t*)Icons::keyboard, sizeof(Icons::keyboard));
	keyboardImage.setResourceId( ID_KEYBOARD );
	colorImage.loadPng((uint8_t*)Icons::color, sizeof(Icons::color));
	colorImage.setResourceId( ID_COLOR );
	tapeImage.loadPng((uint8_t*)Icons::tape, sizeof(Icons::tape));
	tapeImage.setResourceId( ID_TAPE );
    paletteImage.loadPng((uint8_t*)Icons::palette, sizeof(Icons::palette));
	paletteImage.setResourceId( ID_PALETTE );
    cropImage.loadPng((uint8_t*)Icons::crop, sizeof(Icons::crop));
	cropImage.setResourceId( ID_CROP );
    playImage.loadPng((uint8_t*)Icons::play, sizeof(Icons::play));
	playImage.setResourceId( ID_PLAY );
    playhiImage.loadPng((uint8_t*)Icons::playHi, sizeof(Icons::playHi));
	playhiImage.setResourceId( ID_PLAYHI );
    stopImage.loadPng((uint8_t*)Icons::stop, sizeof(Icons::stop));
	stopImage.setResourceId( ID_STOP );
    stophiImage.loadPng((uint8_t*)Icons::stopHi, sizeof(Icons::stopHi));
	stophiImage.setResourceId( ID_STOPHI );
    recordImage.loadPng((uint8_t*)Icons::record, sizeof(Icons::record));
	recordImage.setResourceId( ID_RECORD );
    recordhiImage.loadPng((uint8_t*)Icons::recordHi, sizeof(Icons::recordHi));
	recordhiImage.setResourceId( ID_RECORDHI );
    forwardImage.loadPng((uint8_t*)Icons::forward, sizeof(Icons::forward));
	forwardImage.setResourceId( ID_FORWARD );
    forwardhiImage.loadPng((uint8_t*)Icons::forwardHi, sizeof(Icons::forwardHi));
	forwardhiImage.setResourceId( ID_FORWARDHI );
    rewindImage.loadPng((uint8_t*)Icons::rewind, sizeof(Icons::rewind));
	rewindImage.setResourceId( ID_REWIND );
    rewindhiImage.loadPng((uint8_t*)Icons::rewindHi, sizeof(Icons::rewindHi));
	rewindhiImage.setResourceId( ID_REWINDHI );
	counterImage.loadPng((uint8_t*)Icons::counter, sizeof(Icons::counter));
	counterImage.setResourceId( ID_COUNTER );
    diskImage.loadPng((uint8_t*) Icons::disk, sizeof (Icons::disk));
	diskImage.setResourceId( ID_DISK );
	editImage.loadPng((uint8_t*)Icons::edit, sizeof(Icons::edit));
	editImage.setResourceId( ID_EDIT );
    ejectImage.loadPng((uint8_t*)Icons::eject, sizeof(Icons::eject));
	ejectImage.setResourceId( ID_EJECT );
    fanImage.loadPng((uint8_t*)Icons::fan, sizeof(Icons::fan));
    fanImage.setResourceId( ID_FAN );
    hideImage.loadPng((uint8_t*)Icons::hide, sizeof(Icons::hide));
    hideImage.setResourceId( ID_HIDE );
    fullscreenImage.loadPng((uint8_t*)Icons::fullscreen, sizeof(Icons::fullscreen));
    fullscreenImage.setResourceId( ID_FULLSCREEN );

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
            emuThread->lock();
		    program->power(emulator);
            emuThread->unlock();
	    };	
        sM.system->append( *sM.poweron );

        if ( dynamic_cast<LIBC64::Interface*>(emulator)) {
            sM.poweronAndRemoveExpansions = new GUIKIT::MenuItem;
            sM.poweronAndRemoveExpansions->setIcon(powerImage);
            sM.poweronAndRemoveExpansions->onActivate = [emulator]() {
                emuThread->lock();
                program->power(emulator);
                program->removeExpansion(false);
                emuThread->unlock();
            };
            sM.system->append(*sM.poweronAndRemoveExpansions);
        }
		        
        sM.reset = new GUIKIT::MenuItem;
        sM.reset->onActivate = [emulator]() {
            emuThread->lock();
		    program->reset(emulator);
            emuThread->unlock();
	    };	
        sM.reset->setIcon( powerImage );
        sM.system->append( *sM.reset );
        sM.freeze = new GUIKIT::MenuItem;
        sM.freeze->setIcon( freezeImage );
        sM.freeze->onActivate = [emulator]() {
            emuThread->lock();
		    emulator->freezeButton();
            emuThread->unlock();
	    };	
        sM.freeze->setEnabled(false);
        sM.system->append( *sM.freeze );

        if ( dynamic_cast<LIBC64::Interface*>(emulator)) {
            sM.menu = new GUIKIT::MenuItem;
            sM.menu->setIcon( menuImage );
            sM.menu->onActivate = [emulator]() {
                emuThread->lock();
                emulator->customCartridgeButton();
                emuThread->unlock();
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
            emuView->show(EmuConfigView::TabWindow::Layout::Media);
            emuView->mediaLayout->setMediaView();
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

        sM.geometry = new GUIKIT::MenuItem;
        sM.geometry->setIcon( cropImage );
        sM.geometry->onActivate = [emulator]() {
            auto emuView = EmuConfigView::TabWindow::getView( emulator, true );
            emuView->show(EmuConfigView::TabWindow::Layout::Geometry);
        };
        sM.system->append( *sM.geometry );

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

        if (globalSettings->get<bool>("core_" + emulator->ident, true))
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

        emuThread->lock();
        std::string text = activeEmulator->copyText( );
        emuThread->unlock();
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
        emuThread->lock();
        program->fastForward( false );
        VideoManager::setSynchronize();
        statusHandler->resetFrameCounter();
        emuThread->unlock();
        bool threadedRenderer = globalSettings->get("threaded_renderer", true);

        adaptiveSyncItem.setEnabled( videoSyncItem.checked() && !threadedRenderer );
        dynamicRateControl.setEnabled( (videoSyncItem.checked() || vrrItem.checked()) && !threadedRenderer );
        vrrItem.setEnabled( threadedRenderer || !(videoSyncItem.checked() && adaptiveSyncItem.checked()) );
    };
    bool threadedRenderer = globalSettings->get("threaded_renderer", true);
    bool vsync = globalSettings->get<bool>("video_sync", true);
    bool vrr = globalSettings->get<bool>("vrr_sync", false);
    bool adaptive = globalSettings->get<bool>("adaptive_sync", true);

    if (vsync)
        videoSyncItem.setChecked();

    adaptiveSyncItem.setEnabled( !threadedRenderer && vsync );

    optionsMenu.append(videoSyncItem);

    adaptiveSyncItem.onToggle = [&]() {
        globalSettings->set<bool>("adaptive_sync", adaptiveSyncItem.checked() );
        emuThread->lock();
        program->fastForward( false );
        VideoManager::setSynchronize();
        emuThread->unlock();
        //statusHandler->resetFrameCounter();
        bool threadedRenderer = globalSettings->get("threaded_renderer", true);
        vrrItem.setEnabled( threadedRenderer || !(videoSyncItem.checked() && adaptiveSyncItem.checked()) );
    };
    if ( adaptive )
        adaptiveSyncItem.setChecked();

    optionsMenu.append(adaptiveSyncItem);

    vrrItem.setEnabled( threadedRenderer || !(vsync && adaptive) );

    vrrItem.onToggle = [&]() {
        globalSettings->set<bool>("vrr_sync", vrrItem.checked() );
        emuThread->lock();
        VideoManager::setSynchronize();
        emuThread->unlock();
        bool threadedRenderer = globalSettings->get("threaded_renderer", true);
        dynamicRateControl.setEnabled( (videoSyncItem.checked() || vrrItem.checked()) && !threadedRenderer );
    };
    if ( vrr ) vrrItem.setChecked();
    optionsMenu.append(vrrItem);

    dynamicRateControl.onToggle = [&]() {
        globalSettings->set<bool>("dynamic_rate_control", dynamicRateControl.checked() );
        emuThread->lock();
        VideoManager::setSynchronize();
        emuThread->unlock();
        //audioManager->setRateControl();
    };
    if ( globalSettings->get<bool>("dynamic_rate_control", false) )
        dynamicRateControl.setChecked();

    dynamicRateControl.setEnabled( !threadedRenderer && (vsync || vrr) );

    optionsMenu.append(dynamicRateControl);
        
    optionsMenu.append(*GUIKIT::MenuSeparator::getInstance());
        
    fullscreenItem.onActivate = [this]() {
        emuThread->lock();
        switchFullScreen( !fullScreen(), true );
        emuThread->unlock();
    };
    fullscreenItem.setIcon(fullscreenImage);
        
    optionsMenu.append(fullscreenItem);
        
    optionsMenu.append(*GUIKIT::MenuSeparator::getInstance());
            
    muteItem.onToggle = [&]() {
        globalSettings->set<bool>("audio_mute", muteItem.checked() );
        emuThread->lock();
        audioManager->setVolume();
        emuThread->unlock();
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

        emuThread->lock();
        fileloader->eject( emulator, emulator->getTape(0) );
        emuThread->unlock();
    };    
    
    tapeControlMenu.append( ejectTapeItem );
    
    tapeControlMenu.append( *GUIKIT::MenuSeparator::getInstance() );

	tapePlayItem.onActivate = []() {
		InputManager::activateHotkey(Hotkey::Id::PlayTape, activeEmulator);
	};
	tapeControlMenu.append( tapePlayItem );
	
	tapeStopItem.onActivate = []() {
		InputManager::activateHotkey(Hotkey::Id::StopTape, activeEmulator);
	};
	tapeControlMenu.append( tapeStopItem );
	
	tapeForwardItem.onActivate = []() {
		InputManager::activateHotkey(Hotkey::Id::ForwardTape, activeEmulator);
	};
	tapeControlMenu.append( tapeForwardItem );
	
	tapeRewindItem.onActivate = []() {
		InputManager::activateHotkey(Hotkey::Id::RewindTape, activeEmulator);
	};
	tapeControlMenu.append( tapeRewindItem );
	
	tapeRecordItem.onActivate = []() {
		InputManager::activateHotkey(Hotkey::Id::RecordTape, activeEmulator);
	};
	tapeControlMenu.append( tapeRecordItem );
	
    tapeControlMenu.append(*GUIKIT::MenuSeparator::getInstance());	
    
	tapeResetCounterItem.onActivate = []() {
		InputManager::activateHotkey(Hotkey::Id::ResetTapeCounter, activeEmulator);
	};
	tapeControlMenu.append( tapeResetCounterItem );  

    // speed menu
    speedControlMenu.setIcon( fanImage );
    speedControlMenu.showContextOnly(true);

    fastForwardItem.onToggle = []() {
        emuThread->lock();
        program->toggleFastForward( false );
        emuThread->unlock();
    };
    speedControlMenu.append( fastForwardItem );

    aggressiveFastForwardItem.onToggle = []() {
        emuThread->lock();
        program->toggleFastForward( true );
        emuThread->unlock();
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
            emuThread->lock();
            audioManager->setSynchronize();
            audioManager->setResampler();
            statusHandler->resetFrameCounter();
            emuThread->unlock();
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
    append( speedControlMenu );

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

            emuThread->lock();
            fileloader->eject( emulator, emulator->getDisk(i) );
            emuThread->unlock();
        };
        diskControlMenu.menu.append( diskControlMenu.eject );

        diskControlMenu.menu.append( *GUIKIT::MenuSeparator::getInstance() );

        diskControlMenu.reset.setIcon( powerImage );

        diskControlMenu.reset.onActivate = [i]() {
            auto emulator = activeEmulator;

            if (!activeEmulator)
                emulator = program->getLastUsedEmu();

            emuThread->lock();
            emulator->resetDrive( emulator->getDisk(i) );
            emuThread->unlock();
        };
        diskControlMenu.menu.append( diskControlMenu.reset );

        diskControlMenu.inactive.setIcon( hideImage );

        diskControlMenu.inactive.onActivate = [i]() {
            auto emulator = activeEmulator;

            if (!activeEmulator)
                emulator = program->getLastUsedEmu();

            emuThread->lock();
            emulator->hideDrive( emulator->getDisk(i) );
            emuThread->unlock();
        };
        diskControlMenu.menu.append( diskControlMenu.inactive );

        i++;
    }   
}

auto View::updateSpeedLabels() -> void {
    std::string label;

    if (!activeEmulator)
        return;

    auto stat = activeEmulator->getStatsForSelectedRegion();

    speedItems[0]->setText( GUIKIT::String::formatFloatingPoint( stat.fps, 3) + " FPS ( 100 % )");

    unsigned _size = speedItems.size();

    for(unsigned i = 1; i < _size; i++) {
        if (i == (_size - 2) ) // maximum
            continue;

        float value;
        bool percent;
        getSpeed(i, value, percent);

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

auto View::updateDiskMenu() -> void {
    bool showResetAndHide = dynamic_cast<LIBC64::Interface*>(activeEmulator);

    for(auto& d : diskControlMenus) {
        if (d.reset.enabled() != showResetAndHide) {
            d.reset.setEnabled(showResetAndHide);
            d.inactive.setEnabled(showResetAndHide);
        }
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
        sysMenu.geometry->setText(trans->get("Geometry"));
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
    vrrItem.setText( trans->get("VRR"));
    dynamicRateControl.setText( trans->get("dynamic_rate_control"));

    fullscreenItem.setText( trans->get("fullscreen"));
    
    muteItem.setText( trans->get("mute_audio"));
    fpsItem.setText( trans->get("show_fps"));
    audioBufferItem.setText( trans->get("show_audio_buffer"));
    
    saveItem.setText( trans->get("save_preferences"));
    exit.setText(trans->get("Exit"));
	
	tapeControlMenu.setText( trans->get("Datasette") );
    insertTapeItem.setText( trans->get("insert") );
    ejectTapeItem.setText( trans->get("eject") );
    speedControlMenu.setText( trans->get("Speed") );

    for (auto& diskControlMenu : diskControlMenus) {
        diskControlMenu.insert.setText( trans->get("insert") );
        diskControlMenu.eject.setText( trans->get("eject") );
        diskControlMenu.reset.setText( trans->get("Reset Floppy") );
        diskControlMenu.inactive.setText( trans->get("inactive until reset") );
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
    
    if (videoDriver && videoDriver->hasExclusiveFullscreen())
        switchFullScreen( false );
    
    bool state = !globalSettings->get<bool>("question_media_write", true);
    
    if (!state) {
        if (fullScreen() && inputDriver->mIsAcquired())
            inputDriver->mUnacquire();
        state = message->question(trans->get("question permanent write", {{"%media%", media->name}}));
    }
    
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

    auto& stats = activeEmulator->stats;

    switch (pos) {
        case 0: speed = stats.fps; break;
        case 1: speed = stats.isPal() ? 50.0 : 60.0; break;
        case 2: speed = 5.0; break;
        case 3: speed = 25.0; break;
        case 4: speed = stats.isPal() ? 60.0 : 50.0; break; // alternate
        case 5: speed = 70.0; break;
        case 6: speed = 75.0; break;
        case 7: speed = 80.0; break;
        case 8: speed = 90.0; break;
        case 9: speed = 100.0; break;
        case 10: speed = 120.0; break;
        case 11: speed = 250.0; break; // maximum
        case 12:
            auto settings = program->getSettings( activeEmulator );
            speed = settings->get<float>("custom_speed", 59.95);
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

auto View::threadedRendererWasToggled(bool state) -> void {
    bool vsync = globalSettings->get<bool>("video_sync", true);
    bool vrr = globalSettings->get<bool>("vrr_sync", false);
    bool adaptive = globalSettings->get<bool>("adaptive_sync", true);

    view->adaptiveSyncItem.setEnabled(vsync && !state);
    view->vrrItem.setEnabled(state || !(vsync && adaptive));
    view->dynamicRateControl.setEnabled((vsync || vrr) && !state);
}

auto View::updateEmuUsage() -> void {
    remove(tapeControlMenu);
    remove(speedControlMenu);
    remove(optionsMenu);
    remove(editMenu);
    remove(controlMenu);

    for(auto& sysMenu : sysMenus)
        remove(*sysMenu.system);

    for(auto emulator : emulators) {
        bool useCore = globalSettings->get<bool>("core_" + emulator->ident, true);
        if (!useCore)
            continue;

        for(auto& sysMenu : sysMenus) {
            if (sysMenu.emulator == emulator) {
                append( *sysMenu.system );
                break;
            }
        }
    }

    setConnectors();
    append(controlMenu);
    append(editMenu);
    append(optionsMenu);
    append(speedControlMenu);
    if (activeEmulator->getModelValue( activeEmulator->getModelIdOfEnabledDrives( activeEmulator->getTapeMediaGroup() ) ))
        append(tapeControlMenu);
}

auto View::updateGeometry(bool withViewport) -> void {
    GUIKIT::Geometry defaultGeometry = {100, 100, 800, 600};

    GUIKIT::Geometry geometry = {globalSettings->get<int>("screen_x", defaultGeometry.x)
            ,globalSettings->get<int>("screen_y", defaultGeometry.y)
            ,globalSettings->get<unsigned>("screen_width", defaultGeometry.width)
            ,globalSettings->get<unsigned>("screen_height", defaultGeometry.height)
    };

    setGeometry( geometry );

    if (isOffscreen())
        setGeometry( defaultGeometry );

    if (withViewport)
        updateViewport();
}