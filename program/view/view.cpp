
#include "view.h"
#include "dragndrop.cpp"
#include "../program.h"
#include "../config/config.h"
#include "../emuconfig/config.h"
#include "../tools/status.h"
#include "../input/manager.h"
#include "../config/archiveViewer.h"
#include "../audio/manager.h"
#include "../cmd/cmd.h"

View* view = nullptr;

View::View() {
    message = new Message(this);
}

auto View::build() -> void {
    setTitle( APP_NAME " " VERSION );
    setBackgroundColor(0);
    cocoa.setDisableIconsInTopMenu(true);
    
    GUIKIT::Geometry defaultGeometry = {100, 100, 600, 400};
    
    GUIKIT::Geometry geometry = {settings->get<int>("screen_x", defaultGeometry.x)
        ,settings->get<int>("screen_y", defaultGeometry.y)
        ,settings->get<unsigned>("screen_width", defaultGeometry.width)
        ,settings->get<unsigned>("screen_height", defaultGeometry.height)
    };
    
    setGeometry( geometry );
    
    if (isOffscreen())        
        setGeometry( defaultGeometry );    
    
    append(viewport);
    
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
        configView->setVisible(false);
        
		for(auto emuConfigView : emuConfigViews)
			emuConfigView->setVisible(false);
			
        GUIKIT::Application::quit();
        program->quit();
    };
    
    onMove = [this]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        settings->set<int>("screen_x", geometry.x);
        settings->set<int>("screen_y", geometry.y);
		audioDriver->clear();
    };
    
    onSize = [this]() {
        if (fullScreen()) {
			bool showStatus = !view->exclusiveFullscreen() && settings->get("statusbar_fullscreen", false);
            setStatusVisible( showStatus );
            setMenuVisible(false);
        } else {
            updateMenuBar();
            updateStatusBar();

            GUIKIT::Geometry geometry = this->geometry();
            settings->set<int>("screen_width", geometry.width);
            settings->set<int>("screen_height", geometry.height);
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
	
	winapi.onMenu = []() {
		audioDriver->clear();
	};
	
	GUIKIT::BrowserWindow::onCall = []() {
		audioDriver->clear();
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
    
    GUIKIT::Application::Cocoa::onPreferences = [this] {
        configView->show(ConfigView::TabWindow::Layout::Settings);
    };
    
    GUIKIT::Application::Cocoa::onDock = [this] {
        view->setFocused();
    };
        
    setDragnDrop();        
}

auto View::show() -> void {
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
}

auto View::exclusiveFullscreen() -> bool {
	static auto exclusiveFullscreen = settings->getOrInit("exclusive_fullscreen", false);	
	return *exclusiveFullscreen && fullScreen() && program->isRunning;
}

auto View::updateMenuBar( bool toggle ) -> void {
    
    bool state = settings->get("menubar", true);
    
    if(toggle) {
        state ^= 1;
        settings->set("menubar", state);
    }

    if (menuVisible() == state)
        return;
    
    setMenuVisible( state );
    
    if(toggle)
        updateViewport();
}

auto View::updateStatusBar(bool toggle) -> void {

    bool state = settings->get("statusbar", true);

    if (toggle) {
        state ^= 1;
        settings->set("statusbar", state);
    }

    if (statusVisible() == state)
        return;

    setStatusVisible( state );
    
    if(toggle)
        updateViewport();
}

auto View::setStatusText(const std::string& text, bool critical) -> void {
    auto option = settings->get("video_screen_text", 0, {0u,2u});
    
    if (option == 0) {
        if(statusVisible()) GUIKIT::Window::setStatusText(text);
        videoDriver->showMessage( "" );
    
    } else if (option == 1) {
        if(!statusVisible()) videoDriver->showMessage( text, critical );
        else {
            GUIKIT::Window::setStatusText(text);
            videoDriver->showMessage( "" );
        }
        
    } else {
        if(statusVisible()) GUIKIT::Window::setStatusText(text);
        videoDriver->showMessage( text, critical );          
    }   
    
    if(!program->isRunning || program->isPause ) {
        program->blackScreen();
    }        
}

auto View::updateViewport() -> void {
	unsigned currentHeight = 0;
	bool integerScaling = false;
	int _height;
    GUIKIT::Geometry geometry = this->geometry();
    geometry.x = geometry.y = 0;
    
    if (activeVideoManager) {
        integerScaling = activeVideoManager->integerScaling;
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
	
	if (settings->get<bool>("aspect_correct", false)) {
		
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
    
    for(auto& iM : inputMenus) {
        
        removeMenuTree( iM.input );
        
        iM.inputDevices.clear();
        
        auto emulator = iM.emulator;
        
        for(auto& connector : emulator->connectors) {

            connectorMenu = new GUIKIT::Menu;
            connectorMenu->setText(connector.name);
            connectorMenu->setIcon(plugImage);
            connectorMenu->setVisible();

            auto selectedDevice = emulator->getConnectedDevice(&connector);
            connectorItems.clear();
            GUIKIT::MenuRadioItem* checkItem = nullptr;

            for (auto& device : emulator->devices) {
                if (device.isKeyboard())
                    continue;

                auto item = new GUIKIT::MenuRadioItem;
                item->setText(trans->get(device.name));

                item->onActivate = [emulator, connector, device]() {
                    settings->set<unsigned>(program->ident(emulator, connector.name), device.id);
                    emulator->connect(connector.id, device.id);
                    InputManager::getManager(emulator)->updateMappingsInUse();
                    EmuConfigView::TabWindow::getView(emulator)->inputLayout->updateConnectorButtons();
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

            iM.input->append(*connectorMenu);
        }
        
        iM.input->append( *new GUIKIT::MenuSeparator );
        
        inputItem = new GUIKIT::MenuItem;
		inputItem->setText( trans->get("swap_devices") );
        
        inputItem->onActivate = [emulator]() {
            
            auto connector1 = emulator->getConnector( 0 );
            auto connectedDevice1 = emulator->getConnectedDevice( connector1 );
            
            auto connector2 = emulator->getConnector( 1 );
            auto connectedDevice2 = emulator->getConnectedDevice( connector2 );
            
            emulator->connect( connector1, connectedDevice2 );
            emulator->connect( connector2, connectedDevice1 );
            
            settings->set<unsigned>( program->ident(emulator, connector1->name), connectedDevice2->id);
            settings->set<unsigned>( program->ident(emulator, connector2->name), connectedDevice1->id);
            
            EmuConfigView::TabWindow::getView(emulator)->inputLayout->updateConnectorButtons();
            
            view->checkInputDevice(emulator, connector1, connectedDevice2);
            view->checkInputDevice(emulator, connector2, connectedDevice1);            
        };
        
        inputItem->setIcon(swapperImage);
        iM.input->append(*inputItem);
        
		inputItem = new GUIKIT::MenuItem;
		inputItem->setText( trans->get("config") );
		
		inputItem->onActivate = [emulator]() {
			auto emuConfigView = EmuConfigView::TabWindow::getView( emulator );
			emuConfigView->show(EmuConfigView::TabWindow::Layout::Input);
		};
		inputItem->setIcon(toolsImage);
		iM.input->append(*inputItem);  

        iM.input->append(*new GUIKIT::MenuSeparator);

        inputItem = new GUIKIT::MenuItem;

        inputItem->onActivate = []() {
            configView->show(ConfigView::TabWindow::Layout::Input);
        };
        inputItem->setIcon(keyboardImage);
        inputItem->setText(trans->get("hotkeys"));
        iM.input->append(*inputItem);
    }
}

auto View::updateShader() -> void {
    
	std::vector<GUIKIT::File::Info> shaderList;
	auto folder = settings->get<std::string>("shader_folder", "");
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

auto View::buildMenu() -> void {
    #include "../../data/img/icons.data"
    #include "../../data/resource.h" // for win xp only 
    regionImage.loadPng((uint8_t*)globe, sizeof(globe));
    regionImage.setResourceId( ID_GLOBE );
    filterImage.loadPng((uint8_t*)filter, sizeof(filter));
    filterImage.setResourceId( ID_FILTER );
    powerImage.loadPng((uint8_t*)power, sizeof(power)); 
    powerImage.setResourceId( ID_POWER );
    freezeImage.loadPng((uint8_t*)freeze, sizeof(freeze)); 
    freezeImage.setResourceId( ID_FREEZE );
	poweroffImage.loadPng((uint8_t*)shutdown, sizeof(shutdown));
    poweroffImage.setResourceId( ID_SHUTDOWN );
    firmwareImage.loadPng((uint8_t*)memory, sizeof(memory));
    firmwareImage.setResourceId( ID_MEMORY );
    driveImage.loadPng((uint8_t*)drive, sizeof(drive));
    driveImage.setResourceId( ID_DRIVE );
    swapperImage.loadPng((uint8_t*)swapper, sizeof(swapper));
    swapperImage.setResourceId( ID_SWAPPER );
    scriptImage.loadPng((uint8_t*)script, sizeof(script));
    scriptImage.setResourceId( ID_SCRIPT );
    systemImage.loadPng((uint8_t*)system, sizeof(system));
    systemImage.setResourceId( ID_SYSTEM );
    joystickImage.loadPng((uint8_t*)joystick, sizeof(joystick));
    joystickImage.setResourceId( ID_JOYSTICK );
    volumeImage.loadPng((uint8_t*)volume, sizeof(volume));
    volumeImage.setResourceId( ID_VOLUME );
    plugImage.loadPng((uint8_t*)plug, sizeof(plug));
    plugImage.setResourceId( ID_PLUG );
    displayImage.loadPng((uint8_t*)display, sizeof(display));
    displayImage.setResourceId( ID_DISPLAY );
    toolsImage.loadPng((uint8_t*)tools, sizeof(tools));
    toolsImage.setResourceId( ID_TOOLS );
	quitImage.loadPng((uint8_t*)quit, sizeof(quit));
    quitImage.setResourceId( ID_QUIT );
	keyboardImage.loadPng((uint8_t*)keyboard, sizeof(keyboard));
    keyboardImage.setResourceId( ID_KEYBOARD );
	colorImage.loadPng((uint8_t*)color, sizeof(color));
    colorImage.setResourceId( ID_COLOR );
	tapeImage.loadPng((uint8_t*)tape, sizeof(tape));
    tapeImage.setResourceId( ID_TAPE );
    paletteImage.loadPng((uint8_t*)palette, sizeof(palette));
    paletteImage.setResourceId( ID_PALETTE );
    cropImage.loadPng((uint8_t*)crop, sizeof(crop));
    cropImage.setResourceId( ID_CROP );
    playImage.loadPng((uint8_t*)play, sizeof(play));
    playImage.setResourceId( ID_PLAY );
    playhiImage.loadPng((uint8_t*)playHi, sizeof(playHi));
    playhiImage.setResourceId( ID_PLAYHI );
    stopImage.loadPng((uint8_t*)stop, sizeof(stop));
    stopImage.setResourceId( ID_STOP );
    stophiImage.loadPng((uint8_t*)stopHi, sizeof(stopHi));
    stophiImage.setResourceId( ID_STOPHI );
    recordImage.loadPng((uint8_t*)record, sizeof(record));
    recordImage.setResourceId( ID_RECORD );
    recordhiImage.loadPng((uint8_t*)recordHi, sizeof(recordHi));
    recordhiImage.setResourceId( ID_RECORDHI );
    forwardImage.loadPng((uint8_t*)forward, sizeof(forward));
    forwardImage.setResourceId( ID_FORWARD );
    forwardhiImage.loadPng((uint8_t*)forwardHi, sizeof(forwardHi));
    forwardhiImage.setResourceId( ID_FORWARDHI );
    rewindImage.loadPng((uint8_t*)rewind, sizeof(rewind));
    rewindImage.setResourceId( ID_REWIND );
    rewindhiImage.loadPng((uint8_t*)rewindHi, sizeof(rewindHi));
    rewindhiImage.setResourceId( ID_REWINDHI );
	counterImage.loadPng((uint8_t*)counter, sizeof(counter));
    counterImage.setResourceId( ID_COUNTER );
    diskImage.loadPng((uint8_t*) disk, sizeof (disk));
    diskImage.setResourceId( ID_DISK );
	
    GUIKIT::MenuRadioItem::setGroup(videoNearestItem, videoLinearItem);
		
    for(auto emulator : emulators) {
        auto emuConfigView = EmuConfigView::TabWindow::getView( emulator );        
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
        sM.poweroff = new GUIKIT::MenuItem;
        sM.poweroff->setIcon( poweroffImage );
        sM.poweroff->onActivate = []() {
		    program->powerOff();
	    };	
        sM.system->append( *sM.poweroff );
        
        sM.system->append(*GUIKIT::MenuSeparator::getInstance());
        
        sM.reset = new GUIKIT::MenuItem;
        sM.reset->onActivate = [emulator]() {
		    program->reset(emulator);
	    };	
        sM.reset->setIcon( powerImage );
        sM.system->append( *sM.reset );
        sM.freeze = new GUIKIT::MenuItem;
        sM.freeze->setIcon( freezeImage );
        sM.freeze->onActivate = [emulator]() {
		    emulator->freeze();
	    };	
        sM.freeze->setEnabled(false);
        sM.system->append( *sM.freeze );
        
        sM.system->append(*GUIKIT::MenuSeparator::getInstance());
        
        sM.video = new GUIKIT::MenuItem;
        sM.video->setIcon( displayImage );
        sM.video->onActivate = [emuConfigView]() {
		    emuConfigView->show(EmuConfigView::TabWindow::Layout::Video);
	    };
        sM.system->append( *sM.video );

        if ( dynamic_cast<LIBC64::Interface*>(emulator)) {
            sM.palette = new GUIKIT::MenuItem;
            sM.palette->setIcon(paletteImage);
            sM.palette->onActivate = [emuConfigView]() {
                emuConfigView->show(EmuConfigView::TabWindow::Layout::Palette);
            };
            sM.system->append(*sM.palette);
        }
        
        sM.border = new GUIKIT::MenuItem;
        sM.border->setIcon( cropImage );
        sM.border->onActivate = [emuConfigView]() {
		    emuConfigView->show(EmuConfigView::TabWindow::Layout::Border);
	    };
        sM.system->append( *sM.border );
        
        sM.shaderMenu = new GUIKIT::Menu;
        sM.shaderMenu->setIcon( colorImage );
        sM.system->append( *sM.shaderMenu );
        
        sM.regionMenu = new GUIKIT::Menu;
        sM.regionMenu->setIcon( regionImage );
        sM.system->append( *sM.regionMenu );
        
        sM.pal = new GUIKIT::MenuRadioItem;
        sM.regionMenu->append( *sM.pal );
        sM.ntsc = new GUIKIT::MenuRadioItem;
        sM.regionMenu->append( *sM.ntsc );
        
        GUIKIT::MenuRadioItem::setGroup( *sM.pal, *sM.ntsc );
        
        if (settings->get<unsigned>( program->ident(emulator, "video_region"), 0u, {0u, 1u}) == 0 )
            sM.pal->setChecked();
        else
            sM.ntsc->setChecked();

        auto palItem = sM.pal;
        auto ntscItem = sM.ntsc;
        
        sM.pal->onActivate = [emulator, ntscItem, emuConfigView]() {
            if (emulator == activeEmulator) {
                if (!view->message->question(trans->get("region_change"))) {
                    ntscItem->setChecked();
                    return;
                }
            }
            
            settings->set<unsigned>(program->ident(emulator, "video_region"), 0u);
            VideoManager::getInstance( emulator )->reloadSettings();
            emuConfigView->videoLayout->base.mode.pal.setChecked();
            emuConfigView->videoLayout->updatePresets();

            if (activeEmulator)
                program->power(activeEmulator);
        };

        sM.ntsc->onActivate = [emulator, palItem, emuConfigView]() {
            if (emulator == activeEmulator) {
                if (!view->message->question(trans->get("region_change"))) {
                    palItem->setChecked();
                    return;
                }
            }

            settings->set<unsigned>(program->ident(emulator, "video_region"), 1u);
            VideoManager::getInstance( emulator )->reloadSettings();
            emuConfigView->videoLayout->base.mode.ntsc.setChecked();
            emuConfigView->videoLayout->updatePresets();

            if (activeEmulator)
                program->power(activeEmulator);
        };
        
        sM.system->append(*GUIKIT::MenuSeparator::getInstance());
        
        sM.media = new GUIKIT::MenuItem;
        sM.media->setIcon( driveImage );
        sM.media->onActivate = [emuConfigView]() {
		    emuConfigView->show(EmuConfigView::TabWindow::Layout::Media);
	    };
        sM.system->append( *sM.media );
        
        sM.firmware = new GUIKIT::MenuItem;
        sM.firmware->setIcon( firmwareImage );
        sM.firmware->onActivate = [emuConfigView]() {
		    emuConfigView->show(EmuConfigView::TabWindow::Layout::Firmware);
	    };
        sM.system->append( *sM.firmware );

        sM.diskSwapper = new GUIKIT::MenuItem;
        sM.diskSwapper->setIcon( swapperImage );
        sM.diskSwapper->onActivate = [emuConfigView]() {
		    emuConfigView->show(EmuConfigView::TabWindow::Layout::Swapper);
	    };
        sM.system->append( *sM.diskSwapper );
        sM.system->append(*GUIKIT::MenuSeparator::getInstance());       
        
        sM.systemManagement = new GUIKIT::MenuItem;
        sM.systemManagement->setIcon( systemImage );
        sM.systemManagement->onActivate = [emuConfigView]() {
		    emuConfigView->show(EmuConfigView::TabWindow::Layout::System);
	    };
        sM.system->append( *sM.systemManagement );
        sM.saveState = new GUIKIT::MenuItem;
        sM.saveState->setIcon( scriptImage );
        sM.saveState->onActivate = [emuConfigView]() {
		    emuConfigView->show(EmuConfigView::TabWindow::Layout::States);
	    };
        sM.system->append( *sM.saveState );

        sM.system->append(*GUIKIT::MenuSeparator::getInstance());
        sM.exit = new GUIKIT::MenuItem;
        sM.exit->setIcon( quitImage );
        sM.exit->onActivate = [this]() {
		    onClose();
	    };	
        sM.system->append( *sM.exit );
        
        sysMenus.push_back( sM );
        
        if ( dynamic_cast<LIBAMI::Interface*>(emulator))        
            sM.system->setEnabled( false );
        
        append( *sM.system );
            
        InputMenu iM;
        iM.emulator = emulator;
        iM.input = new GUIKIT::Menu;
        iM.input->setIcon(joystickImage);
        inputMenus.push_back(iM);
        
        if (dynamic_cast<LIBAMI::Interface*> (emulator))
            iM.input->setEnabled(false);
            
        append(*iM.input);
    }
    
    settingsMenu.setIcon(toolsImage);
    append(settingsMenu);
    	
    filterMenu.setIcon(filterImage);
    settingsMenu.append(filterMenu);
    videoNearestItem.onActivate = [&]() {
        settings->set<unsigned>("video_filter", 0);
        program->setVideoFilter();		
    };
    videoLinearItem.onActivate = [&]() {
        settings->set<unsigned>("video_filter", 1);
        program->setVideoFilter();
    };
    switch( settings->get<unsigned>("video_filter", 1u, {0u, 1u}) ) {
        case 0: videoNearestItem.setChecked(); break;
        case 1: videoLinearItem.setChecked(); break;
    }
    filterMenu.append(videoNearestItem);
    filterMenu.append(videoLinearItem);	

	videoItem.setIcon( displayImage );
    videoItem.onActivate = []() {
        configView->show(ConfigView::TabWindow::Layout::Video);
    };

	settingsMenu.append(videoItem);		    
	
	audioItem.onActivate = []() {
        configView->show(ConfigView::TabWindow::Layout::Audio);
    };	
	audioItem.setIcon( volumeImage );
	settingsMenu.append(audioItem);
    
    inputItem.onActivate = []() {
        configView->show(ConfigView::TabWindow::Layout::Input);
    };	
	inputItem.setIcon( keyboardImage );
	settingsMenu.append(inputItem);
	
    settingsMenu.append(*GUIKIT::MenuSeparator::getInstance());
    audioSyncItem.onToggle = [&]() {
        settings->set<bool>("audio_sync", audioSyncItem.checked() );
        audioManager->setSynchronize();
    };
    if ( settings->get<bool>("audio_sync", true) ) audioSyncItem.setChecked();
    settingsMenu.append(audioSyncItem);
    videoSyncItem.onToggle = [&]() {
        settings->set<bool>("video_sync", videoSyncItem.checked() );
        program->setVideoSynchronize();
    };
    if ( settings->get<bool>("video_sync", false) ) videoSyncItem.setChecked();
    settingsMenu.append(videoSyncItem);

    dynamicRateControl.onToggle = [&]() {
        settings->set<bool>("dynamic_rate_control", dynamicRateControl.checked() );
        audioManager->setRateControl();
    };
    if ( settings->get<bool>("dynamic_rate_control", false) ) dynamicRateControl.setChecked();
    settingsMenu.append(dynamicRateControl);

    
    
    settingsMenu.append(*GUIKIT::MenuSeparator::getInstance());
    muteItem.onToggle = [&]() {
        settings->set<bool>("audio_mute", muteItem.checked() );
        audioManager->setVolume();
    };
    if ( settings->get<bool>("audio_mute", false) ) muteItem.setChecked();
    settingsMenu.append(muteItem);
    fpsItem.onToggle = [&]() {
        settings->set<bool>("fps", fpsItem.checked() );
		status->showFps = fpsItem.checked();
    };
    if ( settings->get<bool>("fps", false) ) fpsItem.setChecked();
    settingsMenu.append(fpsItem);
    
    audioBufferItem.onToggle = [&]() {
        settings->set<bool>("show_audio_buffer", audioBufferItem.checked() );
		audioManager->setStatistics();
    };
    if ( settings->get<bool>("show_audio_buffer", false) ) audioBufferItem.setChecked();
    settingsMenu.append(audioBufferItem);

    settingsMenu.append(*GUIKIT::MenuSeparator::getInstance());
   
    configItem.onActivate = []() {
        configView->show(ConfigView::TabWindow::Layout::Settings);
    };
    configItem.setIcon(toolsImage);
    settingsMenu.append(configItem);
    
    saveItem.onActivate = [this]() {
		program->saveSettings();
    };
    saveItem.setIcon(diskImage);
    settingsMenu.append(saveItem);
	
	// prepare Tape Control	
	tapeControlMenu.setIcon( tapeImage );
	
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
	
	tapeResetCounterItem.onActivate = []() {
		InputManager::activateHotkey(Hotkey::Id::ResetTapeCounter);
	};
	tapeControlMenu.append( tapeResetCounterItem );
    
    updateTapeIcons();
}

auto View::showTapeMenu( bool show, Emulator::Interface::TapeMode mode ) -> void {
        
    if (show)
        updateTapeIcons(mode);

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
}

auto View::translate() -> void {
	setConnectors();
	
    for(auto& sysMenu : sysMenus) {
        sysMenu.system->setText(sysMenu.emulator->ident);
        sysMenu.poweron->setText(trans->get("power"));
        sysMenu.reset->setText(trans->get("Soft Reset"));
        sysMenu.poweroff->setText(trans->get("power_off"));
        sysMenu.freeze->setText(trans->get("Freeze"));
        sysMenu.firmware->setText(trans->get("Firmware"));
        sysMenu.media->setText(trans->get("Software"));
        sysMenu.diskSwapper->setText(trans->get("disk_swapper"));
        sysMenu.systemManagement->setText(trans->get("system_management"));
        sysMenu.saveState->setText(trans->get("states"));
        sysMenu.video->setText(trans->get("Video"));
        sysMenu.palette->setText(trans->get("Palette"));
        sysMenu.border->setText(trans->get("Border"));
        sysMenu.shaderMenu->setText(trans->get("Shader"));
        sysMenu.regionMenu->setText( trans->get("region") );
        sysMenu.pal->setText( "PAL" );
        sysMenu.ntsc->setText( "NTSC" );
        sysMenu.exit->setText(trans->get("Exit"));
    }    
    
    for(auto& iM : inputMenus) {
        iM.input->setText( trans->get("input") );
    }
    
    settingsMenu.setText( trans->get("settings"));
    filterMenu.setText( trans->get("filter"));
    videoNearestItem.setText("Video Nearest");
    videoLinearItem.setText("Video Linear");

	videoItem.setText( trans->get("video") );
	audioItem.setText( trans->get("audio") );
    inputItem.setText( trans->get("input") );
    audioSyncItem.setText( trans->get("sync_audio"));
    videoSyncItem.setText( trans->get("sync_video"));
    dynamicRateControl.setText( trans->get("dynamic_rate_control"));

    muteItem.setText( trans->get("mute_audio"));
    fpsItem.setText( trans->get("show_fps"));
    audioBufferItem.setText( trans->get("show_audio_buffer"));

    configItem.setText( trans->get("settings"));	
    saveItem.setText( trans->get("save_changes"));
	
	tapeControlMenu.setText( "Datasette" );
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
    cocoa.setTitleForAppMenuItem(GUIKIT::Window::Cocoa::AppMenuItem::Hide, trans->get("hide_app", {{"%app%", APP_NAME}}));
    cocoa.setTitleForAppMenuItem(GUIKIT::Window::Cocoa::AppMenuItem::HideOthers, trans->get("hide_others"));
    cocoa.setTitleForAppMenuItem(GUIKIT::Window::Cocoa::AppMenuItem::ShowAll, trans->get("show_all"));
    cocoa.setTitleForAppMenuItem(GUIKIT::Window::Cocoa::AppMenuItem::Quit, trans->get("quit", {{"%app%", APP_NAME}}));          
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

auto View::getSysMenu( Emulator::Interface* emulator ) -> View::SystemMenu* {
    
    for (auto& sM : sysMenus) {
        if (sM.emulator == emulator)
            return &sM;
    }
    
    return nullptr;
}

auto View::updateFreeze( Emulator::Interface* emulator ) -> void {

    for (auto& sM : sysMenus) {
        if ((sM.emulator == emulator) && emulator->hasFreezerButton())
            sM.freeze->setEnabled( true );
        else
            sM.freeze->setEnabled( false );
    }
}

auto View::questionToWrite(Emulator::Interface::Media* media) -> bool {
    
    auto file = (GUIKIT::File*)media->guid;
    
    if (file->isArchived())
        // archive, removing of write protection is not supported
        return false;
    
    std::string mediaIdent = media->group->isExpansion() ? media->group->name : media->name;
    
    bool state = message->question( trans->get("override_write_protection", {{"%media%", mediaIdent}}) );
    
    if (state)        
        EmuConfigView::TabWindow::getView( activeEmulator )->mediaLayout->disableWriteProtection(media);
    
    return state;
}
