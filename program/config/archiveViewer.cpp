
#include "archiveViewer.h"
#include "../program.h"
#include "../tools/filepool.h"

ArchiveViewer* archiveViewer = nullptr;

auto ArchiveViewer::build() -> void {
    
    GUIKIT::Geometry defaultGeometry = {100, 100, 400, 350};

    GUIKIT::Geometry geometry = {
        settings->get<int>("screen_archiveviewer_x", defaultGeometry.x)
        ,settings->get<int>("screen_archiveviewer_y", defaultGeometry.y)
        ,settings->get<unsigned>("screen_archiveviewer_width", defaultGeometry.width)
        ,settings->get<unsigned>("screen_archiveviewer_height", defaultGeometry.height)
    };

    setGeometry(geometry);

    if (isOffscreen())
        setGeometry(defaultGeometry);  

    #include "../../data/img/icons.data"
    imgFolderOpen.loadPng((uint8_t*)folderOpen, sizeof(folderOpen) );
    imgFolderClosed.loadPng((uint8_t*)folderClosed, sizeof(folderClosed) );
    imgDocument.loadPng((uint8_t*)document, sizeof(document) );

    layout.append(tv, {~0u, ~0u});
    layout.setMargin(10);

    append(layout);
	translate();

    tv.onActivate = [&]() {
        auto item = tv.selected();
        if (!item) return;
        auto fileItem = (GUIKIT::File::Item*)item->userData();
        if (fileItem->isDirectory) return;
        setVisible(false);
        
        if (onCallback)
            onCallback( fileItem );
    };

    onClose = [this]() {
        filePool->unloadOrphaned();
        setVisible(false);
    };

    onMove = [&]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        settings->set<int>("screen_archiveviewer_x", geometry.x);
        settings->set<int>("screen_archiveviewer_y", geometry.y);
    };

    onSize = [&]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        settings->set<unsigned>("screen_archiveviewer_width", geometry.width);
        settings->set<unsigned>("screen_archiveviewer_height", geometry.height);
    };
}

auto ArchiveViewer::setView(std::vector<GUIKIT::File::Item>& items) -> void {
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
    setVisible();
	setFocused();
}

auto ArchiveViewer::translate() -> void {
	setTitle( trans->get("archive_selector") );
}
