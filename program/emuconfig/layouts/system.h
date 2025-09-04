
#pragma once

#include "../../../guikit/api.h"
#include "../../program.h"
#include "model.h"

namespace EmuConfigView {

struct ExpansionLayout : GUIKIT::FramedVerticalLayout {
    
    struct Line : GUIKIT::HorizontalLayout {
        
        struct Block {
            Emulator::Interface::Expansion* expansion;
            GUIKIT::RadioBox box;
        };
        
        std::vector<Block*> blocks;         
        
        Line();
    };
    
    std::vector<Line*> lines;
        
    auto build( Emulator::Interface* emulator ) -> void;
    
    ExpansionLayout();  
};

struct SystemLayout : GUIKIT::VerticalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    GUIKIT::Timer memorySliderReset;
    
    GUIKIT::HorizontalLayout upperLayout;
    GUIKIT::VerticalLayout leftLayout;
    GUIKIT::VerticalLayout rightLayout;

    ModelLayout memoryModelLayout;
    ModelLayout modelLayout;
    ModelLayout driveModelLayout;
    ModelLayout driveMechanicsLayout;
    ModelLayout performanceModelLayout;
    ExpansionLayout expansionLayout;

    auto translate() -> void;
    auto updateExpansionMemory() -> void;
    auto setExpansion( Emulator::Interface::Expansion* newExpansion ) -> void;
    auto loadSettings() -> void;
    
    SystemLayout( TabWindow* tabWindow );
};

}
