
#define _USE_MATH_DEFINES
#include <cstdio>
#include <cstring>
#include <cmath>
#include "shaderParser.h"
#include "luts.cpp"

auto ShaderParser::loadPreset(std::string path) -> bool {
    clear();
    entryPaths.push_back(path);
    bool res = rootSettings.loadEx(path);
    int passCount;
    GUIKIT::Vector::combine(brokenPaths, rootSettings.getBrokenPaths());

    if (!res)
        return false;

    // we use the first found file which includes ident "shaders"
    // we don't check if there is another file that includes it.
    if (!findRootConfig(rootSettings)) {
        createSinglePass(path);
        goto End;
    }

    passCount = rootSettings.get<unsigned>("shaders", 0);
    if (!passCount) {
        createSinglePass(path);
        goto End;
    }

    shaderPreset.feedback = rootSettings.get<int>("feedback", -1);
    shaderPreset.lumaChroma = rootSettings.get<bool>("luma_chroma", false);

    for(int i = 0; i < passCount; i++)
        parsePass(i);

    if (shaderPreset.passes.empty()) { // wtf
        createSinglePass(path);
        fetchParameters(path, 0);
        return true;
    }

    parseTextures();
End:
    for(int i = 0; i < shaderPreset.passes.size(); i++) {
        ShaderPreset::Pass& pass = shaderPreset.passes[i];
        fetchParameters(pass.src, i);
        loadShader(pass);
    }

    if (path != rootSettings.getPath()) { // there is at least one reference to root config
        std::vector<GUIKIT::Settings*> settingsList;
        applyOverrides(path, settingsList);
    }

    updateCrop();
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
        writeLine(fp, "shaders", std::to_string(shaderPreset.passes.size()));
        if (shaderPreset.feedback >= 0)
            writeLine(fp, "feedback", std::to_string(shaderPreset.feedback) );
        if (shaderPreset.lumaChroma)
            writeLine(fp, "luma_chroma", "true" );

        fputs( "\n", fp );

        for(int i = 0; i < shaderPreset.passes.size(); i++) {
            auto& pass = shaderPreset.passes[i];
            writeLine(fp, i, "shader", GUIKIT::File::buildRelativePath(path, pass.src));
            if (!pass.inUse)
                writeLine(fp, i, "hide", "true");
            if (pass.filter != ShaderPreset::FILTER_UNSPEC)
                writeLine(fp, i, "filter_linear", pass.filter == ShaderPreset::FILTER_NEAREST ? "false" : "true");
            writeLine(fp, i, "wrap_mode", translateWrapMode(pass.wrap));
            writeLine(fp, i, "mipmap_input", pass.mipmap ? "true" : "false");
            writeLine(fp, i, "alias", pass.alias);
            writeLine(fp, i, "float_framebuffer", pass.bufferType == ShaderPreset::BUFFER_FP ? "true" : "false");
            writeLine(fp, i, "srgb_framebuffer", pass.bufferType == ShaderPreset::BUFFER_SRGB ? "true" : "false");

            if (pass.scaleTypeX != ShaderPreset::SCALE_NONE) {
                writeLine(fp, i, "scale_type_x", translateScaleType(pass.scaleTypeX));
                writeLine(fp, i, "scale_x", pass.scaleTypeX == ShaderPreset::SCALE_ABSOLUTE ? std::to_string(pass.absX) : std::to_string(pass.scaleX) );
            }
            if (pass.scaleTypeY != ShaderPreset::SCALE_NONE) {
                writeLine(fp, i, "scale_type_y", translateScaleType(pass.scaleTypeY));
                writeLine(fp, i, "scale_y", pass.scaleTypeY == ShaderPreset::SCALE_ABSOLUTE ? std::to_string(pass.absY) : std::to_string(pass.scaleY) );
            }

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
                if (lut.filter != ShaderPreset::FILTER_UNSPEC)
                    writeLine(fp, lut.id + "_linear", lut.filter == ShaderPreset::FILTER_NEAREST ? "false" : "true" );
                writeLine(fp, lut.id + "_wrap_mode",  translateWrapMode(lut.wrap) );
                writeLine(fp, lut.id + "_mipmap",  lut.mipmap ? "true" : "false" );
            }
        }
    }

    for (auto& param : shaderPreset.params) {
        if (param.initial != param.value) {
            if (!GUIKIT::String::findString(param.id, "autoEmu_")) {
                out = param.id + " = " + std::to_string(param.value) + "\n";
                fputs( out.c_str(), fp );
            }
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
    static const std::string prefix = "#include";
    std::string line;
    int filled;

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

        param.value = rootSettings.get<float>(param.id, param.initial);

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

auto ShaderParser::parseTextures() -> void {
   // if (!luts.size()) {
   //     buildLutBloom();
   //     buildLutMask();
   //     buildLutBandwidth();
   // }

    std::string lutList = rootSettings.get<std::string>("textures", "");

    if (lutList.empty())
        return;

    auto parts = GUIKIT::String::explode(lutList, ";");
    shaderPreset.luts.reserve( parts.size() );

    for(auto& id : parts) {
        if (!rootSettings.find(id))
            continue;

        ShaderPreset::Lut lut;
        lut.id = id;
        lut.path = rootSettings.get<std::string>(id, "");
        lut.data = nullptr;
        if (lut.path.empty())
            continue;
        lut.path = GUIKIT::File::resolveRelativePath( rootSettings.getPath(), lut.path );

        int filter = -1;
        if (rootSettings.find(id + "_linear"))
            filter = (int)rootSettings.get<bool>(id + "_linear", false);

        lut.filter = translateFilter(filter);
        lut.mipmap = rootSettings.get<bool>(id + "_mipmap", false);
        lut.wrap = translateWrapMode( rootSettings.get<std::string>(id + "_wrap_mode", "") );

        if (!loadLUT(lut)) {

//            for (auto& tempLut : luts) {
//                if (lut.id == tempLut.id) {
//                    lut.data = tempLut.data;
//                    lut.width = tempLut.width;
//                    lut.height = tempLut.height;
//                    break;
//                }
//            }
        }

        shaderPreset.luts.push_back(lut);
    }
}

auto ShaderParser::parsePass(unsigned pos) -> bool {
    ShaderPreset::Pass pass;
    std::string strPos = std::to_string(pos);
    std::string path = rootSettings.get<std::string>("shader" + strPos, "");
    if (path.empty())
        return false;

    pass.src = GUIKIT::File::resolveRelativePath( rootSettings.getPath(), path);
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

    pass.inUse = !rootSettings.get<bool>("hide" + strPos, false);

    pass.scaleTypeX = ShaderPreset::SCALE_NONE;
    pass.scaleTypeY = ShaderPreset::SCALE_NONE;
    pass.scaleX = 0.0;
    pass.scaleY = 0.0;

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
            pass.scaleX = rootSettings.get<float>("scale" + strPos, 1.0);
        else
            pass.scaleX = rootSettings.get<float>("scale_x" + strPos, 1.0);
    }

    if (pass.scaleTypeY == ShaderPreset::SCALE_ABSOLUTE) {
        if (rootSettings.find("scale" + strPos))
            pass.absY = rootSettings.get<unsigned>("scale" + strPos, 0);
        else
            pass.absY = rootSettings.get<unsigned>("scale_y" + strPos, 0);
    } else {
        if (rootSettings.find("scale" + strPos))
            pass.scaleY = rootSettings.get<float>("scale" + strPos, 1.0);
        else
            pass.scaleY = rootSettings.get<float>("scale_y" + strPos, 1.0);
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
            param.pass += nextId;

        GUIKIT::Vector::insert( entryPaths, parser->getPresetPath(), 0 );

        shaderPreset.lumaChroma = preset.lumaChroma;
        shaderPreset.feedback = preset.feedback;
    } else {
        int nextId = shaderPreset.passes.size();
        for (auto& param : preset.params)
            param.pass += nextId;

        entryPaths.push_back( parser->getPresetPath() );
    }

    GUIKIT::Vector::combine<ShaderPreset::Pass>(shaderPreset.passes, preset.passes, prepend);
    GUIKIT::Vector::combine<ShaderPreset::Param>(shaderPreset.params, preset.params, prepend);
    GUIKIT::Vector::combine<ShaderPreset::Lut>(shaderPreset.luts, preset.luts, prepend);
    updateCrop();
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
    updateCrop();
}

auto ShaderParser::togglePassUsage(unsigned passId) -> ShaderPreset::Pass* {
    if (passId >= shaderPreset.passes.size())
        return nullptr;

    ShaderPreset::Pass& pass = shaderPreset.passes[passId];
    pass.inUse ^= 1;
    modified = true;
    updateCrop();
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

auto ShaderParser::setPassFormat(unsigned passId, ShaderPreset::BufferType bufferType) -> void {
    if (passId >= shaderPreset.passes.size())
        return;

    ShaderPreset::Pass& pass = shaderPreset.passes[passId];
    pass.bufferType = bufferType;
    modified = true;
}

auto ShaderParser::setPassMipmap(unsigned passId, bool state) -> void {
    if (passId >= shaderPreset.passes.size())
        return;

    ShaderPreset::Pass& pass = shaderPreset.passes[passId];
    pass.mipmap = state;
    modified = true;
}

auto ShaderParser::setPassScaleX(unsigned passId, float scale) -> void {
    if (passId >= shaderPreset.passes.size())
        return;
    ShaderPreset::Pass& pass = shaderPreset.passes[passId];
    if (pass.scaleTypeX == ShaderPreset::SCALE_ABSOLUTE)
        return;

    pass.scaleX = scale;
    modified = true;
}

auto ShaderParser::setPassScaleY(unsigned passId, float scale) -> void {
    if (passId >= shaderPreset.passes.size())
        return;
    ShaderPreset::Pass& pass = shaderPreset.passes[passId];
    if (pass.scaleTypeY == ShaderPreset::SCALE_ABSOLUTE)
        return;

    pass.scaleY = scale;
    modified = true;
}

auto ShaderParser::needMetaData() -> bool {
    for (auto& pass : shaderPreset.passes) {
        if (pass.inUse && (pass.alias == "VICIIGlitches"))
            return true;
    }

    return false;
}

auto ShaderParser::loadShader(ShaderPreset::Pass& pass) -> bool {
    if (pass.src.empty())
        return false;

    GUIKIT::File file(pass.src);
    if (!file.open())
        return false;

    pass.code.assign((char*) file.read(), file.getSize());
    return true;
}

auto ShaderParser::loadLUT(ShaderPreset::Lut& lut) -> bool {
    if (lut.path.empty())
        return false;

    GUIKIT::File file(lut.path);
    if (!file.open())
        return false;

    GUIKIT::Image png;
    if (!png.loadPng(file.read(), file.getSize() ))
        return false;

    lut.data = new uint8_t[png.width * png.height * 4];
    std::memcpy(lut.data, png.data, png.width * png.height * 4);
    lutData.push_back(lut.data);

    lut.width = png.width;
    lut.height = png.height;

    return true;
}

auto ShaderParser::updateCrop() -> void {
    for(auto& pass : shaderPreset.passes)
        pass.crop.release();

    if (!shaderPreset.lumaChroma)
        return;

    for(int i = shaderPreset.passes.size() - 1; i >= 0; i--) {
        ShaderPreset::Pass& pass = shaderPreset.passes[i];
        if (!pass.inUse)
            continue;

        if (GUIKIT::String::findString(pass.alias, "Mask") || GUIKIT::String::findString(pass.alias, "BloomVertical")
            || GUIKIT::String::findString(pass.alias, "Gamma") || GUIKIT::String::findString(pass.alias, "LumaChromaDecoding") ) {
            pass.crop.set({1, 4, 0, 4});
            break;
        }
    }
}

auto ShaderParser::clear() -> void {
    entryPaths.clear();
    brokenPaths.clear();
    shaderPreset.clear();
    rootSettings.clear();
    modified = false;
    for (auto data : lutData)
        delete[] data;
    lutData.clear();
}

ShaderParser::ShaderParser() {
    clear();
}
