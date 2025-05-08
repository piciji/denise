
#pragma once

#include "../../guikit/api.h"
#include "../../emulation/interface.h"

struct ArchiveViewer : public GUIKIT::Window {
    GUIKIT::VerticalLayout layout;
    GUIKIT::TreeView tv;
    GUIKIT::Button loadArchiveNative;
    std::vector<GUIKIT::TreeViewItem*> itemList;
    std::function<void (GUIKIT::File* file, GUIKIT::File::Item*)> onCallback = nullptr;

    GUIKIT::Image imgFolderOpen;
    GUIKIT::Image imgFolderClosed;
    GUIKIT::Image imgDocument;
    bool builded = false;
    bool geometryInitialized = false;
    Emulator::Interface::MediaGroup* nativeGroup;

    unsigned filesSelected = 0;

    GUIKIT::Timer mtimer;

    auto build() -> void;
    auto setView(GUIKIT::File* file, std::vector<GUIKIT::File::Item>& items, bool multiSelection = false) -> void;
	auto translate() -> void;
    auto allowNativeArchive(Emulator::Interface::MediaGroup* group) -> void;
    auto buildMedia(GUIKIT::File* file, std::vector<GUIKIT::File::Item>& items) -> void;
};

extern ArchiveViewer* archiveViewer;
