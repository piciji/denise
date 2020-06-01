
struct AccuracyLayout : GUIKIT::FramedVerticalLayout {
    
    GUIKIT::Label dangerLabel;
    
    struct Block : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox videoCycleAccuracy;
        GUIKIT::CheckBox videoScanlineThread;
        GUIKIT::CheckBox diskHighLoadThread;
        GUIKIT::CheckBox diskIdle
        GUIKIT::CheckBox audioRealtimeThread;
        
        Block();
    } block;
    
    AccuracyLayout();
};

struct FeatureLayout : GUIKIT::FramedVerticalLayout {
    
    struct Line : GUIKIT::HorizontalLayout {
        
        struct Block : GUIKIT::HorizontalLayout {
            Emulator::Interface::Feature* feature;
            GUIKIT::CheckBox checkBox;
			std::vector<GUIKIT::RadioBox*> options;
            GUIKIT::Label label;
            GUIKIT::LineEdit lineEdit;

            Block(Emulator::Interface::Feature* feature);
        };
        std::vector<Block*> blocks;       
        
        Line();
    };
    
    std::vector<Line*> lines;
        
    auto build( Emulator::Interface* emulator ) -> void;
    
    FeatureLayout();
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

    auto build( Emulator::Interface* emulator ) -> void;

    DriveLayout();
};

struct CpuLayout : GUIKIT::FramedVerticalLayout {
	
	struct Selector : GUIKIT::HorizontalLayout {
		std::vector<GUIKIT::RadioBox*> radios;
	} selector;

    auto build( Emulator::Interface* emulator ) -> void;

    CpuLayout();
};

struct ChipsetLayout : GUIKIT::FramedHorizontalLayout {
    
    struct Selector : GUIKIT::HorizontalLayout {
		std::vector<GUIKIT::RadioBox*> radios;
	} selector;
    
    auto build( Emulator::Interface* emulator ) -> void;

    ChipsetLayout();
};

struct RegionLayout : GUIKIT::FramedHorizontalLayout {
	GUIKIT::RadioBox pal;
    GUIKIT::RadioBox ntsc;
	
    RegionLayout();
};

struct SystemLayout : GUIKIT::VerticalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    
    GUIKIT::HorizontalLayout upperLayout;
    GUIKIT::VerticalLayout leftLayout;
    GUIKIT::VerticalLayout rightLayout;
	GUIKIT::HorizontalLayout bottomLayout;

    MemoryLayout memoryLayout;
    DriveLayout driveLayout;
    CpuLayout cpuLayout;
	ChipsetLayout chipsetLayout;
	RegionLayout regionLayout;
    FeatureLayout featureLayout;
    AccuracyLayout accuracyLayout;
    ExpansionLayout expansionLayout;

    auto translate() -> void;
	auto setEnabled(bool state) -> void;
	auto toggleFeature( unsigned id ) -> bool;
    auto stepRangeFeature( unsigned id, int step ) -> int;
	auto updateFeatureWidget( FeatureLayout::Line::Block* block ) -> void;
    auto updateRuntimeFeatureWidgets( ) -> void;
    auto featureIdent( std::string ident ) -> std::string;
    auto activateDrive( Emulator::Interface::MediaGroup* mediaGroup, unsigned requestedCount ) -> void;
    auto updateExpansionMemory() -> void;
    auto getSizeString( unsigned sizeInKb ) -> std::string;
    auto setExpansion( Emulator::Interface::Expansion* newExpansion ) -> void;
    
    SystemLayout( TabWindow* tabWindow );
};
