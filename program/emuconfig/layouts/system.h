
struct MemoryLayout : GUIKIT::FramedVerticalLayout {
    struct Block : GUIKIT::HorizontalLayout {
        Emulator::Interface::MemoryType* memoryType;
        SliderLayout sliderLayout;
        Block();
    };
    std::vector<Block*> blocks;

    auto build( Emulator::Interface* emulator ) -> void;

    MemoryLayout();
};

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

    MemoryLayout memoryLayout;
    ModelLayout memoryNewModelLayout;
    ModelLayout modelLayout;
    ModelLayout driveModelLayout;
    ModelLayout performanceModelLayout;
    ExpansionLayout expansionLayout;

    auto translate() -> void;
    auto updateExpansionMemory() -> void;
    auto getSizeString( unsigned sizeInKb ) -> std::string;
    auto setExpansion( Emulator::Interface::Expansion* newExpansion ) -> void;
    auto loadSettings() -> void;
    
    SystemLayout( TabWindow* tabWindow );
};
