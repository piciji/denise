
struct PathsLayout : GUIKIT::FramedVerticalLayout {

    struct Block : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::LineEdit edit;
        GUIKIT::Button empty;
        GUIKIT::Button select;
        
        Emulator::Interface::DriveGroup* driveGroup;

        Block( Emulator::Interface::DriveGroup* driveGroup );
    };
    std::vector<Block*> blocks; 
    
    PathsLayout();
};

struct DriveGroupLayout : GUIKIT::FramedVerticalLayout {

    struct Block : GUIKIT::VerticalLayout {
        struct Header : GUIKIT::HorizontalLayout {
            GUIKIT::Label deviceName;
            GUIKIT::CheckBox writeprotect;
            GUIKIT::Button eject;
            GUIKIT::Label fileName;

            Header();
        } header;

        struct Selector : GUIKIT::HorizontalLayout {
            GUIKIT::LineEdit edit;
            GUIKIT::Button open;
            GUIKIT::Button openW;

            Selector();
        } selector;

        Emulator::Interface::Drive* drive;
        bool openWritable;
        Block();
        std::vector<Emulator::Interface::Listing> listings;
    };
    std::vector<Block*> blocks;
    Emulator::Interface::DriveGroup* driveGroup;
    GUIKIT::VerticalLayout blockContainer;
	GUIKIT::Button inject;
	GUIKIT::ListView listings; // for c64 disk and prg container formats
    Block* selectedBlock = nullptr;
    TabWindow* tabWindow;
    
    auto build() -> void;
    auto updateVisibility( unsigned count, bool init = false ) -> void;
    auto fillListing(DriveGroupLayout::Block* block) -> void;

    DriveGroupLayout( Emulator::Interface::DriveGroup* driveGroup, TabWindow* tabWindow );
};

struct ImageInsertHelper {
    GUIKIT::File* file;
    GUIKIT::File::Item* item;
    Emulator::Interface::DriveGroup* driveGroup;
    DriveGroupLayout::Block* block;
};

struct TapeCreatorLayout : GUIKIT::FramedHorizontalLayout {
    GUIKIT::Button button;

    TapeCreatorLayout();
};

struct DiskCreatorLayout : GUIKIT::FramedHorizontalLayout {
    GUIKIT::Label formatName;
    GUIKIT::ComboButton format;
    GUIKIT::CheckBox fastFileSystem;
    GUIKIT::CheckBox highDensity;
    GUIKIT::Label diskLabelName;
    GUIKIT::LineEdit diskLabel;
    GUIKIT::Button button;

    DiskCreatorLayout(Emulator::Interface* emulator, std::vector<std::string> formats);
};

struct MemoryCreatorLayout : GUIKIT::FramedHorizontalLayout {
	GUIKIT::Button button;
	
	MemoryCreatorLayout();
};

struct HdCreatorLayout : GUIKIT::FramedVerticalLayout {

    struct Creator : GUIKIT::HorizontalLayout {
        GUIKIT::Label diskSizeName;
        GUIKIT::LineEdit diskSize;
        GUIKIT::Label diskLabelName;
        GUIKIT::LineEdit diskLabel;
        GUIKIT::Button button;

        Creator();
    } creator;

    struct Progress : GUIKIT::HorizontalLayout {
        GUIKIT::ProgressBar bar;
        GUIKIT::Label label;

        Progress();
    } progress;

    HdCreatorLayout();
};

struct DrivesLayout : GUIKIT::TabFrameLayout {
    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    
    GUIKIT::Image diskImage;
    GUIKIT::Image hdImage;
    GUIKIT::Image tapeImage;
    GUIKIT::Image moduleImage;
	GUIKIT::Image memoryImage;
	GUIKIT::Image addImage;    
    GUIKIT::Image pathImage;
    
    std::vector<DriveGroupLayout*> driveGroupLayouts;
    std::vector<std::string> tabs;
    
	GUIKIT::VerticalLayout creatorLayout;    
    TapeCreatorLayout* tapeCreatorLayout = nullptr;
    HdCreatorLayout* hdCreatorLayout = nullptr;
    DiskCreatorLayout* diskCreatorLayout = nullptr;
	MemoryCreatorLayout* memoryCreatorLayout = nullptr;
    
    PathsLayout pathsLayout;

    auto translate() -> void;
    auto updateDriveBlock(DriveGroupLayout::Block* block, FileSetting* setting) -> void;
    auto updateVisibility( Emulator::Interface::DriveGroup* driveGroup, unsigned count ) -> void;
    auto bindSelectorAction( DriveGroupLayout* layout ) -> void;
    auto prepareCreator() -> void;
    auto preparePaths() -> void;	
    auto updateListing( Emulator::Interface::Drive* drive ) -> void;
	auto preselectPath( std::string& groupName ) -> std::string;
	auto savePath( std::string& groupName, std::string path ) -> void;
    auto showC64Listing( DriveGroupLayout* layout ) -> bool;
    auto createImage( unsigned groupId ) -> void;
    auto showDriveGroupLayout( Emulator::Interface::DriveGroup* driveGroup ) -> void;
    auto getDriveGroupLayout( Emulator::Interface::DriveGroup* driveGroup ) -> DriveGroupLayout*;   
    auto insertImage( ImageInsertHelper iih ) -> void;  
    auto eject( Emulator::Interface::DriveGroup* driveGroup ) -> void;
    auto drop( std::string filePath, DriveGroupLayout::Block* block = nullptr ) -> void;   
    auto colorListing( unsigned color, bool foreground ) -> void;

    DrivesLayout(TabWindow* tabWindow);
};
