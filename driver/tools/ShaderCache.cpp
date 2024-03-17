
#include "ShaderCache.h"
#include "../tools/md5.h"
#include "../tools/tools.h"
#include "../tools/spirvReflection.h"

namespace DRIVER {

    auto ShaderCache::write(uint8_t* vs, unsigned vsSize, uint8_t* ps, unsigned psSize, SpirvReflection& reflection) -> void {
        if (!onShaderCacheCallback || cacheFile.empty())
            return;

        std::string refData = "###reflection_textures###";

        for(auto& tex : reflection.textures)
            refData += ";" + tex.name + ";" + std::to_string(tex.binding);

        refData += "\n###reflection_ubo###";

        refData += ";" + std::to_string(reflection.ubo.mask) + ";" + std::to_string(reflection.ubo.size)
                + ";" + std::to_string(reflection.ubo.binding);

        for(auto& var : reflection.ubo.variables) {
            refData += ";" + var.name + ";" + std::to_string(var.size)
                        + ";" + std::to_string(var.type) + ";" + std::to_string(var.offset);
        }

        refData += "\n###reflection_push###";

        refData += ";" + std::to_string(reflection.push.mask) + ";" + std::to_string(reflection.push.size)
                   + ";" + std::to_string(reflection.push.binding);

        for(auto& var : reflection.push.variables)
            refData += ";" + var.name + ";" + std::to_string(var.size) + ";" + std::to_string(var.type) + ";" + std::to_string(var.offset);

        refData += "\n";

        uint8_t* strData = (uint8_t*)refData.data();
        unsigned strLength = refData.length();

        unsigned memorySize = strLength + vsSize + psSize + 12;
        uint8_t* memory = new uint8_t[memorySize];

        copyIntToBuffer<unsigned>(memory + 0, vsSize);
        copyIntToBuffer<unsigned>(memory + 4, psSize);
        copyIntToBuffer<unsigned>(memory + 8, strLength);

        std::memcpy(memory + 12, vs, vsSize);
        if (ps && psSize)
            std::memcpy(memory + 12 + vsSize, ps, psSize);
        std::memcpy(memory + 12 + vsSize + psSize, strData, strLength);

        DiskFile diskFile;
        diskFile.path = cacheFile;
        diskFile.data = memory;
        diskFile.size = memorySize;
        diskFile.isLUT = false;
        onShaderCacheCallback(diskFile);

        delete[] memory;
    }

    auto ShaderCache::read(std::string& vertex, std::string& fragment, SpirvReflection& reflection) -> bool {
        clear();
        if (!onShaderCacheCallback)
            return false;

        std::string src = "";
        std::size_t pos = vertex.find("#line");
        if (pos != std::string::npos)
            src = vertex.substr(pos + 9, 32);

        if (!src.empty()) {
            pos = src.find_first_of(".\"");
            if (pos != std::string::npos)
                src = src.substr(0, pos);
        }

        std::string dataStr;
        MD5 md5;
        md5.update( vertex.data(), vertex.length() );
        md5.update( fragment.data(), fragment.length() );
        md5.finalize();
        std::string md5Hash = md5.hexdigest();

        cacheFile = "cache/" + src + md5Hash + "/" + type + ".dat";
        DiskFile diskFile;
        diskFile.path = cacheFile;
        diskFile.data = nullptr;
        diskFile.size = 0;
        diskFile.isLUT = false;
        onShaderCacheCallback(diskFile);
        cacheData = diskFile.data;

        if (!cacheData)
            return false;

        vertexSize = copyBufferToInt<unsigned>(cacheData);
        fragmentSize = copyBufferToInt<unsigned>(cacheData + 4);
        unsigned strSize = copyBufferToInt<unsigned>(cacheData + 8);

        if ((vertexSize + fragmentSize + strSize + 12) != diskFile.size)
            return false;

        nativeV = cacheData + 12;
        nativeF = cacheData + 12 + vertexSize;
        dataStr.assign((char*)(cacheData + 12 + vertexSize + fragmentSize), strSize);

        reflection.clear();

        pos = dataStr.find_first_of("\n");
        if (pos == std::string::npos)
            return false;

        std::string textures = dataStr.substr(0, pos);
        dataStr = dataStr.substr(pos + 1);

        if (!fetchTextures(textures, reflection))
            return false;

        pos = dataStr.find_first_of("\n");
        if (pos == std::string::npos)
            return false;

        std::string uniforms = dataStr.substr(0, pos);
        dataStr = dataStr.substr(pos + 1);

        if (!fetchUniforms(uniforms, false, reflection))
            return false;

        pos = dataStr.find_first_of("\n");
        if (pos == std::string::npos)
            return false;

        uniforms = dataStr.substr(0, pos);

        if (!fetchUniforms(uniforms, true, reflection))
            return false;

        return true;
    }

    auto ShaderCache::clear() -> void {
        if (cacheData) {
            delete[] cacheData;
            cacheData = nullptr;
        }
        cacheFile = "";
    }

    auto ShaderCache::fetchTextures(std::string& textures, SpirvReflection& reflection) -> bool {
        SpirvTexture spirvTex;
        bool toggle = false;
        auto parts = explode(textures, ";");
        if (!parts.size() || parts[0] != "###reflection_textures###")
            return false;
        removeFromBeginning(parts, 1);

        try {
            for(auto& part : parts) {
                if (toggle) {
                    spirvTex.binding = std::stoi( part ); // can throw exception
                    reflection.textures.push_back(spirvTex);
                } else {
                    spirvTex.name = part;
                }
                toggle ^= 1;
            }

            if (toggle) // binding missing
                return false;
        } catch( ... ) {
            return false;
        }
        return true;
    }

    auto ShaderCache::fetchUniforms(std::string& uniforms, bool push, SpirvReflection& reflection) -> bool {
        SpirvVariable spirvVariable;
        std::string header = push ? "###reflection_push###" : "###reflection_ubo###";
        SpirvBuffer& buffer = push ? reflection.push : reflection.ubo;
        auto parts = explode(uniforms, ";");

        if (!parts.size() || parts[0] != header)
            return false;

        removeFromBeginning(parts, 1);

        if (parts.size() < 3)
            return false;

        try {
            buffer.mask = std::stoi(parts[0]);
            if (buffer.mask == 0)
                return true;
            buffer.size = std::stoi(parts[1]);
            buffer.binding = std::stoi(parts[2]);
            removeFromBeginning(parts, 3);

            while (parts.size()) {
                if ((parts.size() % 4) != 0)
                    return false;

                spirvVariable.name = parts[0];
                spirvVariable.size = std::stoi(parts[1]);
                spirvVariable.type = (spirv_cross::SPIRType::BaseType) std::stoi(parts[2]);
                spirvVariable.offset = std::stoi(parts[3]);

                buffer.variables.push_back(spirvVariable);
                removeFromBeginning(parts, 4);
            }
        } catch( ... ) {
            return false;
        }

        return true;
    }

    auto ShaderCache::removeFromBeginning(std::vector<std::string>& vec, unsigned elements) -> void {
        for(unsigned i = 0; i < elements; i++)
            vec.erase(vec.begin());
    }

    auto ShaderCache::trim(std::string& str) -> std::string& {
        str.erase(0, str.find_first_not_of(' '));
        str.erase(str.find_last_not_of(' ')+1);
        return str;
    }

    auto ShaderCache::explode(std::string str, const std::string& delimiter) -> std::vector<std::string> {
        if (str.empty())
            return {};

        std::string token;

        std::vector<std::string> tokens;
        std::string::size_type n;

        while(true) {

            n = str.find( delimiter );

            if (n == std::string::npos) {
                token = str;
                trim( token );
                if (!token.empty())
                    tokens.push_back( token );
                break;
            }

            token = str.substr(0, n);

            trim( token );

            if (!token.empty())
                tokens.push_back( token );

            n += delimiter.size();
            if (n >= str.size())
                break;

            str = str.substr(n);
        }

        return tokens;
    }

}