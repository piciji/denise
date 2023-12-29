
#include "shaderParser.h"
#include <cstdio>

auto ShaderParser::loadPreset(std::string path) -> bool {
    clear();
    entryPaths.push_back(path);
    bool res = rootSettings.loadEx(path);
    GUIKIT::Vector::combine(brokenPaths, rootSettings.getBrokenPaths());

    if (!res)
        return false;

    // we use the first found file which includes ident "shaders"
    // we don't check if there is another file that includes it.
    if (!findRootConfig(rootSettings)) {
        createSinglePass(path);
        return true;
    }


    int passCount = rootSettings.get<unsigned>("shaders", 0);
    if (!passCount) {
        createSinglePass(path);
        return true;
    }

    shaderPreset.feedback = rootSettings.get<int>("feedback", -1);

    for(int i = 0; i < passCount; i++) {
        if (!parsePass(i)) {

        }
    }

    if (shaderPreset.passes.empty()) { // wtf
        createSinglePass(path);
        return true;
    }

    if (!parseTextures()) {

    }

    for(int i = 0; i < shaderPreset.passes.size(); i++) {
        ShaderPreset::Pass& pass = shaderPreset.passes[i];
        fetchParameters(pass.src, i);
    }

    if (path != rootSettings.getPath()) { // there is at least one reference to root config
        std::vector<GUIKIT::Settings*> settingsList;
        applyOverrides(path, settingsList);
    }

    return true;
}

auto ShaderParser::savePreset(std::string path) -> bool {
    std::string out;
    GUIKIT::File file(path);
    if(!file.open(GUIKIT::File::Mode::Write, true))
        return false;

    auto fp = file.getHandle();

    if (!modified && (entryPaths.size() == 1)) {
        out = "#reference \"" + GUIKIT::File::buildRelativePath(path, entryPaths[0]) + "\"\n";
        fputs( out.c_str(), fp );
    } else {
        writeLine(fp, "shaders", std::to_string(shaderPreset.passes.size()) + "\n");
        if (shaderPreset.feedback >= 0)
            writeLine(fp, "feedback", std::to_string(shaderPreset.feedback) );

        for(int i = 0; i < shaderPreset.passes.size(); i++) {
            auto& pass = shaderPreset.passes[i];
            if (!pass.inUse)
                continue;

            writeLine(fp, i, "shader", GUIKIT::File::buildRelativePath(path, pass.src));
            writeLine(fp, i, "filter_linear", pass.filter == ShaderPreset::FILTER_NEAREST ? "false" : "true");
            writeLine(fp, i, "wrap_mode", translateWrapMode(pass.wrap));
            writeLine(fp, i, "mipmap_input", pass.mipmap ? "true" : "false");
            writeLine(fp, i, "alias", pass.alias);
            writeLine(fp, i, "float_framebuffer", pass.bufferType == ShaderPreset::BUFFER_FP ? "true" : "false");
            writeLine(fp, i, "srgb_framebuffer", pass.bufferType == ShaderPreset::BUFFER_SRGB ? "true" : "false");
            writeLine(fp, i, "scale_type_x", translateScaleType(pass.scaleTypeX));
            writeLine(fp, i, "scale_x", pass.scaleTypeX == ShaderPreset::SCALE_ABSOLUTE ? std::to_string(pass.absX) : std::to_string(pass.scaleX) );
            writeLine(fp, i, "scale_type_y", translateScaleType(pass.scaleTypeY));
            writeLine(fp, i, "scale_y", pass.scaleTypeY == ShaderPreset::SCALE_ABSOLUTE ? std::to_string(pass.absY) : std::to_string(pass.scaleY) );
            if (pass.frameModulo)
                writeLine(fp, i, "frame_count_mod", std::to_string(pass.frameModulo));
            fputs( "\n", fp );
        }

        if (shaderPreset.luts.size()) {
            out = "textures = \"";
            for(auto& lut : shaderPreset.luts)
                out += lut.id + ";";

            out = out.substr(0, out.size() - 1);
            out += "\"\n";
            fputs( out.c_str(), fp );

            for(auto& lut : shaderPreset.luts) {
                writeLine(fp, lut.id, GUIKIT::File::buildRelativePath(path, lut.path) );
                writeLine(fp, lut.id + "_linear", lut.filter == ShaderPreset::FILTER_NEAREST ? "false" : "true" );
                writeLine(fp, lut.id + "_wrap_mode",  translateWrapMode(lut.wrap) );
                writeLine(fp, lut.id + "_mipmap",  lut.mipmap ? "true" : "false" );
            }
        }
    }

    for (auto& param : shaderPreset.params) {
        if (param.initial != param.value) {
            out = param.id + " = " + std::to_string(param.value) + "\n";
            fputs( out.c_str(), fp );
        }
    }

    modified = false;
    entryPaths.clear();
    entryPaths.push_back(path);
    return true;
}

inline auto ShaderParser::writeLine(FILE* fp, std::string key, std::string value) -> void {
    std::string out = key + " = " + value + "\n";
    fputs( out.c_str(), fp );
}

inline auto ShaderParser::writeLine(FILE* fp, unsigned passId, std::string key, std::string value) -> void {
    std::string out = key + std::to_string(passId) + " = " + value + "\n";
    fputs( out.c_str(), fp );
}

auto ShaderParser::createSinglePass(std::string path) -> void {
    ShaderPreset::Pass pass;
    pass.src = path;
    pass.filter = ShaderPreset::Filter::FILTER_UNSPEC;
    pass.scaleTypeX = ShaderPreset::ScaleType::SCALE_INPUT;
    pass.scaleTypeY = ShaderPreset::ScaleType::SCALE_INPUT;
    pass.wrap = ShaderPreset::WrapMode::WRAP_EDGE;
    pass.frameModulo = 0;
    pass.bufferType = ShaderPreset::BufferType::BUFFER_UNORM;
    pass.mipmap = false;
    pass.alias = "";
    pass.scaleX = 1.0;
    pass.scaleY = 1.0;
    pass.absX = 0;
    pass.absY = 0;
    pass.inUse = true;
    shaderPreset.passes.push_back(pass);
}

auto ShaderParser::fetchParameters(std::string path, int passId, int depth) -> void {
    std::string line;
    int filled;
    std::string prefix = "#include";

    if (depth > 16)
        return;

    GUIKIT::File file(path);
    if(!file.open() || !file.getSize()) {
        brokenPaths.push_back(path);
        return;
    }

    auto fp = file.getHandle();

    char chunk[1024];
    while ( fgets(chunk, sizeof(chunk), fp) ) {
        line = chunk;
        GUIKIT::String::remove(line, { "\r\n", "\n" });

        if ((line.size() > prefix.size()) && std::equal(prefix.begin(), prefix.end(), line.begin())) {
            GUIKIT::String::remove(line, {prefix});
            GUIKIT::String::trim(line);
            GUIKIT::String::removeQuote(line);
            fetchParameters( GUIKIT::File::resolveRelativePath(path, line), passId, depth + 1);
            continue;
        }

        if (!GUIKIT::String::foundSubStr(line, "#pragma parameter"))
            continue;

        ShaderPreset::Param param;

        char id[64];
        char desc[64];
        if ((filled = sscanf_s(line.c_str(), "#pragma parameter %63s \"%63[^\"]\" %f %f %f %f",
                                  id, sizeof(id), desc, sizeof(desc), &param.initial, &param.minimum, &param.maximum, &param.step)) < 5)
            continue;

        param.id = id;
        param.desc = desc;

        if (GUIKIT::String::foundSubStr(param.id, "EMPTY_LINE"))
            continue;

        GUIKIT::String::trim(param.desc);

        if (filled == 5)
            param.step  = 0.1f * (param.maximum - param.minimum);

        if (param.isDescriptor() && param.desc.empty())
            continue;

        param.pass = passId;

        if (rootSettings.find(param.id))
            param.value = rootSettings.get<float>(param.id);
        else
            param.value = param.initial;

        addParameter(param);
    }
}

auto ShaderParser::addParameter(ShaderPreset::Param& param) -> void {
    for (auto& _param : shaderPreset.params) {
        if (_param.id == param.id) {
            _param.desc = param.desc;
            _param.value = param.value;
            _param.initial = param.initial;
            _param.maximum = param.maximum;
            _param.minimum = param.minimum;
            _param.step = param.step;
            _param.pass = param.pass;
            return;
        }
    }

    shaderPreset.params.push_back(param);
}

auto ShaderParser::parseTextures() -> bool {
    std::string lutList = rootSettings.get<std::string>("textures", "");

    if (lutList.empty())
        return true;

    auto parts = GUIKIT::String::explode(lutList, ";");
    shaderPreset.luts.reserve( parts.size() );

    for(auto& id : parts) {
        if (!rootSettings.find(id))
            continue;

        ShaderPreset::Lut lut;
        lut.id = id;
        lut.path = rootSettings.get<std::string>(id, "");
        if (lut.path.empty())
            continue;
        lut.path = GUIKIT::File::resolveRelativePath( rootSettings.getPath(), lut.path );

        int filter = -1;
        if (rootSettings.find(id + "_linear"))
            filter = (int)rootSettings.get<bool>(id + "_linear", false);

        lut.filter = translateFilter(filter);
        lut.mipmap = rootSettings.get<bool>(id + "_mipmap", false);
        lut.wrap = translateWrapMode( rootSettings.get<std::string>(id + "_wrap_mode", "") );

        shaderPreset.luts.push_back(lut);
    }
    return true;
}

auto ShaderParser::parsePass(unsigned pos) -> bool {
    ShaderPreset::Pass pass;
    std::string strPos = std::to_string(pos);
    std::string path = rootSettings.get<std::string>("shader" + strPos, "");
    if (!path.empty())
        path = GUIKIT::File::resolveRelativePath( rootSettings.getPath(), path);

    pass.src = path;
    pass.inUse = true;

    int filter = -1;
    if (rootSettings.find("filter_linear" + strPos))
        filter = (int)rootSettings.get<bool>("filter_linear" + strPos, false);

    pass.filter = translateFilter(filter);
    pass.wrap = translateWrapMode( rootSettings.get<std::string>("wrap_mode" + strPos, "") );
    pass.frameModulo = rootSettings.get<unsigned>("frame_count_mod" + strPos, 0);

    if (rootSettings.get<bool>("srgb_framebuffer" + strPos, false))
        pass.bufferType = ShaderPreset::BUFFER_SRGB;
    else if (rootSettings.get<bool>("float_framebuffer" + strPos, false))
        pass.bufferType = ShaderPreset::BUFFER_FP;
    else
        pass.bufferType = ShaderPreset::BUFFER_UNORM;

    pass.mipmap = rootSettings.get<bool>("mipmap_input" + strPos, false);
    pass.alias = rootSettings.get<std::string>("alias" + strPos, "");

    pass.scaleTypeX = ShaderPreset::SCALE_INPUT;
    pass.scaleTypeY = ShaderPreset::SCALE_INPUT;
    pass.scaleX = 1.0;
    pass.scaleY = 1.0;

    std::string scaleType = rootSettings.get<std::string>("scale_type" + strPos, "");
    std::string scaleTypeX = rootSettings.get<std::string>("scale_type_x" + strPos, "");
    std::string scaleTypeY = rootSettings.get<std::string>("scale_type_y" + strPos, "");

    if (!scaleType.empty()) {
        scaleTypeX = scaleType;
        scaleTypeY = scaleType;
    } else if (scaleTypeX.empty() && scaleTypeY.empty()) {
        shaderPreset.passes.push_back(pass);
        return true;
    }

    if (!scaleTypeX.empty())
        pass.scaleTypeX = translateScaleType(scaleTypeX);

    if (!scaleTypeY.empty())
        pass.scaleTypeY = translateScaleType(scaleTypeY);

    if (pass.scaleTypeX == ShaderPreset::SCALE_ABSOLUTE) {
        if (rootSettings.find("scale" + strPos))
            pass.absX = rootSettings.get<unsigned>("scale" + strPos, 0);
        else
            pass.absX = rootSettings.get<unsigned>("scale_x" + strPos, 0);
    } else {
        if (rootSettings.find("scale" + strPos))
            pass.scaleX = rootSettings.get<float>("scale" + strPos, 0.0);
        else
            pass.scaleX = rootSettings.get<float>("scale_x" + strPos, 0.0);
    }

    if (pass.scaleTypeY == ShaderPreset::SCALE_ABSOLUTE) {
        if (rootSettings.find("scale" + strPos))
            pass.absY = rootSettings.get<unsigned>("scale" + strPos, 0);
        else
            pass.absY = rootSettings.get<unsigned>("scale_y" + strPos, 0);
    } else {
        if (rootSettings.find("scale" + strPos))
            pass.scaleY = rootSettings.get<float>("scale" + strPos, 0.0);
        else
            pass.scaleY = rootSettings.get<float>("scale_y" + strPos, 0.0);
    }

    shaderPreset.passes.push_back(pass);
    return true;
}

auto ShaderParser::translateFilter(int filter) -> ShaderPreset::Filter {
    if (filter == 1) return ShaderPreset::FILTER_LINEAR;
    if (filter == 0) return ShaderPreset::FILTER_NEAREST;
    return ShaderPreset::FILTER_UNSPEC;
}

auto ShaderParser::translateScaleType(std::string scaleType) -> ShaderPreset::ScaleType {
    if (scaleType == "source") return ShaderPreset::SCALE_INPUT;
    if (scaleType == "viewport") return ShaderPreset::SCALE_VIEWPORT;
    if (scaleType == "absolute") return ShaderPreset::SCALE_ABSOLUTE;
    return ShaderPreset::SCALE_INPUT;
}

auto ShaderParser::translateScaleType(ShaderPreset::ScaleType scaleType) -> std::string {
    if (scaleType == ShaderPreset::SCALE_INPUT) return "source";
    if (scaleType == ShaderPreset::SCALE_VIEWPORT) return "viewport";
    if (scaleType == ShaderPreset::SCALE_ABSOLUTE) return "absolute";
    return "source";
}

auto ShaderParser::translateWrapMode(std::string wrap) -> ShaderPreset::WrapMode {
    if (wrap == "clamp_to_border") return ShaderPreset::WRAP_BORDER;
    if (wrap == "clamp_to_edge") return ShaderPreset::WRAP_EDGE;
    if (wrap == "repeat") return ShaderPreset::WRAP_REPEAT;
    if (wrap == "mirrored_repeat") return ShaderPreset::WRAP_MIRRORED_REPEAT;
    return ShaderPreset::WRAP_BORDER;
}

auto ShaderParser::translateWrapMode(ShaderPreset::WrapMode wrapMode) -> std::string {
    if (wrapMode == ShaderPreset::WRAP_BORDER) return "clamp_to_border";
    if (wrapMode == ShaderPreset::WRAP_EDGE) return "clamp_to_edge";
    if (wrapMode == ShaderPreset::WRAP_REPEAT) return "repeat";
    if (wrapMode == ShaderPreset::WRAP_MIRRORED_REPEAT) return "mirrored_repeat";
    return "clamp_to_border";
}

auto ShaderParser::findRootConfig(GUIKIT::Settings& settings, int depth) -> bool {
    auto references = settings.getReferences();

    if (references.empty())
        return settings.find("shaders");

    if (depth > 16)
        return false;

    for(auto& reference : references) {
        bool res = settings.loadEx(reference);
        GUIKIT::Vector::combine(brokenPaths, settings.getBrokenPaths());

        if (res) {
            if (findRootConfig(settings, depth + 1))
                return true;
        }
    }

    return false;
}

auto ShaderParser::applyOverrides(std::string& path, std::vector<GUIKIT::Settings*>& settingsList, int depth) -> void {
    auto settings = new GUIKIT::Settings;
    settings->loadEx(path);
    auto references = settings->getReferences();

    if (depth <= 16) {
        for (auto& reference : references)
            applyOverrides(reference, settingsList, depth + 1);
    }

    settingsList.push_back(settings); // order is important

    if (depth == 0) {
        for (auto settings : settingsList) {
            if (!settings->find("shaders")) {
                for(auto& param : shaderPreset.params) {
                    if (settings->find(param.id))
                        param.value = settings->get<float>(param.id);
                }
                for(auto& lut : shaderPreset.luts) {
                    if (settings->find(lut.id))
                        lut.path = GUIKIT::File::resolveRelativePath( settings->getPath(), settings->get<std::string>(lut.id) );
                }
            }

            delete settings;
        }
    }
}

auto ShaderParser::addPreset(ShaderParser* parser, bool prepend) -> void {
    ShaderPreset& preset = parser->shaderPreset;

    if (prepend) {
        int nextId = preset.passes.size();
        for (auto& param : shaderPreset.params)
            param.pass = param.pass + nextId;

        GUIKIT::Vector::insert( entryPaths, parser->getPresetPath(), 0 );

    } else {
        int nextId = shaderPreset.passes.size();
        for (auto& param : preset.params)
            param.pass = param.pass + nextId;

        entryPaths.push_back( parser->getPresetPath() );
    }

    GUIKIT::Vector::combine<ShaderPreset::Pass>(shaderPreset.passes, preset.passes, prepend);
    GUIKIT::Vector::combine<ShaderPreset::Param>(shaderPreset.params, preset.params, prepend);
    GUIKIT::Vector::combine<ShaderPreset::Lut>(shaderPreset.luts, preset.luts, prepend);
}

auto ShaderParser::movePass(unsigned& passId, bool up) -> void {
    if (passId >= shaderPreset.passes.size())
        return;

    ShaderPreset::Pass& pass = shaderPreset.passes[passId];

    if (up) {
        if (passId) {
            std::swap(pass, shaderPreset.passes[--passId]);
            modified = true;
        }
    } else {
        if ((passId+1) < shaderPreset.passes.size()) {
            std::swap(pass, shaderPreset.passes[++passId]);
            modified = true;
        }
    }
}

auto ShaderParser::togglePassUsage(unsigned passId) -> ShaderPreset::Pass* {
    if (passId >= shaderPreset.passes.size())
        return nullptr;

    ShaderPreset::Pass& pass = shaderPreset.passes[passId];
    pass.inUse ^= 1;
    modified = true;

    return &pass;
}

auto ShaderParser::getPresetPathCombined() -> std::string {
    std::string out = "";
    for(auto& path : entryPaths)
        out += GUIKIT::String::getFileName( path, true ) + " + ";

    return out.size() ? out.substr(0, out.size() - 3) : "";
}

auto ShaderParser::setPassFilter(unsigned passId, ShaderPreset::Filter filter) -> void {
    if (passId >= shaderPreset.passes.size())
        return;

    ShaderPreset::Pass& pass = shaderPreset.passes[passId];
    pass.filter = filter;
    modified = true;
}

auto ShaderParser::clear() -> void {
    entryPaths.clear();
    shaderPreset.clear();
    rootSettings.clear();
    brokenPaths.clear();
    modified = false;
}

