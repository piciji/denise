
#include "spirvReflection.h"

auto SpirvReflection::clear() -> void {
    textures.clear();
    ubo.variables.clear();
    push.variables.clear();
    ubo.mask = push.mask = 0;
    ubo.size = push.size = 0;
    ubo.binding = push.binding = ~0;
}

auto SpirvReflection::process(spirv_cross::Compiler& vCompiler, spirv_cross::Compiler& fCompiler,spirv_cross::ShaderResources& vResources,spirv_cross::ShaderResources& fResources) -> bool {
    clear();
    SpirvBuffer vUbo;
    SpirvBuffer fUbo;
    SpirvBuffer vPush;
    SpirvBuffer fPush;

    if (!vResources.sampled_images.empty() || !vResources.storage_buffers.empty() || !vResources.subpass_inputs.empty()
        || !vResources.storage_images.empty() || !vResources.atomic_counters.empty()) {
        error = "Vertex: invalid resource type";
        return false;
    }

    if (!fResources.storage_buffers.empty() || !fResources.subpass_inputs.empty()
        || !fResources.storage_images.empty() || !fResources.atomic_counters.empty()) {
        error = "Fragment: invalid resource type";
        return false;
    }

    if (vResources.stage_inputs.size() != 2) {
        error = "Vertex: expects two attributes";
        return false;
    }

    if (fResources.stage_outputs.size() != 1) {
        error = "Fragment: expects single render target";
        return false;
    }

    if (fCompiler.get_decoration( fResources.stage_outputs[0].id, spv::DecorationLocation) != 0) {
        error = "Fragment: render target must use location 0";
        return false;
    }

    for (int i = 0; i < vResources.stage_inputs.size(); i++) {
        auto location = vCompiler.get_decoration( vResources.stage_inputs[i].id, spv::DecorationLocation);

        if (location > 1) {
            error = "Vertex: expect attributes at location 0 and 1";
            return false;
        }
    }

    if (!reflectBuffer(vResources.uniform_buffers, vCompiler, vUbo)) {
        error = "Vertex uniform " + error;
        return false;
    }

    if (!reflectBuffer(fResources.uniform_buffers, fCompiler, fUbo)) {
        error = "Fragment uniform " + error;
        return false;
    }

    if (!reflectBuffer(vResources.push_constant_buffers, vCompiler, vPush)) {
        error = "Vertex push constant " + error;
        return false;
    }

    if (!reflectBuffer(fResources.push_constant_buffers, fCompiler, fPush)) {
        error = "Fragment push constant " + error;
        return false;
    }

    if (vPush.mask && fPush.mask) {
        if (fPush.binding != vPush.binding) {
            error = "expect same binding for Vertex and Fragment push constant buffer";
            return false;
        }

        if (!combineBuffer(fPush, vPush, push)) {
            return false;
        }
        push.size = std::max(vPush.size, fPush.size);
        push.binding = fPush.binding;
        push.mask = Stage::Vertex | Stage::Fragment;
    } else if (vPush.mask) {
        push = vPush;
        push.mask = Stage::Vertex;
    } else if (fPush.mask) {
        push = fPush;
        push.mask = Stage::Fragment;
    } else {
        push.mask = 0;
    }

    if (push.size > 128) {
        error = "exceeded maximum size of 128 bytes for push constant buffer";
        return false;
    }

    if (vUbo.mask && fUbo.mask) {
        if (fUbo.binding != vUbo.binding) {
            error = "expect same binding for Vertex and Fragment uniform buffer";
            return false;
        }

        if (!combineBuffer(fUbo, vUbo, ubo)) {
            return false;
        }
        ubo.size = std::max(vUbo.size, fUbo.size);
        ubo.binding = fUbo.binding;
        ubo.mask = Stage::Vertex | Stage::Fragment;
    } else if (vUbo.mask) {
        ubo = vUbo;
        ubo.mask = Stage::Vertex;
    } else if (fUbo.mask) {
        ubo = fUbo;
        ubo.mask = Stage::Fragment;
    } else {
        ubo.mask = 0;
    }

    unsigned bindingMask = 0;
    if (ubo.mask)
        bindingMask |= 1 << ubo.binding;

    textures.reserve(fResources.sampled_images.size());

    for(auto& sampledImage : fResources.sampled_images) {
        SpirvTexture spirvTexture;
        spirvTexture.name = sampledImage.name;

        unsigned descriptorSet = fCompiler.get_decoration( sampledImage.id, spv::DecorationDescriptorSet);
        if (descriptorSet != 0) {
            error = "Texture " + spirvTexture.name + " must use descriptor set #0";
            return false;
        }

        spirvTexture.binding = fCompiler.get_decoration( sampledImage.id, spv::DecorationBinding);

        if (spirvTexture.binding >= SPIRV_MAX_BINDINGS) {
            error = "Binding " + std::to_string(spirvTexture.binding) + " is out of range";
            return false;
        }

        if (bindingMask & (1 << spirvTexture.binding)) {
            error = "Binding " + std::to_string(spirvTexture.binding) + " is already in use";
            return false;
        }
        bindingMask |= 1 << spirvTexture.binding;

        textures.push_back(spirvTexture);
    }

    return true;
}

auto SpirvReflection::combineBuffer(const SpirvBuffer& vBuffer, const SpirvBuffer& fBuffer, SpirvBuffer& out) -> bool {
    out.variables.reserve(fBuffer.variables.size());

    for(auto& varV : vBuffer.variables) {
        for(auto& varF : fBuffer.variables) {
            if (varF.name == varV.name) {
                if (varF.offset != varV.offset) {
                    error = "expect same offset for variable " + varV.name + " in Vertex and Fragment";
                    return false;
                }
                if (varF.size != varV.size) {
                    error = "expect same size for variable " + varV.name + " in Vertex and Fragment";
                    return false;
                }

                break;
            }
        }
        out.variables.push_back(varV);
    }

    for(auto& varF : fBuffer.variables) {
        bool found = false;
        for(auto& varV : vBuffer.variables) {
            if (varF.name == varV.name) {
                found = true;
                break;
            }
        }
        if (!found)
            out.variables.push_back(varF);
    }

    return true;
}

auto SpirvReflection::reflectBuffer(const spirv_cross::SmallVector<spirv_cross::Resource>& buffer, const spirv_cross::Compiler& compiler, SpirvBuffer& result ) -> bool {
    auto _elements = buffer.size();
    if (_elements > 1) {
        error = "must use zero or one buffer";
        return false;
    }

    result.mask = _elements == 1 ? 1 : 0;

    if (result.mask) {
        const spirv_cross::Resource& res = buffer[0];

        if (compiler.get_decoration( res.id, spv::DecorationDescriptorSet) != 0) {
            error = "Buffer " + res.name + " must use descriptor set #0";
            return false;
        }

        result.binding = compiler.get_decoration(res.id, spv::DecorationBinding);
        result.size = compiler.get_declared_struct_size( compiler.get_type( buffer[0].base_type_id));
        auto ranges = compiler.get_active_buffer_ranges(res.id);
        if (!ranges.size()) { // declared but not used
            result.mask = 0;
            return true;
        }

        result.variables.reserve(ranges.size());

        for (auto& range : ranges) {
            SpirvVariable v;
            auto& type = compiler.get_type( compiler.get_type(res.base_type_id).member_types[range.index]);
            if (!type.array.empty())
                continue;

            v.type = type.basetype;
            v.size = type.columns * type.vecsize * sizeof(float);
            v.name = compiler.get_member_name( res.base_type_id, range.index);
            v.offset = range.offset;
            result.variables.push_back(v);
        }
    }
    return true;
}

auto SpirvReflection::bindUbo(ShaderPreset* preset, unsigned passId, SemanticMap& map, SemanticBuffer* semanticBuffer) -> bool {
    return bindUniforms(ubo, preset, passId, map, semanticBuffer);
}

auto SpirvReflection::bindPush(ShaderPreset* preset, unsigned passId, SemanticMap& map, SemanticBuffer* semanticBuffer) -> bool {
    return bindUniforms(push, preset, passId, map, semanticBuffer);
}

auto SpirvReflection::bindUniforms(SpirvBuffer& spirvBuffer, ShaderPreset* preset, unsigned passId, SemanticMap& map, SemanticBuffer* semanticBuffer) -> bool {
    semanticBuffer->uniforms.clear();

    for(auto& var : spirvBuffer.variables) {
        bool assignNextUsablePass = false;
        SemanticUniform semUniform;
        semUniform.offset = var.offset;
        semUniform.size = var.size;

        for(int l = 0; l < preset->luts.size(); l++) {
            auto& lut = preset->luts[l];
            std::string name = lut.id  + "Size";
            if (name == var.name) {
                semUniform.data = (void*)((uintptr_t)map.textures[SemanticMap::PassLuts].size + l * map.textures[SemanticMap::PassLuts].stride);
                goto Next;
            }
        }

        for(int p = preset->passes.size() - 1; p >= 0; p--) {
            auto& pass = preset->passes[p];
            std::string name = pass.alias  + "Size";
            if (assignNextUsablePass || (name == var.name)) {
                if (!pass.inUse) {
                    assignNextUsablePass = true;
                    continue;
                }

                semUniform.data = (void*)((uintptr_t)map.textures[SemanticMap::PassOutput].size + p * map.textures[SemanticMap::PassOutput].stride);
                goto Next;
            }
        }

        assignNextUsablePass = false;
        for(int p = preset->passes.size() - 1; p >= 0; p--) {
            auto& pass = preset->passes[p];
            std::string name = pass.alias + "FeedbackSize";
            if (assignNextUsablePass || (name == var.name)) {
                if (!pass.inUse) {
                    assignNextUsablePass = true;
                    continue;
                }

                semUniform.data = (void*)((uintptr_t)map.textures[SemanticMap::PassFeedback].size + p * map.textures[SemanticMap::PassFeedback].stride);
                goto Next;
            }
        }

        for(auto& param : preset->params) {
            if (var.name == param.id) {
                semUniform.data = &param.value;
                goto Next;
            }
        }

        if (var.name == "MVP") {
            semUniform.data = map.uniforms[SemanticMap::MVP];
            goto Next;
        } else if (var.name == "OutputSize") {
            semUniform.data = map.uniforms[SemanticMap::Output];
            goto Next;
        } else if (var.name == "FinalViewportSize") {
            semUniform.data = map.uniforms[SemanticMap::FinalViewport];
            goto Next;
        } else if (var.name == "FrameCount") {
            semUniform.data = map.uniforms[SemanticMap::FrameCount];
            goto Next;
        } else if (var.name == "FrameDirection") {
            semUniform.data = map.uniforms[SemanticMap::FrameDirection];
            goto Next;
        } else if (var.name == "Rotation") {
            semUniform.data = map.uniforms[SemanticMap::Rotation];
            goto Next;
        } else if (var.name == "OriginalSize") {
            semUniform.data = map.textures[SemanticMap::History].size;
            goto Next;
        } else if (var.name == "SourceSize") {
            if (passId > 0) {
                for(int p = passId - 1; p >= 0; p--) {
                    auto& pass = preset->passes[p];
                    if (!pass.inUse)
                        continue;

                    semUniform.data = (void*)((uintptr_t)map.textures[SemanticMap::PassOutput].size + p * map.textures[SemanticMap::PassOutput].stride);
                    goto Next;
                }
            }
            semUniform.data = map.textures[SemanticMap::History].size;
            goto Next;
        } else {
            static const std::string names[] = {"OriginalHistorySize", "PassOutputSize", "PassFeedbackSize"};

            for(int n = 0; n < (sizeof(names) / sizeof(names[0])); n++) {
                auto& name = names[n];

                if (findString(var.name, name)) {
                    int index = 0;
                    std::string strIndex = var.name.substr(name.size());
                    if (isNumber(strIndex))
                        index = stoi(strIndex);

                    if (n == SemanticMap::History) {
                        if (index >= map.textures[SemanticMap::History].maxElements) {
                            error = var.name + " exceeds max history frames of " + std::to_string(map.textures[SemanticMap::History].maxElements);
                            return false;
                        }

                        semUniform.data = (void*)((uintptr_t)map.textures[SemanticMap::History].size + index * map.textures[SemanticMap::History].stride);
                        goto Next;
                    } else {
                        index += getSubChainIndex(preset, passId);

                        if ((n == SemanticMap::PassOutput) && (index >= passId)) {
                            error = var.name + " cannot use output size of future pass " + std::to_string(passId);
                            return false;
                        }

                        if (index >= map.textures[n].maxElements) {
                            error = var.name + " exceeds max textures of " + std::to_string(map.textures[n].maxElements);
                            return false;
                        }

                        for(int p = index; p >= 0; p--) {
                            auto& pass = preset->passes[p];
                            if (!pass.inUse)
                                continue;

                            semUniform.data = (void*)((uintptr_t)map.textures[n].size + index * map.textures[n].stride);
                            goto Next;
                        }
                    }
                }
            }
        }

        error = var.name + " cannot be identified";
        return false;

        Next:
        semanticBuffer->uniforms.push_back(semUniform);
    }

    semanticBuffer->mask = spirvBuffer.mask;
    semanticBuffer->binding = spirvBuffer.binding;
    semanticBuffer->size = (spirvBuffer.size + 0xf) & ~0xf;
    return true;
}

inline auto SpirvReflection::getSubChainIndex(ShaderPreset* preset, unsigned start) -> unsigned {
    if (start >= preset->passes.size())
        return 0;

    for(int i = start; i >= 0; i--) {
        if (preset->passes[i].subChain)
            return i;
    }
    return 0;
}

auto SpirvReflection::bindTextures(ShaderPreset* preset, unsigned passId, SemanticMap& map, std::vector<SemanticTexture>& semanticTextures) -> bool {
    ShaderPreset::Pass& curPass = preset->passes[passId];
    semanticTextures.clear();
    semanticTextures.reserve( textures.size() );
    unsigned& curHistorySize = *(unsigned*)(map.uniforms[SemanticMap::HistorySize]);

    for(auto& tex : textures) {
        bool assignNextUsablePass = false;
        SemanticTexture semTex;
        semTex.binding = tex.binding;
        semTex.filter = curPass.filter;
        semTex.wrap = curPass.wrap;
        semTex.feedbackPass = -1;

        for(int l = 0; l < preset->luts.size(); l++) {
            auto& lut = preset->luts[l];

            if (lut.id == tex.name) {
                semTex.data = map.textures[SemanticMap::PassLuts].image + l * map.textures[SemanticMap::PassLuts].stride;
                semTex.filter = lut.filter;
                semTex.wrap = lut.wrap;
                goto Next;
            }
        }

        for(int p = preset->passes.size() - 1; p >= 0; p--) {
            auto& pass = preset->passes[p];
            if (assignNextUsablePass || (pass.alias == tex.name)) {
                if (!pass.inUse) {
                    assignNextUsablePass = true;
                    continue;
                }

                semTex.data = map.textures[SemanticMap::PassOutput].image + p * map.textures[SemanticMap::PassOutput].stride;
                goto Next;
            }
        }

        assignNextUsablePass = false;
        for(int p = preset->passes.size() - 1; p >= 0; p--) {
            auto& pass = preset->passes[p];
            std::string name = pass.alias + "Feedback";
            if (assignNextUsablePass || (name == tex.name)) {
                if (!pass.inUse) {
                    assignNextUsablePass = true;
                    continue;
                }
                //pass.feedback = true;
                semTex.feedbackPass = p;
                semTex.data = map.textures[SemanticMap::PassFeedback].image + p * map.textures[SemanticMap::PassFeedback].stride;
                goto Next;
            }
        }

        if (tex.name == "Original") {
            semTex.data = (uintptr_t)map.textures[SemanticMap::History].image;
            goto Next;
        }

        if (tex.name == "Source") {
            if (passId > 0) {
                for(int p = passId - 1; p >= 0; p--) {
                    auto& pass = preset->passes[p];
                    if (!pass.inUse)
                        continue;

                    semTex.data = map.textures[SemanticMap::PassOutput].image + p * map.textures[SemanticMap::PassOutput].stride;
                    goto Next;
                }
            }
            semTex.data = map.textures[SemanticMap::History].image;
            goto Next;
        }

        static const std::string names[] = {"OriginalHistory", "PassOutput", "PassFeedback"};

        for(int n = 0; n < (sizeof(names) / sizeof(names[0])); n++) {
            auto& name = names[n];

            if (findString(tex.name, name)) {
                int index = 0;
                std::string strIndex = tex.name.substr(name.size());
                if (isNumber(strIndex))
                    index = stoi(strIndex);

                if (n == SemanticMap::History) {
                    if (index >= map.textures[SemanticMap::History].maxElements) {
                        error = tex.name + " exceeds max history frames of " + std::to_string(map.textures[SemanticMap::History].maxElements);
                        return false;
                    }

                    if (index > curHistorySize)
                        curHistorySize = index;

                    semTex.data = ((uintptr_t)map.textures[SemanticMap::History].image + index * map.textures[SemanticMap::History].stride);
                    goto Next;
                } else {
                    index += getSubChainIndex(preset, passId);

                    if ((n == SemanticMap::PassOutput) && (index >= passId)) {
                        error = tex.name + " cannot use output of future pass " + std::to_string(passId);
                        return false;
                    }

                    if (index >= map.textures[n].maxElements) {
                        error = tex.name + " exceeds max textures of " + std::to_string(map.textures[n].maxElements);
                        return false;
                    }

                    for(int p = index; p >= 0; p--) {
                        auto& pass = preset->passes[p];
                        if (!pass.inUse)
                            continue;

                        if (n == SemanticMap::PassFeedback)
                            semTex.feedbackPass = p;

                        semTex.data = ((uintptr_t)map.textures[n].image + index * map.textures[n].stride);
                        goto Next;
                    }
                }
            }
        }

        error = tex.name + " cannot be identified";
        return false;

        Next:
            semanticTextures.push_back(semTex);
    }

    return true;
}

auto SpirvReflection::findString(const std::string& strHaystack, const std::string& strNeedle) -> bool {

    auto it = std::search(
            strHaystack.begin(), strHaystack.end(),
            strNeedle.begin(),   strNeedle.end(),
            [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
    );

    return (it != strHaystack.end() );
}

auto SpirvReflection::isNumber(const std::string& str) -> bool {
    return !str.empty() && std::find_if(str.begin(), str.end(), [](char c) { return !std::isdigit(c); }) == str.end();
}