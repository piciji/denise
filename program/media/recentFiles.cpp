
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
        genericEntries = settings->get<unsigned>("generic_entries", 25, { 5, MAX_RECENT_ENTIRIES });
}

auto RecentFiles::save() -> void {
    if (!settings)
        return;

    settings->save(path);
}

auto RecentFiles::add(Emulator::Interface::MediaGroup* group, bool alternate, const FileIdent& fileIdent, bool updateGeneric) -> void {
    if (fileIdent.path.empty())
        return;

    load();

    auto s = getStorage(group, alternate);
    s->files.clear();
    unsigned entries = getEntries(group);
    s->files.reserve(entries);
    s->files.push_back(fileIdent);
    FileIdent fiCur;

    for (int i = 0; i < entries; i++) {
        fiCur.path = settings->get<std::string>(getIdentPath(group, alternate, i), "");

        if (!fiCur.path.empty()) {
            fiCur.id = settings->get<unsigned>(getIdentId(group, alternate, i), 0);

            if (fiCur.path != fileIdent.path || fiCur.id != fileIdent.id) {
                fiCur.file = settings->get<std::string>(getIdentFile(group, alternate, i), GUIKIT::String::getFileName(fiCur.path ));
                s->files.push_back(fiCur);
            }
        }

        if (s->files.size() >= entries)
            break;
    }

    for (int i = 0; i < entries; i++) {
        if (i >= s->files.size()) {
            settings->set<std::string>(getIdentPath(group, alternate, i), "");
            settings->set<std::string>(getIdentFile(group, alternate, i), "");
            settings->set<unsigned>(getIdentId(group, alternate, i), 0);
        } else {
            settings->set<std::string>(getIdentPath(group, alternate, i), s->files[i].path);
            settings->set<std::string>(getIdentFile(group, alternate, i), s->files[i].file);
            settings->set<unsigned>(getIdentId(group, alternate, i), s->files[i].id);
        }
    }

    if (group && updateGeneric)
        add(nullptr, false, fileIdent);
}

auto RecentFiles::list(Emulator::Interface::MediaGroup* group, bool alternate, const FileIdent& fileIdent) -> std::vector<FileIdent>& {
    load();

    auto s = getStorage(group, alternate);

    if (fileIdent.path.empty())
        return s->files;

    for(auto& fiCur : s->files) {
        if (fiCur.path == fileIdent.path && fiCur.id == fileIdent.id)
            return s->files;
    }

    add(group, alternate, fileIdent, false);

    return s->files;
}

inline auto RecentFiles::getIdentBase(Emulator::Interface::MediaGroup* group, bool alternate) -> std::string {
    std::string out = group ? group->name : "recent";
    if (alternate)
        out += "_alt";
    return out;
}

auto RecentFiles::getIdentId(Emulator::Interface::MediaGroup* group, bool alternate, unsigned pos) -> std::string {
    return getIdentBase(group, alternate) + "_id_" + std::to_string(pos);
}

auto RecentFiles::getIdentFile(Emulator::Interface::MediaGroup* group, bool alternate, unsigned pos) -> std::string {
    return getIdentBase(group, alternate) + "_file_" + std::to_string(pos);
}

auto RecentFiles::getIdentPath(Emulator::Interface::MediaGroup* group, bool alternate, unsigned pos) -> std::string {
    return getIdentBase(group, alternate) + "_" + std::to_string(pos);
}

auto RecentFiles::getStorage(Emulator::Interface::MediaGroup* group, bool alternate) -> Storage* {
    for(auto _s : storage) {
        if (_s->group == group && _s->alternate == alternate)
            return _s;
    }

    auto* s = new Storage;
    s->group = group;
    s->alternate = alternate;
    s->files.clear();
    unsigned entries = getEntries(group);
    s->files.reserve(entries);
    std::string line;
    FileIdent fileIdent;

    for (int i = 0; i < entries; i++) {
        fileIdent.path = settings->get<std::string>(getIdentPath(group, alternate, i), "");

        if (!fileIdent.path.empty()) {
            fileIdent.file = settings->get<std::string>(getIdentFile(group, alternate, i), GUIKIT::String::getFileName(fileIdent.path) );
            fileIdent.id = settings->get<unsigned>(getIdentId(group, alternate, i), 0);
            s->files.push_back(fileIdent);
        }
    }
    storage.push_back(s);
    return s;
}

auto RecentFiles::clear(Emulator::Interface::MediaGroup* group, bool alternate) -> void {
    load();
    for (int i = 0; i < MAX_RECENT_ENTIRIES; i++) {
        settings->remove(getIdentPath(group, alternate, i));
        settings->remove(getIdentFile(group, alternate, i));
        settings->remove(getIdentId(group, alternate, i));
    }
    
    for (auto _s : storage) {
        if (_s->group == group && _s->alternate == alternate) {
            _s->files.clear();
            break;
        }
    }
}

auto RecentFiles::getEntries(Emulator::Interface::MediaGroup* group) -> unsigned {
    load();
    return std::min(MAX_RECENT_ENTIRIES, group ? groupEntries : genericEntries);
}

auto RecentFiles::setGenericEntries(unsigned entries) -> void {
    if (entries > MAX_RECENT_ENTIRIES)
        entries = MAX_RECENT_ENTIRIES;

    load();
    if (settings) {
        settings->set<unsigned>("generic_entries", entries);
        genericEntries = entries;
    }
}
