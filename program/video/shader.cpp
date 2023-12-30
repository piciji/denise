
#define _USE_MATH_DEFINES
#include <cmath>

#include "shader.h"
#include "manager.h"
#include "../../driver/tools/shaderpass.h"
#include "../program.h"
#include "../view/view.h"
#include "../tools/sincFirFilter.h"
#include "../tools/mathVector.h"
#include "../tools/gaussianBlur.h"

#include "mask.cpp"

#define _doubleToStr(x) GUIKIT::String::convertDoubleToString(x)

#include "shader_glsl.cpp"
#include "shader_hlsl.cpp"

auto Shader::build() -> void {
    auto format = videoDriver->shaderFormat();
    if(format == DRIVER::Video::ShaderType::NotSupported)
        return;
    
    auto filter = vManager->settings->get<unsigned>( "video_filter", 1u, {0u, 1u});
    preset.clear();
    preset.feedback = -1;
    preset.bufferType = ShaderPreset::BUFFER_FP;

    ShaderPreset::Pass passX;
    passX.inUse = true;
    passX.native = true;
    passX.wrap = ShaderPreset::WRAP_EDGE;
    passX.frameModulo = 0;
    passX.bufferType = ShaderPreset::BUFFER_FP;
    passX.scaleTypeX = ShaderPreset::SCALE_INPUT;
    passX.scaleTypeY = ShaderPreset::SCALE_INPUT;
    passX.dontScaleIfInterlace = false;
    passX.mipmap = false;
    passX.crop.active = false;

	if (vManager->crtMode == VideoManager::CrtMode::Gpu) {
        passX.src = buildOutputEncoding(format);
        passX.filter = ShaderPreset::FILTER_NEAREST;
        passX.alias = "outputEncoding";
        passX.scaleX = vManager->firSharp == 0 ? 1.0 : 2.0;
        passX.scaleY = 1.0;
        preset.passes.push_back(passX);
		
        if (vManager->randomLineOffset) {
            passX.src = buildRandomLineOffset(format);
            passX.filter = ShaderPreset::FILTER_NEAREST;
            passX.alias = "randomLine";
            passX.scaleX = 1.0;
            passX.scaleY = 1.0;
            preset.passes.push_back(passX);
        }

        if (vManager->useLumaDelay() ) {
            passX.src = buildLumaLatency(format);
            passX.filter = ShaderPreset::FILTER_NEAREST;
            passX.alias = "lumaLatency";
            passX.scaleX = 1.0;
            passX.scaleY = 1.0;
            preset.passes.push_back(passX);
        }

        if (vManager->lumaNoise || vManager->chromaNoise) {
            passX.src = buildNoise(format);
            passX.filter = ShaderPreset::FILTER_NEAREST;
            passX.alias = "noise";
            passX.scaleX = 1.0;
            passX.scaleY = 1.0;
            preset.passes.push_back(passX);
        }

        passX.src = buildBandwidthReduction(format);
        passX.filter = ShaderPreset::FILTER_NEAREST;
        passX.alias = "bandwidth";
        passX.scaleX = vManager->firSharp == 0 ? 2.0 : 1.0;
        passX.scaleY = 1.0;
        preset.passes.push_back(passX);

        passX.src = buildDelayLineAndConvertToRgb(format);
        passX.filter = ShaderPreset::FILTER_NEAREST;
        passX.alias = "delayLine";
        passX.scaleX = 1.0;
        passX.scaleY = 1.0;
        preset.passes.push_back(passX);

        passX.src = buildGammaAndScanlines(format);
        passX.filter = ShaderPreset::FILTER_NEAREST;
        passX.alias = "scanlines";
        passX.scaleX = 1.0;
        passX.scaleY = 2.0;
        passX.dontScaleIfInterlace = true;
        preset.passes.push_back(passX);

        passX.dontScaleIfInterlace = false;

        if (vManager->bloomGlow) {
            passX.src = buildBloom(format, true);
            passX.filter = filter == 1 ? ShaderPreset::FILTER_LINEAR : ShaderPreset::FILTER_NEAREST;
            passX.alias = "bloomPhase1";
            passX.scaleX = 1.0;
            passX.scaleY = 1.0;
            passX.mipmap = true;
            preset.passes.push_back(passX);

            passX.src = buildBloom(format, false);
            passX.filter = ShaderPreset::FILTER_NEAREST;
            passX.alias = "bloom";
            passX.scaleX = 1.0;
            passX.scaleY = 1.0;
            passX.mipmap = false;
            preset.passes.push_back(passX);
        }

        passX.src = "";
        passX.filter = filter == 1 ? ShaderPreset::FILTER_LINEAR : ShaderPreset::FILTER_NEAREST;
        passX.alias = "crop";
        passX.scaleX = 1.0;
        passX.scaleY = 1.0;
        passX.mipmap = false;
        // remove first non visible line(s). it was needed as delay line to calculate first visible line
        passX.crop.set({2, SHADER_OFFSCREEN_WIDTH << 1, 0, SHADER_OFFSCREEN_WIDTH << 1});
        preset.passes.push_back(passX);

        passX.crop.active = false;
        // post shading runs after external shaders
        if (vManager->useMask()) {
            passX.src = buildMask(format);
            passX.filter = filter == 1 ? ShaderPreset::FILTER_LINEAR : ShaderPreset::FILTER_NEAREST;
            passX.alias = "crtMask";
            passX.scaleX = vManager->hires ? 2.0 : 1.0;
            passX.scaleY = vManager->hires ? 2.0 : 1.0;
            passX.mipmap = true;
            preset.passes.push_back(passX);
        }

        if (vManager->radialDistortion) {
            passX.src = buildRadialDistortion(format);
            passX.filter = filter == 1 ? ShaderPreset::FILTER_LINEAR : ShaderPreset::FILTER_NEAREST;
            passX.alias = "radialDistortion";
            passX.scaleX = vManager->distortionHires ? 2.0 : 1.0;
            passX.scaleY = vManager->distortionHires ? 2.0 : 1.0;
            passX.mipmap = true;
            preset.passes.push_back(passX);
        }

        addParams();

        videoDriver->setShader(preset);
	}
}

auto Shader::addParams() -> void {
    ShaderPreset::Param param;
    ShaderPreset::DynamicTexture dynamicTexture;
    ShaderPreset::Lut lut;
    int passId;
    unsigned cropTop = vManager->emulator->cropTop();
    unsigned cropLeft = vManager->emulator->cropLeft();

    passId = getPassId("outputEncoding");
    if (passId >= 0) {
        param.pass = passId;
        param.id = "rotU";
        param.value = std::cos(vManager->phaseError * M_PI / 180.0);;
        preset.params.push_back(param);

        param.id = "rotV";
        param.value = std::sin(vManager->phaseError * M_PI / 180.0);;
        preset.params.push_back(param);

        param.id = "oddLine";
        param.value = vManager->laceMode ? ((cropTop >> 1) & 1) : (cropTop & 1);
        preset.params.push_back(param);

        if (vManager->isC64()) {
            param.id = "BA";
            param.value = vManager->baGlitch;
            preset.params.push_back(param);
            param.id = "AEC";
            param.value = vManager->aecGlitch;
            preset.params.push_back(param);
            param.id = "PHI0";
            param.value = vManager->phi0Glitch;
            preset.params.push_back(param);
            param.id = "RAS";
            param.value = vManager->rasGlitch;
            preset.params.push_back(param);
            param.id = "CAS";
            param.value = vManager->casGlitch;
            preset.params.push_back(param);
            param.id = "cyclePixel";
            param.value = float((cropLeft + (vManager->pal ? 2 : 4)) & 7);
            preset.params.push_back(param);
        }
    }

    passId = getPassId("randomLine");
    if (passId >= 0) {
        param.pass = passId;
        param.id = "lineFactor";
        param.value = vManager->randomLineOffset;
        preset.params.push_back(param);
    }

    passId = getPassId("noise");
    if (passId >= 0) {
        param.pass = passId;
        param.id = "lumaNoise";
        param.value = vManager->lumaNoise;
        preset.params.push_back(param);

        param.id = "chromaNoise";
        param.value = vManager->chromaNoise;
        preset.params.push_back(param);
    }

    passId = getPassId("lumaLatency");
    if (passId >= 0) {
        param.pass = passId;
        param.id = "lumaRise";
        param.value = vManager->lumaRise == 0.0 ? 1.0f : (float) vManager->lumaRise;
        preset.params.push_back(param);

        param.id = "lumaFall";
        param.value = vManager->lumaFall == 0.0 ? 1.0f : (float) vManager->lumaFall;
        preset.params.push_back(param);
    }

    passId = getPassId("delayLine");
    if (passId >= 0) {
        param.pass = passId;

        float _hanBar = (double) vManager->hanoverBars / 128.0;
        float _hanBarAlt = (double) vManager->hanoverBarsAlt / 128.0;
        if (_hanBarAlt == 0.0)
            _hanBarAlt = 1.0;

        param.id = "hanoverBars";
        param.value = _hanBar;
        preset.params.push_back(param);

        param.id = "hanoverBarsAlt";
        param.value = _hanBarAlt;
        preset.params.push_back(param);

        param.id = "oddLine";
        param.value = vManager->laceMode ? ((cropTop >> 1) & 1) : (cropTop & 1);
        preset.params.push_back(param);
    }

    passId = getPassId("scanlines");
    if (passId >= 0) {
        dynamicTexture.pass = passId;
        dynamicTexture.id = "gammaWithShade";
        dynamicTexture.data = &vManager->preCalcScanlineF[0];
        dynamicTexture.width = 512 * 3;
        preset.dynamicTextures.push_back(dynamicTexture);
        dynamicTexture.id = "gamma";
        dynamicTexture.data = &vManager->preCalcF[0];
        dynamicTexture.width = 256 * 3;
        preset.dynamicTextures.push_back(dynamicTexture);
    }

    passId = getPassId("bloom");
    if (passId >= 0) {
        param.pass = passId;
        param.id = "weight";
        param.value = vManager->bloomWeight;
        preset.params.push_back(param);

        param.id = "glow";
        param.value = vManager->bloomGlow;
        preset.params.push_back(param);
    }

    passId = getPassId("crtMask");
    if (passId >= 0) {
        param.pass = passId;
        param.id = "lightFromCenter";
        param.value = vManager->lightFromCenter;
        preset.params.push_back(param);
        param.id = "luminance";
        param.value = vManager->maskLevel ? vManager->maskLuminance : vManager->luminance;
        preset.params.push_back(param);
        param.id = "maskLevel";
        param.value = vManager->maskLevel;
        preset.params.push_back(param);

        float scaleX, scaleY;
        scaleMask(scaleX, scaleY);
        param.id = "maskScaleX";
        param.value = scaleX;
        preset.params.push_back(param);
        param.id = "maskScaleY";
        param.value = scaleY;
        preset.params.push_back(param);

        GUIKIT::Image* useImage;
        switch(vManager->maskType) {
            default:
            case VideoManager::MaskType::Aperture: useImage = &imageAperture; break;
            case VideoManager::MaskType::ShadowMask: useImage = &imageShadowMask; break;
            case VideoManager::MaskType::SlotMask: useImage = &imageSlotMask; break;
        }
        lut.id = "maskLayer";
        lut.mipmap = true;
        lut.filter = ShaderPreset::FILTER_LINEAR;
        lut.wrap = ShaderPreset::WRAP_REPEAT;
        lut.width = useImage->width;
        lut.height = useImage->height;
        lut.data = (uint32_t*)useImage->data;
    }

    passId = getPassId("radialDistortion");
    if (passId >= 0) {
        param.pass = passId;
        param.id = "Factor";
        param.value = vManager->radialDistortion;
        preset.params.push_back(param);
        param.id = "Scale";
        param.value = calcRadialScale( vManager->radialDistortion );
        preset.params.push_back(param);
    }

    paramsDirty = false;
}

auto Shader::getPassId(const std::string& ident) -> int {
    for(int i = 0; i < preset.passes.size(); i++) {
        ShaderPreset::Pass& pass = preset.passes[i];

        if (pass.alias == ident)
            return 0;
    }
    return -1;
}


//auto Shader::transferDataToShader() -> void {
//    transferOutputEncoding();
//    transferLumaLatency();
//    transferNoise();
//    transferDelayLine();
//    transferGammaAndScanlines();
//    transferRadialDistortion();
//	transferMaskTexture();
//    transferMask();
//    transferLuminance();
//    transferRandomLine();
//	transferBloom();
//}

auto Shader::transferBloom() -> void {
	setAttribute("bloom", "weight", vManager->bloomWeight);
	setAttribute("bloom", "glow", vManager->bloomGlow);
}

auto Shader::transferDelayLine() -> void {
    
    float _hanBar = (double) vManager->hanoverBars / 128.0;
    float _hanBarAlt = (double) vManager->hanoverBarsAlt / 128.0;
    if (_hanBarAlt == 0.0)
        _hanBarAlt = 1.0;

    setAttribute("delayLine", "hanoverBars", _hanBar);
    setAttribute("delayLine", "hanoverBarsAlt", _hanBarAlt);
    if (lace)
        setAttribute("delayLine", "oddLine", (int)((vManager->emulator->cropTop() >> 1) & 1));
    else
        setAttribute("delayLine", "oddLine", (int) (vManager->emulator->cropTop() & 1));
}

auto Shader::transferOutputEncoding() -> void {
    float rotU = std::cos(vManager->phaseError * M_PI / 180.0); 
    float rotV = std::sin(vManager->phaseError * M_PI / 180.0); 

    setAttribute("outputEncoding", "rotU", rotU);
    setAttribute("outputEncoding", "rotV", rotV);
    if (lace)
        setAttribute("outputEncoding", "oddLine", (int)((vManager->emulator->cropTop() >> 1) & 1) );
    else
        setAttribute("outputEncoding", "oddLine", (int)(vManager->emulator->cropTop() & 1 ) );

    if (!vManager->isC64())
        return;

    setAttribute("outputEncoding", "BA", vManager->baGlitch);
    setAttribute("outputEncoding", "AEC", vManager->aecGlitch);
    setAttribute("outputEncoding", "PHI0", vManager->phi0Glitch);
    setAttribute("outputEncoding", "RAS", vManager->rasGlitch);
    setAttribute("outputEncoding", "CAS", vManager->casGlitch);

    unsigned cropLeft = vManager->emulator->cropLeft();
    cropLeft += vManager->pal ? 2 : 4;
    cropLeft &= 7;

    setAttribute("outputEncoding", "cyclePixel", (int)cropLeft );
}

auto Shader::transferNoise() -> void {
    setAttribute("noise", "lumaNoise", vManager->lumaNoise );
    setAttribute("noise", "chromaNoise", vManager->chromaNoise );
}

auto Shader::transferLumaLatency() -> void {
	setAttribute("lumaLatency", "lumaRise", vManager->lumaRise == 0.0 ? 1.0f : (float)vManager->lumaRise );
    setAttribute("lumaLatency", "lumaFall", vManager->lumaFall == 0.0 ? 1.0f : (float)vManager->lumaFall );   
}

auto Shader::transferRadialDistortion() -> void {
    setAttribute("radialDistortion", "Factor", vManager->radialDistortion );
    setAttribute("radialDistortion", "Scale", calcRadialScale( vManager->radialDistortion ) );
}

auto Shader::transferLuminance() -> void {
    setAttribute( "crtMask", "lightFromCenter", vManager->lightFromCenter );
    setAttribute( "crtMask", "luminance", vManager->maskLevel ? vManager->maskLuminance : vManager->luminance );
}

auto Shader::transferRandomLine() -> void {
    setAttribute( "randomLine", "lineFactor", vManager->randomLineOffset );
}

auto Shader::transferMaskTexture() -> void {
	
	GUIKIT::Image* useImage = nullptr;
	
	switch(vManager->maskType) {
		
		default:
		case VideoManager::MaskType::Aperture:
			useImage = &imageAperture;
			break;
		case VideoManager::MaskType::ShadowMask:
			useImage = &imageShadowMask;
			break;
		case VideoManager::MaskType::SlotMask:
			useImage = &imageSlotMask;
			break;					
	}
	
	if (useImage)
		videoDriver->setShaderAttribute( "crtMask", "maskLayer", (uint32_t*)useImage->data, useImage->width, useImage->height );
}

auto Shader::scaleMask(float& scaleX, float& scaleY) -> void {
	// 1 inch = 25.4 mm
	// x dpi -> x dots = 25.4 mm
	// 1 dot = y mm (how much mm takes one dot)
	// y = 25.4 mm / x dots
    auto format = videoDriver->shaderFormat();
    int mipmappingScale = 0;

    if (format == ShaderFormat::GLSL)
        mipmappingScale = 1;

	float oneDotWidth = 25.4f / (float)vManager->maskDpi;
	
	// dot pitch means distance between two red holes in mask   
	scaleX = ((float)(vManager->emulator->cropWidth() ) * oneDotWidth) / vManager->maskPitch;
    
    switch(vManager->maskType) {
		
		default:
		case VideoManager::MaskType::Aperture:
        case VideoManager::MaskType::SlotMask: {    
     
            scaleY = ((float)(vManager->emulator->cropWidth() << mipmappingScale) * scaleX) / ((float)(vManager->emulator->cropHeight() ));
        } break;
        case VideoManager::MaskType::ShadowMask: {
             
            scaleY = ((float)(vManager->emulator->cropWidth()) * scaleX) / (float)(vManager->emulator->cropHeight() );
        } break;                    
    }
    
	//setAttribute( "crtMask", "maskLevel", vManager->maskLevel );
	//setAttribute( "crtMask", "maskScaleX", scaleX );
	//setAttribute( "crtMask", "maskScaleY", scaleY );
}

auto Shader::setAttribute(std::string program, std::string attribute, float value) -> void {
    
    videoDriver->setShaderAttribute( program, attribute, value );
}

auto Shader::setAttribute(std::string program, std::string attribute, int value) -> void {
    
    videoDriver->setShaderAttribute( program, attribute, value );
}

auto Shader::calcRadialScale(float intensity) -> float {

    auto radialDistortion = [intensity]( MathVector::Vec2 xy ) -> MathVector::Vec2 {

        MathVector::Vec2 center = MathVector::Sub( xy, {0.5, 0.5} );
        float dist = MathVector::Dot(center, center);
        dist *= intensity;

        return MathVector::Add( xy, MathVector::Mul( center, (1.0 + dist) * dist ) );
    };

    float offset = 4.0;
    float width = (float)(vManager->emulator->cropWidth());
    float xL = offset / width;
    float xR = (width - offset) / width;

    return 1.0f / sqrt( MathVector::Distance( radialDistortion( {xL * intensity, 0.0} ), radialDistortion( {1.0f - intensity + xR * intensity, 0.0} ) ) );
}

auto Shader::buildOutputEncoding(ShaderFormat& format) -> std::string {
    switch(format) {
        case ShaderFormat::GLSL:
            return buildOutputEncodingGLSL();
        case ShaderFormat::HLSL:
            return buildOutputEncodingHLSL();
    }
    return "";
}

auto Shader::buildLumaLatency(ShaderFormat& format) -> std::string {
    switch(format) {
        case ShaderFormat::GLSL:
            return buildLumaLatencyGLSL();
        case ShaderFormat::HLSL:
            return buildLumaLatencyHLSL();
    }
    return "";
}

auto Shader::buildNoise(ShaderFormat& format) -> std::string {
    switch(format) {
        case ShaderFormat::GLSL:
            return buildNoiseGLSL();
        case ShaderFormat::HLSL:
            return buildNoiseHLSL();
    }
    return "";
}

auto Shader::buildRandomLineOffset(ShaderFormat& format) -> std::string {
    switch(format) {
        case ShaderFormat::GLSL:
            return buildRandomLineOffsetGLSL();
        case ShaderFormat::HLSL:
            return buildRandomLineOffsetHLSL();
    }
    return "";
}

auto Shader::buildBandwidthReduction(ShaderFormat& format) -> std::string {
    switch(format) {
        case ShaderFormat::GLSL:
            return buildBandwidthReductionGLSL();
        case ShaderFormat::HLSL:
            return buildBandwidthReductionHLSL();
    }
    return "";
}

auto Shader::buildDelayLineAndConvertToRgb(ShaderFormat& format) -> std::string {
    switch(format) {
        case ShaderFormat::GLSL:
            return buildDelayLineAndConvertToRgbGLSL();
        case ShaderFormat::HLSL:
            return buildDelayLineAndConvertToRgbHLSL();
    }
    return "";
}

auto Shader::buildGamma(ShaderFormat& format) -> std::string {
    switch(format) {
        case ShaderFormat::GLSL:
            return buildGammaGLSL();
        case ShaderFormat::HLSL:
            return buildGammaHLSL();
    }
    return "";
}

auto Shader::buildGammaAndScanlines(ShaderFormat& format) -> std::string {
    switch(format) {
        case ShaderFormat::GLSL:
            return buildGammaAndScanlinesGLSL();
        case ShaderFormat::HLSL:
            return buildGammaAndScanlinesHLSL();
    }
    return "";
}

auto Shader::buildRadialDistortion(ShaderFormat& format) -> std::string {
    switch(format) {
        case ShaderFormat::GLSL:
            return buildRadialDistortionGLSL();
        case ShaderFormat::HLSL:
            return buildRadialDistortionHLSL();
    }
    return "";
}

auto Shader::buildMask(ShaderFormat& format) -> std::string {
    switch(format) {
        case ShaderFormat::GLSL:
            return buildMaskGLSL();
        case ShaderFormat::HLSL:
            return buildMaskHLSL();
    }
    return "";
}

auto Shader::buildBloom(ShaderFormat& format, bool phase1) -> std::string {
    switch(format) {
        case ShaderFormat::GLSL:
            return buildBloomGLSL(phase1);
        case ShaderFormat::HLSL:
            return buildBloomHLSL(phase1);
    }
    return "";
}

auto Shader::addPreset(std::string path, bool prepend, std::vector<std::string>& brokenPaths) -> ShaderPreset* {
    if (!parser)
        return loadPreset(path, brokenPaths);

    ShaderParser* tempParser = new ShaderParser;

    bool res = tempParser->loadPreset(path);
    GUIKIT::Vector::combine(brokenPaths, tempParser->brokenPaths);

    if (!res) {
        delete tempParser;
        return nullptr;
    }

    ShaderPreset* preset = &parser->shaderPreset;

    parser->addPreset( tempParser, prepend );

    delete tempParser;
    recreate = true;
    return preset;
}

auto Shader::loadPreset(const std::string& path, std::vector<std::string>& brokenPaths) -> ShaderPreset* {
    ShaderParser* tempParser = new ShaderParser;

    bool res = tempParser->loadPreset(path);
    GUIKIT::Vector::combine(brokenPaths, tempParser->brokenPaths);

    if (!res) {
        delete tempParser;
        return nullptr;
    }

    if (parser) {
        parser->clear();
        delete parser;
    }

    parser = tempParser;
    recreate = true;

    return &parser->shaderPreset;
}

auto Shader::savePreset(std::string path) -> bool {
    if (parser)
        return parser->savePreset(path);

    return false;
}

auto Shader::getPreset(std::vector<std::string>& brokenPaths) -> ShaderPreset* {
    if (parser) {
        GUIKIT::Vector::combine(brokenPaths, parser->brokenPaths);
        return &parser->shaderPreset;
    }

    return nullptr;
}

auto Shader::getPreset() -> ShaderPreset* {
    if (parser)
        return &parser->shaderPreset;

    return nullptr;
}

auto Shader::clearPreset() -> void {
    if (parser)
        parser->clear();

    recreate = true;
}

auto Shader::getPresetPathCombined() -> std::string {
    if (!parser)
        return "";

    return parser->getPresetPathCombined();
}

auto Shader::getPresetPath() -> std::string {
    if (parser)
        return parser->getPresetPath();

    return "";
}

auto Shader::movePass(unsigned& passId, bool up) -> void {
    if (parser)
        parser->movePass(passId, up);
}

auto Shader::togglePassUsage(unsigned passId) -> ShaderPreset::Pass* {
    if (parser)
        return parser->togglePassUsage(passId);

    return nullptr;
}

auto Shader::setPassFilter(unsigned passId, ShaderPreset::Filter filter) -> void {
    if (parser)
        parser->setPassFilter(passId, filter);
}

Shader::Shader(VideoManager* vManager) {
    this->vManager = vManager;
	buildMaskTexture();
}

Shader::~Shader() {

}
