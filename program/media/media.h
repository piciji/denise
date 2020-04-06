
#pragma once

struct Message;
struct FileSetting;

#include "../../guikit/api.h"
#include "../program.h"

namespace MediaView {

struct MediaWindow;

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

            Header(Emulator::Interface::Media* media);
        } header;

        struct Selector : GUIKIT::HorizontalLayout {            
            GUIKIT::LineEdit edit;
            GUIKIT::ComboButton combo;          
            GUIKIT::Label jumperLabel;
            std::vector<GUIKIT::CheckBox*> jumpers;
            GUIKIT::Button open;
            GUIKIT::Widget spacer;          

            Selector(Emulator::Interface::Media* media);
        } selector;

        Emulator::Interface::Media* media;        
        std::vector<Emulator::Interface::Listing> listings;
        MediaGroupLayout* layout;
        Block(Emulator::Interface::Media* media);
    };
    std::vector<Block*> blocks;
    Emulator::Interface::MediaGroup* mediaGroup;
    GUIKIT::VerticalLayout blockContainer;
	GUIKIT::Button inject;
	GUIKIT::ListView listings; // for c64 disk and prg container formats
    Block* selectedBlock = nullptr;
    MediaWindow* mediaWindow;
    
    auto build() -> void;
    auto updateVisibility( unsigned count, bool init = false ) -> void;
    auto updateListing(MediaGroupLayout::Block* block) -> void;
    auto fillListing( std::vector<Emulator::Interface::Listing>& emuListings ) -> void;
    auto fillListing( std::vector<std::string>& emuListings ) -> void;
    auto showOnlyConnectedDevices() -> bool;
    auto getBlock(Emulator::Interface::Media* media) -> Block*;

    MediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup, MediaWindow* mediaWindow );
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

struct CartCreatorLayout : GUIKIT::FramedHorizontalLayout {	
    GUIKIT::ComboButton format;
    GUIKIT::Button button;
	
	CartCreatorLayout();
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

struct MediaWindow : public GUIKIT::Window {
    
    Emulator::Interface* emulator;
    bool useCustomFont = false;
	Message* message;
    GUIKIT::Setting* alternateFileDialog = nullptr;
	
    GUIKIT::BrowserWindow* fileDialogPtr = nullptr;
    
	GUIKIT::TabFrameLayout tabView;	
    
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
    CartCreatorLayout* flashCreatorLayout = nullptr;
        
    GUIKIT::SwitchLayout carts;    
    GUIKIT::HorizontalLayout cartWrapper;
    GUIKIT::FramedVerticalLayout cartSelectorFrame;
    GUIKIT::ListView cartList;
    GUIKIT::Button insertCart;
    GUIKIT::Button removeCart;
    std::vector<MediaGroupLayout*> cartLayouts;
    
    PathsLayout pathsLayout;
    	
	GUIKIT::Timer mtimer;
    GUIKIT::Timer ftimer;
    
    struct {
        MediaGroupLayout::Block* block = nullptr;
        std::string filePath;
    } lastPreview;

    auto build() -> void;	
    auto show() -> void;
	auto showDelayed() -> void;
    auto ident( std::string name ) -> std::string;
	static auto getView( Emulator::Interface* emulator ) -> MediaWindow*;
	
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
    auto createImage( Emulator::Interface::MediaGroup* mediaGroup ) -> void;
    auto showMediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup ) -> void;
    auto getMediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup ) -> MediaGroupLayout*;   
    auto insertImage( MediaGroupLayout::Block* block, GUIKIT::File* file, GUIKIT::File::Item* item ) -> void;
    auto insertImage( Emulator::Interface::Media* media, GUIKIT::File* file, GUIKIT::File::Item* item ) -> void;
    auto eject( Emulator::Interface::MediaGroup* mediaGroup ) -> void;
    auto drop( std::string filePath, MediaGroupLayout::Block* block = nullptr ) -> void;   
    auto colorListing( unsigned color, bool foreground ) -> void;
    auto getMediaGroupTransIdent( Emulator::Interface::MediaGroup* mediaGroup ) -> std::string;
    auto disableWriteProtection(Emulator::Interface::Media* media) -> void;
    auto updateJumper(Emulator::Interface::Media* media) -> void;
    auto changeWriteProtection(Emulator::Interface::Media* media, bool state) -> void;
    auto previewFile( std::string file, MediaGroupLayout::Block* block = nullptr ) -> std::vector<std::string>;
    auto getActiveLayout() -> MediaGroupLayout*;
    auto resetPreview(bool light = false) -> void;
    auto insertFile( MediaGroupLayout::Block* block, std::string filePath, bool autoLoad = false, unsigned selection = 0) -> bool;
    auto anyLoad() -> void;
    auto convertListing( std::vector<Emulator::Interface::Listing>& emuListings ) -> std::vector<std::string>;

    MediaWindow(Emulator::Interface* emulator);
};

}

extern std::vector<MediaView::MediaWindow*> mediaViews;
