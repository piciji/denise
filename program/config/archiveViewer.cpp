
#include "archiveViewer.h"
#include "../program.h"
#include "../tools/filepool.h"
#include "../../data/icons.h"
#include "../view/view.h"

ArchiveViewer* archiveViewer = nullptr;

auto ArchiveViewer::build() -> void {
    cocoa.keepMenuVisibilityOnDisplay();

    imgFolderOpen.loadPng((uint8_t*)Icons::folderOpen, sizeof(Icons::folderOpen) );
    imgFolderClosed.loadPng((uint8_t*)Icons::folderClosed, sizeof(Icons::folderClosed) );
    imgDocument.loadPng((uint8_t*)Icons::document, sizeof(Icons::document) );

    layout.append(tv, {~0u, ~0u});
    layout.setMargin(10);

    append(layout);
	translate();

    tv.onActivate = [&]() {
        auto item = tv.selected();
        if (!item) return;
        auto fileItem = (GUIKIT::File::Item*)item->userData();
        if (fileItem->isDirectory) return;

        if (item->text() == "-") return;

        if (!multiSelection)
            setVisible(false);
        else
            item->setText("-");
        
        if (onCallback)
            onCallback( fileItem );

        filesSelected++;
    };

    onClose = [this]() {
        filePool->unloadOrphaned();
        setVisible(false);
        view->setFocused(100);
    };

    onMove = [&]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        globalSettings->set<int>("screen_archiveviewer_x", geometry.x);
        globalSettings->set<int>("screen_archiveviewer_y", geometry.y);
    };

    onSize = [&](GUIKIT::Window::SIZE_MODE sizeMode) {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        globalSettings->set<unsigned>("screen_archiveviewer_width", geometry.width);
        globalSettings->set<unsigned>("screen_archiveviewer_height", geometry.height);
    };

    builded = true;
}

auto ArchiveViewer::setView(std::vector<GUIKIT::File::Item>& items, bool multiSelection) -> void {
    this->multiSelection = multiSelection;
    this->filesSelected = 0;

    if (!builded) {
        build();
    }

    unsigned fileCount = 0;
    GUIKIT::File::Item* firstFile = nullptr;
    tv.reset();
    for(auto& item : itemList) delete item;
    itemList.clear();

    for( auto& item : items ) {
        if(item.parent) continue;

        std::function<GUIKIT::TreeViewItem* (GUIKIT::File::Item&)> addItem;

        addItem = [&](GUIKIT::File::Item& item) -> GUIKIT::TreeViewItem* {
            auto tvi = new GUIKIT::TreeViewItem;
            itemList.push_back(tvi);
            std::string txt = item.info.name;
            if (!item.isDirectory) {
                txt += " [" + GUIKIT::File::SizeFormated( item.info.size ) + "]";
            }
            tvi->setText( txt );
            tvi->setUserData( (uintptr_t)&item );

            for(auto child : item.childs) {
                GUIKIT::TreeViewItem* citem = addItem(*child);
                tvi->append(*citem);
            }
            if (item.isDirectory) {
                tvi->setImage(imgFolderClosed);
                tvi->setImageSelected(imgFolderOpen);
            } else {
                tvi->setImage(imgDocument);
                fileCount++;
                if (!firstFile) firstFile = &item;
            }
            return tvi;
        };

        GUIKIT::TreeViewItem* tvi = addItem(item);
        tv.append(*tvi);
    }

    if(fileCount <= 1) {
        setVisible(false);
        if (onCallback)
            onCallback( fileCount == 0 ? nullptr : firstFile);
        
        return;
    }

    if (!geometryInitialized) {
        GUIKIT::Geometry defaultGeometry = {100, 100, 400, 350};
        GUIKIT::Geometry geometry = {
                globalSettings->get<int>("screen_archiveviewer_x", defaultGeometry.x)
                ,globalSettings->get<int>("screen_archiveviewer_y", defaultGeometry.y)
                ,globalSettings->get<unsigned>("screen_archiveviewer_width", defaultGeometry.width)
                ,globalSettings->get<unsigned>("screen_archiveviewer_height", defaultGeometry.height)
        };
        setGeometry(geometry);
        if (isOffscreen())
            setGeometry(defaultGeometry);
        geometryInitialized = true;
    }

    mtimer.setInterval(30);

    mtimer.onFinished = [this]() {
        mtimer.setEnabled(false);
        setVisible();
        setFocused();
    };
    mtimer.setEnabled();
}

auto ArchiveViewer::translate() -> void {
	setTitle( trans->get("archive_selector") );
}
