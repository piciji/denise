
#include "system.h"
#include "../config.h"
#include "../../thread/emuThread.h"
#include "../../view/view.h"
#include "../../input/manager.h"
#include "../../audio/manager.h"
#include "audio.h"
#include "presentation.h"
#include "../../../data/icons.h"

#define mes this->tabWindow->message
#define _settings this->tabWindow->settings

namespace EmuConfigView {

auto ExpansionLayout::build( Emulator::Interface* emulator ) -> void {
    unsigned blocksPerLine = 6;
    auto& expansions = emulator->expansions;

    if (expansions.size() <= 1)
        return;

    Line* line = nullptr;
    unsigned i = 0;
    unsigned lineCount = (expansions.size() / blocksPerLine);
    lineCount += ((expansions.size() % blocksPerLine) != 0) ? 1 : 0;
    std::vector<GUIKIT::RadioBox*> radios;
    
    for( auto& expansion : expansions ) {
        
        if ((i++ % blocksPerLine) == 0) {
            line = new Line();            
            lines.push_back( line );
            append( *line, {~0u, 0u}, ( lines.size() < lineCount ) ? 5 : 0 );
        }
        
        auto block = new Line::Block( );
        block->expansion = &expansion;        
        line->blocks.push_back( block );
        
        line->append( block->box, {0u, 0u}, ((i % blocksPerLine) == 0) ? 0 : 10);
        radios.push_back( &(block->box) );
        
        block->box.setText( expansion.name );
    }
    
    GUIKIT::RadioBox::setGroup( radios );
}

ExpansionLayout::Line::Line() {
    setAlignment(0.5);
}

ExpansionLayout::ExpansionLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));    
}

SystemLayout::SystemLayout(TabWindow* tabWindow) {
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;
    std::vector<unsigned> dim;
    
    memorySliderReset.setInterval(500);
    
    memorySliderReset.onFinished = [this]() {
        if (activeEmulator == this->emulator) {
            emuThread->lock(true);
            program->power(activeEmulator);
            emuThread->unlock();
        }
        
        memorySliderReset.setEnabled(false);
    };

    if (dynamic_cast<LIBC64::Interface*>(emulator))
        dim = { 4, 4 };
    else
        dim = { 1, 3, 3 };

    typedef Emulator::Interface::Model::Purpose Purpose;

    systemModelLayout.build( tabWindow, emulator,
    {Purpose::Cpu, Purpose::GraphicChip, Purpose::SoundChip, Purpose::Cia, Purpose::SubModels, Purpose::Misc}, dim );

    if (dynamic_cast<LIBC64::Interface*>(emulator))
        dim = { 2, 1, 1, 4, 3, 2, 2 };
    else
        dim = { 2, 1, 1, 1, 1, 1 };

    memoryModelLayout.build( tabWindow, emulator, {Purpose::Memory}, { 1, 1, 1 } );
    driveModelLayout.build( tabWindow, emulator, {Purpose::DriveSettings}, dim );
    driveMechanicsLayout.build( tabWindow, emulator, {Purpose::DriveMechanics}, {2, 2} );
    performanceModelLayout.build( tabWindow, emulator, {Purpose::Performance}, { 3 } );

    expansionLayout.build( emulator );

    setMargin(10);

    if (!expansionLayout.lines.empty())
        leftLayout.append(expansionLayout, {~0u, 0u}, 10);

    if (memoryModelLayout.hasElements())
        leftLayout.append(memoryModelLayout, {~0u, 0u}, 10);

    upperLayout.append(leftLayout, {~0u, 0u}, 10);

    if (driveModelLayout.hasElements())
        rightLayout.append(driveModelLayout, {~0u, 0u}, 0);

    upperLayout.append(rightLayout, {~0u, 0u});

    append(upperLayout, {~0u, 0u}, 10);

    if (systemModelLayout.hasElements())
        append(systemModelLayout, {~0u, 0u}, 10);

    if (driveMechanicsLayout.hasElements())
        append(driveMechanicsLayout, {~0u, 0u}, 10);

    if (performanceModelLayout.hasElements())
        append(performanceModelLayout, {~0u, 0u});

    systemModelLayout.setEvents();
    memoryModelLayout.setEvents();
    driveModelLayout.setEvents();
    driveMechanicsLayout.setEvents();
    performanceModelLayout.setEvents();

    for ( auto line : expansionLayout.lines ) {
        for( auto block : line->blocks ) {            
            block->box.onActivate = [this, block]() {

                _settings->set<unsigned>( "expansion", block->expansion->id);
                updateExpansionMemory();

                if (activeEmulator == this->emulator) {
                    emuThread->lock(true);
                    program->power(activeEmulator);
                    emuThread->unlock();
                }
            };
        }
    }

    loadSettings();
}

auto SystemLayout::translate() -> void {
    systemModelLayout.translate();
    driveModelLayout.translate( "drives" );
    driveMechanicsLayout.translate( "drive mechanics" );
    performanceModelLayout.translate( "accuracy and performance" );
    memoryModelLayout.translate( "memory" );

    expansionLayout.setText( trans->get("expansion_port") );

    for( auto line : expansionLayout.lines ) {
        for( auto block : line->blocks ) {
            block->box.setText( trans->get( block->expansion->name ) );
        }
    }

    driveModelLayout.alignSlider( "300.00 RPM" );
    memoryModelLayout.alignSlider( "0.00 MB" );
}

auto SystemLayout::updateExpansionMemory() -> void {
    if (dynamic_cast<LIBAMI::Interface*>(emulator))
        return;

    Emulator::Interface::Model* useModel = nullptr;
    Emulator::Interface::Model* useModel2 = nullptr;
    Emulator::Interface::Expansion* expansionSelected = nullptr;

    for ( auto line : expansionLayout.lines ) {
        for( auto block : line->blocks ) {
            if (block->box.checked()) {
                expansionSelected = block->expansion;
                break;
            }
        }
    }

    if (expansionSelected) {
        if (expansionSelected->id == LIBC64::Interface::ExpansionIdReu ||
            expansionSelected->id == LIBC64::Interface::ExpansionIdReuRetroReplay) {
            useModel = emulator->getModel(LIBC64::Interface::ModelIdReuRam);

            } else if (expansionSelected->id == LIBC64::Interface::ExpansionIdGeoRam) {
                useModel = emulator->getModel(LIBC64::Interface::ModelIdGeoRam);
            } else if (expansionSelected->id == LIBC64::Interface::ExpansionIdSuperCpu) {
                useModel = emulator->getModel(LIBC64::Interface::ModelIdSuperCpuRam);
            } else if (expansionSelected->id == LIBC64::Interface::ExpansionIdSuperCpuReu) {
                useModel = emulator->getModel(LIBC64::Interface::ModelIdSuperCpuRam);
                useModel2 = emulator->getModel(LIBC64::Interface::ModelIdReuRam);
            }
    }

    for (auto line : memoryModelLayout.lines) {
        for (auto block : line->blocks)
            block->setEnabled((useModel && (block->model->id == useModel->id)) || (useModel2 && (block->model->id == useModel2->id)));
    }
}

auto SystemLayout::setExpansion( Emulator::Interface::Expansion* newExpansion ) -> void {

    for ( auto line : expansionLayout.lines ) {
        for( auto block : line->blocks ) {

            if (!newExpansion) {
                if (block->expansion->isEmpty()) {
                    if (!block->box.checked()) {
                        block->box.setChecked();
                        updateExpansionMemory();
                    }

                    return;
                }
            }

            else if (block->expansion == newExpansion) {
                if (!block->box.checked()) {
                    block->box.setChecked();
                    updateExpansionMemory();
                }
				
                return;
            }                
        }
    }
}

auto SystemLayout::loadSettings() -> void {
    auto expansionId = _settings->get<unsigned>( "expansion", 0);

    for ( auto line : expansionLayout.lines ) {
        for( auto block : line->blocks ) {
            if (block->expansion->id == expansionId)
                block->box.setChecked();
        }
    }

    updateExpansionMemory();
    
    systemModelLayout.updateWidgets();

    driveModelLayout.updateWidgets();

    driveMechanicsLayout.updateWidgets();

    performanceModelLayout.updateWidgets();

    memoryModelLayout.updateWidgets();
}

auto SystemModelLayout::updated( Line::Block* block, Emulator::Interface::Model* model ) -> void {
    if (dynamic_cast<LIBC64::Interface*> (this->emulator)) {
        switch(model->id) {
            case LIBC64::Interface::ModelIdVicIIModel: {
                if (tabWindow->presentationLayout)
                    tabWindow->presentationLayout->updatePresets(true, false);
                else if (videoDriver)
                    VideoManager::getInstance( emulator )->reloadSettings(false);

                if (this->emulator == activeEmulator)
                    program->power(activeEmulator);
            } break;

            case LIBC64::Interface::ModelIdSid: {
                if (tabWindow->audioLayout)
                    tabWindow->audioLayout->settingsLayout.updateWidget(LIBC64::Interface::ModelIdSid);
            } break;
        }
    } else {
        switch(model->id) {
            case LIBAMI::Interface::ModelIdRegion:
                if (tabWindow->presentationLayout)
                    tabWindow->presentationLayout->updatePresets(true, false);
                else if (videoDriver)
                    VideoManager::getInstance( emulator )->reloadSettings(false);
                // fallthrough
            case LIBAMI::Interface::ModelIdSystem:
                if (this->emulator == activeEmulator)
                    program->power(activeEmulator);
                break;
        }
    }
}

auto SystemModelLayout::getIdent( Emulator::Interface::Model* model, std::string& tooltip ) -> std::string {
        std::string name = model->name;

    if (dynamic_cast<LIBC64::Interface*>(emulator)) {
        switch (model->id) {
            case LIBC64::Interface::ModelIdSid:
                name = "SID";
                tooltip = "SID tooltip";
                break;

            default:
                return ModelLayout::getIdent(model, tooltip);
        }
    } else if (dynamic_cast<LIBAMI::Interface*>(emulator)) {
        return ModelLayout::getIdent(model, tooltip);
    }

    return name;
}

auto MemoryModelLayout::updated( Line::Block* block, Emulator::Interface::Model* model ) -> void {
    if (dynamic_cast<LIBC64::Interface*> (this->emulator)) {
        switch(model->id) {
            case LIBC64::Interface::ModelIdReuRam:
            case LIBC64::Interface::ModelIdGeoRam:
            case LIBC64::Interface::ModelIdSuperCpuRam:
                if (this->emulator == activeEmulator)
                    program->power(activeEmulator);

                break;
        }
    } else {
        switch(model->id) {
            case LIBAMI::Interface::ModelIdChipMem:
            case LIBAMI::Interface::ModelIdSlowMem:
            case LIBAMI::Interface::ModelIdFastMem:
                if (this->emulator == activeEmulator)
                    program->power(activeEmulator);
                break;
        }
    }
}

auto DriveModelLayout::updated( Line::Block* block, Emulator::Interface::Model* model ) -> void {
    if (dynamic_cast<LIBC64::Interface*> (this->emulator)) {
        switch(model->id) {
            case LIBC64::Interface::ModelIdDiskDrivesConnected:
                if(tabWindow->mediaLayout)
                    tabWindow->mediaLayout->updateVisibility( emulator->getDiskMediaGroup(), block->combo->selection() );
                // fall through
            case LIBC64::Interface::ModelIdTapeDrivesConnected:
            case LIBC64::Interface::ModelIdDriveRam20To3F:
            case LIBC64::Interface::ModelIdDriveRam40To5F:
            case LIBC64::Interface::ModelIdDriveRam60To7F:
            case LIBC64::Interface::ModelIdDriveRam80To9F:
            case LIBC64::Interface::ModelIdDriveRamA0ToBF:
                if (this->emulator == activeEmulator)
                    program->power(activeEmulator);

                break;

            case LIBC64::Interface::ModelIdDiskDriveModel:
                updateVisibillity();
                if (this->emulator == activeEmulator)
                    program->power(activeEmulator);
                break;

            case LIBC64::Interface::ModelIdDriveFastLoader:
                hintDriveSettings();
                break;
        }
    } else {
        switch(model->id) {
            case LIBAMI::Interface::ModelIdDiskDrivesConnected:
                if (tabWindow->mediaLayout)
                    tabWindow->mediaLayout->updateVisibility(emulator->getDiskMediaGroup(), block->combo->selection());

                if (this->emulator == activeEmulator)
                    program->power(activeEmulator);
                break;

            case LIBAMI::Interface::ModelIdHardDrivesConnected:
                if (tabWindow->mediaLayout)
                    tabWindow->mediaLayout->updateVisibility(emulator->getHardDiskMediaGroup(), block->combo->selection());

                if (this->emulator == activeEmulator)
                    program->power(activeEmulator);
                break;
        }
    }
}

auto DriveModelLayout::widgetUpdated( Line::Block* block, Emulator::Interface::Model* model ) -> void {

    if (dynamic_cast<LIBC64::Interface*> (this->emulator)) {
        if (tabWindow->mediaLayout) {
            if (model->id == LIBC64::Interface::ModelIdDiskDrivesConnected )
                tabWindow->mediaLayout->updateVisibility(emulator->getDiskMediaGroup(), block->combo->selection());
        }
    } else {
        if (tabWindow->mediaLayout) {
            if (model->id == LIBAMI::Interface::ModelIdDiskDrivesConnected )
                tabWindow->mediaLayout->updateVisibility(emulator->getDiskMediaGroup(), block->combo->selection());

            if (model->id == LIBAMI::Interface::ModelIdHardDrivesConnected )
                tabWindow->mediaLayout->updateVisibility(emulator->getHardDiskMediaGroup(), block->combo->selection());
        }
    }
}

auto DriveModelLayout::updateVisibillity( ) -> void {
    if (!dynamic_cast<LIBC64::Interface*>(this->emulator))
        return;
    
    auto blockBurstMode = getBlock( LIBC64::Interface::ModelIdCiaBurstMode );
    auto blockDriveModel = getBlock( LIBC64::Interface::ModelIdDiskDriveModel );
    auto selection = blockDriveModel->combo->selection();

    blockBurstMode->checkBox->setEnabled( selection == 3 || selection == 4 || selection == 5 );
}

auto DriveModelLayout::getIdent( Emulator::Interface::Model* model, std::string& tooltip ) -> std::string {
        std::string name = model->name;

    if (dynamic_cast<LIBC64::Interface*>(emulator)) {
        switch (model->id) {
            case LIBC64::Interface::ModelIdDriveRam20To3F:
            case LIBC64::Interface::ModelIdDriveRam40To5F:
            case LIBC64::Interface::ModelIdDriveRam60To7F:
            case LIBC64::Interface::ModelIdDriveRam80To9F:
            case LIBC64::Interface::ModelIdDriveRamA0ToBF:
            case LIBC64::Interface::ModelIdDriveFastLoader:
                tooltip = "";
                break;

            default:
                return ModelLayout::getIdent(model, tooltip);
        }
    } else if (dynamic_cast<LIBAMI::Interface*>(emulator)) {
        return ModelLayout::getIdent(model, tooltip);
    }

    return name;
}

auto DriveModelLayout::getUnit(Emulator::Interface::Model* model) -> std::string {
    if (dynamic_cast<LIBC64::Interface*>(this->emulator)) {
        if (model->id == LIBC64::Interface::ModelIdDiskDriveSpeed || model->id == LIBC64::Interface::ModelIdDiskDriveWobble)
            return " RPM";
    } else {
        if (model->id == LIBAMI::Interface::ModelIdDriveStepperDelay || model->id == LIBAMI::Interface::ModelIdDriveStepperAccess)
            return " ms";

        if (model->id == LIBAMI::Interface::ModelIdDiskDriveSpeed || model->id == LIBAMI::Interface::ModelIdDiskDriveWobble)
            return " RPM";
    }

    return "";
}

auto DriveModelLayout::hintDriveSettings() -> void {
    program->powerOff();

    auto blockFastloader = getBlock( LIBC64::Interface::ModelIdDriveFastLoader );
    auto blockParallel = getBlock( LIBC64::Interface::ModelIdDriveParallelCable );
    auto blockBurst = getBlock( LIBC64::Interface::ModelIdCiaBurstMode );
    auto blockDriveModel = getBlock( LIBC64::Interface::ModelIdDiskDriveModel );
    auto blockRam20 = getBlock( LIBC64::Interface::ModelIdDriveRam20To3F );
    auto blockRam40 = getBlock( LIBC64::Interface::ModelIdDriveRam40To5F );
    auto blockRam60 = getBlock( LIBC64::Interface::ModelIdDriveRam60To7F );
    auto blockRam80 = getBlock( LIBC64::Interface::ModelIdDriveRam80To9F );
    auto blockRamA0 = getBlock( LIBC64::Interface::ModelIdDriveRamA0ToBF );
    auto selection = blockFastloader->combo->selection();

    if (blockBurst->checkBox->checked())
        blockBurst->checkBox->toggle();
    if (!blockParallel->checkBox->checked())
        blockParallel->checkBox->toggle();
    if (blockRam20->checkBox->checked())
        blockRam20->checkBox->toggle();
    if (blockRam40->checkBox->checked())
        blockRam40->checkBox->toggle();
    if (blockRam60->checkBox->checked())
        blockRam60->checkBox->toggle();
    if (blockRam80->checkBox->checked())
        blockRam80->checkBox->toggle();
    if (blockRamA0->checkBox->checked())
        blockRamA0->checkBox->toggle();

    if (selection == 0) {
        blockParallel->checkBox->toggle();
    } else if (selection == 1) { // SpeedDOS
        blockDriveModel->combo->setSelection(1);
    } else if (selection == 2) { // DolphinDOS v2
        blockRam80->checkBox->toggle();
        blockDriveModel->combo->setSelection(1);
    } else if (selection == 3) { // DolphinDOS v2 Ultimate
        blockRam40->checkBox->toggle();
        blockRam60->checkBox->toggle();
        blockDriveModel->combo->setSelection(1);
    } else if (selection == 4) { // DolphinDOS v3 1541
        blockRam60->checkBox->toggle();
        blockDriveModel->combo->setSelection(1);
    } else if (selection == 5) { // DolphinDOS v3 157x
        blockRam60->checkBox->toggle();
        blockDriveModel->combo->setSelection(4);
    } else if (selection == 6) { // ProfDOS v1 1541
        blockRamA0->checkBox->toggle();
        blockDriveModel->combo->setSelection(0);
    } else if (selection == 7) { // ProfDOS R4 1541
        blockRam40->checkBox->toggle();
        blockDriveModel->combo->setSelection(0);
    } else if (selection == 8) { // ProfDOS R5 1570
        blockRam40->checkBox->toggle();
        blockDriveModel->combo->setSelection(3);
    } else if (selection == 9) { // ProfDOS R6 1571
        blockRam40->checkBox->toggle();
        blockDriveModel->combo->setSelection(4);
    } else if (selection == 10) { // PrologicDOS Classic 1541
        blockRam80->checkBox->toggle();
        blockDriveModel->combo->setSelection(0);
    } else if (selection == 11) { // PrologicDOS 1541
        blockRam80->checkBox->toggle();
        blockDriveModel->combo->setSelection(0);
    } else if (selection == 12) { // Turbo Trans
        blockRamA0->checkBox->toggle();
        blockDriveModel->combo->setSelection(0);
    } else if (selection == 13) { // Pro Speed 1571
        blockRam80->checkBox->toggle();
        blockDriveModel->combo->setSelection(4);
    } else if (selection == 14) { // StarDOS
        blockParallel->checkBox->toggle();
        blockDriveModel->combo->setSelection(1);
    } else if (selection == 15) { // Supercard
        blockParallel->checkBox->toggle();
        blockRam60->checkBox->toggle();
        blockDriveModel->combo->setSelection(1);
    } else if (selection == 16) { // Disk Demon 1541
        blockRam40->checkBox->toggle();
        blockDriveModel->combo->setSelection(0);
    }

    blockDriveModel->combo->onChange();

    program->power(emulator);
}

auto PerformanceModelLayout::updated( Line::Block* block, Emulator::Interface::Model* model ) -> void {
    if (dynamic_cast<LIBC64::Interface*> (this->emulator)) {
        switch(model->id) {
            case LIBC64::Interface::ModelIdCycleAccurateVideo:
                program->setWarp(Program::Warp::Off);

                if (this->emulator == activeEmulator)
                    program->power(activeEmulator);
                break;

            case LIBC64::Interface::ModelIdDiskThread:
            case LIBC64::Interface::ModelIdDiskOnDemand:
                program->setWarp(Program::Warp::Off);
                break;
            default: break;
        }
    }
}

auto MechanicsModelLayout::updated( Line::Block* block, Emulator::Interface::Model* model ) -> void {
    if (dynamic_cast<LIBC64::Interface*> (this->emulator)) {
        switch(model->id) {
            case LIBC64::Interface::ModelIdEmulateDriveMechanics:
                updateVisibillity();
                break;

            case LIBC64::Interface::ModelIdDriveAcceleration:
            case LIBC64::Interface::ModelIdDriveDeceleration:
                widgetUpdated(block, model);
                break;
            default: break;
        }
    }
}

auto MechanicsModelLayout::updateVisibillity( ) -> void {
    if (!dynamic_cast<LIBC64::Interface*>(this->emulator))
        return;

    auto blockEnable = getBlock( LIBC64::Interface::ModelIdEmulateDriveMechanics );
    auto blockStepper = getBlock( LIBC64::Interface::ModelIdDriveStepperDelay );
    auto blockAcc = getBlock( LIBC64::Interface::ModelIdDriveAcceleration );
    auto blockDec = getBlock( LIBC64::Interface::ModelIdDriveDeceleration );
    bool enabled = blockEnable->checkBox->checked();

    blockStepper->sliderLayout->setEnabled( enabled );
    blockAcc->sliderLayout->setEnabled( enabled );
    blockDec->sliderLayout->setEnabled( enabled );
}

auto MechanicsModelLayout::blockWillAppend( Line* line, Line::Block* block ) -> void {
    if (!dynamic_cast<LIBC64::Interface*> (this->emulator))
        return;

    auto model = block->model;

    if (model->id == LIBC64::Interface::ModelIdDriveAcceleration || model->id == LIBC64::Interface::ModelIdDriveDeceleration) {
        if (!curveImg) {
            curveImg = new GUIKIT::Image;
            curveImg->loadPng((uint8_t*)Icons::sine, sizeof(Icons::sine));
        }
        block->imageView = new GUIKIT::ImageView;
        block->imageView->setImage( curveImg );
        line->append(*block->imageView, {curveImg->width, curveImg->height}, 3 );
    }
}

auto MechanicsModelLayout::widgetUpdated( Line::Block* block, Emulator::Interface::Model* model ) -> void {
    if (!dynamic_cast<LIBC64::Interface*> (this->emulator))
        return;

    std::string uri;

    switch (model->id) {
        default:
            return;

        case LIBC64::Interface::ModelIdDriveDeceleration: {
            auto val = tabWindow->settings->get<int>( _underscore(model->name), model->defaultValue, model->range );
            uri = "https://www.wolframalpha.com/input?i=plot+%5B%2F%2Fmath%3A300.0+*+%280.4+%5E%28%28";
            uri += std::to_string( (float)val / model->scaler );
            uri += "%2F65536.0%29*%28x%2F256.0%29+%29%29%2F%2F%5D+from+%5B%2F%2Fnumber%3A0%2F%2F%5D+to+%5B%2F%2Fnumber%3A500000%2F%2F%5D";
        } break;

        case LIBC64::Interface::ModelIdDriveAcceleration: {
            auto val = tabWindow->settings->get<int>( _underscore(model->name), model->defaultValue, model->range );
            uri = "https://www.wolframalpha.com/input?i=plot+%5B%2F%2Fmath%3A300.0+*+%28-0.4+%5E%28%28";
            uri += std::to_string( (float)val / model->scaler );
            uri += "%2F65536.0%29*%28x%2F256.0%29+%29%29+%2B+300.0%2F%2F%5D+from+%5B%2F%2Fnumber%3A0%2F%2F%5D+to+%5B%2F%2Fnumber%3A500000%2F%2F%5D";
        } break;
    }

    block->imageView->setUri( uri );
}

auto MechanicsModelLayout::getUnit(Emulator::Interface::Model* model) -> std::string {
    if (dynamic_cast<LIBC64::Interface*>(this->emulator)) {
        if (model->id == LIBC64::Interface::ModelIdDriveStepperDelay)
            return " ms";
    }

    return "";
}

}
