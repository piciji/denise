
struct AccuracyLayout : GUIKIT::FramedVerticalLayout {
    
    GUIKIT::Label dangerLabel;
    
    struct Block : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox videoCycleAccuracy;
        GUIKIT::CheckBox videoScanlineThread;
        GUIKIT::CheckBox diskHighLoadThread;
        GUIKIT::CheckBox diskIdle;
        GUIKIT::CheckBox audioRealtimeThread;
        
        Block();
    } block;
    
    AccuracyLayout();
};

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

struct DriveLayout : GUIKIT::FramedVerticalLayout {

    struct DriveCountFrame : GUIKIT::HorizontalLayout {
        struct DriveCount : GUIKIT::HorizontalLayout {
            Emulator::Interface::MediaGroup* mediaGroup;
            GUIKIT::Label name;
            GUIKIT::ComboButton combo;
            DriveCount();
        };
        std::vector<DriveCount*> driveCounter;
        
    } driveCountFrame;
    
    SliderLayout speed;
    SliderLayout wobble;
	GUIKIT::CheckBox tapeWobble;

    auto build( Emulator::Interface* emulator ) -> void;

    DriveLayout();
};

struct SystemLayout : GUIKIT::VerticalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    
    GUIKIT::HorizontalLayout upperLayout;
    GUIKIT::VerticalLayout leftLayout;
    GUIKIT::VerticalLayout rightLayout;

    MemoryLayout memoryLayout;
    DriveLayout driveLayout;
    ModelLayout modelLayout;
    AccuracyLayout accuracyLayout;
    ExpansionLayout expansionLayout;

    auto translate() -> void;
	auto setEnabled(bool state) -> void;    
    auto activateDrive( Emulator::Interface::MediaGroup* mediaGroup, unsigned requestedCount ) -> void;
    auto updateExpansionMemory() -> void;
    auto getSizeString( unsigned sizeInKb ) -> std::string;
    auto setExpansion( Emulator::Interface::Expansion* newExpansion ) -> void;
    
    SystemLayout( TabWindow* tabWindow );
};
