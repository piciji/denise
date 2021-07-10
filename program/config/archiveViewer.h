
#pragma once

#include "../../guikit/api.h"

struct ArchiveViewer : public GUIKIT::Window {
    GUIKIT::VerticalLayout layout;
    GUIKIT::TreeView tv;
    std::vector<GUIKIT::TreeViewItem*> itemList;
    std::function<void (GUIKIT::File::Item*)> onCallback = nullptr;

    GUIKIT::Image imgFolderOpen;
    GUIKIT::Image imgFolderClosed;
    GUIKIT::Image imgDocument;
    bool builded = false;

    auto build() -> void;
    auto setView(std::vector<GUIKIT::File::Item>& items) -> void;
	auto translate() -> void;
};

extern ArchiveViewer* archiveViewer;
