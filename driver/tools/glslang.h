
#pragma once

#include <limits>
#include "glslang/Public/ResourceLimits.h"
#include "glslang/Public/ShaderLang.h"
#include "SPIRV/GlslangToSpv.h"

struct GLSlang {

    auto compileFragment(const std::string& code, std::vector<unsigned>& out, std::string& error) -> bool {
        return compile(code, EShLangFragment, out, error);
    }

    auto compileVertex(const std::string& code, std::vector<unsigned>& out, std::string& error) -> bool {
        return compile(code, EShLangVertex, out, error);
    }

    auto compile(const std::string& code, EShLanguage language, std::vector<unsigned>& out, std::string& error) -> bool {
        glslang::InitializeProcess();
        glslang::TProgram program;
        std::string str;
        glslang::TShader::ForbidIncluder forbidIncluder; // all includes are already resolved
        glslang::TShader shader(language);

        const char* src = code.c_str();
        shader.setStrings(&src, 1);

        EShMessages messages = static_cast<EShMessages>(EShMsgDefault | EShMsgVulkanRules | EShMsgSpvRules);

        if (!shader.preprocess(GetDefaultResources(), 110, ENoProfile, false, false, messages, &str, forbidIncluder)) {
            error += std::string(shader.getInfoLog()) + "\n";
            error += std::string(shader.getInfoDebugLog()) + "\n";
            glslang::FinalizeProcess();
            return false;
        }

        if (!shader.parse(GetDefaultResources(), 110, false, messages)) {
            error += std::string(shader.getInfoLog()) + "\n";
            error += std::string(shader.getInfoDebugLog()) + "\n";
            glslang::FinalizeProcess();
            return false;
        }

        program.addShader(&shader);

        if (!program.link(messages)) {
            error += std::string(shader.getInfoLog()) + "\n";
            error += std::string(shader.getInfoDebugLog()) + "\n";
            glslang::FinalizeProcess();
            return false;
        }

        GlslangToSpv(*program.getIntermediate(language), out);
        glslang::FinalizeProcess();
        return true;
    }
};