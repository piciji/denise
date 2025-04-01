
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

    layout.append(tv, {~0u, ~0u}, 10);
    layout.append(loadArchiveNative, { 0u, 0u });
    layout.setMargin(10);

    append(layout);
	translate();

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

auto ArchiveViewer::setView(GUIKIT::File* file, std::vector<GUIKIT::File::Item>& items, bool multiSelection) -> void {
    this->filesSelected = 0;

    if (!builded) {
        build();
    }

    tv.onActivate = [this, file, multiSelection]() {
        auto item = tv.selected();
        if (!item) return;
        auto fileItem = (GUIKIT::File::Item*)item->userData();
        if (fileItem->isDirectory) return;

        if (item->text() == "-") return;

        if (!multiSelection)
            setVisible(false);
        else
            item->setText("-");

        if (file && onCallback)
            onCallback(file, fileItem);

        filesSelected++;
    };

    if (loadArchiveNative.enabled()) {
        std::vector<GUIKIT::File::Item>* pItems = &items;

        loadArchiveNative.onActivate = [this, file, pItems]() {
            if (!dynamic_cast<LIBAMI::Interface*>(activeEmulator))
                return;

            GUIKIT::File::Item* itemSelected = nullptr;
            auto _selected = tv.selected();
            if (_selected)
                itemSelected = (GUIKIT::File::Item*)_selected->userData();

            std::vector<GUIKIT::File::Item>& items = *pItems;
            std::vector<Emulator::Interface::Item> _items;
            _items.resize(items.size());

            for (auto& item : items) {
                Emulator::Interface::Item& _item = _items[item.id];
                _item.id = item.id;
                _item.name = item.info.name;
                _item.data.ptr = file->archiveData(item.id);
                _item.data.size = file->archiveDataSize(item.id);
                _item.isGroup = item.isDirectory;
                _item.parent = item.parent ? &_items[item.parent->id] : nullptr;
                _item.primary = itemSelected && !item.isDirectory && &item == itemSelected;

                for (auto child : item.childs)
                    _item.childs.push_back( &_items[child->id] );
            }

            auto fileName = file->getFileName(true, true);
            auto result = dynamic_cast<LIBAMI::Interface*>(activeEmulator)->buildDisk(fileName, _items);

            if (result.ptr) {
                std::string _path = program->generatedFolder(activeEmulator, "disksave_folder", "disksave", true);
                _path += fileName + ".adf";
                
                GUIKIT::File* newFile = filePool->get(_path);

                if (!newFile->open(GUIKIT::File::Mode::Write)) {
                    delete result.ptr;
                    return;
                }
                
                if (!newFile->write(result.ptr, result.size)) {
                    delete result.ptr;
                    return;
                }
                newFile->reset();
                auto& itemsNew = newFile->scanArchive();

                if (itemsNew.size()) {
                    file->unload();
                    setVisible(false);
                    if (onCallback)
                        onCallback(newFile, &itemsNew[0]);
                } else
                    newFile->unload();
            }
        };
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
            onCallback( file, fileCount == 0 ? nullptr : firstFile);
        
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
        setFocused(100);
    };
    mtimer.setEnabled();
}

auto ArchiveViewer::translate() -> void {
	setTitle( trans->getA("archive_selector") );
    loadArchiveNative.setText(trans->getA("load archive native"));
}

auto ArchiveViewer::showNativeArchive(bool state) -> void {
    loadArchiveNative.setEnabled(state);
}
