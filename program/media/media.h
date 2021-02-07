
#pragma once

struct Message;
struct FileSetting;

#include "../../guikit/api.h"
#include "../program.h"
#include "../emuconfig/config.h"

namespace MediaView {

struct MediaLayout;
struct MediaGroupLayout;

struct NavElement {
    GUIKIT::TreeViewItem* tvi;
    MediaGroupLayout* mediaGroupLayout;
    GUIKIT::Layout* altLayout;
};

struct SwapperControlLayout : GUIKIT::HorizontalLayout {
	GUIKIT::CheckBox writeProtect;
    GUIKIT::Widget spacer;    
    GUIKIT::Button openButton;
    GUIKIT::Button ejectButton;    
    
    SwapperControlLayout();
};

struct SwapperLayout : GUIKIT::VerticalLayout {

    MediaLayout* mediaLayout;
    Emulator::Interface* emulator;
    unsigned switchId;
    
    GUIKIT::ListView listView;
    SwapperControlLayout controls;

    auto translate() -> void;
	auto getSetting( unsigned pos ) -> FileSetting*;
    auto preselectPath( ) -> std::string;
	auto savePath( std::string path ) -> void;
    auto loadSettings() -> void;
    
    SwapperLayout(MediaLayout* mediaLayout);
};

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
    auto getBlock(Emulator::Interface::MediaGroup* mediaGroup) -> Block*;
    
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
    MediaLayout* mediaLayout;
    
    auto build() -> void;
    auto updateVisibility( unsigned count, bool init = false ) -> void;    
    auto updateListing(MediaGroupLayout::Block* block) -> void;
    auto fillListing( std::vector<Emulator::Interface::Listing>& emuListings ) -> void;
    auto fillListing( std::vector<GUIKIT::BrowserWindow::Listing>& emuListings ) -> void;
    auto showOnlyConnectedDevices() -> bool;
    auto getBlock(Emulator::Interface::Media* media) -> Block*;
    auto applyFont(unsigned fontSize) -> void;
    auto setJumperSettings(Emulator::Interface::Media* media) -> void;
    auto loadSettings() -> void;

    MediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup, MediaLayout* mediaLayout );
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

struct FlashCreatorLayout : GUIKIT::FramedHorizontalLayout {	
    GUIKIT::ComboButton format;
    GUIKIT::Button button;
	
	FlashCreatorLayout();
};

struct EpromCreatorLayout : GUIKIT::FramedHorizontalLayout {
    GUIKIT::ComboButton format;
    GUIKIT::Button button;

    EpromCreatorLayout();
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

struct MediaLayout : GUIKIT::HorizontalLayout {
    
    EmuConfigView::TabWindow* tabWindow;
    Emulator::Interface* emulator;
    
    GUIKIT::Settings* settings;
    bool useCustomFont = false;
	Message* message;
    GUIKIT::Setting* alternateFileDialog = nullptr;
    GUIKIT::TreeViewItem* expansionParent = nullptr;
	
    GUIKIT::BrowserWindow* fileDialogPtr = nullptr;    
    
	GUIKIT::Image diskImage;
    GUIKIT::Image hdImage;
    GUIKIT::Image tapeImage;
    GUIKIT::Image expansionImage;
	GUIKIT::Image memoryImage;
	GUIKIT::Image addImage;    
    GUIKIT::Image pathImage;
    GUIKIT::Image swapperImage;
    GUIKIT::Image imgFolderOpen;
    GUIKIT::Image imgFolderClosed;
    GUIKIT::Image imgDocument;
    
    std::vector<NavElement> navElements;
    
	GUIKIT::VerticalLayout creatorLayout;    
    TapeCreatorLayout* tapeCreatorLayout = nullptr;
    HdCreatorLayout* hdCreatorLayout = nullptr;
    DiskCreatorLayout* diskCreatorLayout = nullptr;
	MemoryCreatorLayout* memoryCreatorLayout = nullptr;
    FlashCreatorLayout* flashCreatorLayout = nullptr;
    EpromCreatorLayout* epromCreatorLayout = nullptr;
                   
    GUIKIT::FramedVerticalLayout moduleFrame;
    GUIKIT::SwitchLayout moduleSwitch;
    GUIKIT::TreeView mediaTree;
    GUIKIT::Button bootCart;
    GUIKIT::Button deactivateCart;
    
    PathsLayout pathsLayout;
    SwapperLayout* swapperLayout = nullptr;
    	
    GUIKIT::Timer ftimer;
    
    struct {
        MediaGroupLayout::Block* block = nullptr;
        std::string filePath;
    } lastPreview;

    auto build() -> void;	
    auto show() -> void;
    auto showDiskSwapper() -> void;
    auto updateSwitchLayout() -> void;
	
	auto translate() -> void;
    auto updateMediaBlock(MediaGroupLayout::Block* block, FileSetting* fSetting) -> void;
    auto updateVisibility( Emulator::Interface::MediaGroup* mediaGroup, unsigned count ) -> void;
    auto updateExpansionBootButtonVisibility() -> void;
    auto bindSelectorAction( MediaGroupLayout* layout ) -> void;
    auto prepareCreator() -> void;
    auto preparePaths() -> void;	
    auto updateListing( Emulator::Interface::Media* media ) -> void;
	auto preselectPath( std::string& groupName ) -> std::string;
	auto savePath( std::string& groupName, std::string path ) -> void;
    auto showC64Listing( MediaGroupLayout* layout ) -> bool;
    auto createImage( Emulator::Interface::MediaGroup* mediaGroup, bool secondaryRom = false ) -> void;
    auto showMediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup ) -> void;
    auto getMediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup ) -> MediaGroupLayout*;   
    auto insertImage( MediaGroupLayout::Block* block, GUIKIT::File* file, GUIKIT::File::Item* item ) -> void;
    auto insertImage( Emulator::Interface::Media* media, GUIKIT::File* file, GUIKIT::File::Item* item ) -> void;
    auto eject( Emulator::Interface::MediaGroup* mediaGroup, bool secondaryOnly = false ) -> void;
    auto drop( std::string filePath, MediaGroupLayout::Block* block = nullptr ) -> void;   
    auto colorListing( unsigned color, bool foreground ) -> void;
    auto getMediaGroupTransIdent( Emulator::Interface::MediaGroup* mediaGroup ) -> std::string;
    auto updateJumper(Emulator::Interface::Media* media) -> void;
    auto updateWriteProtection( Emulator::Interface::Media* media, bool state ) -> void;
    auto previewFile( std::string file, MediaGroupLayout::Block* block = nullptr ) -> std::vector<GUIKIT::BrowserWindow::Listing>;
    auto getActiveLayout() -> MediaGroupLayout*;
    auto resetPreview(bool light = false) -> void;
    auto insertFile( MediaGroupLayout::Block* block, std::string filePath, bool autoLoad = false, unsigned selection = 0) -> bool;
    auto anyLoad( bool mIsAcquiredBefore ) -> void;
    auto convertListing( std::vector<Emulator::Interface::Listing>& emuListings, bool loadCommand ) -> std::vector<std::string>;
	auto convertListing( std::vector<Emulator::Interface::Listing>& emuListings ) -> std::vector<GUIKIT::BrowserWindow::Listing>;
    auto updateListingFont( unsigned fontSize ) -> void;
    auto updateListings( ) -> void;
    auto applyPreviewFont(unsigned fontSize) -> void;
    auto loadSettings() -> void;

    MediaLayout(EmuConfigView::TabWindow* tabWindow);
};

}

