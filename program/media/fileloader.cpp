
#include "fileloader.h"
#include "autoloader.h"
#include "../emuconfig/config.h"
#include "../../data/resource.h"
#include "../view/view.h"
#include "media.h"
#include "../tools/filepool.h"
#include "../config/archiveViewer.h"
#include "../states/states.h"
#include "../cmd/cmd.h"
#include "../firmware/manager.h"
#include "../thread/emuThread.h"
#include "../tools/DiskFinder.h"
#include "../view/status.h"
#include "../audio/manager.h"
#include "recentFiles.h"

#define HideMouseIfWasBefore \
    if (mIsAcquiredBefore && !inputDriver->mIsAcquired() && view->fullScreen() && fileDialogPtr && fileDialogPtr->detached()) \
        inputDriver->mAcquire();

#define USE_TRAPS 0x80
#define OVERRIDE_SPEEDER 0x40

Fileloader* fileloader = nullptr;

Fileloader::Fileloader() {
    foregroundTimer.setInterval(150);

    foregroundTimer.onFinished = [this]() {
        foregroundTimer.setEnabled(false);

        if (fileDialogPtr && fileDialogPtr->visible())
            fileDialogPtr->setForeground();
    };
    queuePreview.status = 0;
}

Fileloader::~Fileloader() {
    for (auto recentFile : recentFiles)
        delete recentFile;
}

auto Fileloader::load(Emulator::Interface* emulator, Emulator::Interface::Media* media) -> void {
    auto group = media->group;
    auto suffix = group->suffix;
    auto settings = program->getSettings( emulator );
    auto emuView = EmuConfigView::TabWindow::getView( emulator );
    auto prevMode = settings->get<unsigned>("dialog_preview_mode", dynamic_cast<LIBC64::Interface*>(emulator) ? PREV_DIALOG : PREV_OFF, {0,2});

    if ((prevMode == PREV_SOFTWARE) && emuView && emuView->visible()) {
        emuView->setFocused();
    }

    if (group->isDisk()) {
        auto prgGroup = emulator->getPRGMediaGroup();
        if (prgGroup)
            GUIKIT::Vector::combine(suffix, prgGroup->suffix);
    }

    GUIKIT::Vector::combine(suffix, GUIKIT::File::suppportedCompressionExtensions());

    if (fileDialogPtr) {
        fileDialogPtr->close();
        delete fileDialogPtr;
        resetPreview( emulator );
    }

    fileDialogPtr = new GUIKIT::BrowserWindow;

    fileDialogPtr->setDefaultButtonText( trans->get("insert") );

    fileDialogPtr->setTemplateId( IDD_FILE_TEMPLATE );

    fileDialogPtr->resizeTemplate( true, -6 );

    fileDialogPtr->setTitle(trans->get("select_" + group->name + "_image"));

    fileDialogPtr->setPath( preselectPath( settings, group->name, group->isDrive() && (media->id > 0) ) );

    fileDialogPtr->setFilters({ GUIKIT::BrowserWindow::transformFilter(trans->get(group->name + "_image"), suffix ),
                                trans->get("all_files")});

    fileDialogPtr->setOnChangeCallback( [this, emulator, media](std::string file, bool multi) {

        return this->previewFile(file, emulator, media);
    } );

    fileDialogPtr->addCustomButton( trans->get("Autostart"), [this, emulator, media](std::vector<std::string> filePaths, unsigned selection) {
        if (filePaths.size() == 0)
            return false;
        std::string filePath = filePaths[0];
        if (filePath.empty())
            return false;

        uint8_t _a = 1;
        if (this->fileDialogPtr->hasChecked() && dynamic_cast<LIBC64::Interface*>(emulator))
            _a |= OVERRIDE_SPEEDER;

        return this->insertFile(emulator, media, filePath, _a, selection);
    }, IDC_BUTTON );

    //if ((prevMode != PREV_ALTERNATE) && group->isDrive() && dynamic_cast<LIBC64::Interface*>(emulator) ) {
    if (group->isDrive() && dynamic_cast<LIBC64::Interface*>(emulator) ) {
        fileDialogPtr->addCustomButton( trans->get("VDT Autostart"), [this, emulator, media](std::vector<std::string> filePaths, unsigned selection) {
            if (filePaths.size() == 0)
                return false;
            std::string filePath = filePaths[0];
            if (filePath.empty())
                return false;

            uint8_t _a = 1 | USE_TRAPS;
            if (this->fileDialogPtr->hasChecked() && dynamic_cast<LIBC64::Interface*>(emulator))
                _a |= OVERRIDE_SPEEDER;

            return this->insertFile(emulator, media, filePath, _a, selection);
        }, IDC_BUTTON1 );
    }

    if (prevMode == PREV_DIALOG) {
        fileDialogPtr->addContentView(IDC_LIST, [this, media, emulator, settings](std::string filePath, unsigned selection) {
            auto _useTraps = false;

            if (dynamic_cast<LIBC64::Interface*>(emulator)) {
                if (media->group->isTape())
                    _useTraps = settings->get<bool>("autostart_tape_traps_on_dblclick", false);
                if (media->group->isDisk())
                    _useTraps = settings->get<bool>("autostart_traps_on_dblclick", false);
            }

            if (filePath.empty())
                return false;

            uint8_t _a = 1;
            if (_useTraps)
                _a |= USE_TRAPS;
            if (this->fileDialogPtr->hasChecked() && dynamic_cast<LIBC64::Interface*>(emulator))
                _a |= OVERRIDE_SPEEDER;

            return insertFile(emulator, media, filePath, _a, selection);
        });

        applyPreviewFont( emulator, settings->get<unsigned>("dialog_preview_fontsize", 11, {6, 14}) );
        fileDialogPtr->setContentViewWidth( settings->get<unsigned>("dialog_preview_width", 450, {200, 600}) );
        fileDialogPtr->setContentViewHeight( settings->get<unsigned>("dialog_preview_height", 200, {50, 400}) );

        auto videoManager = VideoManager::getInstance( emulator );
        unsigned foregroundColor = videoManager->getForegroundColor();
        unsigned backgroundColor = videoManager->getBackgroundColor();

        fileDialogPtr->setContentViewBackground( backgroundColor );
        fileDialogPtr->setContentViewForeground( foregroundColor );

        if (settings->get<bool>("software_preview_commodore_hi", true ))
            fileDialogPtr->setContentViewSelection( backgroundColor, foregroundColor );

        if (dynamic_cast<LIBAMI::Interface*>(emulator) )
            fileDialogPtr->setContentViewFirstRow( backgroundColor, foregroundColor );

        fileDialogPtr->setContentViewColorTooltips(true);
    }

    fileDialogPtr->setCallbacks( [this, emulator, media](std::vector<std::string> filePaths, unsigned selection) {
        if (filePaths.size()) {
            std::string filePath = filePaths[0];
            insertFile(emulator, media, filePath);
        }
    }, [this, emulator]() {
            this->resetPreview(emulator);
    } );

    fileDialogPtr->setWindow( *view ).setNonModal();

    if (group->isDisk() && dynamic_cast<LIBC64::Interface*>(emulator)) {
        if (emulator->getModelValue(LIBC64::Interface::ModelIdDriveFastLoader) || settings->get<unsigned>("use_firmware", 0))
            fileDialogPtr->addCheckButton(false, trans->getA("without speeder"), [settings](bool checked) {});
    }

    std::string filePath = fileDialogPtr->open();

    if (fileDialogPtr && fileDialogPtr->detached()) {
        return;
    }

    if (filePath.empty()) {
        if (GUIKIT::Application::loop)
            this->resetPreview(emulator);

        return;
    }

    insertFile(emulator, media, filePath);
}

auto Fileloader::anyLoad( Emulator::Interface* emulator, bool mIsAcquiredBefore ) -> void {
	auto settings = program->getSettings( emulator );
    auto prevMode = settings->get<unsigned>("dialog_preview_mode", dynamic_cast<LIBC64::Interface*>(emulator) ? PREV_DIALOG : PREV_OFF, {0,2});
    bool trapped = false;
    if (dynamic_cast<LIBC64::Interface*>(emulator))
        trapped = settings->get("autostart_traps_on_dblclick", false) || settings->get("autostart_tape_traps_on_dblclick", false);

    auto emuView = EmuConfigView::TabWindow::getView( emulator );

    if ((prevMode == PREV_SOFTWARE) && emuView && emuView->visible()) {
        emuView->setFocused();
    }	    

    if (fileDialogPtr) {
        fileDialogPtr->close();
        resetPreview( emulator );
        delete fileDialogPtr;
    }

    fileDialogPtr = new GUIKIT::BrowserWindow;

    fileDialogPtr->setTemplateId( IDD_FILE_TEMPLATE );

    if (prevMode == PREV_DIALOG) {
        fileDialogPtr->addContentView( IDC_LIST, [this, settings, emulator, mIsAcquiredBefore](std::string filePath, unsigned selection) {
            if (filePath.empty())
                return false;

            settings->set<std::string>("anyload_path", GUIKIT::File::getPath( filePath ) );

            emuThread->lock();
            autoloader->init( {filePath}, false, Autoloader::Mode::AutoStartDblClick, selection );
            autoloader->setEmulator( emulator );
            if (this->fileDialogPtr->hasChecked() && dynamic_cast<LIBC64::Interface*>(emulator))
                autoloader->overrideSpeeder();
            autoloader->loadFiles();
            emuThread->unlock();

            this->resetPreview(emulator);

            HideMouseIfWasBefore

            return true;
        } );

        applyPreviewFont( emulator, settings->get<unsigned>("dialog_preview_fontsize", 11, {6, 14}) );
        fileDialogPtr->setContentViewWidth( settings->get<unsigned>("dialog_preview_width", 450, {200, 600}) );
        fileDialogPtr->setContentViewHeight( settings->get<unsigned>("dialog_preview_height", 200, {50, 400}) );

        auto videoManager = VideoManager::getInstance( emulator );
        unsigned foregroundColor = videoManager->getForegroundColor();
        unsigned backgroundColor = videoManager->getBackgroundColor();
        fileDialogPtr->setContentViewBackground( backgroundColor );
        fileDialogPtr->setContentViewForeground( foregroundColor );

        if (settings->get<bool>("software_preview_commodore_hi", true ))
            fileDialogPtr->setContentViewSelection( backgroundColor, foregroundColor );

        if (dynamic_cast<LIBAMI::Interface*>(emulator) )
            fileDialogPtr->setContentViewFirstRow( backgroundColor, foregroundColor );

        fileDialogPtr->setContentViewColorTooltips(true);
    }

    fileDialogPtr->setPath( settings->get<std::string>( "anyload_path", "") );

    fileDialogPtr->setFilters({trans->get("all_files")});

    fileDialogPtr->setOnChangeCallback( [this, emulator](std::string file, bool multi) {

        if (multi && dynamic_cast<LIBAMI::Interface*>(emulator)) {
            // amiga disk / harddisk multi selection
            auto extension = GUIKIT::String::getExtension(file, "exe");
            auto mediaGroups = emulator->getDriveMediaGroups();
            std::vector<GUIKIT::BrowserWindow::Listing> out;

            for (auto mediaGroup : mediaGroups) {
                if (GUIKIT::Vector::find(mediaGroup->suffix, extension)) {
                    for (auto media : mediaGroup->media)
                        out.push_back({ media.name });
                }
            }
            return out;
        }

        return this->previewFile(file, emulator);
    } );

    fileDialogPtr->addCustomButton( trans->get("insert"), [this, emulator, settings, mIsAcquiredBefore](std::vector<std::string> filePaths, unsigned selection) {
        if (filePaths.size() == 0)
            return false;
        std::string filePath = filePaths[0];
        if (filePath.empty())
            return false;

        settings->set<std::string>("anyload_path", GUIKIT::File::getPath( filePath ) );

        emuThread->lock();
        autoloader->init( filePaths, false, Autoloader::Mode::Open );
        autoloader->setEmulator( emulator );
        if (this->fileDialogPtr->hasChecked() && dynamic_cast<LIBC64::Interface*>(emulator))
            autoloader->overrideSpeeder();
        autoloader->loadFiles();
        emuThread->unlock();

        resetPreview(emulator);

        HideMouseIfWasBefore

        return true;
    }, IDC_BUTTON );

    if (dynamic_cast<LIBC64::Interface*>(emulator) ) {
        fileDialogPtr->addCustomButton( trans->get( "VDT Autostart" ), [this, emulator, settings, mIsAcquiredBefore](std::vector<std::string> filePaths, unsigned selection) {
            if (filePaths.size() == 0)
                return false;
            std::string filePath = filePaths[0];
            if (filePath.empty())
                return false;
            settings->set<std::string>("anyload_path", GUIKIT::File::getPath( filePath ) );

            emuThread->lock();
            autoloader->init( {filePath}, false, Autoloader::Mode::AutoStartSecondary, selection );
            autoloader->setEmulator( emulator );
            if (this->fileDialogPtr->hasChecked() && dynamic_cast<LIBC64::Interface*>(emulator))
                autoloader->overrideSpeeder();
            autoloader->loadFiles();
            emuThread->unlock();

            resetPreview(emulator);

            HideMouseIfWasBefore

            return true;
        }, IDC_BUTTON1 );
		
        if (trapped) {
            fileDialogPtr->addCustomButton( trans->get( "Autostart" ), [this, emulator, settings, mIsAcquiredBefore](std::vector<std::string> filePaths, unsigned selection) {
                if (filePaths.size() == 0)
                    return false;
                std::string filePath = filePaths[0];
                if (filePath.empty())
                    return false;
                settings->set<std::string>("anyload_path", GUIKIT::File::getPath( filePath ) );

                emuThread->lock();
                autoloader->init( {filePath}, false, Autoloader::Mode::AutoStartPrimary, selection );
                autoloader->setEmulator( emulator );
                if (this->fileDialogPtr->hasChecked() && dynamic_cast<LIBC64::Interface*>(emulator))
                    autoloader->overrideSpeeder();
                autoloader->loadFiles();
                emuThread->unlock();

                resetPreview(emulator);

                HideMouseIfWasBefore

                return true;
            }, IDC_BUTTON2 );
        }
    }

    fileDialogPtr->setCallbacks( [this, emulator, settings, mIsAcquiredBefore](std::vector<std::string> filePaths, unsigned selection) {
        if (filePaths.size()) {
            std::string filePath = filePaths[0];
            settings->set<std::string>("anyload_path", GUIKIT::File::getPath(filePath));

            emuThread->lock();
            autoloader->init(filePaths, false, Autoloader::Mode::AutoStartDblClick, selection);
            autoloader->setEmulator( emulator );
            if (this->fileDialogPtr->hasChecked() && dynamic_cast<LIBC64::Interface*>(emulator))
                autoloader->overrideSpeeder();
            autoloader->loadFiles();
            emuThread->unlock();
        }
        resetPreview(emulator);

        HideMouseIfWasBefore

    }, [this, emulator, mIsAcquiredBefore]() {
        this->resetPreview(emulator);

        if (view) {
            view->setForeground();
            view->setFocused();
        }

        HideMouseIfWasBefore
    } );

    fileDialogPtr->resizeTemplate( true, -6 );   

    std::string buttonTxt = "Autostart";
    if (trapped) {
        fileDialogPtr->hideOkButton();
        buttonTxt = "Ok";
    }
	
	fileDialogPtr->setDefaultButtonText( trans->get( buttonTxt ) );
	
	//fileDialogPtr->setDefaultButtonTooltip( tooltip );

    fileDialogPtr->setWindow( *view ).setNonModal();

    std::vector<std::string> filePaths;

    if (dynamic_cast<LIBC64::Interface*>(emulator)) {
        fileDialogPtr->setTitle(trans->get("select image"));

        if (emulator->getModelValue(LIBC64::Interface::ModelIdDriveFastLoader) || settings->get<unsigned>("use_firmware", 0))
            fileDialogPtr->addCheckButton(false, trans->getA("without speeder"), [settings](bool checked) {}, GUIKIT::BrowserWindow::CheckButton::Mode::Default, trans->getA("prevent speeder tooltip"));
        
        std::string filePath = fileDialogPtr->open();
        filePaths.push_back(filePath);
    } else {
        fileDialogPtr->setTitle(trans->get("select image multi"));
        fileDialogPtr->addCheckButton(settings->get<bool>("loader_order_selected", false), trans->get("order selected"), [settings](bool checked) {
            settings->set<bool>( "loader_order_selected", checked );
        }, GUIKIT::BrowserWindow::CheckButton::Mode::OrderBySelected);
        fileDialogPtr->setContentViewHint( trans->get("multi file selection"), trans->get("swapper multi hint tooltip") );

        filePaths = fileDialogPtr->openMulti();
    }

    if (fileDialogPtr && fileDialogPtr->detached()) {
        // cocoa/gtk don't block for modeless dialog
        // it handles OK state in callback
        return;
    }

    if ( filePaths.size() && !filePaths[0].empty() ) {
        settings->set<std::string>("anyload_path", GUIKIT::File::getPath( filePaths[0] ) );

        emuThread->lock();
        autoloader->init( filePaths, false, Autoloader::Mode::AutoStartDblClick, fileDialogPtr ? fileDialogPtr->getContentViewSelection() : 0 );
        autoloader->setEmulator( emulator );
        if (fileDialogPtr->hasChecked() && dynamic_cast<LIBC64::Interface*>(emulator))
            autoloader->overrideSpeeder();
        autoloader->loadFiles();
        emuThread->unlock();
    } else if (view) {
        view->setFocused(100);
    }

    resetPreview(emulator);
    view->setForeground();
    view->setFocused();
    if (mIsAcquiredBefore && view->fullScreen())
        inputDriver->mAcquire();
}

auto Fileloader::applyPreviewFont(Emulator::Interface* emulator, unsigned fontSize) -> void {
    auto customFont = GUIKIT::Window::getCustomFont(emulator);

    if (customFont)
        fileDialogPtr->setContentViewFont(customFont->name + ", " + std::to_string(fontSize + customFont->sizeAdjust), dynamic_cast<LIBC64::Interface*>(emulator));
    else
        fileDialogPtr->setContentViewFont(GUIKIT::Font::system(fontSize));
}

auto Fileloader::previewFile( std::string filePath, Emulator::Interface* emulator, Emulator::Interface::Media* media ) -> std::vector<GUIKIT::BrowserWindow::Listing> {
    Emulator::Interface::MediaGroup* mediaGroup = nullptr;

    if (media)
        mediaGroup = media->group;

    std::unique_lock<std::mutex> lck(previewMutex);
    uint8_t _status = queuePreview.status;
    queuePreview.emulator = emulator;
    queuePreview.filePath = filePath;
    queuePreview.media = media;
    queuePreview.status = 3;
    queuePreview.listings = {};
    lck.unlock();

    if (!previewTimer.onFinished) {
        previewTimer.onFinished = [this]() {
            std::unique_lock<std::mutex> lck(previewMutex);
            Emulator::Interface* emulator = queuePreview.emulator;
            auto prevMode = program->getSettings( emulator )->get<unsigned>("dialog_preview_mode", dynamic_cast<LIBC64::Interface*>(emulator) ? PREV_DIALOG : PREV_OFF, {0,2});
            uint8_t _status = queuePreview.status;
            std::string filePath = queuePreview.filePath;
            Emulator::Interface::Media* media = queuePreview.media;
            std::vector<GUIKIT::BrowserWindow::Listing> listings = queuePreview.listings;
            lck.unlock();

            if ( (_status & 0xc) == 0) {
                return; // keep waiting
            }

            if (_status & 4) {
                if (queuePreview.lastMedia)
                    resetPreview(emulator);

            } else {

                if (queuePreview.lastMedia && (queuePreview.lastMedia != media))
                    resetPreview(emulator);

                Emulator::Interface::MediaGroup* group = media->group;

                auto emuView = EmuConfigView::TabWindow::getView( emulator, prevMode == PREV_SOFTWARE );
                if (emuView) {
                    if (!emuView->mediaLayout && (prevMode == PREV_SOFTWARE))
                        emuView->prepareLayout( EmuConfigView::TabWindow::Layout::Media);

                    if(emuView->mediaLayout)
                        emuView->mediaLayout->fillListing(media, listings, !queuePreview.lastMedia);
                }

                queuePreview.lastMedia = media;

                if (emuView && emuView->mediaLayout && (prevMode != PREV_OFF) )
                    emuView->mediaLayout->showMediaGroupLayout(group);

                if (fileDialogPtr)
                    fileDialogPtr->setListings(listings);

                if ((queuePreview.status & 16) && listings.size()) {
                    program->getSettings( emulator )->set<std::string>("anyload_path", GUIKIT::File::getPath(filePath));
                }

                if ((prevMode == PREV_SOFTWARE) && emuView) {
                    emuView->setLayout( EmuConfigView::TabWindow::Layout::Media );

                    if ( !emuView->visible() ) {
                        emuView->setVisible();
                        emuView->setFocused();
                        foregroundTimer.setEnabled();
                    } else if ( emuView->minimized() ) {
                        emuView->restore();
                        foregroundTimer.setEnabled();
                    } else if ( GUIKIT::Application::isWinApi() && !emuView->focused() && view->fullScreen() ) {
                        emuView->setForeground();
                        if (fileDialogPtr && fileDialogPtr->visible())
                            fileDialogPtr->setForeground();
                    }
                }
            }

            queuePreview.status = 0;
            previewTimer.setEnabled(false);
        };
    }
    if (!previewTimer.enabled()) {
        previewTimer.setInterval( 20 );
        previewTimer.setEnabled();
    }

    if (_status & 1)
        return {};

    std::thread worker( [this] {

        while(1) {
            queuePreview.status &= ~2;
            GUIKIT::File file;
            std::string fileName;
            std::string filePath;
            Emulator::Interface* emulator;
            Emulator::Interface::Media* media;
            std::string extension;
            Emulator::Interface::MediaGroup* group = nullptr;
            std::vector<Emulator::Interface::Listing> listings;

            std::unique_lock<std::mutex> lck(previewMutex);
            emulator = queuePreview.emulator;
            filePath = queuePreview.filePath;
            media = queuePreview.media;
            lck.unlock();

            file.setFile( filePath );
            file.setReadOnly();
            auto items = file.scanArchive();

            if ((file.getSize() == 0) || (items.size() != 1)) {
                if (queuePreview.status & 2)
                    continue;

                queuePreview.status &= ~1;
                queuePreview.status |= 4;
                break;
            }

            fileName = items[0].info.name;
            queuePreview.fileName = fileName;
            extension = GUIKIT::String::getExtension(fileName, "exe");
            GUIKIT::String::toLowerCase( extension );

            for(auto& mediaGroup : emulator->mediaGroups) {

                if (media && (media->group != &mediaGroup) )
                    continue;

                if (mediaGroup.isDisk()) {
                    if ( GUIKIT::Vector::find( mediaGroup.suffix, extension ) ) {
                        listings = emulator->getDiskPreview(file.archiveData(0), file.archiveDataSize(0), media);
                        group = &mediaGroup;
                        break;
                    } else if (media) {
                        auto prgGroup = emulator->getPRGMediaGroup();
                        if (prgGroup && GUIKIT::Vector::find(prgGroup->suffix, extension)) {
                            listings = emulator->getProgramPreview(file.archiveData(0), file.archiveDataSize(0));
                            group = &mediaGroup;
                            break;
                        }
                    }
                }

                if (mediaGroup.isHardDisk()) {
                    if (GUIKIT::Vector::find(mediaGroup.suffix, extension)) {
                        Emulator::Interface::Media previewMedia;
                        previewMedia.guid = uintptr_t(&file);
                        previewMedia.id = media ? media->id : 0;
                        listings = emulator->getHardDiskPreview(!file.isArchived() ? nullptr : file.archiveData(0), file.archiveDataSize(0), &previewMedia);
                        group = &mediaGroup;
                        break;
                    }
                }

                if (mediaGroup.isTape()) {                    
                    if ( GUIKIT::Vector::find( mediaGroup.suffix, extension ) ) {
                        listings = emulator->getTapePreview(file.archiveData(0), file.archiveDataSize(0), media);
                        group = &mediaGroup;
                        break;
                    }
                }

                if (mediaGroup.isProgram()) {
                    if ( GUIKIT::Vector::find( mediaGroup.suffix, extension ) ) {
                        listings = emulator->getProgramPreview(file.archiveData(0), file.archiveDataSize(0));
                        group = &mediaGroup;
                        break;
                    }
                }

                if (mediaGroup.isExpansion()) {
                    if (GUIKIT::Vector::find(mediaGroup.suffix, extension)) {
                        listings = emulator->getExpansionPreview(file.archiveData(0), file.archiveDataSize(0));
                        group = &mediaGroup;
                        break;
                    }
                }
            }

            if (!group) {
                if (queuePreview.status & 2)
                    continue;

                queuePreview.status &= ~1;
                queuePreview.status |= 4;
                break;
            }

            auto convertedListings = convertListing( emulator, listings );

            lck.lock();
            queuePreview.listings = convertedListings;
            if (!media) {
                queuePreview.media = group->selected ? group->selected : &group->media[0];
                queuePreview.status |= 16;
            }
            lck.unlock();

            if (!(queuePreview.status & 2)) {
                queuePreview.status &= ~1;
                queuePreview.status |= 8;
                break;
            }
        }
    });
    worker.detach();
    return {};
}

auto Fileloader::eject(Emulator::Interface* emulator, Emulator::Interface::MediaGroup* mediaGroup, bool secondaryOnly) -> void {

    for( auto& media : mediaGroup->media ) {
        if (!secondaryOnly || media.secondary)
            eject( emulator, &media);
    }
}

auto Fileloader::eject(Emulator::Interface* emulator, Emulator::Interface::Media* media) -> void {

    if ( !media->group->isExpansion() ) {
        emulator->ejectMedium(media);
        filePool->assign( _ident(emulator, media->name), nullptr);
        if (!cmd->noGui)
            States::getInstance( emulator )->updateImage( nullptr, media );
    } else {
        if (!cmd->noGui)
            States::getInstance(emulator)->forcePowerNextLoad = true;
    }
    // an expansion can't be removed while emulation is running.
    // but we have to cut the file link or EasyFlash could
    // write back data, even when user has removed file from UI.
    media->guid = (uintptr_t)nullptr;

    if (view && activeEmulator && media->group->isTape())
        view->updateTapeIcons();

    filePool->assign( _ident(emulator, media->name + "store"), nullptr);
    filePool->unloadOrphaned();

    auto fSetting = FileSetting::getInstance(emulator, _underscore(media->name) );
    fSetting->init(false);

    auto emuView = EmuConfigView::TabWindow::getView( emulator );

    if (emuView && emuView->mediaLayout) {
        emuView->mediaLayout->ejectImage( media );
    }
}

auto Fileloader::insertFile( Emulator::Interface* emulator, Emulator::Interface::Media* media, std::string filePath, uint8_t autoLoad, unsigned selection ) -> bool {

    auto settings = program->getSettings( emulator );
    auto emuView = EmuConfigView::TabWindow::getView( emulator );

    GUIKIT::File* file = filePool->get(filePath);
    if (!file)
        return false;

    auto folderPath = GUIKIT::File::buildRelativePath(file->getPath());
    settings->set<std::string>(_underscoreEx(media->group->name) + "_folder_auto", folderPath);

    if (!file->exists())
        return program->errorMediumSize(file, emuView ? emuView->message : view->message), false;
    if (!media->group->isHardDisk() && !file->isSizeValid(MAX_MEDIUM_SIZE))
        return program->errorMediumSize(file, emuView ? emuView->message : view->message), false;
    if (media->group->isHardDisk() && !file->isSizeValid(MAX_HARDDISK_SIZE))
        return program->errorMediumSize(file, emuView ? emuView->message : view->message), false;

    auto& items = file->scanArchive();

    archiveViewer->onCallback = [this, media, emulator, autoLoad, selection, settings](GUIKIT::File* file, GUIKIT::File::Item* item) {

        auto emuView = EmuConfigView::TabWindow::getView( emulator );

        if (!item || (item->info.size == 0) )
            return program->errorOpen( file, item, emuView ? emuView->message : view->message );

        emuThread->lock();
        if (emuView && emuView->mediaLayout)
            emuView->mediaLayout->insertImage(media, file, item);
        else
            insertImage( emulator, media, file, item );

        if (media->group->isDrive()) {
            autoloader->setOnlyForFirstDrive(emulator, media);
            settings->set<int>("swap_pos", -1, false);
        }

        if (autoLoad & 1) {
            autoload(emulator, media, selection, autoLoad & USE_TRAPS, autoLoad & OVERRIDE_SPEEDER);
        }
        emuThread->unlock();
    };
    archiveViewer->allowNativeArchive(dynamic_cast<LIBAMI::Interface*>(emulator) ? media->group : nullptr);
    archiveViewer->setView(file, items);

    return true;
}

auto Fileloader::autoload(Emulator::Interface* emulator, Emulator::Interface::Media* media, unsigned selection, bool trapped, bool forceOverrideSpeeder) -> void {
    auto mediaGroup = media->group;
    auto settings = program->getSettings( emulator );
    auto emuView = EmuConfigView::TabWindow::getView( emulator );

    autoloader->set(emulator, media, trapped, selection);

    if (mediaGroup->isDrive()) {
        autoloader->activateDrive( emulator, mediaGroup, 1 );
    }

    if (mediaGroup->isExpansion()) {
        settings->set<unsigned>("expansion", mediaGroup->expansion->id);
        if (emuView && emuView->systemLayout)
            emuView->systemLayout->setExpansion( mediaGroup->expansion );
    }

    program->power( emulator );

    if (!mediaGroup->isExpansion())
        program->removeExpansion();
    else if (statusHandler && activeEmulator->isExpansionUnsupported())
        statusHandler->setMessage(trans->getA("unsupported cartridge"), 3, true);

    bool forceStandardKernal = false;
    if (media->group->isTape()) {
        forceStandardKernal = settings->get<bool>("autostart_tape_standard_kernal", false);
    }

    auto useExpansion = emulator->getExpansion();

    if (!dynamic_cast<LIBC64::Interface*>(emulator) )
        trapped = false;
    else if (trapped && (useExpansion && !useExpansion->isEmpty() && !useExpansion->isFastloader() && !useExpansion->isRS232() && !useExpansion->isTurboCart()))
        trapped = false;

    bool trapsWithSpeeder = trapped && mediaGroup->isDisk() && settings->get<bool>("autostart_speeder_traps", false);
    uint8_t options = (uint8_t)trapped;
    if (forceOverrideSpeeder) options |= 0x80;
    else if (trapped) {
        if (!trapsWithSpeeder)  options |= 0x80;
        else                    options |= 2;
    }

    if (forceStandardKernal || trapped || forceOverrideSpeeder) {
        auto fManager = FirmwareManager::getInstance( emulator );
        if (fManager->getStoreLevelInUse() > 0)
            fManager->insertDefault( trapsWithSpeeder );
    }

    if (mediaGroup->selected)
        emulator->selectListing(mediaGroup->selected, selection, "", options);
    else
        emulator->selectListing(media, selection, "", options);

    if (audioManager)
        audioManager->drive.reset(mediaGroup, true);

    if (emuView) {
        auto fSetting = FileSetting::getInstance(emulator, _underscore(media->name) );
        if (fSetting)
            program->updateSaveIdent(emulator, fSetting);
    }

    if (mediaGroup->isTape())
        view->updateTapeIcons(Emulator::Interface::TapeMode::Play);

    view->setFocused(100);

    if (mediaGroup->isDrive()) {
        program->initAutoWarp(mediaGroup);
        settings->set<int>("swap_pos", -1, false);
    }
}

auto Fileloader::updateFileSetting(FileSetting* fSetting, GUIKIT::File* file, GUIKIT::File::Item* item ) -> void {
    auto path = GUIKIT::File::buildRelativePath(file->getFile());

    fSetting->setPath(path, !cmd->autoload);
    fSetting->setFile(item->info.name, !cmd->autoload);
    fSetting->setId(item->id, !cmd->autoload);
}

auto Fileloader::insertImage(Emulator::Interface* emulator, Emulator::Interface::Media* media, GUIKIT::File* file, GUIKIT::File::Item* item, int options) -> void {
    bool fromState = options & 1;
    bool dontUpdateSelected = options & 2;

    auto mediaGroup = media->group;

    auto settings = program->getSettings( emulator );

    auto fSetting = FileSetting::getInstance( emulator, _underscore(media->name ) );

    unsigned size = file->archiveDataSize(item->id);

    auto data = (mediaGroup->isTape() || mediaGroup->isHardDisk()) && !file->isArchived() ? nullptr : file->archiveData(item->id);

    if (!mediaGroup->isExpansion() || media->secondary) {
        emulator->ejectMedium(media);
        updateFileSetting(fSetting, file, item);
        media->guid = uintptr_t(file);

        emulator->insertMedium(media, data, size);
        emulator->writeProtect(media, fSetting->writeProtect);
        if (!mediaGroup->isProgram())
            filePool->assign(_ident(emulator, media->name), file);

        if (mediaGroup->isHardDisk() && mediaGroup->expansion->pcbs.size())
            settings->set<unsigned>(_underscore(media->name) + "_pcb", 0);
    } else {

        if (mediaGroup->expansion->pcbs.size())
            settings->set<unsigned>( _underscore(media->name) + "_pcb", 0);

        updateFileSetting(fSetting, file, item);
    }

    auto recentFile = fileloader->getRecentFile(emulator);
    recentFile->add(*mediaGroup, GUIKIT::File::buildRelativePath(file->getFile()));

    emulator->getListing(media);

    if (!fromState && view && activeEmulator && mediaGroup->isTape())
        view->updateTapeIcons();

    if (!dontUpdateSelected && mediaGroup->selected && !media->secondary ) {
        mediaGroup->selected = media;
        settings->set<unsigned>(_underscore(mediaGroup->name) + "_selected", media->id);
    }

    if (!mediaGroup->isProgram())
        filePool->assign(_ident(emulator, media->name + "store"), file);

    filePool->unloadOrphaned();

    if (!cmd->noGui) {
        if (!mediaGroup->isExpansion())
            States::getInstance(emulator)->updateImage(fSetting, media);
        else
            States::getInstance(emulator)->forcePowerNextLoad = true;

        if (!fromState && mediaGroup->isDrive() && fSetting)
            program->updateSaveIdent( emulator, fSetting );
    }
}

auto Fileloader::convertListing( Emulator::Interface* emulator, std::vector<Emulator::Interface::Listing>& emuListings ) -> std::vector<GUIKIT::BrowserWindow::Listing> {

    std::vector<GUIKIT::BrowserWindow::Listing> list;
    auto customFont = GUIKIT::Window::getCustomFont(emulator);
    auto settings = program->getSettings( emulator );

    bool useTooltips = settings->get<bool>("software_preview_tooltips", true );

    for (auto& listing : emuListings) {

        GUIKIT::BrowserWindow::Listing browserListing;

        std::vector<uint8_t> utf8;

        for (auto& code : listing.line ) {

            unsigned useCode = code;
            if (customFont)
                useCode |= customFont->modifier;

            GUIKIT::Utf8::encode(useCode, utf8);
        }

        browserListing.entry = std::string((const char*) utf8.data(), utf8.size());

        if (useTooltips) {
            utf8.clear();

            for (auto& code : listing.loadCommand ) {

                unsigned useCode = code;
                if (customFont)
                    useCode |= customFont->modifier;

                GUIKIT::Utf8::encode(useCode, utf8);
            }

            browserListing.tooltip = std::string((const char*) utf8.data(), utf8.size());
        }

        list.push_back( browserListing );
    }

    return list;
}

auto Fileloader::resetPreview(Emulator::Interface* emulator, bool light) -> void {
    auto emuView = EmuConfigView::TabWindow::getView( emulator );

    if (emuView && emuView->mediaLayout)
        emuView->mediaLayout->resetPreview(light);

    queuePreview.lastMedia = nullptr;
    queuePreview.status = 0;
    previewTimer.setEnabled(false);
}

auto Fileloader::insertCurrentPreview(Emulator::Interface::MediaGroup* mediaGroup) -> void {
    if (fileDialogPtr && fileDialogPtr->visible()) {
        fileDialogPtr->close();
        delete fileDialogPtr;
        fileDialogPtr = nullptr;

        if (queuePreview.lastMedia && queuePreview.lastMedia->group == mediaGroup) {
            emuThread->lock(); // important
            insertFile(queuePreview.emulator, queuePreview.lastMedia, queuePreview.filePath);
        }
    }
}

auto Fileloader::preselectPath( GUIKIT::Settings* settings, std::string& groupName, bool lastPathFirst ) -> std::string {
    std::string baseFolderIdent = _underscoreEx(groupName) + "_folder";
    std::string path;

    for(int i = 0; i < 2; i++) {
        std::string useIdent = baseFolderIdent;
        if (lastPathFirst)
            useIdent += "_auto";

        lastPathFirst ^= 1;

        path = settings->get<std::string>( useIdent, "" );
        path = GUIKIT::File::resolveRelativePath(path);

        if ( !path.empty() )
            break;
    }

    return path;
}

auto Fileloader::loadSettings(Emulator::Interface* emulator) -> void {

    for(auto& group : emulator->mediaGroups) {
        for(auto& media : group.media) {
            auto fSetting = FileSetting::getInstance(emulator, _underscore(media.name) );
            fSetting->update();
        }
    }

    // swapper
    for (unsigned i = 1; i < SWAPPER_SLOTS; i++) {
        auto fSetting = FileSetting::getInstance( emulator, "swapper_" + std::to_string( i ) );
        fSetting->update();
    }
}

auto Fileloader::getSwapPos(Emulator::Interface* emulator) -> unsigned {
    auto settings = program->getSettings( emulator );
    Emulator::Interface::Media* media = autoloader->getLatestDrive(emulator);
    if (media->group->isDisk()) {
        auto mediaId = settings->get<unsigned>("access_floppy", 0u, {0u, 3u});
        auto media2 = emulator->getEnabledDisk(mediaId);
        if (media2)
            media = media2;
    }

    auto fSetting = FileSetting::getInstance( activeEmulator, _underscore(media->name ) );
    if (fSetting->path.empty())
        return 0;

    DiskFinder diskFinder(fSetting->path);
    return diskFinder.getDiskPos();
}

auto Fileloader::getSwapMedia(Emulator::Interface* emulator, int swapPos, FileSetting* fSetting) -> Emulator::Interface::Media* {
    auto settings = program->getSettings( emulator );
    Emulator::Interface::Media* media = nullptr;

    if (fSetting->path.empty() || (swapPos == 0) ) {
        media = autoloader->getLatestDrive(emulator);
    } else {
        std::string fileSuffix = GUIKIT::String::getExtension(fSetting->file, "exe");
        GUIKIT::String::toLowerCase( fileSuffix );

        auto driveGroups = emulator->getDriveMediaGroups();
        for (auto group: driveGroups) {
            for (auto& suffix: group->suffix) {
                if (suffix == fileSuffix) {
                    media = &group->media[0];
                    break;
                }
            }
            if (media)
                break;
        }
    }

    if (media && media->group->isDisk()) {
        auto mediaId = settings->get<unsigned>("access_floppy", 0u, {0u, 3u});
        auto media2 = emulator->getEnabledDisk(mediaId);
        if (media2)
            media = media2;
    }

    return media;
}

auto Fileloader::insertSwapDisk(Emulator::Interface* emulator, unsigned swapPos) -> Emulator::Interface::Media* {
    GUIKIT::File* file;
    program->getSettings( emulator )->set<int>("swap_pos", swapPos, false);
    FileSetting* fSetting = FileSetting::getInstance( emulator, "swapper_" + std::to_string(swapPos) );
    Emulator::Interface::Media* media = getSwapMedia(emulator, swapPos, fSetting);
    if (!media)
        return nullptr;

    FileSetting fs;
    if (fSetting->path.empty() || (swapPos == 0) ) {
        fSetting = &fs;
        // auto create
        auto srcSetting = FileSetting::getInstance(emulator, _underscore(media->name) );

        if (srcSetting->path.empty() && (&media->group->media[0] != media) ) {
            srcSetting = FileSetting::getInstance(emulator, _underscore(media->group->media[0].name) );
            if (srcSetting->path.empty())
                return nullptr;
        }

        if (srcSetting->path.empty())
            return nullptr;

        DiskFinder diskFinder( srcSetting->path );

        auto result = diskFinder.findNext( swapPos );

        if (result != "") {
            fSetting->file = result;
            fSetting->path = diskFinder.filePath + result;
            fSetting->id = 0;
            fSetting->writeProtect = false;
        }
    }

    file = filePool->get(GUIKIT::File::resolveRelativePath(fSetting->path));

    GUIKIT::File::Item item;
    item.id = fSetting->id;
    item.info.name = fSetting->file;

    if (!file || !file->exists() || !file->isSizeValid(MAX_MEDIUM_SIZE) ||
        (file->archiveData(fSetting->id) == nullptr)
            ) {
        statusHandler->setMessage(trans->get("file_open_error", {{ "%path%", fSetting->file }}), 2, true);
        return nullptr;
    }

    filePool->assign( _ident(emulator, "swapper_" + std::to_string(swapPos)), file);

    auto fSetting2 = FileSetting::getInstance( emulator, _underscore(media->name ) );
    fSetting2->setWriteProtect(fSetting->writeProtect);

    auto emuView = EmuConfigView::TabWindow::getView( emulator );
    if (emuView && emuView->mediaLayout)
        emuView->mediaLayout->insertImage( media, file, &item );
    else
        fileloader->insertImage( emulator, media, file, &item );

    autoloader->setOnlyForFirstDrive(emulator, media);

    statusHandler->setMessage( trans->get("insert drive", {{"%drive%", media->name},{"%file%", fSetting->file}}) );

    return media;
}

auto Fileloader::getRecentFile(Emulator::Interface* emulator) -> RecentFiles* {
    for (auto recentFile : recentFiles) {
        if (recentFile->emulator == emulator)
            return recentFile;
    }

    auto _ident = emulator->ident;
    GUIKIT::String::toLowerCase(_ident);

    std::string path = program->generatedFolder("") + _ident + "_recent.ini";

    auto recentFile = new RecentFiles(emulator, path);
    recentFiles.push_back(recentFile);
    return recentFile;
}
