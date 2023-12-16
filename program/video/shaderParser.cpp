
#include "shaderParser.h"
#include <cstdio>

auto ShaderParser::loadPreset(std::string path) -> bool {
    if (!rootSettings.loadEx(path)) {
        return false;
    }

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

    if (passCount > 64)
        passCount = 64;

    shaderPreset.feedback = rootSettings.get<int>("feedback", -1);

    for(int i = 0; i < passCount; i++) {
        if (parsePass(i)) {}
    }

    if (shaderPreset.passes.empty()) { // wtf
        createSinglePass(path);
        return true;
    }

    if (!parseTextures()) {

    }

    if (path != rootSettings.getPath()) { // there is at least one reference to root config
        std::vector<GUIKIT::Settings*> settingsList;
        applyOverrides(path, settingsList);
    }

    return true;
}

auto ShaderParser::createSinglePass(std::string path) -> void {
    ShaderPreset::Pass pass;
    pass.src = path;
    pass.filter = ShaderPreset::Filter::FILTER_UNSPEC;
    pass.scaleTypeX = ShaderPreset::ScaleType::SCALE_INPUT;
    pass.scaleTypeY = ShaderPreset::ScaleType::SCALE_INPUT;
    pass.wrap = ShaderPreset::WrapType::WRAP_EDGE;
    pass.frameModulo = 0;
    pass.bufferType = ShaderPreset::BufferType::BUFFER_UNORM;
    pass.mipmap = false;
    pass.alias = "";
    pass.scaleX = 1.0;
    pass.scaleY = 1.0;
    pass.absX = 0;
    pass.absY = 0;
    shaderPreset.passes.push_back(pass);
}

auto ShaderParser::fetchParameters() -> void {
    std::string line;
    int filled;

    for(auto& pass : shaderPreset.passes) {
        GUIKIT::File file(pass.src);
        if(!file.open() || !file.getSize())
            continue;

        auto fp = file.getHandle();

        char chunk[1024];
        while ( fgets(chunk, sizeof(chunk), fp) ) {
            line = chunk;
            if (!GUIKIT::String::foundSubStr(line, "#pragma parameter"))
                continue;

            ShaderPreset::Param param;

            char id[64];
            char desc[64];
            if ((filled = std::sscanf(line.c_str(), "#pragma parameter %63s \"%63[^\"]\" %f %f %f %f",
                                      id, desc, &param.initial, &param.minimum, &param.maximum, &param.step)) < 5)
                continue;

            param.id = id;
            param.desc = desc;

            if (filled == 5)
                param.step  = 0.1f * (param.maximum - param.minimum);

            param.pass = pass.id;

            if (rootSettings.find(param.id))
                param.value = rootSettings.get<float>(param.id);
            else
                param.value = param.initial;

            shaderPreset.params.push_back(param);
        }
    }
}

auto ShaderParser::parseTextures() -> bool {
    std::string lutList = rootSettings.get<std::string>("textures", "");

    if (lutList.empty())
        return true;

    auto parts = GUIKIT::String::explode(lutList, ";");
    shaderPreset.luts.reserve( parts.size() );

    for(auto& id : parts) {
        if (!rootSettings.find(id))
            return false;

        ShaderPreset::Lut lut;
        lut.id = id;
        lut.path = rootSettings.get<std::string>(id, "");
        if (lut.path.empty())
            return false;
        lut.path = GUIKIT::File::resolveRelativePath( rootSettings.getPath(), lut.path );
        lut.filter = translateFilter( rootSettings.get<int>(id + "_linear", -1) );
        lut.mipmap = rootSettings.get<bool>(id + "_mipmap", false);
        lut.wrap = translateWrapType( rootSettings.get<std::string>(id + "_wrap_mode", "") );

        shaderPreset.luts.push_back(lut);
    }
}

auto ShaderParser::parsePass(unsigned pos) -> bool {
    ShaderPreset::Pass pass;
    std::string strPos = std::to_string(pos);
    std::string path = rootSettings.get<std::string>("shader" + strPos, "");
    if (!path.empty())
        path = GUIKIT::File::resolveRelativePath( rootSettings.getPath(), path);

    pass.src = path;
    pass.filter = translateFilter( rootSettings.get<int>("filter" + strPos, -1) );
    pass.wrap = translateWrapType( rootSettings.get<std::string>("wrap_mode" + strPos, "") );
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
    pass.id = shaderPreset.passes.size();

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

    if (!scaleTypeX.empty()) {
        if (scaleTypeX == "source") pass.scaleTypeX = ShaderPreset::SCALE_INPUT;
        else if (scaleTypeX == "viewport") pass.scaleTypeX = ShaderPreset::SCALE_VIEWPORT;
        else if (scaleTypeX == "absolute") pass.scaleTypeX = ShaderPreset::SCALE_ABSOLUTE;
        else
            return false;
    }

    if (!scaleTypeY.empty()) {
        if (scaleTypeY == "source") pass.scaleTypeY = ShaderPreset::SCALE_INPUT;
        else if (scaleTypeY == "viewport") pass.scaleTypeY = ShaderPreset::SCALE_VIEWPORT;
        else if (scaleTypeY == "absolute") pass.scaleTypeY = ShaderPreset::SCALE_ABSOLUTE;
        else
            return false;
    }

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

auto ShaderParser::translateWrapType(std::string wrap) -> ShaderPreset::WrapType {
    if (wrap == "clamp_to_border") return ShaderPreset::WRAP_BORDER;
    if (wrap == "clamp_to_edge") return ShaderPreset::WRAP_EDGE;
    if (wrap == "repeat") return ShaderPreset::WRAP_REPEAT;
    if (wrap == "mirrored_repeat") return ShaderPreset::WRAP_MIRRORED_REPEAT;
    return ShaderPreset::WRAP_BORDER;
}

auto ShaderParser::findRootConfig(GUIKIT::Settings& settings, int depth) -> bool {
    auto references = settings.getReferences();

    if (references.empty())
        return settings.find("shaders");

    if (depth > 16)
        return false;

    for(auto& reference : references) {
        if (settings.loadEx(reference)) {
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

    if (!references.empty() && (depth > 16))
        return;

    for(auto& reference : references)
        applyOverrides(reference, settingsList, depth + 1);

    settingsList.push_back(settings); // order is important

    if (depth == 0) {
        for (auto settings : settingsList) {
            if (!settings->find("shaders")) {
                for(auto& param : shaderPreset.params) {
                    if (settings->find(param.id))
                        param.value = rootSettings.get<float>(param.id);
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

