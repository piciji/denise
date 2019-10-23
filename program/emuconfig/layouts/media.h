
struct PathsLayout : GUIKIT::FramedVerticalLayout {

    struct Block : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::LineEdit edit;
        GUIKIT::Button empty;
        GUIKIT::Button select;
        
        Emulator::Interface::MediaGroup* mediaGroup;

        Block( Emulator::Interface::MediaGroup* mediaGroup );
    };
    std::vector<Block*> blocks; 
    
    PathsLayout();
};

struct MediaGroupLayout : GUIKIT::FramedVerticalLayout {

    struct Block : GUIKIT::VerticalLayout {
        struct Header : GUIKIT::HorizontalLayout {
            GUIKIT::RadioBox inUse;
            GUIKIT::Label deviceName;
            GUIKIT::CheckBox writeprotect;
            GUIKIT::Button eject;
            GUIKIT::Label fileName;

            Header();
        } header;

        struct Selector : GUIKIT::HorizontalLayout {            
            GUIKIT::LineEdit edit;
            GUIKIT::ComboButton combo;
            GUIKIT::Button open;
            GUIKIT::Widget spacer;
            GUIKIT::Button openW;            

            Selector();
        } selector;

        Emulator::Interface::Media* media;
        bool openWritable;
        Block();
        std::vector<Emulator::Interface::Listing> listings;
    };
    std::vector<Block*> blocks;
    Emulator::Interface::MediaGroup* mediaGroup;
    GUIKIT::VerticalLayout blockContainer;
	GUIKIT::Button inject;
	GUIKIT::ListView listings; // for c64 disk and prg container formats
    Block* selectedBlock = nullptr;
    TabWindow* tabWindow;
    
    auto build() -> void;
    auto updateVisibility( unsigned count, bool init = false ) -> void;
    auto fillListing(MediaGroupLayout::Block* block) -> void;
    auto showOnlyConnectedDevices() -> bool;

    MediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup, TabWindow* tabWindow );
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

struct MediaLayout : GUIKIT::TabFrameLayout {
    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    
    GUIKIT::Image diskImage;
    GUIKIT::Image hdImage;
    GUIKIT::Image tapeImage;
    GUIKIT::Image expansionImage;
	GUIKIT::Image memoryImage;
	GUIKIT::Image addImage;    
    GUIKIT::Image pathImage;
    
    std::vector<MediaGroupLayout*> mediaGroupLayouts;
    std::vector<std::string> tabs;
    
	GUIKIT::VerticalLayout creatorLayout;    
    TapeCreatorLayout* tapeCreatorLayout = nullptr;
    HdCreatorLayout* hdCreatorLayout = nullptr;
    DiskCreatorLayout* diskCreatorLayout = nullptr;
	MemoryCreatorLayout* memoryCreatorLayout = nullptr;
    
    PathsLayout pathsLayout;

    auto translate() -> void;
    auto updateMediaBlock(MediaGroupLayout::Block* block, FileSetting* setting) -> void;
    auto updateVisibility( Emulator::Interface::MediaGroup* mediaGroup, unsigned count ) -> void;
    auto bindSelectorAction( MediaGroupLayout* layout ) -> void;
    auto prepareCreator() -> void;
    auto preparePaths() -> void;	
    auto updateListing( Emulator::Interface::Media* media ) -> void;
	auto preselectPath( std::string& groupName ) -> std::string;
	auto savePath( std::string& groupName, std::string path ) -> void;
    auto showC64Listing( MediaGroupLayout* layout ) -> bool;
    auto createImage( unsigned groupId ) -> void;
    auto showMediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup ) -> void;
    auto getMediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup ) -> MediaGroupLayout*;   
    auto insertImage( MediaGroupLayout* layout, MediaGroupLayout::Block* block, GUIKIT::File* file, GUIKIT::File::Item* item ) -> void;
    auto insertImage( Emulator::Interface::Media* media, GUIKIT::File* file, GUIKIT::File::Item* item ) -> void;
    auto eject( Emulator::Interface::MediaGroup* mediaGroup ) -> void;
    auto drop( std::string filePath, MediaGroupLayout::Block* block = nullptr ) -> void;   
    auto colorListing( unsigned color, bool foreground ) -> void;

    MediaLayout(TabWindow* tabWindow);
};
