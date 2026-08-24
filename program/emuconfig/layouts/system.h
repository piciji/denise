
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

struct SystemModelLayout : ModelLayout {
    auto updated( Line::Block* block, Emulator::Interface::Model* model ) -> void override;
    auto getIdent( Emulator::Interface::Model* model, std::string& tooltip ) -> std::string override;
};

struct MemoryModelLayout : ModelLayout {
    auto updated( Line::Block* block, Emulator::Interface::Model* model ) -> void override;
};

struct DriveModelLayout : ModelLayout {
    auto updated( Line::Block* block, Emulator::Interface::Model* model ) -> void override;
    auto updateVisibillity( ) -> void override;
    auto widgetUpdated( Line::Block* block, Emulator::Interface::Model* model ) -> void override;

    auto getIdent( Emulator::Interface::Model* model, std::string& tooltip ) -> std::string override;
    auto hintDriveSettings() -> void;
    auto getUnit(Emulator::Interface::Model* model) -> std::string override;
};

struct PerformanceModelLayout : ModelLayout {
    auto updated( Line::Block* block, Emulator::Interface::Model* model ) -> void override;
};

struct MechanicsModelLayout : ModelLayout {
    GUIKIT::Image* curveImg = nullptr;

    auto updated( Line::Block* block, Emulator::Interface::Model* model ) -> void override;
    auto updateVisibillity( ) -> void override;
    auto blockWillAppend( Line* line, Line::Block* block ) -> void override;
    auto widgetUpdated( Line::Block* block, Emulator::Interface::Model* model ) -> void override;
    auto getUnit(Emulator::Interface::Model* model) -> std::string override;
};

struct SystemLayout : GUIKIT::VerticalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    GUIKIT::Timer memorySliderReset;
    
    GUIKIT::HorizontalLayout upperLayout;
    GUIKIT::VerticalLayout leftLayout;
    GUIKIT::VerticalLayout rightLayout;

    MemoryModelLayout memoryModelLayout;
    SystemModelLayout systemModelLayout;
    DriveModelLayout driveModelLayout;
    MechanicsModelLayout driveMechanicsLayout;
    PerformanceModelLayout performanceModelLayout;
    ExpansionLayout expansionLayout;

    auto translate() -> void;
    auto updateExpansionMemory() -> void;
    auto setExpansion( Emulator::Interface::Expansion* newExpansion ) -> void;
    auto loadSettings() -> void;
    
    SystemLayout( TabWindow* tabWindow );
};

}
