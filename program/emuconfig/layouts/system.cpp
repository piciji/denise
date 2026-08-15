
#include "system.h"
#include "../config.h"
#include "../../thread/emuThread.h"
#include "../../view/view.h"
#include "../../input/manager.h"
#include "../../audio/manager.h"

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

    modelLayout.build( tabWindow, emulator,
    {Emulator::Interface::Model::Purpose::Cpu, Emulator::Interface::Model::Purpose::GraphicChip, Emulator::Interface::Model::Purpose::SoundChip,
    Emulator::Interface::Model::Purpose::Cia, Emulator::Interface::Model::Purpose::SubModels, Emulator::Interface::Model::Purpose::Misc}, dim );

    if (dynamic_cast<LIBC64::Interface*>(emulator))
        dim = { 2, 1, 1, 4, 3, 2, 2 };
    else
        dim = { 2, 1, 1, 1, 1, 1 };

    memoryModelLayout.build( tabWindow, emulator, {Emulator::Interface::Model::Purpose::Memory}, { 1, 1, 1 } );
    driveModelLayout.build( tabWindow, emulator, {Emulator::Interface::Model::Purpose::DriveSettings}, dim );
    driveMechanicsLayout.build( tabWindow, emulator, {Emulator::Interface::Model::Purpose::DriveMechanics}, {2, 2} );
    performanceModelLayout.build( tabWindow, emulator, {Emulator::Interface::Model::Purpose::Performance}, { 3 } );

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

    if (modelLayout.hasElements())
        append(modelLayout, {~0u, 0u}, 10);

    if (driveMechanicsLayout.hasElements())
        append(driveMechanicsLayout, {~0u, 0u}, 10);

    if (performanceModelLayout.hasElements())
        append(performanceModelLayout, {~0u, 0u});

    modelLayout.setEvents();
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
    modelLayout.translate();
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

    memoryModelLayout.setVisibility(useModel, useModel2);
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
    
    modelLayout.updateWidgets();

    driveModelLayout.updateWidgets();

    driveMechanicsLayout.updateWidgets();

    performanceModelLayout.updateWidgets();

    memoryModelLayout.updateWidgets();
}

}
