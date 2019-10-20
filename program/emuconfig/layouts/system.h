
struct FeatureLayout : GUIKIT::FramedVerticalLayout {
    
    struct Line : GUIKIT::HorizontalLayout {
        
        struct Block : GUIKIT::HorizontalLayout {
            unsigned typeId;
            GUIKIT::CheckBox checkBox;
            GUIKIT::Label label;
            GUIKIT::LineEdit lineEdit;
            GUIKIT::Label dangerLabel;

            Block(bool switched);
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
        GUIKIT::Label name;
        GUIKIT::Label value;
        GUIKIT::HorizontalSlider slider;

        Block(bool disable);
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

struct DriveLayout : GUIKIT::FramedHorizontalLayout {

    struct DriveCount : GUIKIT::HorizontalLayout {
        Emulator::Interface::MediaGroup* mediaGroup;
        GUIKIT::Label name;
        GUIKIT::ComboButton combo;
        DriveCount();
    };
    std::vector<DriveCount*> driveCounter;

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
    FeatureLayout featureLayout;
    ExpansionLayout expansionLayout;

    auto translate() -> void;
	auto setEnabled(bool state) -> void;
	auto toggleFeature( unsigned id ) -> bool;
    auto updateFeature( unsigned id, int step ) -> int;
	auto updateFeatureWidget( FeatureLayout::Line::Block* block ) -> void;
    auto updateRuntimeFeatureWidgets( ) -> void;
    auto featureIdent( std::string ident ) -> std::string;
    auto activateDrive( Emulator::Interface::MediaGroup* mediaGroup, unsigned requestedCount ) -> void;
    auto updateExpansionMemory() -> void;
    auto getSizeString( unsigned sizeInKb ) -> std::string;
    auto handleExpansionIfAutoBoot(bool cartNeeded) -> void;
    
    SystemLayout( TabWindow* tabWindow );
};
