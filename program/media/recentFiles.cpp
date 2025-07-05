
#include "recentFiles.h"
#include "../../guikit/api.h"

RecentFiles::RecentFiles(Emulator::Interface* emulator, const std::string& path) {
    this->emulator = emulator;
    this->path = path;
    settings = nullptr;
}

RecentFiles::~RecentFiles() {
    if (settings)
        delete settings;

    for(auto s : storage)
        delete s;
}

auto RecentFiles::load() -> void {
    if (settings)
        return;

    settings = new GUIKIT::Settings;

    if (settings->load(path)) {

    }
}

auto RecentFiles::save() -> void {
    if (!settings)
        return;

    settings->save(path);
}

auto RecentFiles::add(Emulator::Interface::MediaGroup* group, const std::string& curPath) -> void {
    if (curPath.empty())
        return;

    load();

    auto s = getStorage(group);    
    s->files.clear();
    s->files.reserve(maxEntries);

    std::string line;
    s->files.push_back(curPath);

    for (int i = 0; i < maxEntries; i++) {
        line = settings->get<std::string>(getIdent(group, i), "");

        if (!line.empty()) {
            if (line != curPath)
                s->files.push_back(line);
        }

        if (s->files.size() >= maxEntries)
            break;
    }

    for (int i = 0; i < maxEntries; i++) {
        if (i >= s->files.size())
            settings->set<std::string>(getIdent(group, i), "");
        else
            settings->set<std::string>(getIdent(group, i), s->files[i]);
    }

    if (group) {
        add(nullptr, curPath);
        save();
    }
}

auto RecentFiles::list(Emulator::Interface::MediaGroup* group, const std::string& curPath) -> std::vector<std::string>& {
    load();

    auto s = getStorage(group);

    if (curPath.empty())
        return s->files;

    for(auto& file : s->files) {
        if (file == curPath)
            return s->files;
    }

    add(group, curPath);

    return s->files;
}

auto RecentFiles::getIdent(Emulator::Interface::MediaGroup* group, unsigned pos) -> std::string {
    std::string out = group ? group->name : "recent";
    return out + "_" + std::to_string(pos);
}

auto RecentFiles::getStorage(Emulator::Interface::MediaGroup* group) -> Storage* {
    for(auto _s : storage) {
        if (_s->group == group)
            return _s;
    }

    Storage* s = new Storage;
    s->group = group;
    s->files.clear();
    s->files.reserve(maxEntries);
    std::string line;

    for (int i = 0; i < maxEntries; i++) {
        line = settings->get<std::string>(getIdent(group, i), "");

        if (!line.empty())
            s->files.push_back(line);
    }
    storage.push_back(s);
    return s;
}

auto RecentFiles::clear(Emulator::Interface::MediaGroup* group) -> void {
    for (int i = 0; i < maxEntries; i++)
        settings->remove(getIdent(group, i));

    save();
    
    for (auto _s : storage) {
        if (_s->group == group) {
            _s->files.clear();
            _s->files.reserve(maxEntries);
            break;
        }
    }
}
