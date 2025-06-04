
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

auto RecentFiles::add(Emulator::Interface::MediaGroup& group, const std::string& path) -> void {
    std::vector<std::string> out;
    if (path.empty())
        return;

    load();

    std::string line;
    out.push_back(path);

    for (int i = 0; i < maxEntries; i++) {
        line = settings->get<std::string>(getIdent(group, i), "");

        if (!line.empty()) {
            if (line != path)
                out.push_back(line);
        }
    }

    for (int i = 0; i < maxEntries; i++) {
        if (i >= out.size())
            settings->set<std::string>(getIdent(group, i), "");
        else
            settings->set<std::string>(getIdent(group, i), out[i]);
    }

    save();
}

auto RecentFiles::list(Emulator::Interface::MediaGroup& group, const std::string& curPath) -> std::vector<std::string> {
    std::vector<std::string> out;
    load();

    std::string line;

    bool found = curPath.empty();

    for (int i = 0; i < maxEntries; i++) {
        line = settings->get<std::string>(getIdent(group, i), "");
        if (!line.empty()) {
            if (!found && (line == curPath)) {
                found = true;
            }

            out.push_back(line);
        }
    }

    if (!found) {
        add(group, curPath);
        out.insert(out.begin(), curPath);
        if (out.size() > maxEntries)
            out.pop_back();
    }

    return out;
}


auto RecentFiles::getIdent(Emulator::Interface::MediaGroup& group, unsigned pos) -> std::string {
    return group.name + "_" + std::to_string(pos);
}
