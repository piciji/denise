
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

    if (settings->load(path))
        genericEntries = settings->get<unsigned>("generic_entries", 25, {5, maxEntries});
}

auto RecentFiles::save() -> void {
    if (!settings)
        return;

    settings->save(path);
}

auto RecentFiles::add(Emulator::Interface::MediaGroup* group, bool alternate, const std::string& curPath, bool updateGeneric) -> void {
    if (curPath.empty())
        return;

    load();

    auto s = getStorage(group, alternate);
    s->files.clear();
    unsigned entries = getEntries(group);
    s->files.reserve(entries);

    std::string line;
    s->files.push_back(curPath);    

    for (int i = 0; i < entries; i++) {
        line = settings->get<std::string>(getIdent(group, alternate, i), "");

        if (!line.empty()) {
            if (line != curPath)
                s->files.push_back(line);
        }

        if (s->files.size() >= entries)
            break;
    }

    for (int i = 0; i < entries; i++) {
        if (i >= s->files.size())
            settings->set<std::string>(getIdent(group, alternate, i), "");
        else
            settings->set<std::string>(getIdent(group, alternate, i), s->files[i]);
    }

    if (group && updateGeneric)
        add(nullptr, false, curPath);
}

auto RecentFiles::list(Emulator::Interface::MediaGroup* group, bool alternate, const std::string& curPath) -> std::vector<std::string>& {
    load();

    auto s = getStorage(group, alternate);

    if (curPath.empty())
        return s->files;

    for(auto& file : s->files) {
        if (file == curPath)
            return s->files;
    }

    add(group, alternate, curPath, false);

    return s->files;
}

auto RecentFiles::getIdent(Emulator::Interface::MediaGroup* group, bool alternate, unsigned pos) -> std::string {
    std::string out = group ? group->name : "recent";
    if (alternate)
        out += "_alt";
    return out + "_" + std::to_string(pos);
}

auto RecentFiles::getStorage(Emulator::Interface::MediaGroup* group, bool alternate) -> Storage* {
    for(auto _s : storage) {
        if (_s->group == group && _s->alternate == alternate)
            return _s;
    }

    Storage* s = new Storage;
    s->group = group;
    s->alternate = alternate;
    s->files.clear();
    unsigned entries = getEntries(group);
    s->files.reserve(entries);
    std::string line;

    for (int i = 0; i < entries; i++) {
        line = settings->get<std::string>(getIdent(group, alternate, i), "");

        if (!line.empty())
            s->files.push_back(line);
    }
    storage.push_back(s);
    return s;
}

auto RecentFiles::clear(Emulator::Interface::MediaGroup* group, bool alternate) -> void {
    load();
    for (int i = 0; i < maxEntries; i++)
        settings->remove(getIdent(group, alternate, i));
    
    for (auto _s : storage) {
        if (_s->group == group && _s->alternate == alternate) {
            _s->files.clear();
            break;
        }
    }
}

auto RecentFiles::getEntries(Emulator::Interface::MediaGroup* group) -> unsigned {
    load();
    return std::min(maxEntries, group ? groupEntries : genericEntries);
}

auto RecentFiles::setGenericEntries(unsigned entries) -> void {
    if (entries > maxEntries)
        entries = maxEntries;

    load();
    if (settings) {
        settings->set<unsigned>("generic_entries", entries);
        genericEntries = entries;
    }
}
