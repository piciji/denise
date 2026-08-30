
#include "view.h"
#include "../program.h"
#include "../config/config.h"
#include "../config/layouts/settings.h"
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
#include "../tools/chronos.h"
#include "../emuconfig/layouts/presentation.h"
#include "../emuconfig/layouts/audio.h"
#include "../emuconfig/layouts/input.h"
#include "../debugger/debugger.h"
#include "../helper/fileHelper.h"
#include "../helper/miscHelper.h"
#include <sstream>

View* view = nullptr;

View::View() : GUIKIT::Window(GUIKIT::Window::Hints::Video) {
    message = new Message(this);
}

auto View::build() -> void {
    setTitle( APP_NAME " " VERSION );
    setBackgroundColor(0);
    cocoa.setDisableIconsInTopMenu(true);

    updateGeometry();
    
    append(viewport);
    
    loadImages();

    statusBar.setFont( GUIKIT::Font::system() );
        
	statusHandler->init(&statusBar);

    append(statusBar);
		
    if (!cmd->noGui) {                
        
        buildMenu();
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
            if (activeVideoManager && emuThread->enabled) {
                videoDriver->lockResize();
                updateViewport();
                videoDriver->unlockResize();
            } else
                updateViewport();
      
			if (activeVideoManager) {
                if (!emuThread->enabled) {
                    emuThread->lock();
                    videoDriver->redraw();
                    videoDriver->freeContext();
                    emuThread->unlock();
                }
			}
        }

    //    if (!emuThread->enabled)
	//	    audioDriver->clear();
    };

    onResizeStart = [this] {
        videoDriver->hintResizing(true);

        if (activeVideoManager /*&& !fullScreen() && !requestFullscreenSwitch*/) {
            if (videoDriver->needResizingPreparations(emuThread->enabled)) {
                emuThread->lock();
                videoDriver->prepareResizing();
                videoDriver->changeThreadPriorityToRealtime(false);
                emuThread->unlock();
                customResizeMode = true;
            }
        }
        GUIKIT::Size screenRatio = {0,0};
        auto settings = Program::getSettings( activeEmulator );

        if (settings->get<bool>("aspect_correct_resizing", false)) {
            switch(videoDriver->getAspectRatio()) {
                case 1: screenRatio = {4,3}; break;
                case 2:
                case 3: videoDriver->getIntegerScalingDimension(screenRatio.width, screenRatio.height); break;
            }
        }
        return screenRatio;
    };

    onResizeEnd = [this]() {

        if (customResizeMode) {
            emuThread->lock();
            videoDriver->endResizing();
            videoDriver->changeThreadPriorityToRealtime(true);
            emuThread->unlock();
            customResizeMode = false;
        }
        
        videoDriver->hintResizing(false);
    };
    
    onWillFullscreen = [this]() {
        emuThread->lock();
        videoDriver->changeThreadPriorityToRealtime(false);
      //  emuThread->unlock();
    };
    
    onWillUnfullscreen = [this]() {
        emuThread->lock();
        videoDriver->changeThreadPriorityToRealtime(false);
        //emuThread->unlock();
    };
    
    onFullscreen = [this]() {
        emuThread->lock();
        videoDriver->changeThreadPriorityToRealtime(true);
        emuThread->unlock();
    };
    
    onUnfullscreen = [this]() {
        emuThread->lock();
        videoDriver->changeThreadPriorityToRealtime(true);
        emuThread->unlock();
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

    onInactive = [this]() {
        videoDriver->activateApp(false);
    };

    onActive = [this]() {
        videoDriver->activateApp(true);
    };

    onMinimize = [this]() {
        if (program->quitInProgress || program->hasActiveDebugger())
            return;
        static auto pauseFocusLoss = globalSettings->getOrInit("pause_focus_loss", false);
        program->isPause &= ~2;
        program->isPause |= (!!*pauseFocusLoss) << 1;
    };

    onUnminimize = [this]() {
        this->updateViewport();
        statusHandler->resetFrameCounter();
    };

    onRealize = [this]() {
		updateViewport();
        program->finishStartup();
    };

    onFocus = [this]() {
        if (!program->hasActiveDebugger())
            program->isPause &= ~2;
    };

    onUnFocus = [this]() {
        if (program->quitInProgress)
            return;
        static auto pauseFocusLoss = globalSettings->getOrInit("pause_focus_loss", false);

        if (inputDriver && inputDriver->mIsAcquired())
            inputDriver->mUnacquire();

        if (program->hasActiveDebugger())
            return;

        program->isPause &= ~2;
        program->isPause |= (!!*pauseFocusLoss) << 1;
    };
	
	winapi.onMenu = []() {
	//	audioDriver->clear();
	};
    
    onKeyPress = [this](bool keyDown, uint16_t keyCode) {
        inputDriver->sentUIKeyPresses(keyDown, keyCode);
    };
	
	GUIKIT::BrowserWindow::onCall = []() {
        if (!emuThread->enabled || !videoDriver->hasThreaded())
		    audioDriver->clear();
	};

    GUIKIT::Application::Cocoa::onOpenFile = [this] (std::string fileName) {
        if (cmd->hasContent)
            return;

        emuThread->lock();
        autoloader->init( {fileName}, Autoloader::Mode::AutoStart );
        
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
        for (auto rF : fileloader->recentFiles)
            rF->save();
		SettingsHelper::saveSettings();
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

    GUIKIT::Monitor::onFullscreenRefreshChange = [this](float rate) {
        if (!activeEmulator)
            return;
        auto settings = Program::getSettings( activeEmulator );
        if (!settings->get<bool>("fullscreen_setting_adjust_emu_speed", false))
            return;

        overrideFullscreenRefreshRate = rate;
        emuThread->lock();
        audioManager->setSynchronize();
        audioManager->setResampler();
        statusHandler->resetFrameCounter();
        emuThread->unlock();
    };
	
	anyloadTimer.setInterval(40);
    displayChangeTimer.setInterval(500);
    displayChangeTimer.onFinished = [this]() {
        displayChangeTimer.setInterval(500);
        displayChangeTimer.setEnabled(false);
        //statusHandler->setMessage( std::to_string(GUIKIT::Monitor::getCurrentRefreshRate()) );
        emuThread->lock();
        videoDriver->waitRenderThread();

		if (videoDriver && fullscreenSetting.inUse
			&& emuThread->enabled
			&& videoDriver->hasThreaded())
            videoDriver->forceResize();
		
		else if (!requestFullscreenSwitch && !fullScreen()) {
			videoDriver->forceResize();
		}

        VideoManager::setSynchronize();
		requestFullscreenSwitch = false;
        emuThread->unlock();
    };
    
	fullscreenOnStartUp.setInterval(800);
	fullscreenOnStartUp.onFinished = [this]() {
		fullscreenOnStartUp.setEnabled(false);
		emuThread->lock();
        switchFullScreen(true);
        emuThread->unlock();
	};
    
    cursorHideTimer.setInterval(1000);
    cursorHideTimer.onFinished = [this]() {
        cursorHideTimer.setEnabled(false);
        if (cursorHideTimer.data() & 1) {
            view->setForeground();
            view->setFocused();
        }
        inputDriver->mAcquire();
    };
	
    viewport.onMouseRelease = [this](GUIKIT::Mouse::Button button) {
        inputDriver->sentUIMousePresses(false, (DRIVER::Input::Button)button);
    };
    
	viewport.onMousePress = [this](GUIKIT::Mouse::Button button) {
        inputDriver->sentUIMousePresses(true, (DRIVER::Input::Button)button);
        
	    if (button == GUIKIT::Mouse::Button::Left) {

	        if (videoDriver->visibleSplashScreen()) {
	            emuThread->lock(true);

	            int result = cursorForPlaceholderInUpperTriangle();
	            if (result != -1)
	                videoDriver->hideSplashScreen();

	            if (result == 1)
	                program->power(program->getEmulator("C64"));
	            else if (result == 0)
	                program->power(program->getEmulator("Amiga"));

	            emuThread->unlock();
	        } else if ( grabMouseLeft && !inputDriver->mIsAcquired()) {
	            prepareCursorHide(200);
	        }
	    }
	};
    
    viewport.onMouseMove = [this](GUIKIT::Position& pos, int deltaX, int deltaY) {
#ifdef __APPLE__
        inputDriver->sentUIMouseMovement(deltaX, deltaY);
#endif
    };
	
	viewport.onMouseLeave = []() {

	};
	
    setDragnDrop();

    viewport.hideCursorByInactivity(2000);
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
    // aspect correct viewport doesn't fill up the complete window.
    // thats why, we have to set drop event on whole window too.
    // but viewport is on top of window, so we simply set drop event on both.
    onDrop = [this]( std::vector<std::string> files ) {
        //statusHandler->setMessage("drop");
        emuThread->lock();
        dropZone = 0;
        videoDriver->enableDragnDropOverlay(false);
        viewport.onDrop( files );
        emuThread->unlock();
    };

    viewport.onDragEnter = [this]( std::vector<std::string> files ) {
        //statusHandler->setMessage("enter");
        emuThread->lock();
        auto slots = autoloader->needSlotsForDragnDrop(files);
        videoDriver->setDragnDropOverlaySlots(slots);
        videoDriver->enableDragnDropOverlay(true);
        dropZone = 0;
        emuThread->unlock();
    };

    viewport.onDragLeave = [this]() {
        //statusHandler->setMessage("leave");
        emuThread->lock();
        videoDriver->enableDragnDropOverlay(false);
        emuThread->unlock();
    };

    viewport.onDragMove = [this](int x, int y) {
        if (!viewport.onDragEnter) {
            videoDriver->setDragnDropOverlaySlots(4);
            videoDriver->enableDragnDropOverlay(true);
        }

        dropZone = videoDriver->sendDragnDropOverlayCoordinates(x, y);
        //statusHandler->setMessage(std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(dropZone) );
    };
    
    viewport.onDrop = [this]( std::vector<std::string> files ) {
        Autoloader::Mode mode = Autoloader::Mode::DragnDrop;
        unsigned selection = 0;
        if (dropZone & 0x8000) {
            selection = dropZone & 0xff;
            mode = (dropZone & 0x100) ? Autoloader::Mode::AutoStartWithSlot : Autoloader::Mode::OpenWithSlot;
        }

        emuThread->lock();
        dropZone = 0;
        videoDriver->enableDragnDropOverlay(false);
        autoloader->init( files, mode, selection );
        autoloader->loadFiles();

        bool activeCore = globalSettings->get<bool>("core_" + activeEmulator->ident, true);
        if (!activeCore) {
            globalSettings->set<bool>("core_" + activeEmulator->ident, true);
            view->updateEmuUsage();
            if (configView && configView->settingsLayout) {
                configView->settingsLayout->activateCore(activeEmulator);
            }
        }

        view->setFocused(100);
        emuThread->unlock();
    };

    viewport.setDroppable();

    setDroppable();
}

auto View::switchFullScreen(bool fullScreen, bool forceUnacquire) -> void {
	requestFullscreenSwitch = true;
    if(!forceUnacquire && fullScreen) {
        if (GUIKIT::Application::isCocoa())
            prepareCursorHide(1000);
        else
            inputDriver->mAcquire();
    } else {
        cursorHideTimer.setEnabled(false);
        inputDriver->mUnacquire();
    }

    if (!fullScreen && videoDriver) {
        videoDriver->disableExclusiveFullscreen();
    }

    GUIKIT::Window::setFullScreen(fullScreen);
    if (videoDriver->hasExclusiveFullscreen())
        displayChangeTimer.setInterval(600);
    displayChangeTimer.setEnabled();

    if (!fullScreen && (overrideFullscreenRefreshRate != 0.0) ) {
        overrideFullscreenRefreshRate = 0.0;
        audioManager->setResampler();
        if (statusHandler)
            statusHandler->resetFrameCounter();

    }

    if (statusHandler)
        statusHandler->updateOnScreenFPS();
}

auto View::prepareCursorHide(unsigned interval, bool withFocus) -> void {
    cursorHideTimer.setInterval(interval);
    cursorHideTimer.setData(withFocus ? 1 : 0);
    cursorHideTimer.setEnabled();
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

auto View::updateRecentList(Emulator::Interface* emulator) -> void {
    auto recentFile = fileloader->getRecentFile(emulator);

    auto list = recentFile->list(nullptr);
    auto sysMenu = getSysMenu(emulator);
    auto recentSoftware = sysMenu->recentSoftware;
    
    bool clearFooter = false;
    unsigned i = 0;
    for(auto& fileIdent : list) {
        if (fileIdent.path.empty())
            continue;

        if (i >= recentFile->getEntries())
            break;

        if (!sysMenu->recents[i]) {
            GUIKIT::MenuItem* item;
            item = new GUIKIT::MenuItem;
            item->onActivate = [this, item]() {
                emuThread->lock();
                autoloader->init({ GUIKIT::File::resolveRelativePath(item->filePath()) }, Autoloader::Mode::DragnDrop);
                autoloader->setArchiveId( (int)item->state.fileId );
                autoloader->loadFiles();
                emuThread->unlock();
            };
            sysMenu->recents[i] = item;
            if (!clearFooter) {                
                recentSoftware->remove(*sysMenu->recentControlSeparator);
                recentSoftware->remove(*sysMenu->recentControl);
                clearFooter = true;
            }
            recentSoftware->append(*item);
        }

        sysMenu->recents[i]->setText(fileIdent.file);
        sysMenu->recents[i]->setFileIdent(fileIdent.path, fileIdent.id);
        i++;
    }

    if (clearFooter) {
        recentSoftware->append(*sysMenu->recentControlSeparator);
        recentSoftware->append(*sysMenu->recentControl);
    }
    
    if (recentSoftware->enabled() == recentSoftware->childs.empty())
        recentSoftware->setEnabled(!recentSoftware->childs.empty());
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
        
        auto settings = Program::getSettings( emulator );
        
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
        std::string txt = dynamic_cast<LIBAMI::Interface*>(emulator) ? "swap joypads Port2" : "swap Ports";
		inputItem->setText(emulator->ident + " " + trans->getA(txt) );
        
        inputItem->onActivate = [emulator, settings]() {
            emuThread->lock();
            auto connector1 = emulator->getConnector( 0 );
            auto connectedDevice1 = emulator->getConnectedDevice( connector1 );
            
            auto connector2 = emulator->getConnector( 1 );
            auto connectedDevice2 = emulator->getConnectedDevice( connector2 );

            if (dynamic_cast<LIBAMI::Interface*>(emulator)) {
                for (auto& device : emulator->devices) {
                    if (!device.isJoypad())
                        continue;
                    if (!connectedDevice2 || !connectedDevice2->isJoypad()) {
                        connectedDevice2 = &device;
                        break;
                    }
                    if (connectedDevice2 != &device) {
                        connectedDevice2 = &device;
                        break;
                    }
                }

                emulator->connect( connector2, connectedDevice2 );
                auto manager = InputManager::getManager(emulator);
                manager->updateMappingsInUse();
                emuThread->unlock();

                settings->set<unsigned>( _underscore(connector2->name), connectedDevice2->id);
                view->checkInputDevice(emulator, connector2, connectedDevice2);
            } else {
                emulator->connect( connector1, connectedDevice2 );
                emulator->connect( connector2, connectedDevice1 );

                emuThread->unlock();

                settings->set<unsigned>( _underscore(connector1->name), connectedDevice2->id);
                settings->set<unsigned>( _underscore(connector2->name), connectedDevice1->id);

                view->checkInputDevice(emulator, connector1, connectedDevice2);
                view->checkInputDevice(emulator, connector2, connectedDevice1);
            }

            auto emuView = EmuConfigView::TabWindow::getView(emulator);
            if (emuView && emuView->inputLayout)
                emuView->inputLayout->updateConnectorButtons();
        };
        
        inputItem->setIcon(swapperImage);
        controlMenu.append(*inputItem);

        inputItem = new GUIKIT::MenuItem;
        inputItem->setText(emulator->ident + " " + trans->get("config") + "..." );

        inputItem->onActivate = [emulator]() {
            auto emuView = EmuConfigView::TabWindow::getView( emulator, true );
            emuView->show(EmuConfigView::TabWindow::Layout::Control);
        };
        inputItem->setIcon(toolsImage);
        controlMenu.append(*inputItem);
    }
}

auto View::updateShader(Emulator::Interface* emulator) -> void {
    for(auto& sM : sysMenus) {
        if (sM.emulator != emulator)
            continue;

        if (!sM.shaderFavourites.size())
            continue;

        auto vManager = VideoManager::getInstance( sM.emulator );
        std::string loaded = vManager->getPresetPath();
        if (vManager->crtMode != VideoManager::CrtMode::Gpu) {
            sM.shaderFavourites[0].item->setChecked();
            break;
        }

        bool found = false;

        for(auto& fav : sM.shaderFavourites) {
            if(fav.path == loaded) {
                fav.item->setChecked();
                found = true;
                break;
            }
        }

        if (!found)
            sM.shaderFavourites[0].item->setChecked();
    }
}

auto View::buildShader() -> void {
    bool visible = videoDriver && videoDriver->shaderSupport();

    for(auto& sM : sysMenus) {
        removeMenuTree( sM.shaderMenu );
        auto emulator = sM.emulator;
        auto settings = Program::getSettings(emulator);
		auto vManager = VideoManager::getInstance( emulator );
        bool shaderActive = vManager->crtMode == VideoManager::CrtMode::Gpu;

        std::string loaded = vManager->getPresetPath();
        sM.shaderFavourites.clear();
        std::vector<GUIKIT::MenuRadioItem*> items;

        GUIKIT::MenuRadioItem* noneItem = new GUIKIT::MenuRadioItem;
        GUIKIT::MenuRadioItem* checkedItem = noneItem;
        noneItem->setText(trans->getA("none"));
        noneItem->setChecked();
        noneItem->onActivate = [emulator, vManager]() {
            emuThread->lock();
            auto emuView = EmuConfigView::TabWindow::getView(emulator);
            if (emuView && emuView->presentationLayout)
                emuView->presentationLayout->unloadShader();
            else
                vManager->clearPreset();
            emuThread->unlock();
        };

        sM.shaderMenu->append(*noneItem);
        items.push_back(noneItem);

        int i = 0;
        while(1) {
            std::string fav = settings->get<std::string>( "shader_fav_" + std::to_string(i), "");
            fav = GUIKIT::File::resolveRelativePath(fav);
            if (fav.empty())
                break;

            sM.shaderFavourites.push_back({fav, nullptr});
            i++;
        }

        if (sM.shaderFavourites.size() == 0) {
            sM.shaderMenu->setEnabled( false );
            continue;
        }

        sM.shaderMenu->setEnabled();

        for (auto& fav : sM.shaderFavourites) {
            fav.item = new GUIKIT::MenuRadioItem;
            fav.item->setText(GUIKIT::String::getFileName(fav.path, true));

            if (loaded == fav.path)
                checkedItem = fav.item;

            std::string shaderPath = fav.path;
            fav.item->onActivate = [emulator, vManager, shaderPath]() {
                emuThread->lock();
                auto emuView = EmuConfigView::TabWindow::getView(emulator);

                if (emuView && emuView->presentationLayout)
                    emuView->presentationLayout->loadShader(shaderPath);
                else {
                    program->activateGPU(emulator, true);
                    vManager->loadPreset(shaderPath);
                }
                emuThread->unlock();
            };
            sM.shaderMenu->append(*fav.item);
            items.push_back(fav.item);
        }

        sM.shaderFavourites.insert(sM.shaderFavourites.begin(), {"none", noneItem});

        GUIKIT::MenuRadioItem::setGroup(items);
        if (checkedItem && shaderActive)
            checkedItem->setChecked();

        for(auto child : sM.shaderMenu->childs)
            child->setEnabled(visible);
    }
}

auto View::updatePauseCheck() -> void {

    if ((program->isPause & 1) != pauseItem.checked() )
        pauseItem.setChecked(program->isPause & 1);
}

auto View::updateWarpCheck() -> void {
    bool warpNormal = program->warp.mode == Program::Warp::Normal;
    bool warpAggressive = program->warp.mode == Program::Warp::Aggressive;
    bool fastForward = program->warp.mode == Program::Warp::FastForward;

    if (warpNormal != warpItem.checked())
        warpItem.setChecked(warpNormal);

    if (warpAggressive != aggressiveWarpItem.checked())
        aggressiveWarpItem.setChecked(warpAggressive);

    if (fastForward != fastForwardItem.checked())
        fastForwardItem.setChecked(fastForward);
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
    powerImage.loadPng((uint8_t*)Icons::power, sizeof(Icons::power));
    freezeImage.loadPng((uint8_t*)Icons::freeze, sizeof(Icons::freeze));
    menuImage.loadPng((uint8_t*)Icons::menu, sizeof(Icons::menu));
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
    fanImage.loadPng((uint8_t*)Icons::fan, sizeof(Icons::fan));
    hideImage.loadPng((uint8_t*)Icons::hide, sizeof(Icons::hide));
    fullscreenImage.loadPng((uint8_t*)Icons::fullscreen, sizeof(Icons::fullscreen));
    infoImage.loadPng((uint8_t*)Icons::info, sizeof(Icons::info));
    gearsImage.loadPng((uint8_t*)Icons::gears, sizeof(Icons::gears));
    delImage.loadPng((uint8_t*) Icons::del, sizeof (Icons::del));
    openImage.loadPng((uint8_t*)Icons::open, sizeof(Icons::open));
    clearImage.loadPng((uint8_t*)Icons::clear, sizeof(Icons::clear));
    insertImage.loadPng((uint8_t*)Icons::insert, sizeof(Icons::insert));
    screenshotImage.loadPng((uint8_t*)Icons::screenshot, sizeof(Icons::screenshot));
    debugImage.loadPng((uint8_t*)Icons::debug, sizeof(Icons::debug));

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

    ledGreen2Image.loadPng((uint8_t*) Icons::ledGreen2, sizeof (Icons::ledGreen2));
    ledGreen2DimImage.loadPng((uint8_t*) Icons::ledGreen2Dim, sizeof (Icons::ledGreen2Dim));
    ledRed2Image.loadPng((uint8_t*) Icons::ledRed2, sizeof (Icons::ledRed2));
    ledYellowImage.loadPng((uint8_t*) Icons::ledYellow, sizeof (Icons::ledYellow));

    ledGreenRoundImage.loadPng((uint8_t*)Icons::ledGreenRound, sizeof(Icons::ledGreenRound));
    ledRedRoundImage.loadPng((uint8_t*) Icons::ledRedRound, sizeof (Icons::ledRedRound));
    ledOffRoundImage.loadPng((uint8_t*)Icons::ledOffRound, sizeof(Icons::ledOffRound));
    recordAudioImage.loadPng((uint8_t*)Icons::recordAudio, sizeof(Icons::recordAudio));
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
            emuThread->lock(true);
		    program->power(emulator);
            emuThread->unlock();
	    };	
        sM.system->append( *sM.poweron );

        if ( dynamic_cast<LIBC64::Interface*>(emulator)) {
            sM.poweronAndRemoveExpansions = new GUIKIT::MenuItem;
            sM.poweronAndRemoveExpansions->setIcon(powerImage);
            sM.poweronAndRemoveExpansions->onActivate = [emulator]() {
                emuThread->lock(true);
                MiscHelper::removeExpansion(emulator);
                program->power(emulator);
                view->updateCartButtons( emulator );
                emuThread->unlock();
            };
            sM.system->append(*sM.poweronAndRemoveExpansions);
            sM.poweronAndRemoveDisks = nullptr;
        } else {
            sM.poweronAndRemoveDisks = new GUIKIT::MenuItem;
            sM.poweronAndRemoveDisks->setIcon(powerImage);
            sM.poweronAndRemoveDisks->onActivate = [emulator]() {
                emuThread->lock(true);
                for(auto& media : emulator->getDiskMediaGroup()->media)
                    fileloader->eject( emulator, &media );
                program->power(emulator);
                emuThread->unlock();
            };
            sM.system->append(*sM.poweronAndRemoveDisks);
            sM.poweronAndRemoveExpansions = nullptr;
        }
		        
        sM.reset = new GUIKIT::MenuItem;
        sM.reset->onActivate = [emulator]() {
            emuThread->lock(true);
		    program->reset(emulator);
            emuThread->unlock();
	    };	
        sM.reset->setIcon( powerImage );
        sM.system->append( *sM.reset );

        if ( dynamic_cast<LIBC64::Interface*>(emulator)) {
            sM.freeze = new GUIKIT::MenuItem;
            sM.freeze->setIcon( freezeImage );
            sM.freeze->onActivate = [emulator]() {
                emuThread->lock();
                emulator->freezeButton();
                emuThread->unlock();
            };
            sM.freeze->setEnabled(false);
            sM.system->append( *sM.freeze );

            sM.menu = new GUIKIT::MenuItem;
            sM.menu->setIcon( menuImage );
            sM.menu->onActivate = [emulator]() {
                emuThread->lock();
                emulator->customCartridgeButton();
                emuThread->unlock();
            };
            sM.menu->setEnabled(false);
            sM.system->append(*sM.menu);
            sM.mhz2LED = new GUIKIT::MenuItem;
            sM.mhz2LED->setIcon(menuImage);
            sM.mhz2LED->onActivate = [emulator]() {
                if (emulator == activeEmulator)
                    statusHandler->toggleLED(Emulator::Interface::LedId::MHz2);
            };
            sM.system->append(*sM.mhz2LED);
            sM.powerLED = nullptr;
            sM.capsLED = nullptr;
        } else {
            sM.powerLED = new GUIKIT::MenuItem;
            sM.powerLED->setIcon( menuImage );
            sM.powerLED->onActivate = [emulator]() {
                if (emulator == activeEmulator)
                    statusHandler->toggleLED(Emulator::Interface::LedId::Power);
            };
            sM.system->append(*sM.powerLED);

            sM.capsLED = new GUIKIT::MenuItem;
            sM.capsLED->setIcon(menuImage);
            sM.capsLED->onActivate = [emulator]() {
                if (emulator == activeEmulator)
                    statusHandler->toggleLED(Emulator::Interface::LedId::CapsLock);
            };
            sM.system->append(*sM.capsLED);
            sM.mhz2LED = nullptr;
            sM.menu = nullptr;
            sM.freeze = nullptr;
        }

        sM.system->append(*GUIKIT::MenuSeparator::getInstance());
		
		sM.loadSoftware = new GUIKIT::MenuItem;
        sM.loadSoftware->setIcon(insertImage);
        sM.loadSoftware->onActivate = [this, emulator]() {			
            setAnyload( emulator );
	    };
        sM.system->append( *sM.loadSoftware );

        sM.recentSoftware = new GUIKIT::Menu;
        sM.recentSoftware->setIcon(openImage);
        for(auto& recent : sM.recents)
            recent = nullptr;

        sM.recentControl = new GUIKIT::Menu;        
        sM.recentClearEntries = new GUIKIT::MenuItem;
        sM.recentClearEntries->setIcon(clearImage);
        sM.recentClearEntries->onActivate = [this, emulator]() {
            emuThread->lock();
            auto recent = fileloader->getRecentFile(emulator);
            if (recent) {
                recent->clear();
                clearRecentList(emulator);
            }
            emuThread->unlock();
        };
        sM.recentSeparator = new GUIKIT::MenuSeparator;
        sM.recentControlSeparator = new GUIKIT::MenuSeparator;
        sM.recentControl->append(*sM.recentClearEntries);
        sM.recentControl->append(*sM.recentSeparator);

        int entries = 10;
        sM.recentEntries.resize(7);
        auto recent = fileloader->getRecentFile(emulator);
        auto curEntries = recent->getEntries();
        GUIKIT::MenuRadioItem* checkedRe = nullptr;

        for(auto& rE : sM.recentEntries) {
            rE = new GUIKIT::MenuRadioItem;            

            rE->onActivate = [this, emulator, entries]() {
                emuThread->lock();

                auto recent = fileloader->getRecentFile(emulator);
                if (recent) {
                    recent->setGenericEntries(entries);
                    clearRecentList(emulator);
                    updateRecentList(emulator);
                }
                emuThread->unlock();
            };
            sM.recentControl->append(*rE);
            if (curEntries == entries)
                checkedRe = rE;
            entries += 5;
        }        
        GUIKIT::MenuRadioItem::setGroup(sM.recentEntries);
        if (checkedRe)
            checkedRe->setChecked();

        sM.system->append(*sM.recentSoftware);
        
        sM.media = new GUIKIT::MenuItem;
        sM.media->setIcon( driveImage );
        sM.media->onActivate = [emulator]() {
            auto emuView = EmuConfigView::TabWindow::getView( emulator, true );
            emuView->show(EmuConfigView::TabWindow::Layout::Media);
           // emuView->mediaLayout->setMediaView();
	    };
        sM.system->append( *sM.media );

        sM.states = new GUIKIT::Menu;
        sM.states->setIcon( scriptImage );

        sM.save = new GUIKIT::MenuItem;
        sM.save->onActivate = [emulator]() {
            if (emulator == activeEmulator) {
                emuThread->lock();
                States::getInstance( emulator )->save();
                emuThread->unlock();
            }
        };
        sM.states->append( *sM.save );

        sM.slotUp = new GUIKIT::MenuItem;
        sM.slotUp->onActivate = [emulator]() {
            if (emulator == activeEmulator) {
                emuThread->lock();
                States::getInstance( activeEmulator )->changeSlot( false );
                emuThread->unlock();
            }
        };
        sM.states->append( *sM.slotUp );

        sM.slotDown = new GUIKIT::MenuItem;
        sM.slotDown->onActivate = [emulator]() {
            if (emulator == activeEmulator) {
                emuThread->lock();
                States::getInstance( activeEmulator )->changeSlot( true );
                emuThread->unlock();
            }
        };
        sM.states->append( *sM.slotDown );

        sM.load = new GUIKIT::MenuItem;
        sM.load->onActivate = [emulator]() {
            emuThread->lock(true);
            States::getInstance( emulator )->load();
            emuThread->unlock();
        };
        sM.states->append( *sM.load );

        sM.system->append( *sM.states );

        sM.system->append(*GUIKIT::MenuSeparator::getInstance());

        sM.shaderMenu = new GUIKIT::Menu;
        sM.shaderMenu->setIcon( colorImage );
        sM.system->append( *sM.shaderMenu );
		
		sM.system->append(*GUIKIT::MenuSeparator::getInstance());
        
		sM.systemManagement = new GUIKIT::MenuItem;
        sM.systemManagement->setIcon( systemImage );
        sM.systemManagement->onActivate = [emulator]() {
            auto emuView = EmuConfigView::TabWindow::getView( emulator, true );
            emuView->show(EmuConfigView::TabWindow::Layout::System);
	    };
        sM.system->append( *sM.systemManagement );

        sM.debugger = new GUIKIT::Menu;
        sM.debugger->setIcon( debugImage );

        sM.debuggerCpu = new GUIKIT::MenuItem;
        sM.debuggerCpu->onActivate = [emulator]() {
            emuThread->lock();
            program->openDebugger(emulator, DebuggerTheme::CPU);
            emuThread->unlock();
        };
        sM.debugger->append( *sM.debuggerCpu );

        sM.debuggerMem = new GUIKIT::MenuItem;
        sM.debuggerMem->onActivate = [emulator]() {
            emuThread->lock();
            program->openDebugger(emulator, DebuggerTheme::Memory);
            emuThread->unlock();
        };
        sM.debugger->append( *sM.debuggerMem );

        if ( dynamic_cast<LIBC64::Interface*>(emulator)) {
            sM.debuggerSCPU = new GUIKIT::MenuItem;
            sM.debuggerSCPU->onActivate = [emulator]() {
                emuThread->lock();
                program->openDebugger(emulator, DebuggerTheme::SCPU);
                emuThread->unlock();
            };
            sM.debugger->append( *sM.debuggerSCPU );

            sM.debuggerMemSCPU = new GUIKIT::MenuItem;
            sM.debuggerMemSCPU->onActivate = [emulator]() {
                emuThread->lock();
                program->openDebugger(emulator, DebuggerTheme::MemorySCPU);
                emuThread->unlock();
            };
            sM.debugger->append( *sM.debuggerMemSCPU );

        } else {
            sM.debuggerMemSCPU = nullptr;
            sM.debuggerSCPU = nullptr;
        }

        sM.debuggerVideo = new GUIKIT::MenuItem;
        sM.debuggerVideo->onActivate = [emulator]() {
            emuThread->lock();
            program->openDebugger(emulator, DebuggerTheme::Video);
            emuThread->unlock();
        };
        sM.debugger->append( *sM.debuggerVideo );

        if ( dynamic_cast<LIBC64::Interface*>(emulator)) {
            sM.debuggerSid = new GUIKIT::MenuItem;
            sM.debuggerSid->onActivate = [emulator]() {
                emuThread->lock();
                program->openDebugger(emulator, DebuggerTheme::SID);
                emuThread->unlock();
            };
            sM.debugger->append( *sM.debuggerSid );
            sM.debuggerCopper = nullptr;
            sM.debuggerBlitter = nullptr;
            sM.debuggerAgnus = nullptr;
            sM.debuggerPaula = nullptr;
            sM.debuggerSerial = nullptr;
        } else {
            sM.debuggerCopper = new GUIKIT::MenuItem;
            sM.debuggerCopper->onActivate = [emulator]() {
                emuThread->lock();
                program->openDebugger(emulator, DebuggerTheme::Copper);
                emuThread->unlock();
            };
            sM.debugger->append( *sM.debuggerCopper );

            sM.debuggerBlitter = new GUIKIT::MenuItem;
            sM.debuggerBlitter->onActivate = [emulator]() {
                emuThread->lock();
                program->openDebugger(emulator, DebuggerTheme::Blitter);
                emuThread->unlock();
            };
            sM.debugger->append( *sM.debuggerBlitter );

            sM.debuggerAgnus = new GUIKIT::MenuItem;
            sM.debuggerAgnus->onActivate = [emulator]() {
                emuThread->lock();
                program->openDebugger(emulator, DebuggerTheme::Agnus);
                emuThread->unlock();
            };
            sM.debugger->append( *sM.debuggerAgnus );

            sM.debuggerPaula = new GUIKIT::MenuItem;
            sM.debuggerPaula->onActivate = [emulator]() {
                emuThread->lock();
                program->openDebugger(emulator, DebuggerTheme::Paula);
                emuThread->unlock();
            };
            sM.debugger->append( *sM.debuggerPaula );

            sM.debuggerSerial = new GUIKIT::MenuItem;
            sM.debuggerSerial->onActivate = [emulator]() {
                emuThread->lock();
                program->openDebugger(emulator, DebuggerTheme::Serial);
                emuThread->unlock();
            };
            sM.debugger->append( *sM.debuggerSerial );

            sM.debuggerSid = nullptr;
        }

        sM.debuggerDma = new GUIKIT::MenuItem;
        sM.debuggerDma->onActivate = [emulator]() {
            emuThread->lock();
            program->openDebugger(emulator, DebuggerTheme::DMA);
            emuThread->unlock();
        };
        sM.debugger->append( *sM.debuggerDma );

        sM.debuggerCia = new GUIKIT::MenuItem;
        sM.debuggerCia->onActivate = [emulator]() {
            emuThread->lock();
            program->openDebugger(emulator, DebuggerTheme::CIA);
            emuThread->unlock();
        };
        sM.debugger->append( *sM.debuggerCia );

        if ( dynamic_cast<LIBC64::Interface*>(emulator)) {
            int nr = 8;
            for (auto& debuggerDrive : sM.debuggerDrives) {
                debuggerDrive.menu = new GUIKIT::Menu;
                debuggerDrive.menu->setText( "Drive" + std::to_string( nr ));
                debuggerDrive.cpu = new GUIKIT::MenuItem;
                debuggerDrive.cpu->onActivate = [emulator, nr]() {
                    emuThread->lock();
                    switch (nr) {
                        default:
                        case 8: program->openDebugger(emulator, DebuggerTheme::Drive8CPU); break;
                        case 9: program->openDebugger(emulator, DebuggerTheme::Drive9CPU); break;
                        case 10: program->openDebugger(emulator, DebuggerTheme::Drive10CPU); break;
                        case 11: program->openDebugger(emulator, DebuggerTheme::Drive11CPU); break;
                    }
                    emuThread->unlock();
                };
                debuggerDrive.menu->append( *debuggerDrive.cpu );

                debuggerDrive.mem = new GUIKIT::MenuItem;
                debuggerDrive.mem->onActivate = [emulator, nr]() {
                    emuThread->lock();
                    switch (nr) {
                        default:
                        case 8: program->openDebugger(emulator, DebuggerTheme::Drive8Memory); break;
                        case 9: program->openDebugger(emulator, DebuggerTheme::Drive9Memory); break;
                        case 10: program->openDebugger(emulator, DebuggerTheme::Drive10Memory); break;
                        case 11: program->openDebugger(emulator, DebuggerTheme::Drive11Memory); break;
                    }
                    emuThread->unlock();
                };
                debuggerDrive.menu->append( *debuggerDrive.mem );

                debuggerDrive.via = new GUIKIT::MenuItem;
                debuggerDrive.via->onActivate = [emulator, nr]() {
                    emuThread->lock();
                    switch (nr) {
                        default:
                        case 8: program->openDebugger(emulator, DebuggerTheme::Drive8VIA); break;
                        case 9: program->openDebugger(emulator, DebuggerTheme::Drive9VIA); break;
                        case 10: program->openDebugger(emulator, DebuggerTheme::Drive10VIA); break;
                        case 11: program->openDebugger(emulator, DebuggerTheme::Drive11VIA); break;
                    }
                    emuThread->unlock();
                };
                debuggerDrive.menu->append( *debuggerDrive.via );

                sM.debugger->append( *debuggerDrive.menu );
                nr++;
            }
        }

        sM.system->append( *sM.debugger );
            
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

        sM.geometry = new GUIKIT::MenuItem;
        sM.geometry->setIcon( cropImage );
        sM.geometry->onActivate = [emulator]() {
            auto emuView = EmuConfigView::TabWindow::getView( emulator, true );
            emuView->show(EmuConfigView::TabWindow::Layout::Geometry);
        };
        sM.system->append( *sM.geometry );

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

        sM.misc = new GUIKIT::MenuItem;
        sM.misc->setIcon( toolsImage );
        sM.misc->onActivate = [emulator]() {
            auto emuView = EmuConfigView::TabWindow::getView( emulator, true );
            emuView->show(EmuConfigView::TabWindow::Layout::Misc);
        };
        sM.system->append( *sM.misc );
        
        sysMenus.push_back( sM );
        updateRecentList(sM.emulator);

        if (globalSettings->get<bool>("core_" + emulator->ident, true))
            append( *sM.system );
            
        InputMenu iM;
        iM.emulator = emulator;
        inputMenus.push_back(iM);
    }

    controlMenu.setIcon(joystickImage);
    append(controlMenu);
	
    // Misc Control
    recordScreen.setIcon(screenshotImage);
    recordScreen.onActivate = [this]() {
        emuThread->lock();
        takeScreenshot();
        emuThread->unlock();
    };
    miscMenu.append(recordScreen);

    miscMenu.append(*GUIKIT::MenuSeparator::getInstance());

    recordScaled.onActivate = [this]() {
        emuThread->lock();
        globalSettings->set<unsigned>("screenshot_unscaled", 0);
        recordWithEffects.setEnabled();
        emuThread->unlock();
    };
    miscMenu.append(recordScaled);

    recordUnscaled.onActivate = [this]() {
        emuThread->lock();
        globalSettings->set<unsigned>("screenshot_unscaled", 1);
        recordWithEffects.setEnabled(false);
        emuThread->unlock();
    };
    miscMenu.append(recordUnscaled);

    recordUnscaledNoBorder.onActivate = [this]() {
        emuThread->lock();
        globalSettings->set<unsigned>("screenshot_unscaled", 2);
        recordWithEffects.setEnabled(false);
        emuThread->unlock();
    };
    miscMenu.append(recordUnscaledNoBorder);

    recordUnscaledMonitor.onActivate = [this]() {
        emuThread->lock();
        globalSettings->set<unsigned>("screenshot_unscaled", 3);
        recordWithEffects.setEnabled(false);
        emuThread->unlock();
    };
    miscMenu.append(recordUnscaledMonitor);

    GUIKIT::MenuRadioItem::setGroup(recordScaled, recordUnscaled, recordUnscaledNoBorder, recordUnscaledMonitor);

    unsigned _unscaled = globalSettings->get<unsigned>("screenshot_unscaled", 0);
    
    switch (_unscaled) {
        default:
        case 0: recordScaled.setChecked(); break;
        case 1: recordUnscaled.setChecked(); break;
        case 2: recordUnscaledNoBorder.setChecked(); break;
        case 3: recordUnscaledMonitor.setChecked(); break;
    }

    miscMenu.append(*GUIKIT::MenuSeparator::getInstance());

    recordMergedFrames.onToggle = [this]() {
        emuThread->lock();
        globalSettings->set<bool>("screenshot_merge_frames", recordMergedFrames.checked());
        emuThread->unlock();
    };
    recordMergedFrames.setChecked(globalSettings->get<bool>("screenshot_merge_frames", false));
    miscMenu.append(recordMergedFrames);

    recordWithEffects.onToggle = [this]() {
        emuThread->lock();
        globalSettings->set<bool>("screenshot_with_effects", recordWithEffects.checked());
        emuThread->unlock();
    };
    recordWithEffects.setChecked(globalSettings->get<bool>("screenshot_with_effects", true));
    recordWithEffects.setEnabled(_unscaled == 0);
    miscMenu.append(recordWithEffects);

    recordScreenSettings.onActivate = [this]() {
        auto emuView = EmuConfigView::TabWindow::getView(activeEmulator, true);
        emuView->show(EmuConfigView::TabWindow::Layout::Presentation);
        if (emuView->presentationLayout)
            emuView->presentationLayout->selectViewScreenshot();
    };
    miscMenu.append(recordScreenSettings);

    miscMenu.append(*GUIKIT::MenuSeparator::getInstance());

    recordAudio.onActivate = [this]() {    
        program->toggleRecord();
    };
    recordAudio.setIcon(recordAudioImage);
    miscMenu.append(recordAudio);

    recordAudioSettings.onActivate = [this]() {
        auto emuView = EmuConfigView::TabWindow::getView(activeEmulator, true);
        emuView->show(EmuConfigView::TabWindow::Layout::Audio);
        if (emuView->audioLayout)
            emuView->audioLayout->selectViewAudioRecord();
    };
    miscMenu.append(recordAudioSettings);

    miscMenu.append(*GUIKIT::MenuSeparator::getInstance());

    miscMenu.append(copyItem);

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

    miscMenu.append( pasteItem );

	miscMenu.setIcon(editImage);
    append( miscMenu );

    optionsMenu.setIcon(toolsImage);
    append(optionsMenu);

    driversItem.setIcon( gearsImage );
    driversItem.onActivate = []() {
        ConfigView::TabWindow::open(ConfigView::TabWindow::Layout::Drivers);
    };

    optionsMenu.append(driversItem);

    settingsItem.onActivate = []() {
        ConfigView::TabWindow::open(ConfigView::TabWindow::Layout::Settings);
    };
    settingsItem.setIcon(toolsImage);
    optionsMenu.append(settingsItem);

    optionsMenu.append(*GUIKIT::MenuSeparator::getInstance());

    videoSyncItem.onToggle = [&]() {
        globalSettings->set<bool>("video_sync", videoSyncItem.checked() );
        emuThread->lock();
        program->setWarp( Program::Warp::Off );
        VideoManager::setSynchronize();
        statusHandler->resetFrameCounter();
        emuThread->unlock();
    };

    bool vsync = globalSettings->get<bool>("video_sync", true);
    bool vrr = globalSettings->get<bool>("vrr_sync", false);

    if (vsync)
        videoSyncItem.setChecked();

    optionsMenu.append(videoSyncItem);

    vrrItem.onToggle = [&]() {
        globalSettings->set<bool>("vrr_sync", vrrItem.checked() );
        emuThread->lock();
        VideoManager::setSynchronize();
        emuThread->unlock();
    };
    if (vrr)
        vrrItem.setChecked();

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
    if ( globalSettings->get<bool>("fps", true) ) fpsItem.setChecked();
    statusTextMenu.append(fpsItem);

    volumeItem.onToggle = [&]() {
        globalSettings->set<bool>("volume_control", volumeItem.checked() );
        statusHandler->updateVolume( volumeItem.checked() );
    };
    if ( globalSettings->get<bool>("volume_control", true) ) volumeItem.setChecked();
    statusTextMenu.append(volumeItem);

    statusTextMenu.setIcon(infoImage);
    optionsMenu.append(statusTextMenu);

    saveItem.onActivate = []() {
        emuThread->lock();
        for (auto rF : fileloader->recentFiles)
            rF->save();
        SettingsHelper::saveSettings();
        emuThread->unlock();
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

    warpItem.onToggle = []() {
        emuThread->lock();
        program->toggleWarp( Program::Warp::Normal );
        emuThread->unlock();
    };
    speedControlMenu.append( warpItem );

    aggressiveWarpItem.onToggle = []() {
        emuThread->lock();
        program->toggleWarp( Program::Warp::Aggressive );
        emuThread->unlock();
    };
    speedControlMenu.append( aggressiveWarpItem );

    warpDisableShader.onToggle = [this]() {
        emuThread->lock();
        program->warp.disableShader = warpDisableShader.checked();
        globalSettings->set<bool>("warp_disable_shader", warpDisableShader.checked());
        emuThread->unlock();
    };

    if ( globalSettings->get<bool>("warp_disable_shader", false) ) {
        warpDisableShader.setChecked();
        program->warp.disableShader = true;
    }

    speedControlMenu.append( warpDisableShader );

    speedControlMenu.append( *GUIKIT::MenuSeparator::getInstance() );

    fastForwardItem.onToggle = []() {
        emuThread->lock();
        program->toggleWarp( Program::Warp::FastForward );
        emuThread->unlock();
    };
    speedControlMenu.append( fastForwardItem );

    customizeFFItem.onActivate = [this]() {
        if (!fpsFastforwardWindow)
            fpsFastforwardWindow = new FpsWindow(this, FpsWindow::Mode::FASTFORWARD);
        fpsFastforwardWindow->show();
    };
    speedControlMenu.append( customizeFFItem );

    speedControlMenu.append( *GUIKIT::MenuSeparator::getInstance() );

    pauseItem.onToggle = [this]() {
        this->togglePause();
    };
    speedControlMenu.append( pauseItem );

    fpsRefreshItems.reserve( 15 );
    for (int i = 1; i <= 15; i++) {
        auto item = new GUIKIT::MenuRadioItem;
        item->setText( std::to_string(i * 200 ) + " ms" );
        item->onActivate = [this, i]() {
            emuThread->lock();
            unsigned delay = i * 200;
            Program::getSettings( activeEmulator )->set<unsigned>("fps_refresh", delay);
            statusHandler->setFpsRefresh();
            emuThread->unlock();
        };
        fpsRefreshItems.push_back( item );
        fpsRefreshMenu.append( *item );
    }
    GUIKIT::MenuRadioItem::setGroup( fpsRefreshItems );
    fpsPresentationMenu.append( fpsRefreshMenu );

    fpsDecimalItems.reserve( 4 );
    for (int i = 0; i <= 3; i++) {
        auto item = new GUIKIT::MenuRadioItem;
        item->setText( std::to_string(i) );
        item->onActivate = [this, i]() {
            emuThread->lock();
            Program::getSettings( activeEmulator )->set<unsigned>("fps_decimal_point", i);
            statusHandler->setFpsRefresh();
            emuThread->unlock();
        };
        fpsDecimalItems.push_back( item );
        fpsDecimalMenu.append( *item );
    }

    GUIKIT::MenuRadioItem::setGroup( fpsDecimalItems );
    fpsPresentationMenu.append( fpsDecimalMenu );

    speedControlMenu.append( fpsPresentationMenu );

    speedControlMenu.append( *GUIKIT::MenuSeparator::getInstance() );

    GUIKIT::MenuRadioItem* speedItem;

    unsigned steps = 12;

    for(unsigned i = 0; i <= steps; i++) {
        if (i == (steps - 1))
            speedItem = &maximumSpeedItem;
        else
            speedItem = new GUIKIT::MenuRadioItem;

        speedItem->onActivate = [this, i]() {
            auto settings = Program::getSettings( activeEmulator );
            settings->set<unsigned>("speed_profile", i);
            emuThread->lock();
            overrideFullscreenRefreshRate = 0.0f;
            audioManager->setSynchronize();
            audioManager->setResampler();
            statusHandler->resetFrameCounter();
            emuThread->unlock();
        };
        speedControlMenu.append( *speedItem );

        if (i == 1 || i == (steps - 1))
            speedControlMenu.append( *GUIKIT::MenuSeparator::getInstance() );

        if (i == steps) {
            customizeSpeedItem.onActivate = [this]() {
                if (!fpsCustomWindow)
                    fpsCustomWindow = new FpsWindow(this, FpsWindow::Mode::CUSTOM);
                fpsCustomWindow->show();
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

        diskControlMenu.resetAndEject.setIcon( powerImage );

        diskControlMenu.resetAndEject.onActivate = [i]() {
            emuThread->lock(true);
            auto media = activeEmulator->getEnabledDisk( i );
            if (media)
                fileloader->eject( activeEmulator, media );
            program->power(activeEmulator);
            emuThread->unlock();
        };

        diskControlMenu.menu.append( diskControlMenu.resetAndEject );

        diskControlMenu.menu.append( *GUIKIT::MenuSeparator::getInstance() );

        diskControlMenu.clearSave.setIcon( delImage );

        diskControlMenu.clearSave.onActivate = [i]() {
            emuThread->lock();
            auto media = activeEmulator->getDisk(i);
            auto path = FileHelper::getAssignedSaveFile( media );
            if (path.empty()) {
                emuThread->unlock();
                return;
            }

            GUIKIT::File file(path);
            if (file.exists()) {
                if (file.del()) {
                    auto fSetting = FileSetting::getInstance( activeEmulator, _underscore( media->name ) );
                    GUIKIT::File* _file = filePool->get(GUIKIT::File::resolveRelativePath(fSetting->path));

                    activeEmulator->insertMedium(media, _file->archiveData( fSetting->id ), _file->archiveDataSize(fSetting->id));

                    auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );
                    if (emuView && emuView->mediaLayout)
                        emuView->mediaLayout->updateListing( media );

                    statusHandler->setMessage(trans->getA("save file deleted"), true);
                }
            }
            emuThread->unlock();
        };

        diskControlMenu.menu.append( diskControlMenu.clearSave );

        diskControlMenu.savePath.setIcon( openImage );

        diskControlMenu.savePath.onActivate = [this]() {

            if (!savePathWindow) {
                savePathWindow = new SavePathWindow;
                savePathWindow->setTitle( trans->getA("disksaves") );
                savePathWindow->mainLayout.savePath.setImage( &openImage );
                savePathWindow->mainLayout.defaultPath.setImage( &delImage );
                savePathWindow->append( savePathWindow->mainLayout );

                MiscHelper::centerGeometry( savePathWindow, {500u, 100u}, this->geometry() );

                savePathWindow->mainLayout.defaultPath.onActivate = [this]() {
                    auto settings = Program::getSettings( activeEmulator );
                    settings->set<std::string>("disksave_folder", "");
                    savePathWindow->mainLayout.edit.setText( FileHelper::generatedFolder(activeEmulator, "disksave_folder", "disksave", FileHelper::FLAG_VIEW) );
                };

                savePathWindow->mainLayout.savePath.onActivate = [this]() {
                    auto settings = Program::getSettings( activeEmulator );
                    auto curPath = settings->get<std::string>("disksave_folder", "");
                    if (!curPath.empty())
                        curPath = GUIKIT::File::resolveRelativePath(curPath);

                    auto path = GUIKIT::BrowserWindow()
                        .setTitle(trans->get("select_disksave_folder"))
                        .setPath(curPath)
                        .setWindow(*this)
                        .directory();

                    if (!path.empty()) {
                        path = GUIKIT::File::buildRelativePath(path);
                        settings->set<std::string>("disksave_folder", path);
                        savePathWindow->mainLayout.edit.setText( path );
                    }
                    savePathWindow->setVisible( false );
                };
            }

            savePathWindow->mainLayout.edit.setText( FileHelper::generatedFolder(activeEmulator, "disksave_folder", "disksave", FileHelper::FLAG_VIEW) );
            savePathWindow->setVisible();
            savePathWindow->setFocused();
        };

        diskControlMenu.menu.append( diskControlMenu.savePath );

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

    power.power.setIcon( powerImage );
    power.reset.setIcon( powerImage );

    power.power.onActivate = []() {
        emuThread->lock(true);
        program->power(activeEmulator);
        emuThread->unlock();
    };

    power.reset.onActivate = []() {
        emuThread->lock(true);
        program->reset(activeEmulator);
        emuThread->unlock();
    };

    power.menu.append( power.power );
    power.menu.append( power.reset );
    power.menu.append( *new GUIKIT::MenuSeparator );

    auto amiEmu = program->getEmulator("Amiga");
    auto model = amiEmu->getModel( LIBAMI::Interface::ModelId::ModelIdAudioFilter );
    if (model) {
        int i = 0;
        for([[maybe_unused]] auto& option : model->options) {
            auto item = new GUIKIT::MenuRadioItem;

            item->onActivate = [i]() {
                auto model = activeEmulator->getModel( LIBAMI::Interface::ModelId::ModelIdAudioFilter );
                if (!model)
                    return;
                auto settings = Program::getSettings(activeEmulator);
                if (!settings)
                    return;

                settings->set<unsigned>( _underscore(model->name), i );
                auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );
                if (emuView && emuView->audioLayout) {
                    auto block = emuView->audioLayout->settingsLayout.getBlock(model->id);
                    if (block && (i < block->options.size() ) )
                        block->options[i]->setChecked();
                }

                emuThread->lock();
                activeEmulator->setModelValue( model->id, i );
                emuThread->unlock();
            };
            i++;
            power.filters.push_back(item);
            power.menu.append( *item );
        }

        GUIKIT::MenuRadioItem::setGroup(power.filters);
    }
}

auto View::updatePowerMenu() -> void {
    if (!dynamic_cast<LIBAMI::Interface*>(activeEmulator))
        return;
    auto model = activeEmulator->getModel( LIBAMI::Interface::ModelId::ModelIdAudioFilter );
    if (!model)
        return;

    auto val = activeEmulator->getModelValue(model->id);

    if (val < power.filters.size())
        power.filters[val]->setChecked();
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

        if (label != speedItems[i]->text())
            speedItems[i]->setText( label );
    }

    auto settings = Program::getSettings( activeEmulator );
    unsigned speedProfile = settings->get<unsigned>("speed_profile", 1, {0, (unsigned)speedItems.size() - 1});
    if (!speedItems[speedProfile]->checked())
        speedItems[speedProfile]->setChecked();
}

auto View::updateDiskMenu() -> void {
    bool showResetAndHide = dynamic_cast<LIBC64::Interface*>(activeEmulator);

    for(auto& d : diskControlMenus) {
        if (d.clearSave.enabled() != !showResetAndHide) {
            d.clearSave.setEnabled(!showResetAndHide);
            d.savePath.setEnabled(!showResetAndHide);
        }

        if (d.reset.enabled() != showResetAndHide) {
            d.reset.setEnabled(showResetAndHide);
            d.inactive.setEnabled(showResetAndHide);
        }
    }
}

auto View::updateMouseGrab() -> void {
    auto settings = Program::getSettings( activeEmulator );
    grabMouseLeft = settings->get<bool>("grab_mouse_left", dynamic_cast<LIBAMI::Interface*>(activeEmulator));
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
    if (!dynamic_cast<LIBC64::Interface*>(activeEmulator))
        return;
    
    tapeStopItem.setIcon( mode == TapeMode::Stop ? stophiImage : stopImage );
    tapePlayItem.setIcon( mode == TapeMode::Play ? playhiImage : playImage );    
    tapeRecordItem.setIcon( mode == TapeMode::Record ? recordhiImage : recordImage );
    tapeForwardItem.setIcon( mode == TapeMode::Forward ? forwardhiImage : forwardImage );
    tapeRewindItem.setIcon( mode == TapeMode::Rewind ? rewindhiImage : rewindImage );
	tapeResetCounterItem.setIcon( counterImage );

    auto useExpansion = activeEmulator->getExpansion();

    if (!useExpansion->isTurboCart())
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

        if (sysMenu.poweronAndRemoveExpansions)
		    sysMenu.poweronAndRemoveExpansions->setText(trans->get("Hard Reset + Unplug Cart"));
        if (sysMenu.poweronAndRemoveDisks)
            sysMenu.poweronAndRemoveDisks->setText(trans->get("Hard Reset + Eject Disks"));

        sysMenu.reset->setText(trans->get("Soft Reset"));
        if (sysMenu.freeze)
            sysMenu.freeze->setText(trans->get("Freeze"));
        if (sysMenu.menu)
            sysMenu.menu->setText(trans->get("cartridge button"));
        if (sysMenu.powerLED)
            sysMenu.powerLED->setText(trans->get("toggle Power LED"));
        if (sysMenu.capsLED)
            sysMenu.capsLED->setText(trans->get("toggle Caps Lock LED"));
        if (sysMenu.mhz2LED)
            sysMenu.mhz2LED->setText(trans->getA("toggle 2 MHz LED"));
        sysMenu.loadSoftware->setText(trans->get("load software") + "...");
        sysMenu.recentSoftware->setText(trans->getA("Recent Files"));
        sysMenu.media->setText(trans->get("Software") + "...");
        sysMenu.states->setText(trans->get("states"));
        sysMenu.save->setText(trans->get("Savestate"));
        sysMenu.slotUp->setText(trans->get("Incslot"));
        sysMenu.slotDown->setText(trans->get("Decslot"));
        sysMenu.load->setText(trans->get("Loadstate"));

        sysMenu.systemManagement->setText(trans->get("system_management") + "...");
        sysMenu.debugger->setText(trans->get("Debugger"));
        sysMenu.debuggerCpu->setText(getReadable( DebuggerTheme::CPU ));
        sysMenu.debuggerMem->setText(getReadable( DebuggerTheme::Memory ));
        sysMenu.debuggerCia->setText(getReadable( DebuggerTheme::CIA ));

        sysMenu.debuggerVideo->setText(getReadable( DebuggerTheme::Video, sysMenu.emulator ));
        if (sysMenu.debuggerSid)
            sysMenu.debuggerSid->setText(getReadable( DebuggerTheme::SID ));
        if (sysMenu.debuggerCopper)
            sysMenu.debuggerCopper->setText( getReadable( DebuggerTheme::Copper ));
        if (sysMenu.debuggerBlitter)
            sysMenu.debuggerBlitter->setText(getReadable( DebuggerTheme::Blitter ));
        if (sysMenu.debuggerAgnus)
            sysMenu.debuggerAgnus->setText( getReadable( DebuggerTheme::Agnus ));
        if (sysMenu.debuggerPaula)
            sysMenu.debuggerPaula->setText( getReadable( DebuggerTheme::Paula ));
        if (sysMenu.debuggerSerial)
            sysMenu.debuggerSerial->setText( getReadable( DebuggerTheme::Serial ));

        sysMenu.debuggerDma->setText(getReadable( DebuggerTheme::DMA ));

        if (sysMenu.debuggerMemSCPU)
            sysMenu.debuggerMemSCPU->setText(getReadable( DebuggerTheme::MemorySCPU ));

        if (sysMenu.debuggerSCPU)
            sysMenu.debuggerSCPU->setText(getReadable( DebuggerTheme::SCPU ));

        if (dynamic_cast<LIBC64::Interface*>(sysMenu.emulator)) {
            for (auto& debuggerDrive : sysMenu.debuggerDrives) {
                debuggerDrive.cpu->setText( getReadable( DebuggerTheme::Drive8CPU ) );
                debuggerDrive.mem->setText( getReadable( DebuggerTheme::Drive8Memory ) );
                debuggerDrive.via->setText( getReadable( DebuggerTheme::Drive8VIA ) );
            }
        }

        sysMenu.audio->setText(trans->get("Audio") + "...");
        sysMenu.firmware->setText(trans->get("Firmware") + "...");
        sysMenu.configurations->setText(trans->get("Configurations") + "...");
        sysMenu.presentation->setText(trans->get("Presentation") + "...");
        sysMenu.palette->setText(trans->get("Palette") + "...");
        sysMenu.geometry->setText(trans->get("Geometry") + "...");
        sysMenu.misc->setText(trans->get("Miscellaneous") + "...");

        sysMenu.shaderMenu->setText(trans->get("Shader"));
        sysMenu.recentControl->setText(trans->getA("preferences"));
        sysMenu.recentClearEntries->setText(trans->getA("Clear all entries"));

        int i = 10;
        for (auto rE : sysMenu.recentEntries) {
            rE->setText(trans->get("list length", {{"%count%", std::to_string(i) }} ));
            i += 5;
        }
    }    

    miscMenu.setText(trans->get("Miscellaneous"));
    pasteItem.setText( trans->get("Paste") );
    copyItem.setText( trans->get("Copy") );
    recordScreen.setText(trans->getA("take screenshot"));
    recordMergedFrames.setText(trans->getA("merge frames"));    
    recordWithEffects.setText(trans->getA("including effects"));
    recordScreenSettings.setText(trans->getA("screenshot settings") + "...");

    recordAudio.setText(trans->getA("record audio"));
    recordAudioSettings.setText(trans->getA("record audio settings") + "...");

    recordScaled.setText(trans->getA("scaled"));
    recordUnscaled.setText(trans->getA("unscaled"));
    updateScreenshotUI();

    controlMenu.setText( trans->get("control") );
    
    optionsMenu.setText( trans->get("options"));

    driversItem.setText( trans->get("driver") + "...");
    settingsItem.setText( trans->get("settings") + "...");

    videoSyncItem.setText( trans->get("Video Sync"));
    vrrItem.setText( trans->get("VRR"));
    dynamicRateControl.setText( trans->get("dynamic_rate_control"));

    fullscreenItem.setText( trans->get("fullscreen"));

    statusTextMenu.setText(trans->get("screen_status"));

    muteItem.setText( trans->get("mute_audio"));
    fpsItem.setText( trans->get("show_fps"));
    volumeItem.setText( trans->get("show volume"));
    
    saveItem.setText( trans->get("save_preferences"));
    exit.setText(trans->get("Exit"));
	
	tapeControlMenu.setText( trans->get("Datasette") );
    insertTapeItem.setText( trans->get("insert") );
    ejectTapeItem.setText( trans->get("eject") );
    speedControlMenu.setText( trans->get("Speed") );

    for (auto& diskControlMenu : diskControlMenus) {
        diskControlMenu.insert.setText( trans->get("insert") );
        diskControlMenu.eject.setText( trans->get("eject") );
        diskControlMenu.resetAndEject.setText( trans->get("Hard Reset + Eject") );
        diskControlMenu.reset.setText( trans->get("Reset Floppy") );
        diskControlMenu.inactive.setText( trans->get("inactive until reset") );
        diskControlMenu.clearSave.setText( trans->get("clear save file") );
        diskControlMenu.savePath.setText( trans->get("disksaves") );
    }

    power.power.setText( trans->get("Hard Reset") );
    power.reset.setText( trans->get("Soft Reset") );

    auto amiEmu = program->getEmulator("Amiga");
    auto model = amiEmu->getModel( LIBAMI::Interface::ModelId::ModelIdAudioFilter );
    if (model) {
        int i = 0;
        for (auto& option: model->options) {
            std::string _trans = i == 0 ? (trans->getA("PAULA Filter", true) + " ") : "";
            _trans += trans->getA(option);
            power.filters[i++]->setText( _trans );
        }
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
    fpsPresentationMenu.setText( trans->getA("FPS View") );
    fpsDecimalMenu.setText( trans->getA("Decimal Place") );
    fpsRefreshMenu.setText( trans->getA("Refresh") );

    warpItem.setText( trans->get("Toggle Warp") );
    aggressiveWarpItem.setText( trans->get("Toggle Warp aggressive") );
    warpDisableShader.setText( trans->get("no shader when warp") );

    fastForwardItem.setText( trans->get("Toggle Fast Forward") );
    customizeFFItem.setText( trans->get("customize speed") );

    maximumSpeedItem.setText( trans->get("maximum speed") );
    customizeSpeedItem.setText( trans->get("customize speed") );

    if (savePathWindow)
        savePathWindow->setTitle( trans->getA("disksaves") );

    setAudioRecordText();
}

auto View::updateScreenshotUI() -> void {
    if (activeEmulator && dynamic_cast<LIBC64::Interface*>(activeEmulator)) {
        recordUnscaledNoBorder.setText("320x200");
        recordUnscaledMonitor.setText("384x272");
    } else {
        recordUnscaledNoBorder.setText("320x256");
        recordUnscaledMonitor.setText("344x280");
    }
}

auto View::getViewportHandle(bool driverChange) -> uintptr_t {
    return viewport.handle(driverChange);
}

auto View::loadCursor() -> void {

    pencilImage.loadPng((uint8_t*)Icons::pencil, sizeof(Icons::pencil));
    
    crosshairImage.loadPng((uint8_t*)Icons::crosshair, sizeof(Icons::crosshair));
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

        if (sM.freeze) {
            if (sM.freeze->enabled() != state)
                sM.freeze->setEnabled(state);
        }

        if (!sM.menu)
            continue;
         
        state = (sM.emulator == emulator) && emulator->hasCustomCartridgeButton();
        
        if (sM.menu->enabled() != state)
            sM.menu->setEnabled( state );
    }

    copyItem.setEnabled(!!dynamic_cast<LIBC64::Interface*>(emulator));
    pasteItem.setEnabled(!!dynamic_cast<LIBC64::Interface*>(emulator));
}

auto View::questionToWrite(Emulator::Interface::Media* media) -> bool {
    
    auto file = (GUIKIT::File*)media->guid;
    
    if (cmd->debug || cmd->noGui || !file)
        return false;

    if (activeEmulator && dynamic_cast<LIBC64::Interface*>(activeEmulator)) {
        if (file->isArchived() || file->isReadOnly())
            // for C64 archive, removing of write protection is not supported
            return false;
    }
    
    bool state = !globalSettings->get<bool>("question_media_write", true);
    
    if (!state) {
        if (videoDriver && videoDriver->hasExclusiveFullscreen())
            switchFullScreen( false );

        bool _acquired = inputDriver->mIsAcquired();

        if (_acquired)
            inputDriver->mUnacquire();
        state = message->question(trans->get("question permanent write", {{"%media%", media->name}}));
        if(_acquired)
            inputDriver->mAcquire();
    }
    
    return state;
}

auto View::getSpeedBySelectedProfile(float& speed, bool& percent) -> unsigned {
    auto settings = Program::getSettings( activeEmulator );
    auto& warp = program->warp;

    if (warp.mode == Program::Warp::FastForward) {
        speed = warp.ff_speed;
        percent = warp.ff_percent;
        return ~0;
    }

    if (overrideFullscreenRefreshRate != 0.0) {
        speed = overrideFullscreenRefreshRate;
        percent = false;
        return ~0;
    }

    auto speedProfile = settings->get<unsigned>("speed_profile", 1, {0, (unsigned)speedItems.size() - 1});
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
            auto settings = Program::getSettings( activeEmulator );
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

    auto settings = Program::getSettings( activeEmulator );
    unsigned speedProfile = settings->get<unsigned>("speed_profile", 1, {0, (unsigned)speedItems.size() - 1});

    return speedProfile == (speedItems.size() - 1);
}

auto View::isMaximumSpeed() -> bool {
    if (!activeEmulator)
        return false;

    if (program->warp.mode == Program::Warp::FastForward)
        return false;

    auto settings = Program::getSettings( activeEmulator );
    unsigned speedProfile = settings->get<unsigned>("speed_profile", 1, {0, (unsigned)speedItems.size() - 1});

    return speedProfile == (speedItems.size() - 2);
}

auto View::updateEmuUsage() -> void {
    remove(tapeControlMenu);
    remove(speedControlMenu);
    remove(optionsMenu);
    remove(miscMenu);
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
    append(miscMenu);
    append(optionsMenu);
    append(speedControlMenu);
    if (activeEmulator->getModelValue( activeEmulator->getModelIdOfEnabledDrives( activeEmulator->getTapeMediaGroup() ) ))
        append(tapeControlMenu);
}

auto View::updateGeometry(bool withViewport) -> void {
    MiscHelper::applyGeometry( this, globalSettings, "screen", {100, 100, 800, 600} );

    if (withViewport)
        updateViewport();
}

auto View::adjustToEmu(bool withViewport) -> void {
   if (fullScreen())
        return;

    auto driverViewport = videoDriver->getViewport();

    globalSettings->set<int>("screen_x", geometry().x + driverViewport.x );
    globalSettings->set<int>("screen_y", geometry().y + driverViewport.y );
    globalSettings->set<unsigned>("screen_width", driverViewport.width);
    globalSettings->set<unsigned>("screen_height", driverViewport.height);

    updateGeometry(withViewport);
}

auto View::updateToHoldDimension() -> void {
    if (view->fullScreen())
        return;
    
    auto settings = Program::getSettings(activeEmulator);
    globalSettings->set<unsigned>("screen_width", settings->get<unsigned>("view_hold_width", 800));
    globalSettings->set<unsigned>("screen_height", settings->get<unsigned>("view_hold_height", 600));

    updateGeometry(true);
}

auto View::clearRecentList(Emulator::Interface* emulator) -> void {
    auto sysMenu = getSysMenu(emulator);
    if (!sysMenu)
        return;

    for (auto& recent : sysMenu->recents) {
        if (recent) {
            delete recent;
            recent = nullptr;
        }
    }
    sysMenu->recentSoftware->reset();
    sysMenu->recentSoftware->setEnabled(false);
}

auto View::takeScreenshot() -> void {
    videoDriver->waitRenderThread();
    auto settings = Program::getSettings(activeEmulator);
    auto _path = FileHelper::generatedFolder(activeEmulator, "screen_record_path", "recordings/screenshots", FileHelper::FLAG_CREATE);
    auto _file = settings->get<std::string>("save_ident", "screenshot");
    auto screenshotFormat = settings->get<std::string>("screen_record_format", "png");
    bool withEffects = globalSettings->get<bool>("screenshot_with_effects", true);
    bool mergeTwoFrames = globalSettings->get<bool>("screenshot_merge_frames", false);
    unsigned unscaled = globalSettings->get<unsigned>("screenshot_unscaled", 0);
    bool usePalete = settings->get<bool>("screen_palette", true);
    unsigned screenGun = settings->get<unsigned>("screen_gun", 1, {1, 120});
    unsigned screenGunEach = settings->get<unsigned>("screen_gun_each", 1, { 1, 60 });
    bool delayScreenshot = settings->get<unsigned>("screen_shot_delay", false);

    _path += _file + "_#ident#";

    screenshot.sharedMutex.lock();
    if (screenshotFormat == "bmp") {
        screenshot.path = _path + ".bmp";
        screenshot.type = GUIKIT::Image::Type::BMP;
    } else if (screenshotFormat == "jpg") {
        screenshot.path = _path + ".jpg";
        screenshot.type = GUIKIT::Image::Type::JPG;
    } else if (screenshotFormat == "tga") {
        screenshot.path = _path + ".tga";
        screenshot.type = GUIKIT::Image::Type::TGA;
    } else if (screenshotFormat == "gif") {
        screenshot.path = _path + ".gif";
        screenshot.type = GUIKIT::Image::Type::GIF;
    } else {
        screenshot.path = _path + ".png";
        screenshot.type = GUIKIT::Image::Type::PNG;
    }

    screenshot.writePalette = (unscaled > 0) && dynamic_cast<LIBC64::Interface*>(activeEmulator) &&
        ((screenshot.type == GUIKIT::Image::Type::GIF) || ((screenshot.type == GUIKIT::Image::Type::BMP) && usePalete && !mergeTwoFrames));

    screenshot.animatedGif = (screenshot.type == GUIKIT::Image::Type::GIF) && (unscaled > 0) && dynamic_cast<LIBC64::Interface*>(activeEmulator)
        && ((screenGun > 1) || mergeTwoFrames);

    screenshot.withEffects = withEffects;
    screenshot.unscaled = unscaled;
    screenshot.twoFrames = mergeTwoFrames;
    screenshot.pause = 0;
    screenshot.saveState = false;
    screenshot.interval = screenGunEach;
    screenshot.intervalPos = delayScreenshot ? (50 * 3) : 1;

    if (screenshot.animatedGif) {
        screenshot.gun = screenGun > 1 ? screenGun : 2;
        VideoManager::takeScreenShots = screenshot.gun;
    } else {
        screenshot.gun = screenGun > 1 ? 1 : 0;
        VideoManager::takeScreenShots = mergeTwoFrames ? 2 : 1;
        if (screenGun)
            VideoManager::takeScreenShots *= screenGun;

        if (mergeTwoFrames)
            screenshot.interval = 1;
    }
    clearScreenshotBuffer();
    screenshot.sharedMutex.unlock();
}

auto View::clearScreenshotBuffer() -> void {
    for (auto ptr : screenshot.buffer) {
        if (ptr)
            delete[] ptr;
    }
    screenshot.buffer.clear();
    screenshot.bufferSize = 0;
}

auto View::setAudioRecordText() -> void {
    if (!audioManager)
        return;

    if(!audioManager->record.run())
        recordAudio.setText(trans->getA("record audio start"));
    else
        recordAudio.setText(trans->getA("record audio stop"));
}

auto View::updateFPSMenu() -> void {
    auto _settings = Program::getSettings( activeEmulator );
    auto decimals = _settings->get<unsigned>("fps_decimal_point", 1, {0u, 3u});
    auto delay = _settings->get<unsigned>("fps_refresh", 1000u, {200u, 3000u});

    if (decimals >= fpsDecimalItems.size())
        decimals = 1;
    fpsDecimalItems[decimals]->setChecked();
    unsigned position = delay / 200;
    if (position)
        position--;

    if (position >= fpsRefreshItems.size())
        position = 4;
    fpsRefreshItems[position]->setChecked();
}

View::FpsWindow::Top::Top() {
    append( fpsLineEdit, {80u, 0u});
    append( spacer, {~0u, 0u} );
    append( fpsRadioBox, {0u, 0u}, 10);
    append( percentRadioBox, {0u, 0u});

    GUIKIT::RadioBox::setGroup( fpsRadioBox, percentRadioBox );
    setAlignment( 0.5 );
}

View::FpsWindow::Center::Center() {
    append( label, {0u, 0u} );
    append( spacer, {~0u, 0u} );
    append( each, {0u, 0u}, 7 );
    append( each4th, {0u, 0u}, 7 );
    append( each8th, {0u, 0u}, 7 );
    append( each16th, {0u, 0u} );

    GUIKIT::RadioBox::setGroup( each, each4th, each8th, each16th );

    setAlignment( 0.5 );
}

View::FpsWindow::Bottom::Bottom() {
    append( cancel, {0u, 0u} );
    append( spacer, {~0u, 0u} );
    append( apply, {0u, 0u} );

    setAlignment( 0.5 );
}

View::SavePathWindow::MainLayout::MainLayout() {
    edit.setEditable( false );
    append( edit, {~0u, 0u}, 10 );
    append( spacer, {0u, ~0u} );
    append( defaultPath, {0u, 0u}, 10 );
    append( savePath, {0u, 0u} );

    setMargin(10);
    setAlignment( 0.5 );
}

auto View::FpsWindow::show() -> void {
    unsigned _width = 230;
    unsigned _height = 100;

    if (mode == Mode::FASTFORWARD) {
        _width = 300;
        _height = 130;
    }

    MiscHelper::centerGeometry( this, {_width, _height}, view->geometry() );

    auto settings = Program::getSettings( activeEmulator );
    auto speed = settings->get<float>(getIdent(), mode == Mode::CUSTOM ? 59.95 : 500.0);
    auto percent = settings->get<bool>(getIdent() + "_percent", false);
    std::string label = GUIKIT::String::formatFloatingPoint(speed, 3, true);

    top.fpsLineEdit.setText( label );
    if (percent)
        top.percentRadioBox.setChecked();
    else
        top.fpsRadioBox.setChecked();

    if (mode == Mode::FASTFORWARD ) {
        auto each = settings->get<unsigned>(getIdent() + "_each", 0, {0, 3});
        switch (each) {
            case 0:
            default: center.each.setChecked(); break;
            case 1: center.each4th.setChecked(); break;
            case 2: center.each8th.setChecked(); break;
            case 3: center.each16th.setChecked(); break;
        }
        center.setEnabled( );
    } else
        center.setEnabled( false );

    setVisible();
    setFocused();
}

auto View::FpsWindow::build() -> void {

    bottom.apply.onActivate = [this]() {
        std::string userInput = top.fpsLineEdit.text();

        GUIKIT::String::replace(userInput, ",", ".");

        if (userInput.empty() || !GUIKIT::String::isFloatNumber( userInput ) ) {
            return;
        }

        std::stringstream ss( userInput );
        float out = 0.0;
        ss >> out;

        if (out < 1.0)
            return;

        unsigned each = 0;
        if (center.each4th.checked()) each = 1;
        else if (center.each8th.checked()) each = 2;
        else if (center.each16th.checked()) each = 3;

        auto _settings = Program::getSettings( activeEmulator );
        _settings->set<std::string>(getIdent(), userInput);
        _settings->set<bool>(getIdent() + "_percent", top.percentRadioBox.checked());
        _settings->set<unsigned>(getIdent() + "_each", each);
        if (mode == Mode::FASTFORWARD)
            program->initFastForward();

        if (mode == Mode::FASTFORWARD && program->warp.mode == Program::Warp::FastForward) {
            emuThread->lock();
            program->setWarp( Program::Warp::FastForward, program->warp.manuell );
            emuThread->unlock();
        } else if (mode == Mode::CUSTOM && view->isCustomSpeed()) {
            emuThread->lock();
            view->overrideFullscreenRefreshRate = 0.0f;
            audioManager->setResampler();
            statusHandler->resetFrameCounter();
            emuThread->unlock();
        }
        view->updateSpeedLabels();

        setVisible( false );
    };

    bottom.cancel.onActivate = [this]() {
        setVisible( false );
    };

    top.fpsRadioBox.setText( trans->get("FPS") );
    top.percentRadioBox.setText( trans->get("Percent") );
    bottom.apply.setText( trans->get("Apply") );
    bottom.cancel.setText( trans->get("Cancel") );

    center.label.setText( trans->getA("show images") );
    center.each.setText( trans->get("all") );
    center.each4th.setText( trans->getA("4.") );
    center.each8th.setText( trans->getA("8.") );
    center.each16th.setText( trans->getA("16.") );

    layout.setMargin( 10 );
    layout.append(top, {~0u, 0u}, mode == Mode::FASTFORWARD ? 10 : 20);
    if (mode == Mode::FASTFORWARD )
        layout.append(center, {~0u, 0u}, 20);
    layout.append(bottom, {~0u, 0u});

    append( layout );
}

auto View::FpsWindow::getIdent() const -> std::string {
    switch (mode) {
        default:
        case Mode::CUSTOM:
            return "custom_speed";
        case Mode::FASTFORWARD:
            return "fastforward_speed";
    }
}

auto View::getReadable(DebuggerTheme theme, Emulator::Interface* emulator) -> std::string {
    switch ( theme ) {
        default:
            return "";
        case DebuggerTheme::CPU:
            return "CPU";
        case DebuggerTheme::SCPU:
            return "SCPU";
        case DebuggerTheme::Drive8CPU:
            return emulator ? "Drive8 CPU" : "CPU";
        case DebuggerTheme::Drive9CPU:
            return emulator ? "Drive9 CPU" : "CPU";
        case DebuggerTheme::Drive10CPU:
            return emulator ? "Drive10 CPU" : "CPU";
        case DebuggerTheme::Drive11CPU:
            return emulator ? "Drive11 CPU" : "CPU";
        case DebuggerTheme::Memory:
            return "Memory";
        case DebuggerTheme::MemorySCPU:
            return "Memory SCPU";
        case DebuggerTheme::Drive8Memory:
            return emulator ? "Drive8 Memory" : "Memory";
        case DebuggerTheme::Drive9Memory:
            return emulator ? "Drive9 Memory" : "Memory";
        case DebuggerTheme::Drive10Memory:
            return emulator ? "Drive10 Memory" : "Memory";
        case DebuggerTheme::Drive11Memory:
            return emulator ? "Drive11 Memory" : "Memory";
        case DebuggerTheme::Drive8VIA:
            return emulator ? "Drive8 VIA" : "VIA";
        case DebuggerTheme::Drive9VIA:
            return emulator ? "Drive9 VIA" : "VIA";
        case DebuggerTheme::Drive10VIA:
            return emulator ? "Drive10 VIA" : "VIA";
        case DebuggerTheme::Drive11VIA:
            return emulator ? "Drive11 VIA" : "VIA";
        case DebuggerTheme::CIA:
            return "CIA";
        case DebuggerTheme::Video:
            return !emulator ? "Video" : (dynamic_cast<LIBC64::Interface*>(emulator) ? "VIC-II" : "Denise");
        case DebuggerTheme::DMA:
            return "DMA";
        case DebuggerTheme::SID:
            return "SID";
        case DebuggerTheme::Copper:
            return "Copper";
        case DebuggerTheme::Blitter:
            return "Blitter";
        case DebuggerTheme::Agnus:
            return "Agnus";
        case DebuggerTheme::Paula:
            return "Paula";
        case DebuggerTheme::Serial:
            return "Serial";
    }

    return "";
}