
#include "imageViewer.h"
#include "../program.h"
#include "../thread/emuThread.h"

auto ImageViewer::build() -> void {
    this->emulator = emulator;

    openFolder.setText( trans->getA("folder") );
    label.setText( trans->getA("select image to test shader") );

    verticalLayout.setMargin(10);
    verticalLayout.append(label, {0u, 0u}, 5);
    verticalLayout.append(listView, {~0u, ~0u}, 10);

    horizontalLayout.append(spacer, {~0u, 0u});
    horizontalLayout.append(openFolder, {0u, 0u});
    verticalLayout.append(horizontalLayout, {~0u, 0u});

    append(verticalLayout);

    openFolder.onActivate = [this]() {

        std::string filePath = GUIKIT::BrowserWindow()
            .setWindow(*this)
            .setTitle(trans->getA("folder"))
            .directory();

        if (filePath.empty())
            return;

        globalSettings->set<std::string>("image_view_path", filePath);

        updateList(filePath);
    };

    listView.onChange = [this]() {
        if (!listView.selected() || !activeVideoManager)
            return unload();

        auto selection = listView.selection();

        if (!selection)
            return unload();

        auto fileName = listView.text(selection, 0);

        std::string filePath = globalSettings->get<std::string>("image_view_path","");
        filePath = GUIKIT::File::beautifyPath(filePath);

        GUIKIT::File file( filePath + fileName );

        if (!file.open())
            return unload();

        uint8_t* data = file.read();

        if (!data)
            return unload();

        emuThread->lock();
        overrideImage.free();
        VideoManager::hidePlaceHolder();
        if (overrideImage.load( data, file.getSize()))
            VideoManager::placeHolderFrames = ~0;

        emuThread->unlock();
    };

    std::string filePath = globalSettings->get<std::string>("image_view_path","");

    if (!filePath.empty())
        updateList(filePath);
}

auto ImageViewer::unload() -> void {
    emuThread->lock();
    VideoManager::hidePlaceHolder();
    overrideImage.free();
    emuThread->unlock();
}

auto ImageViewer::updateList(const std::string& filePath) -> void {
    listView.reset();
    auto items = GUIKIT::File::getFolderListAlt( filePath, {".jpg", ".png", ".gif", ".bmp"}, false);
    if (items.empty())
        return;
  
    listView.append({ trans->getA("none") });

    for(auto& item : items) {
        listView.append({item});
    }
}
