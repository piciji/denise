
#include "configurations.h"
#include "../config.h"
#include "../../../data/icons.h"
#include "../../view/view.h"
#include "../../thread/emuThread.h"
#include "presentation.h"
#include "../../video/palette.h"
#include "audio.h"
#include "../../cmd/cmd.h"
#include "../../view/status.h"
#include "../../states/states.h"
#include "../../audio/manager.h"
#include "../../firmware/manager.h"
#include "../../media/fileloader.h"
#include "../../media/autoloader.h"
#include "../../helper/fileHelper.h"
#include "firmware.h"
#include "geometry.h"
#include "input.h"
#include "misc.h"
#include "palette.h"
#include "system.h"

#define mes this->tabWindow->message
#define _settings this->tabWindow->settings

namespace EmuConfigView {

MemoryPatternLayout::FirstLine::FirstLine() {
    append( valueLabel, {0u, 0u}, 10 );
    append( valueStepper, {0u, 0u}, 10 );
    append( invertValueEveryLabel, {0u, 0u}, 10 );
    append( invertValueEveryCombo, {0u, 0u} );
    
    valueStepper.setRange(0, 0xff);
    
    invertValueEveryCombo.append( "0 bytes", 0 );
    invertValueEveryCombo.append( "1 byte", 1 );       
    
    unsigned i = 2;
    while(i < (64 * 1024)) {
        invertValueEveryCombo.append( std::to_string(i) + " bytes", i );
        i <<= 1;
    }
    
    setAlignment(0.5);
}

MemoryPatternLayout::SecondLine::SecondLine() {
    append( valueLabel, {0u, 0u}, 10 );
    append( valueStepper, {0u, 0u}, 10 );
    append( invertValueEveryLabel, {0u, 0u}, 10 );
    append( invertValueEveryCombo, {0u, 0u} );

    valueStepper.setRange(0, 0xff);

    invertValueEveryCombo.append( "0 bytes", 0 );
    invertValueEveryCombo.append( "1 byte", 1 );

    unsigned i = 2;
    while(i < (64 * 1024)) {
        invertValueEveryCombo.append( std::to_string(i) + " bytes", i );
        i <<= 1;
    }

    setAlignment(0.5);
}

MemoryPatternLayout::ThirdLine::ThirdLine() {
    
    append( lengthRandomLabel, {0u, 0u}, 10 );
    append( lengthRandomCombo, {0u, 0u}, 10 );
    append( repeatRandomEveryLabel, {0u, 0u}, 10 );
    append( repeatRandomEveryCombo, {0u, 0u} );

    lengthRandomCombo.append("0 bytes", 0);
    lengthRandomCombo.append("1 byte", 1);
    repeatRandomEveryCombo.append("0 bytes", 0);
    repeatRandomEveryCombo.append("1 byte", 1);
    
    unsigned i = 2;
    while (i < (64 * 1024)) {
        lengthRandomCombo.append(std::to_string(i) + " bytes", i);
        repeatRandomEveryCombo.append(std::to_string(i) + " bytes", i);

        i <<= 1;
    }
    
    setAlignment(0.5);
}

MemoryPatternLayout::FourthLine::FourthLine() {
    
    append( randomChanceLabel, {0u, 0u}, 10 );
    append( randomChanceStepper, {0u, 0u}, 10 );
    append( offsetLabel, {0u, 0u}, 10 );
    append( offsetCombo, {0u, 0u} );
    
    randomChanceStepper.setRange(0, 1000);

    offsetCombo.append( "0 bytes", 0 );
    offsetCombo.append( "1 byte", 1 );

    unsigned i = 2;
    while(i < (64 * 1024)) {
        offsetCombo.append( std::to_string(i) + " bytes", i );
        i <<= 1;
    }
    
    setAlignment(0.5);
}

MemoryPatternLayout::FifthLine::FifthLine() {

    append( preConfigured1, {0u, 0u}, 10 );
    append( preConfigured2, {0u, 0u}, 10 );
    append( preConfigured3, {0u, 0u }, 10);
    append( preConfigured4, {0u, 0u} );

    setAlignment(0.5);
}

MemoryPatternLayout::MemoryPatternLayout(TabWindow* tabWindow) {
    
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
    GUIKIT::Label test;
    test.setFont( GUIKIT::Font::system("bold", true) );
    test.setText( "0000: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 " );
    auto size = test.minimumSize();

    append( firstLine, {0u, 0u}, 10 );
    append( secondLine, {0u, 0u}, 10 );
    append( thirdLine, {0u, 0u}, 10 );
    append( fourthLine, {0u, 0u}, 10 );
    append( preview, {size.width + tabWindow->getScrollbarWidth(), size.height * 17}, 10 );
    append( fifthLine, {0u, 0u} );

    preview.setFont( GUIKIT::Font::system("", true) );
    preview.setForegroundColor( 0x666666 );
    preview.setEditable(false);
}

SettingsLayout::Control::Control() {
    append( load, {0u, 0u}, 10 );
    append( save, {0u, 0u}, 10 );
    append( remove, {0u, 0u}, 10 );
    append( create, {0u, 0u}, 10);
    append( edit, {~0u, 0u}, 10 );
    append( clear, { 0u, 0u }, 10);
    append( search, { 0u, 0u });

    load.setEnabled(false);
    remove.setEnabled(false);

    setAlignment(0.5);
}

SettingsLayout::Active::Active() {
    append(activeLabel,{0u, 0u}, 10);
    append(fileLabel,{~0u, 0u});
    append(undockButton,{0u, 0u}, 10);
    append(standardButton,{0u, 0u});

    fileLabel.setFont(GUIKIT::Font::system("bold"));

    setAlignment(0.5);
}

SettingsLayout::Options::Options(Emulator::Interface* emulator) {
    append(startWithLastConfigCheckbox, {0u, 0u}, 15 );
    if(dynamic_cast<LIBC64::Interface*>(emulator)) {
        append(labelAutoStart, {0u, 0u}, 5 );
        append(boxDefault, {0u, 0u}, 5 );
        append(boxDisk, {0u, 0u}, 5 );
        append(boxTape, {0u, 0u}, 5 );
        append(boxPrg, {0u, 0u} );

        GUIKIT::RadioBox::setGroup( boxDefault, boxDisk, boxTape, boxPrg );
    }
    setAlignment( 0.5 );
}

SettingsLayout::SettingsLayout(Emulator::Interface* emulator)
: options( emulator ) {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));

    append(control, {~0u, 0u}, 5);
    append(active, {~0u, 0u}, 5);
    append(options,{~0u, 0u}, 5);
    append(treeView, { ~0u, ~0u }, 5);
}

ConfigurationsFolderLayout::ConfigurationsFolderLayout() {
    append( label, {0u, 0u}, 10 );
    append( pathEdit, {~0u, 0u}, 10 );
    append( standard, {0u, 0u}, 10 );
    append( select, {0u, 0u} );

    pathEdit.setEditable( false );

    label.setFont(GUIKIT::Font::system("bold"));
    setAlignment(0.5);
}

StateFastLayout::Top::Top() {
    append(label,{0u, 0u}, 10);
    append(edit,{~0u, 0u}, 10);
    append(find,{0u, 0u}, 10);
    append(hotkeys,{0u, 0u});
    setAlignment(0.5);
}

StateFastLayout::Options::Options() {
    append(autoLabel,{0u, 0u}, 5);
    append(autoIdentOff,{0u, 0u}, 5);
    append(autoIdentOn,{0u, 0u}, 5);
    append(autoIdentCutFollowUp,{0u, 0u});
    append(spacer, {~0u, 0u});
    append(screenshot, { 0u, 0u });

    GUIKIT::RadioBox::setGroup(autoIdentOff, autoIdentOn, autoIdentCutFollowUp);
    setAlignment(0.5);
}

StateFastLayout::Selector::Selector() {
    listView.setHeaderVisible();
    listView.setHeaderText({ "", "", "" });

    append(listView, { ~0u, ~0u }, 10);
    append(preview, { 200u, 150u });
}

StateFastLayout::StateFastLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));

    append(top,{~0u, 0u}, 5);
    append(options,{~0u, 0u}, 5);
    append(selector, { ~0u, ~0u }, 5);
}

StateDirectLayout::StateDirectLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));

    append(load,{0u, 0u}, 20);
    append(save,{0u, 0u});
    setAlignment(0.5);
}

ConfigurationsLayout::ConfigurationsLayout(TabWindow* tabWindow)
: settings( tabWindow->emulator ) {

    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;

    setMargin(10);
    if(dynamic_cast<LIBC64::Interface*>(emulator))
        memoryPattern = new MemoryPatternLayout(tabWindow);

    moduleList.setHeaderText( { "" } );
    moduleList.setHeaderVisible( false );
    moduleList.append( {"settings"} );
    moduleList.append( {"states"} );
    if (memoryPattern)
        moduleList.append( {"memory"} );

    imgFolderOpen.loadPng((uint8_t*)Icons::folderOpen, sizeof(Icons::folderOpen));
    imgFolderClosed.loadPng((uint8_t*)Icons::folderClosed, sizeof(Icons::folderClosed));
    imgDocument.loadPng((uint8_t*)Icons::document, sizeof(Icons::document));

    settingsImage.loadPng((uint8_t*)Icons::settings, sizeof(Icons::settings));
    scriptImage.loadPng((uint8_t*)Icons::script, sizeof(Icons::script));
    if (memoryPattern)
        memImage.loadPng((uint8_t*)Icons::memory, sizeof(Icons::memory));

    searchImage.loadPng((uint8_t*)Icons::search, sizeof(Icons::search));
    clearImage.loadPng((uint8_t*)Icons::clear, sizeof(Icons::clear));
    addImage.loadPng((uint8_t*)Icons::add, sizeof(Icons::add));
    delImage.loadPng((uint8_t*)Icons::del, sizeof(Icons::del));
    saveImage.loadPng((uint8_t*)Icons::disk, sizeof(Icons::disk));
    openImage.loadPng((uint8_t*)Icons::open, sizeof(Icons::open));

    settings.control.load.setImage(&scriptImage);
    settings.control.save.setImage(&saveImage);
    settings.control.create.setImage(&addImage);
    settings.control.remove.setImage(&delImage);
    settings.control.clear.setImage(&clearImage);
    settings.control.search.setImage(&searchImage);

    settingsFolder.select.setImage(&openImage);
    stateFolder.select.setImage(&openImage);

    moduleList.setImage(0, 0, settingsImage);
    moduleList.setImage(1, 0, scriptImage);
    if (memoryPattern)
        moduleList.setImage(2, 0, memImage);

    moduleList.setSelection(0);
    moduleFrame.append( moduleList, { GUIKIT::Font::scale(140), GUIKIT::Font::scale(100)} );
    moduleFrame.setPadding(10);
    moduleFrame.setFont( GUIKIT::Font::system("bold") );

    moduleList.onChange = [this]() {

        if (!moduleList.selected())
            return;

        auto _sel = moduleList.selection();

        moduleSwitch.setSelection( _sel );
        if (memoryPattern && _sel == 2) {
            GUIKIT::HorizontalLayout::alignChildrenVertically( {&memoryPattern->firstLine, &memoryPattern->secondLine, &memoryPattern->thirdLine} );
            memoryPattern->updateLayout();
        }
    };

    append( moduleFrame, {0u, 0u}, 10 );

    settingsFrame.append( settings, {~0u, ~0u}, 5 );
    settingsFrame.append( settingsFolder, {~0u, 0u} );

    statesFrame.append( stateFast, {~0u, ~0u}, 5 );
    statesFrame.append( stateDirect, {~0u, 0u}, 5 );
    statesFrame.append( stateFolder, {~0u, 0u} );

    append( moduleSwitch, {~0u, ~0u} );
    moduleSwitch.setLayout( 0, settingsFrame, {~0u, ~0u} );
    moduleSwitch.setLayout( 1, statesFrame, {~0u, ~0u} );
    if(memoryPattern)
        moduleSwitch.setLayout( 2, *memoryPattern, {~0u, ~0u} );

    if(memoryPattern) {

        memoryPattern->preview.onChange = [this]() {
            //   mes->warning( "on change" );
        };

        memoryPattern->preview.onFocus = [this]() {
            //	mes->warning( "on focus" );
        };

        memoryPattern->firstLine.valueStepper.onChange = [this]() {

            _settings->set<unsigned>("memory_value", (unsigned)(memoryPattern->firstLine.valueStepper.getValue()));

            this->updateMemoryPreview();
        };

        memoryPattern->firstLine.invertValueEveryCombo.onChange = [this]() {

            _settings->set<unsigned>("memory_invert_every", (unsigned)(memoryPattern->firstLine.invertValueEveryCombo.userData()));

            this->updateMemoryPreview();
        };

        memoryPattern->secondLine.valueStepper.onChange = [this]() {

            _settings->set<unsigned>("memory_second_value", (unsigned)(memoryPattern->secondLine.valueStepper.getValue()));

            this->updateMemoryPreview();
        };

        memoryPattern->secondLine.invertValueEveryCombo.onChange = [this]() {

            _settings->set<unsigned>("memory_second_invert_every", (unsigned)(memoryPattern->secondLine.invertValueEveryCombo.userData()));

            this->updateMemoryPreview();
        };

        memoryPattern->thirdLine.lengthRandomCombo.onChange = [this]() {

            _settings->set<unsigned>("memory_random_pattern", (unsigned)(memoryPattern->thirdLine.lengthRandomCombo.userData()));

            this->updateMemoryPreview();
        };

        memoryPattern->thirdLine.repeatRandomEveryCombo.onChange = [this]() {

            _settings->set<unsigned>("memory_random_repeat", (unsigned) (memoryPattern->thirdLine.repeatRandomEveryCombo.userData()));

            this->updateMemoryPreview();
        };

        memoryPattern->fourthLine.randomChanceStepper.onChange = [this]() {

            _settings->set<unsigned>("random_chance", (unsigned) (memoryPattern->fourthLine.randomChanceStepper.getValue()));

            this->updateMemoryPreview();
        };

        memoryPattern->fourthLine.offsetCombo.onChange = [this]() {

            _settings->set<unsigned>("memory_offset", (unsigned) (memoryPattern->fourthLine.offsetCombo.userData()));

            this->updateMemoryPreview();
        };

        memoryPattern->fifthLine.preConfigured1.onActivate = [this]() {
            memoryPattern->firstLine.valueStepper.setValue(0);
            memoryPattern->firstLine.invertValueEveryCombo.setSelectionByUserId(64);
            memoryPattern->secondLine.valueStepper.setValue(0);
            memoryPattern->secondLine.invertValueEveryCombo.setSelectionByUserId(0);
            memoryPattern->thirdLine.lengthRandomCombo.setSelectionByUserId(0);
            memoryPattern->thirdLine.repeatRandomEveryCombo.setSelectionByUserId(0);
            memoryPattern->fourthLine.randomChanceStepper.setValue(0);
            memoryPattern->fourthLine.offsetCombo.setSelectionByUserId(0);

            _settings->set<unsigned>("memory_value", 0);
            _settings->set<unsigned>("memory_invert_every", 64);
            _settings->set<unsigned>("memory_second_value", 0);
            _settings->set<unsigned>("memory_second_invert_every", 0);
            _settings->set<unsigned>("memory_random_pattern", 0);
            _settings->set<unsigned>("memory_random_repeat", 0);
            _settings->set<unsigned>("random_chance", 0);
            _settings->set<unsigned>("memory_offset", 0);

            this->updateMemoryPreview();
        };

        memoryPattern->fifthLine.preConfigured2.onActivate = [this]() {
            memoryPattern->firstLine.valueStepper.setValue(256);
            memoryPattern->firstLine.invertValueEveryCombo.setSelectionByUserId(64);
            memoryPattern->secondLine.valueStepper.setValue(0);
            memoryPattern->secondLine.invertValueEveryCombo.setSelectionByUserId(0);
            memoryPattern->thirdLine.lengthRandomCombo.setSelectionByUserId(1);
            memoryPattern->thirdLine.repeatRandomEveryCombo.setSelectionByUserId(256);
            memoryPattern->fourthLine.randomChanceStepper.setValue(0);
            memoryPattern->fourthLine.offsetCombo.setSelectionByUserId(0);

            _settings->set<unsigned>("memory_value", 255);
            _settings->set<unsigned>("memory_invert_every", 64);
            _settings->set<unsigned>("memory_second_value", 0);
            _settings->set<unsigned>("memory_second_invert_every", 0);
            _settings->set<unsigned>("memory_random_pattern", 1);
            _settings->set<unsigned>("memory_random_repeat", 256);
            _settings->set<unsigned>("random_chance", 0);
            _settings->set<unsigned>("memory_offset", 0);

            this->updateMemoryPreview();
        };

        memoryPattern->fifthLine.preConfigured3.onActivate = [this]() {
            memoryPattern->firstLine.valueStepper.setValue(0);
            memoryPattern->firstLine.invertValueEveryCombo.setSelectionByUserId(4);
            memoryPattern->secondLine.valueStepper.setValue(255);
            memoryPattern->secondLine.invertValueEveryCombo.setSelectionByUserId(16384);
            memoryPattern->thirdLine.lengthRandomCombo.setSelectionByUserId(0);
            memoryPattern->thirdLine.repeatRandomEveryCombo.setSelectionByUserId(0);
            memoryPattern->fourthLine.randomChanceStepper.setValue(1);
            memoryPattern->fourthLine.offsetCombo.setSelectionByUserId(2);

            _settings->set<unsigned>("memory_value", 0);
            _settings->set<unsigned>("memory_invert_every", 4);
            _settings->set<unsigned>("memory_second_value", 255);
            _settings->set<unsigned>("memory_second_invert_every", 16384);
            _settings->set<unsigned>("memory_random_pattern", 0);
            _settings->set<unsigned>("memory_random_repeat", 0);
            _settings->set<unsigned>("random_chance", 1);
            _settings->set<unsigned>("memory_offset", 2);

            this->updateMemoryPreview();
        };

        memoryPattern->fifthLine.preConfigured4.onActivate = [this]() {
            memoryPattern->firstLine.valueStepper.setValue(255);
            memoryPattern->firstLine.invertValueEveryCombo.setSelectionByUserId(2);
            memoryPattern->secondLine.valueStepper.setValue(128);
            memoryPattern->secondLine.invertValueEveryCombo.setSelectionByUserId(1);
            memoryPattern->thirdLine.lengthRandomCombo.setSelectionByUserId(0);
            memoryPattern->thirdLine.repeatRandomEveryCombo.setSelectionByUserId(0);
            memoryPattern->fourthLine.randomChanceStepper.setValue(0);
            memoryPattern->fourthLine.offsetCombo.setSelectionByUserId(0);

            _settings->set<unsigned>("memory_value", 255);
            _settings->set<unsigned>("memory_invert_every", 2);
            _settings->set<unsigned>("memory_second_value", 128);
            _settings->set<unsigned>("memory_second_invert_every", 1);
            _settings->set<unsigned>("memory_random_pattern", 0);
            _settings->set<unsigned>("memory_random_repeat", 0);
            _settings->set<unsigned>("random_chance", 0);
            _settings->set<unsigned>("memory_offset", 0);

            this->updateMemoryPreview();
        };
    }
    settings.treeView.onChange = [this](GUIKIT::TreeViewItem* selectedBefore) {
        if (!settings.control.load.enabled())
            settings.control.load.setEnabled();

        GUIKIT::File::Info* info;
        GUIKIT::TreeViewItem* selected = settings.treeView.selected();
        if (selected) {
            info = (GUIKIT::File::Info*)selected->userData();
            if (info) {
                if (!info->isDir)
                    selected->setText(GUIKIT::String::getFileName(info->name) + " -> " + info->date);

                settings.control.remove.setEnabled(!info->isDir);
            }
        }

        if (selectedBefore) {
            info = (GUIKIT::File::Info*)selectedBefore->userData();
            if (info && !info->isDir)
                selectedBefore->setText(GUIKIT::String::getFileName(info->name));
        }
    };

    settings.options.startWithLastConfigCheckbox.onToggle = [this](bool checked) {
        globalSettings->set<bool>( this->emulator->ident + "_load_last_settings", checked );
    };

    settings.options.startWithLastConfigCheckbox.setChecked( globalSettings->get<bool>( this->emulator->ident + "_load_last_settings", false ) );

    settings.options.boxDefault.onActivate = [this]() {
        _settings->set<unsigned>("config_autostart", 0);
    };
    settings.options.boxDisk.onActivate = [this]() {
        _settings->set<unsigned>("config_autostart", 1);
    };
    settings.options.boxTape.onActivate = [this]() {
        _settings->set<unsigned>("config_autostart", 2);
    };
    settings.options.boxPrg.onActivate = [this]() {
        _settings->set<unsigned>("config_autostart", 3);
    };

    settings.treeView.onActivate = [this]() {

        settings.control.load.onActivate();
    };

    if (program->portable || GUIKIT::Application::isGtk())
        settings.active.remove( settings.active.undockButton );

    settings.active.undockButton.onActivate = [this]() {

        if (mes->question( trans->get("undock settings") ) ) {
            emuThread->lock();
            SettingsHelper::undockSettings();
            PresentationLayout::displayFonts.clear();

            for(auto _emulator : emulators) {
                PaletteManager* paletteManager = PaletteManager::getInstance(_emulator);
                if (paletteManager)
                    paletteManager->removeEditablePalettes();
            }
            for (auto view : emuConfigViews) {
                if (view->configurationsLayout) {
                    view->configurationsLayout->settings.active.remove(view->configurationsLayout->settings.active.undockButton);
                    view->configurationsLayout->updateSettingsList();
                    view->configurationsLayout->updateStorePaths();
                }

                if(view->audioLayout)
                    view->audioLayout->updateRecordingPath();

                if (view->paletteLayout)
                    view->paletteLayout->loadSettings();
                if (view->presentationLayout) {
                    view->presentationLayout->fillFontTypeList();
                    view->presentationLayout->updateRecordingPath();
                }
            }
            emuThread->unlock();
        }
    };

    settings.active.standardButton.onActivate = [this]() {

        if (!cmd->hasCustomConfig(emulator) && ("" == globalSettings->get<std::string>( emulator->ident + "_custom_settings", "")))
            return;

        emuThread->lock(true);

        if (!this->load(program->settingsFileFromEmuFolder(emulator->ident + "_"), false)) {
            if (!this->load(program->settingsFile(emulator->ident + "_"))) {
                emuThread->unlock();
                return;
            }
        }

        globalSettings->set<std::string>(emulator->ident + "_custom_settings", "");
        cmd->removeCustomConfig(emulator);
        settings.active.fileLabel.setText( trans->get("default") );

        emuThread->unlock();
    };

    settings.control.load.onActivate = [this]() {
        auto& _tree = settings.treeView;
        GUIKIT::TreeViewItem* selected = _tree.selected();
        if (!selected)
            return;

        GUIKIT::File::Info* info = (GUIKIT::File::Info*)selected->userData();
        if (info->isDir)
            return;

        std::string path = FileHelper::getSettingsFolder(emulator) + info->name;

        emuThread->lock(true);
        if (this->load(path)) {
            globalSettings->set<std::string>(emulator->ident + "_custom_settings", info->name);
            cmd->removeCustomConfig(emulator);
            settings.active.fileLabel.setText(info->name);
        }
        emuThread->unlock();
    };

    settings.control.save.onActivate = [this]() {
        std::string fileName;
        GUIKIT::File::Info* info = nullptr;
        auto& _tree = settings.treeView;
        GUIKIT::TreeViewItem* selected = _tree.selected();
        if (selected) {
            info = (GUIKIT::File::Info*)selected->userData();
        }

        if (!selected || (info && info->isDir)) {
            if (cmd->hasCustomConfig(emulator))
                return;

            fileName = globalSettings->get<std::string>(emulator->ident + "_custom_settings", "");
            if (fileName.empty())
                return;
        } else {
            fileName = info->name;
        }

        std::string path = FileHelper::getSettingsFolder(emulator, true) + fileName;

        GUIKIT::File file(path);

        if (file.exists()) {
            if (!mes->question(trans->get("file_exist_error",{
                    {"%path%", path}})))
                return;
        }

        if (!_settings->save(path))
            mes->error(trans->get("file_creation_error",{
                {"%path%", path}}));
    };

    settings.control.create.onActivate = [this]() {
        const std::string basePath = FileHelper::getSettingsFolder(emulator, true);

        std::string filePath = GUIKIT::BrowserWindow()
            .setWindow(*this->tabWindow)
            .setTitle(trans->get("select setting"))
            .setPath(basePath)
            .setFilters({ trans->get("all_files") })
            .save();

        if (filePath.empty()) {
            GUIKIT::TreeViewItem* selected = settings.treeView.selected();
            if (selected) {
                GUIKIT::File::Info* info = (GUIKIT::File::Info*)selected->userData();
                if (info)
                    filePath = info->name;
            }
            updateSettingsList(filePath);
            return;
        }

        GUIKIT::File file(filePath);

        if (file.exists()) {
            if (!mes->question(trans->get("file_exist_error", { {"%path%", filePath} })))
                return;
        }

        if (_settings->save(filePath)) {
            GUIKIT::String::remove(filePath, { basePath });
            if (statusHandler)
                statusHandler->setMessage(trans->get("file_creation_success", { {"%path%", filePath} }));

            globalSettings->set(emulator->ident + "_custom_settings", filePath);
            cmd->removeCustomConfig(emulator);
            settings.active.fileLabel.setText(filePath);
            updateSettingsList(filePath);

        } else
            mes->error(trans->get("file_creation_error", { {"%path%", filePath} }));
    };

    settings.control.remove.onActivate = [this]() {
        auto& _tree = settings.treeView;
        GUIKIT::TreeViewItem* selected = _tree.selected();
        if (!selected)
            return;

        GUIKIT::File::Info* info = (GUIKIT::File::Info*)selected->userData();
        if (info->isDir)
            return;

        std::string path = FileHelper::getSettingsFolder(emulator) + info->name;

        GUIKIT::File file( path );

        if (file.exists()) {
            if (!mes->question(trans->get("file deletion confirmation",{
                    {"%path%", path}
                })))
                return;
        }

        if (!file.del())
            mes->error( trans->get("file deletion error", {{"%path%", path}}) );
        else {
            if (!cmd->hasCustomConfig(emulator) && (info->name == globalSettings->get<std::string>(emulator->ident + "_custom_settings", ""))) {
                globalSettings->set<std::string>(emulator->ident + "_custom_settings", "");

                emuThread->lock(true);

                if (!this->load(program->settingsFileFromEmuFolder(emulator->ident + "_"), false)) {
                    if (!this->load(program->settingsFile(emulator->ident + "_"))) {
                        emuThread->unlock();
                        return;
                    }
                }

                settings.active.fileLabel.setText(trans->get("default"));

                emuThread->unlock();
            }

            auto parent = selected->parentItem();
            if (parent) {
                parent->remove(*selected);
                auto info = (GUIKIT::File::Info*)selected->userData();
                parent->setSelected();
                if (info)
                    delete info;
                delete selected;
            }
        }
    };

    settings.control.search.onActivate = [this]() {
        updateSettingsList("", settings.control.edit.text());
    };

    settings.control.clear.onActivate = [this]() {
        settings.control.edit.setText("");
        updateSettingsList();
    };

    settingsFolder.select.onActivate = [this]() {

        auto path = GUIKIT::BrowserWindow()
                .setTitle(trans->get("select settings folder"))
                .setWindow(*this->tabWindow)
                .directory();

        if (path.empty())
            return;

        if (!cmd->hasCustomConfig(emulator) && (globalSettings->get<std::string>(emulator->ident + "_custom_settings", "") != ""))
            settings.active.standardButton.onActivate();

        path = GUIKIT::File::buildRelativePath(path);
        settingsFolder.pathEdit.setEnabled();
        settingsFolder.pathEdit.setText( path );

        globalSettings->set<std::string>(emulator->ident + "_settings_path", path);

        updateSettingsList();
    };

    settingsFolder.standard.onActivate = [this]() {
        if (!cmd->hasCustomConfig(emulator) && (globalSettings->get<std::string>(emulator->ident + "_custom_settings", "") != ""))
            settings.active.standardButton.onActivate();

        globalSettings->set<std::string>( emulator->ident + "_settings_path", "" );

        settingsFolder.pathEdit.setText(FileHelper::getSettingsFolder(emulator));

        settingsFolder.pathEdit.setEnabled(false);

        updateSettingsList();
    };

    if (!settings.options.startWithLastConfigCheckbox.checked() || cmd->hasCustomConfig(emulator))
        settings.active.fileLabel.setText(trans->get("default"));
    else
        settings.active.fileLabel.setText( globalSettings->get<std::string>(emulator->ident + "_custom_settings", trans->get("default")));

    // states

    stateFast.top.hotkeys.onActivate = [this]() {
        this->tabWindow->show(EmuConfigView::TabWindow::Layout::Control);
        this->tabWindow->inputLayout->triggerHotkeyMode();
    };

    stateFast.top.edit.onChange = [this]() {
        _settings->set<std::string>( "save_ident", stateFast.top.edit.text());
        _settings->set<unsigned>( "save_slot", 0);
    };

    stateFast.options.autoIdentOff.onActivate = [this]() {
        _settings->set<unsigned>( "auto_save_mode", 0);
    };

    stateFast.options.autoIdentOn.onActivate = [this]() {
        _settings->set<unsigned>( "auto_save_mode", 1);
    };

    stateFast.options.autoIdentCutFollowUp.onActivate = [this]() {
        _settings->set<unsigned>( "auto_save_mode", 2);
    };

    stateFast.options.screenshot.onToggle = [this](bool checked) {
        _settings->set<bool>("save_screenshot", checked);
        if (!checked)
            stateFast.selector.preview.setImage(nullptr);
    };

    stateFast.selector.listView.onChange = [this]() {
        if (!_settings->get<bool>("save_screenshot", true))
            return;
        auto& lview = stateFast.selector.listView;
        auto selection = lview.selection();
        auto ident = lview.text(selection, 1);
        auto path = FileHelper::generatedFolder(emulator, "states_folder", "states") + ident + ".png";

        GUIKIT::Image* image = &stateFast.selector.image;
        image->free();

        GUIKIT::File file(path);
        if (file.open()) {
            if (image->loadPng(file.read(), file.getSize())) {
                stateFast.selector.preview.setImage(image);
                return;
            }
        }
        stateFast.selector.preview.setImage(nullptr);
    };

    stateFast.selector.listView.onActivate = [this]() {
        auto& lview = stateFast.selector.listView;
        auto selection = lview.selection();
        auto pos = lview.text(selection, 0);
        _settings->set<unsigned>( "save_slot", std::stoul(pos));

        unsigned statePos = 0;
        std::string baseName = splitFile(lview.text(selection, 1), statePos);
        stateFast.top.edit.setText( baseName );
        _settings->set<std::string>("save_ident", baseName);

        emuThread->lock(true);
        States::getInstance(emulator)->load(lview.text(selection, 1), true);
        emuThread->unlock();
        view->setFocused(100);
    };

    stateFast.top.edit.onReturn = [this]() {
        stateFast.top.find.onActivate();
    };

    stateFast.top.find.onActivate = [this]() {
        auto fileName = stateFast.top.edit.text();
        if (fileName.empty()) {
            fileName = "savestate";
        }

        auto infos = GUIKIT::File::getFolderList(FileHelper::generatedFolder(emulator, "states_folder", "states"), fileName);

        std::vector<StateLine> lines;

        for(auto& info : infos) {
            if (GUIKIT::String::endsWith(info.name, ".images") || GUIKIT::String::endsWith(info.name, ".png"))
                continue;

            unsigned statePos = 0;
            splitFile( info.name, statePos );

            lines.push_back( {statePos, info.name, info.date} );
        }

        std::sort(lines.begin(), lines.end());

        auto& lView = stateFast.selector.listView;
        lView.lockRedraw();
        lView.reset();
        
        for(auto& line : lines ) {
            lView.append({ std::to_string(line.pos), line.fileName, line.date }, true);
        }
        lView.autoSizeColumns();
        lView.unlockRedraw();

        stateFast.selector.preview.setImage(nullptr);
    };

    stateDirect.load.onActivate = [this]() {
        auto path = GUIKIT::File::resolveRelativePath(_settings->get<std::string>("save_direct_folder", ""));

        std::string filePath = GUIKIT::BrowserWindow()
            .setWindow(*this->tabWindow)
            .setTitle(trans->get("select_savestate"))
            .setPath(path)
            .setFilters( { trans->get("state") + " (*.sav)", trans->get("all_files")} )
            .open();

        if (filePath.empty())
            return;

        path = GUIKIT::File::buildRelativePath(GUIKIT::File::getPath(filePath));
        _settings->set<std::string>("save_direct_folder", path);

        emuThread->lock(true);
        States::getInstance(emulator)->load(filePath);
        emuThread->unlock();
        view->setFocused(100);
    };

    stateDirect.save.onActivate = [this]() {

        if (activeEmulator != emulator)
            return mes->error( trans->get("no emulation active") );

        auto path = GUIKIT::File::resolveRelativePath(_settings->get<std::string>("save_direct_folder", ""));

        std::string filePath = GUIKIT::BrowserWindow()
            .setWindow(*this->tabWindow)
            .setTitle(trans->get("select_savestate"))
            .setPath(path)
            .setFilters( { trans->get("state") + " (*.sav)", trans->get("all_files")} )
            .save();

        if (filePath.empty())
            return;


        auto fn = GUIKIT::String::getFileName(filePath);
        if (GUIKIT::String::getExtension(fn, "") == "")
            filePath += ".sav";

        path = GUIKIT::File::buildRelativePath(GUIKIT::File::getPath(filePath));
        _settings->set<std::string>("save_direct_folder", path);

        emuThread->lock();
        States::getInstance( emulator )->save( filePath );
        emuThread->unlock();
    };

    stateFolder.select.onActivate = [this]() {
        auto path = GUIKIT::BrowserWindow()
                .setTitle(trans->get("select_states_folder"))
                .setWindow(*this->tabWindow)
                .directory();

        if (!path.empty()) {
            path = GUIKIT::File::buildRelativePath(path);
            _settings->set<std::string>( "states_folder", path);
            stateFolder.pathEdit.setText(path);
            stateFolder.pathEdit.setEnabled();
        }
    };

    stateFolder.standard.onActivate = [this]() {
        _settings->set<std::string>("states_folder", "");
        stateFolder.pathEdit.setText(FileHelper::generatedFolder(emulator, "states_folder", "states"));
        stateFolder.pathEdit.setEnabled(false);
    };

    loadSettings();

    updateSettingsList();
}

auto ConfigurationsLayout::updateSettingsList(const std::string& expandFile, const std::string& search) -> void {
    bool filter = !search.empty();
    std::vector<GUIKIT::TreeViewItem*> items;
    settings.treeView.itemsRecursive(items);
    settings.treeView.reset();
    for(auto item : items) {
        GUIKIT::File::Info* info = (GUIKIT::File::Info*)item->userData();
        delete info;
        delete item;
    }

    std::string path = FileHelper::getSettingsFolder(emulator);
    auto treeViewItem = new GUIKIT::TreeViewItem;
    auto info = new GUIKIT::File::Info;
    treeViewItem->setText(path);
    info->isDir = true;
    treeViewItem->setUserData((uintptr_t)info);

    GUIKIT::File::appendFolderToTreeView(path, treeViewItem, search);

    if (treeViewItem->itemCount() == 0) {
        delete treeViewItem;
        delete info;
    } else {
        items.clear();
        treeViewItem->setImage(imgFolderClosed);
        treeViewItem->setImageExpanded(imgFolderOpen);
        treeViewItem->setExpanded();
        settings.treeView.append(*treeViewItem);

        treeViewItem->itemsRecursive(items);
        for (auto item : items) {
            GUIKIT::File::Info* info = (GUIKIT::File::Info*)item->userData();
            if (info->isDir) {
                item->setImage(imgFolderClosed);
                item->setImageExpanded(imgFolderOpen);
            } else {
                item->setImage(imgDocument);
                if (filter || (expandFile == info->name)) {
                    std::vector<GUIKIT::TreeViewItem*> expandedItems;
                    auto _item = item->parentItem();
                    while (_item) {
                        expandedItems.push_back(_item);
                        _item = _item->parentItem();
                    }
                    std::reverse(expandedItems.begin(), expandedItems.end());
                    for(auto _item : expandedItems)
                        _item->setExpanded();
                }
            }
        }
    }
}

auto ConfigurationsLayout::load( std::string path, bool showError ) -> bool {

    GUIKIT::File file(path);

    if (!file.exists()) {
        if (showError)
            mes->error(trans->get("file_open_error",{
                {"%path%", path}
            }));
        return false;
    }

    std::string _audioFloppyFolder = audioManager->drive.getFloppyFolder(emulator, false);
    std::string _audioFloppyFolderExt = audioManager->drive.getFloppyFolder(emulator, true);
    std::string _audioTapeFolder = _settings->get<std::string>("audio_tape_folder", "");

    if (activeEmulator)
        program->powerOff();

    if (!_settings->load(path)) {
        if (showError)
            mes->error(trans->get("file_open_error",{
                {"%path%", path}
            }));
        return false;
    }

    program->initEmulator(this->emulator);

    auto inputManager = InputManager::getManager(this->emulator);
    inputManager->resetMappings();
    inputManager->updateAnalogSensitivity();
    inputManager->updateAutofireFrequency();
    inputManager->updateMiscSettings();
    inputManager->bindHids();

    auto firmwareManager = FirmwareManager::getInstance(this->emulator);
    firmwareManager->reload();

    PaletteManager* paletteManager = PaletteManager::getInstance(this->emulator);
    if (paletteManager)
        paletteManager->load();

    std::string audioFloppyFolder = audioManager->drive.getFloppyFolder(emulator, false);
    std::string audioFloppyFolderExt = audioManager->drive.getFloppyFolder(emulator, true);
    std::string audioTapeFolder = _settings->get<std::string>("audio_tape_folder", "");

    if (_audioFloppyFolder != audioFloppyFolder)
        audioManager->drive.unload(this->emulator, this->emulator->getDiskMediaGroup(), false);

    if (_audioFloppyFolderExt != audioFloppyFolderExt)
        audioManager->drive.unload(this->emulator, this->emulator->getDiskMediaGroup(), true);

    if (_audioTapeFolder != audioTapeFolder)
        audioManager->drive.unload(this->emulator, this->emulator->getTapeMediaGroup(), false);

    view->updateDeviceSelection(this->emulator);

    if(this->tabWindow->audioLayout) this->tabWindow->audioLayout->loadSettings();

    if(this->tabWindow->geometryLayout) this->tabWindow->geometryLayout->loadSettings();

    if(this->tabWindow->firmwareLayout) this->tabWindow->firmwareLayout->loadSettings();

    if(this->tabWindow->inputLayout) this->tabWindow->inputLayout->loadSettings();

    if(this->tabWindow->miscLayout) this->tabWindow->miscLayout->loadSettings();

    if(this->tabWindow->paletteLayout) this->tabWindow->paletteLayout->loadSettings();

    if(this->tabWindow->systemLayout) this->tabWindow->systemLayout->loadSettings();

    if(this->tabWindow->presentationLayout)
        this->tabWindow->presentationLayout->loadSettings();
    else if (videoDriver)
        VideoManager::getInstance( emulator )->reloadSettings(true);

    if(this->tabWindow->mediaLayout)
        this->tabWindow->mediaLayout->loadSettings();
    else
        fileloader->loadSettings(this->emulator);

    loadSettings();

    view->buildShader();

    unsigned ca = 0;
    if (dynamic_cast<LIBC64::Interface*>(this->emulator))
        ca = _settings->get<unsigned>("config_autostart", 0, {0, 3});

    if (!ca) {
        program->power(this->emulator);
        autoloader->set(this->emulator, nullptr, false, 0);
    } else {
        Emulator::Interface::Media* media = nullptr;
        bool trapped = false;
        switch (ca) {
            default:
            case 1:
                trapped = _settings->get<bool>("use_disk_traps", false);
                media = emulator->getDisk( 0 );
                break;
            case 2:
                trapped = _settings->get<bool>("use_tape_traps", false);
                media = emulator->getTape( 0 );
                break;
            case 3:
                media = emulator->getPRG( 0 );
                break;
        }
        fileloader->autoload(this->emulator, media, 0, trapped);
    }

    return true;
}

auto ConfigurationsLayout::splitFile( std::string file, unsigned& pos ) -> std::string {

    auto parts = GUIKIT::String::split( file, '_' );
    if (parts.size() < 2)
        return file;

    auto ident = parts[ parts.size() - 1 ];

    try {
        pos = std::stoi( ident );
    } catch(...) {
        pos = 0;
    }

    std::size_t end = file.find_last_of("_");
    if (end == std::string::npos)
        return file;

    file = file.erase(end);

    return file;
}

auto ConfigurationsLayout::updateSaveIdent( std::string fileName ) -> void {

    stateFast.top.edit.setText( fileName );
}

auto ConfigurationsLayout::loadSettings() -> void {
    auto autoSaveMode = _settings->get<unsigned>( "auto_save_mode", 2);
    switch(autoSaveMode) {
        case 0: stateFast.options.autoIdentOff.setChecked(); break;
        case 1: stateFast.options.autoIdentOn.setChecked(); break;
        default:
        case 2: stateFast.options.autoIdentCutFollowUp.setChecked(); break;
    }

    stateFast.options.screenshot.setChecked(_settings->get<bool>("save_screenshot", true));

    stateFast.top.edit.setText( _settings->get<std::string>( "save_ident", "") );

    updateStorePaths();

    if(memoryPattern) {
        Emulator::Interface::MemoryPattern pattern;
        program->getMemoryPatternFromConfig(emulator, pattern);

        memoryPattern->firstLine.valueStepper.setValue( pattern.value );
        memoryPattern->firstLine.invertValueEveryCombo.setSelectionByUserId( pattern.invertEvery );
        memoryPattern->secondLine.valueStepper.setValue( pattern.secondValue );
        memoryPattern->secondLine.invertValueEveryCombo.setSelectionByUserId( pattern.secondInvertEvery );

        memoryPattern->thirdLine.lengthRandomCombo.setSelectionByUserId( pattern.randomPatternLength );
        memoryPattern->thirdLine.repeatRandomEveryCombo.setSelectionByUserId( pattern.repeatRandomPattern );

        memoryPattern->fourthLine.randomChanceStepper.setValue( pattern.randomChance );
        memoryPattern->fourthLine.offsetCombo.setSelectionByUserId( pattern.offset );

        updateMemoryPreview();
    }
    stateFast.selector.preview.setImage(nullptr);

    if (dynamic_cast<LIBC64::Interface*>(emulator)) {
        unsigned ca = _settings->get<unsigned>("config_autostart", 0);
        switch (ca) {
            case 0:
            default: settings.options.boxDefault.setChecked(); break;
            case 1: settings.options.boxDisk.setChecked(); break;
            case 2: settings.options.boxTape.setChecked(); break;
            case 3: settings.options.boxPrg.setChecked(); break;
        }
    }
}

auto ConfigurationsLayout::updateStorePaths() -> void {
    std::string _statesFolder = _settings->get<std::string>("states_folder", "");
    stateFolder.pathEdit.setText(FileHelper::generatedFolder(emulator, "states_folder", "states"));
    stateFolder.pathEdit.setEnabled(!_statesFolder.empty());

    std::string _settingsFolder = globalSettings->get<std::string>(emulator->ident + "_settings_path", "");
    settingsFolder.pathEdit.setText(FileHelper::getSettingsFolder(emulator));
    settingsFolder.pathEdit.setEnabled(!_settingsFolder.empty());
}

auto ConfigurationsLayout::updateMemoryPreview() -> void {
    program->setMemoryPattern(emulator);

    unsigned size = emulator->getMemorySize();

    uint8_t* pattern = new uint8_t[ size ];

    emulator->getMemoryInitPattern( pattern );

    char hex[6];

    std::string out = "";

    unsigned addr = 0;

    uint8_t i;

    while(true) {

        snprintf( hex, 6, "%04x", addr );

        out += (std::string)hex;
        out += ": ";

        for (i = 0; i < 16; i++, addr++) {

            snprintf(hex, 6, "%02x", pattern[addr]);

            out += (std::string)hex;
            out += " ";
        }

        out += "\r\n";

        if (addr == size)
            break;

        if ((addr & 0xff) == 0)
            out += "\r\n";
    }

    delete[] pattern;

    memoryPattern->preview.setText( out );
}

auto ConfigurationsLayout::translate() -> void {

    settings.control.load.setText( trans->get("load") );
    settings.control.save.setText( trans->get("save") );
    settings.control.create.setText( trans->get("create") );
    settings.control.remove.setText( trans->get("remove") );

    settingsFolder.label.setText( trans->get("folder", {}, true) );
    settingsFolder.standard.setText( trans->getA("default") );

    settings.setText( trans->get("settings") );
    settings.active.activeLabel.setText( trans->get("active setting", {}, true) );
    settings.active.undockButton.setText( trans->get("undock") );
    settings.active.standardButton.setText( trans->get("default") );
    settings.options.startWithLastConfigCheckbox.setText( trans->get("Start with last loaded Settings") );
    settings.options.labelAutoStart.setText( trans->getA("Autostart", true) );
    settings.options.boxDefault.setText( trans->getA("Default") );
    settings.options.boxDisk.setText( trans->getA("Disk") );
    settings.options.boxTape.setText( trans->getA("Tape") );
    settings.options.boxPrg.setText( trans->getA("PRG") );

    stateFolder.label.setText( trans->get("folder", {}, true) );
    stateFolder.standard.setText( trans->getA("default") );

    stateFast.top.label.setText( trans->get("labelling", {}, true) );
    stateFast.top.find.setText( trans->get("find") );
    stateFast.top.hotkeys.setText( trans->get("hotkeys") );
    stateFast.selector.listView.setHeaderText({"#", trans->get("file"), trans->get("date")});

    stateFast.options.autoLabel.setText( trans->getA("automatic savestate identifier", true) );
    stateFast.options.autoIdentOff.setText( trans->getA("off") );
    stateFast.options.autoIdentOn.setText( trans->get("on") );
    stateFast.options.autoIdentCutFollowUp.setText( trans->get("cut off follow-up disks") );
    stateFast.options.screenshot.setText(trans->getA("screenshot"));
    
    stateDirect.load.setText( trans->get("load") );
    stateDirect.save.setText( trans->get("save") );
    
    stateFast.setText( trans->get("fast_save") );
    stateDirect.setText( trans->get("direct_save") );
    
    moduleList.setText( 0, 0, trans->get( "settings" ) ); 
    moduleList.setText( 1, 0, trans->get( "states" ) );
    if (memoryPattern)
        moduleList.setText( 2, 0, trans->get( "memory" ) );
    
    moduleFrame.setText( trans->get("selection") );
    
    if (cmd->hasCustomConfig(emulator) || (globalSettings->get<std::string>( emulator->ident + "_custom_settings", "" ) == ""))
        settings.active.fileLabel.setText( trans->get("default") );
    
    if(memoryPattern) {
        memoryPattern->setText( trans->get("memory reset initialisation") );

        memoryPattern->firstLine.valueLabel.setText( trans->getA( "value memory cell", true ) );
        memoryPattern->firstLine.invertValueEveryLabel.setText( trans->getA( "invert value every", true ) );

        memoryPattern->secondLine.valueLabel.setText( trans->getA( "second value memory cell", true ) );
        memoryPattern->secondLine.invertValueEveryLabel.setText( trans->getA( "invert second value every", true ) );

        memoryPattern->thirdLine.lengthRandomLabel.setText( trans->getA( "length random pattern", true ) );
        memoryPattern->thirdLine.repeatRandomEveryLabel.setText( trans->getA( "repeat random every", true ) );

        memoryPattern->fourthLine.randomChanceLabel.setText( trans->getA( "random chance", true ) );
        memoryPattern->fourthLine.offsetLabel.setText( trans->getA( "first byte offset", true ) );

        memoryPattern->fifthLine.preConfigured1.setText( trans->get( "pre-configured 1" ) );
        memoryPattern->fifthLine.preConfigured2.setText( trans->get( "pre-configured 2" ) );
        memoryPattern->fifthLine.preConfigured3.setText( trans->get( "pre-configured 3" ) );
        memoryPattern->fifthLine.preConfigured4.setText( trans->get( "pre-configured 4" ) );

        GUIKIT::HorizontalLayout::alignChildrenVertically( {&memoryPattern->firstLine, &memoryPattern->secondLine, &memoryPattern->thirdLine, &memoryPattern->fourthLine} );

        memoryPattern->updateLayout();
    }
}

}
