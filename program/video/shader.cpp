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

auto Shader::sendToDriver(bool retry) -> bool {
    std::vector<ShaderPass*> out;

    if (primaryPass) {
        delete primaryPass;
        primaryPass = nullptr;
    }
	
    auto extPrimary = getPrimary(externalPasses);
    auto intPrimary = getPrimary(internalPasses);

    if (extPrimary || intPrimary) {

        primaryPass = new ShaderPass;
        primaryPass->primary = true;
        primaryPass->internalFormatMatchesData = false;

        if (extPrimary) {            
            primaryPass->format = extPrimary->format;
            primaryPass->wrap = extPrimary->wrap;
            primaryPass->modulo = extPrimary->modulo;
			primaryPass->filter = extPrimary->filter;
            primaryPass->relativeHeight = extPrimary->relativeHeight;
            primaryPass->relativeWidth = extPrimary->relativeWidth;
        }

        if (intPrimary) {
            primaryPass->format = intPrimary->format;
            primaryPass->wrap = intPrimary->wrap;
            primaryPass->vertex = intPrimary->vertex;
            primaryPass->fragment = intPrimary->fragment;
			primaryPass->filter = intPrimary->filter;
            primaryPass->internalFormatMatchesData = intPrimary->internalFormatMatchesData;
            primaryPass->mipmap = intPrimary->mipmap;
        }

        out.push_back(primaryPass);
    }
    
    for (auto pass : internalPasses) {
        if (!pass->primary)
            out.push_back(pass);
    }   
    
	for (auto pass : externalPasses) {
        if (!pass->primary)
            out.push_back(pass);
    }  

    for (auto pass : internalPassesPost) {
        if (!pass->primary)
            out.push_back(pass);
    }
    
	videoDriver->setShader( out );    
	
	std::string error = "";
	
	for( auto pass : out )
        if (pass->error != "")
            error += "\n" + pass->ident + ": " + pass->error;

    if (!retry && !error.empty()) {        
        view->message->error( error );   
        removeIncompleteShader();
        sendToDriver( true ); // try again
    } else
        transferDataToShader();

	return error.empty();
}

auto Shader::getPrimary(std::vector<ShaderPass*>& passes) -> ShaderPass* {
    for (auto pass : passes)
        if (pass->primary)
            return pass;  
    
    return nullptr;
}  

auto Shader::removeIncompleteShader() -> void {
			
	std::vector<std::string> shaders;
	std::vector<ShaderPass*> usePasses;		
	
	for (auto pass : externalPasses) {
		if (!pass->error.empty())
			continue;
		
		for(auto pass2 : externalPasses) {
			
			if (pass2->error.empty())
				continue;
			
			if (pass->ident == pass2->ident) {
				pass->error = "error";				
				break;
			}
		}
	}
	
	for (auto pass : externalPasses) {
		if (pass->error.empty()) {
			usePasses.push_back( pass );
			if (!pass->primary)
				shaders.push_back( pass->ident );	
		} else
			delete pass;
	}
	
	externalPasses = usePasses;		

	if (externalPasses.size() == 1)
        clean(externalPasses); 
	
	GUIKIT::String::removeDuplicates( shaders );
	
    std::string shaderList = "";    
	for(auto& s : shaders) {
        
        if (!shaderList.empty())
            shaderList += "###";
        
		shaderList += s;
    }
	
	vManager->settings->set<std::string>("shader", shaderList);
}      

auto Shader::loadInternal() -> void {
	
    clean(internalPasses);
    clean(internalPassesPost);
    auto format = videoDriver->shaderFormat();
    if(format == DRIVER::Video::ShaderType::NotSupported)
        return;
    
    auto filter = vManager->settings->get<unsigned>( "video_filter", 1u, {0u, 1u});
    ShaderPass* pass;

	if (vManager->crtMode == VideoManager::CrtMode::Gpu) {
		
		pass = new ShaderPass;
        addBaseProps(pass);
		pass->fragment = buildOutputEncoding(format);
		pass->ident = "outputEncoding";
		pass->filter = "nearest";
		pass->relativeWidth = vManager->firSharp == 0 ? 100 : 200;
        pass->relativeHeight = 100;
		internalPasses.push_back( pass );
		
        if (vManager->randomLineOffset) {
            pass = new ShaderPass;
            pass->primary = false;
            addBaseProps(pass);
            pass->fragment = buildRandomLineOffset(format);
            pass->ident = "randomLine";
            pass->relativeHeight = 100;
            pass->relativeWidth = 100;
            pass->filter = "nearest";
            internalPasses.push_back(pass);
        }

        if (vManager->useLumaDelay() ) {
            pass = new ShaderPass;
            addBaseProps(pass);
            pass->fragment = buildLumaLatency(format);
            pass->ident = "lumaLatency";
            pass->filter = "nearest";
            pass->relativeWidth = 100;
            pass->relativeHeight = 100;
            internalPasses.push_back( pass );
        }

        if (vManager->lumaNoise || vManager->chromaNoise) {
            ShaderPass* pass = new ShaderPass;
            addBaseProps(pass);
            pass->fragment = buildNoise(format);
            pass->ident = "noise";
            pass->filter = "nearest";
            pass->relativeWidth = 100;
            pass->relativeHeight = 100;
            internalPasses.push_back( pass );
        }
        
        pass = new ShaderPass;
        pass->primary = false;
        addBaseProps(pass);
        pass->fragment = buildBandwidthReduction(format);
        pass->ident = "bandwidth";
		pass->filter = "nearest";
		pass->relativeWidth = vManager->firSharp == 0 ? 200 : 100;
        pass->relativeHeight = 100;
        internalPasses.push_back(pass);

		pass = new ShaderPass;
		pass->primary = false;
		addBaseProps(pass);
		pass->fragment = buildDelayLineAndConvertToRgb(format);
		pass->ident = "delayLine";
		pass->filter = "nearest";
		pass->relativeWidth = 100;
		pass->relativeHeight = 100;
		internalPasses.push_back(pass);

        if (vManager->scanlines && !lace) {
            pass = new ShaderPass;
            pass->primary = false;
            addBaseProps(pass);
            pass->fragment = buildGammaAndScanlines(format);
            pass->ident = "scanlines";
            pass->relativeWidth = 100;
            pass->relativeHeight = 200;
            pass->filter = "nearest";
            internalPasses.push_back(pass);

        } else {
			pass = new ShaderPass;
			pass->primary = false;
			addBaseProps(pass);
			pass->fragment = buildGamma(format);
			pass->ident = "gamma";
			pass->relativeWidth = 100;
			pass->relativeHeight = 100;
			pass->filter = "nearest";
			internalPasses.push_back( pass );
		}

        if (vManager->bloomGlow) {
            pass = new ShaderPass;
            pass->primary = false;
            addBaseProps(pass);
            pass->fragment = buildBloom(format, true);
            pass->ident = "bloomPhase1";
            pass->relativeWidth = 100;
            pass->relativeHeight = (vManager->scanlines || lace) ? 100 : 200;
            pass->filter = filter == 1 ? "linear" : "nearest";
            pass->mipmap = true;
            internalPasses.push_back(pass);

            pass = new ShaderPass;
            pass->primary = false;
            addBaseProps(pass);
            pass->fragment = buildBloom(format, false);
            pass->ident = "bloom";
            pass->relativeWidth = 100;
            pass->relativeHeight = 100;
            pass->filter = "nearest";
            internalPasses.push_back(pass);
        }
                      
        pass = new ShaderPass;
        pass->primary = false;
        addBaseProps(pass);
		pass->ident = "crop";
        // remove first non visible line(s). it was needed as delay line to calculate first visible line
        pass->crop.top = (vManager->scanlines || vManager->bloomGlow || lace) ? 2 : 1;
        pass->crop.bottom = (vManager->scanlines && !lace) ? 1 : 0;
        // same here, we have some pixel offscreen to calculate bandwidth reduction
		pass->crop.left = SHADER_OFFSCREEN_WIDTH << 1;
		pass->crop.right = SHADER_OFFSCREEN_WIDTH << 1;
        pass->filter = filter == 1 ? "linear" : "nearest";
		internalPasses.push_back( pass );
                    
        pass = new ShaderPass;
        pass->primary = true;
        addBaseProps(pass);
		pass->ident = "primary";		
		pass->filter = "nearest";
		internalPasses.push_back( pass );
        
	} else if (vManager->usePostShading()) {     
        
        pass = new ShaderPass;
        pass->primary = true;
		pass->ident = "primary";
		pass->filter = filter == 1 ? "linear" : "nearest";
		internalPasses.push_back( pass );

        if (vManager->bloomGlow) {
            pass = new ShaderPass;
            pass->primary = false;
            addBaseProps(pass);
            pass->fragment = buildBloom(format, true);
            pass->ident = "bloomPhase1";
            pass->relativeWidth = 100;
            pass->relativeHeight = 100;
            pass->filter = filter == 1 ? "linear" : "nearest";
            pass->mipmap = true;
            internalPasses.push_back(pass);

            pass = new ShaderPass;
            pass->primary = false;
            addBaseProps(pass);
            pass->fragment = buildBloom(format, false);
            pass->ident = "bloom";
            pass->relativeWidth = 100;
            pass->relativeHeight = 100;
            pass->filter = "nearest";
            internalPasses.push_back(pass);
        }
    }

	// post shading runs after external shaders
	if (vManager->usePostShading()) {
		pass = new ShaderPass;
		pass->primary = false;
		pass->external = false;
		pass->fragment = buildMask(format);
		pass->ident = "crtMask";

		pass->relativeHeight = vManager->hires ? 200 : 100;
		pass->relativeWidth = vManager->hires ? 200 : 100;
        normaliseDimension( pass->relativeWidth, pass->relativeHeight );
		pass->filter = filter == 1 ? "linear" : "nearest";
        pass->mipmap = true;
		internalPassesPost.push_back( pass );
	}

    if (vManager->radialDistortion) {
        pass = new ShaderPass;
        pass->primary = false;
        pass->external = false;
        pass->fragment = buildRadialDistortion(format);
        pass->ident = "radialDistortion";
        pass->relativeHeight = vManager->distortionHires ? 200 : 100;
        pass->relativeWidth = vManager->distortionHires ? 200 : 100;
        pass->mipmap = true;
        pass->filter = filter == 1 ? "linear" : "nearest";
        internalPassesPost.push_back(pass);
    }
}

auto Shader::normaliseDimension( unsigned& widthScale, unsigned& heightScale ) -> void {

    if (vManager->crtMode == VideoManager::CrtMode::Gpu) {
        if (vManager->scanlines || vManager->bloomGlow || lace);
		else
            heightScale <<= 1;            
    } else {            
        widthScale <<= 1;

        //if (!vManager->useCrtMode() || !vManager->scanlines)
        if (vManager->scanlines || lace);
        else
            heightScale <<= 1;
    }
}

auto Shader::addBaseProps( ShaderPass* pass ) -> void {

	pass->wrap = "border";
    pass->external = false;
	
    if (!pass->primary) {
        pass->format = "rgba32f";
		pass->internalFormatMatchesData = true;
        
	} else if (vManager->shaderInputPrecision) {
		pass->format = "rgba32f";
		pass->internalFormatMatchesData = true;
	} else {
		pass->format = "rgba8";
		pass->internalFormatMatchesData = true;
	}			
}

auto Shader::loadExternal() -> bool {

    externalLoaded = true; 
    loadErrors.clear();    
    clean(externalPasses);

    GUIKIT::Settings* shaderSetting = nullptr;

    std::string ident = "shader_folder_";
    program->appendShaderFormat(ident);
    auto folder = globalSettings->get<std::string>(ident, "");
    if (folder.empty())
        folder = program->shaderFolder();
    
    if (folder.empty())
        return true;

    folder = GUIKIT::File::beautifyPath(folder);

    auto shaders = getActiveShaders();
    if (shaders.empty())
        return true;        

    ShaderPass* ioPass = new ShaderPass;
    ioPass->primary = true;
    externalPasses.push_back(ioPass);
	
    for (auto& shaderIdent : shaders) {
        std::string path = folder + shaderIdent;

        if (!GUIKIT::File::isDir(path + "/")) {
			auto pass = new ShaderPass;
			pass->ident = shaderIdent;			
            std::string effect = loadShader(folder, shaderIdent, pass);
            if (effect.empty()) {
				delete pass;
				continue;
			}            
            pass->fragment = effect;			
            externalPasses.push_back(pass);			
            continue;
        }
        path += "/";

        if (shaderSetting) delete shaderSetting;

        shaderSetting = new GUIKIT::Settings;
        if (!shaderSetting->load(path + "manifest.bml", 1024 * 1024, true)) {
            if (!shaderSetting->load(path + "manifest", 1024 * 1024, true)) {
                loadErrors.push_back(path + "manifest");
                continue;
            }
        }

        auto input = shaderSetting->find("input");
        if (input) mapPass(input, ioPass);

        auto output = shaderSetting->find("output");
        if (output) mapPass(output, ioPass);

        auto programs = shaderSetting->findMulti("program");

        for (auto program : programs) {
            auto pass = new ShaderPass;
			pass->ident = shaderIdent;
            mapPass(program, pass, path);
            externalPasses.push_back(pass);		
        }				
    }

    if (shaderSetting)
        delete shaderSetting;

    std::string error = "";

    if (!loadErrors.empty()) {
        for(auto& loadError : loadErrors)
            error += trans->get("file_open_error", {{"%path%", loadError}}) + "\n";            
    }

    if (externalPasses.size() == 1)
        clean(externalPasses);    
    
    if (!error.empty()) {
        removeIncompleteShader();
        view->message->warning( error );  
    }

	return error.empty();
}

auto Shader::mapPass(GUIKIT::Setting* theme, ShaderPass* pass, std::string path) -> void {

    for (auto setting : theme->childs) {
        if (setting->getIdent() == "filter") {
            pass->filter = setting->value;
        } else if (setting->getIdent() == "wrap") {
            pass->wrap = setting->value;
        } else if (setting->getIdent() == "format") {
            pass->format = setting->value;
        } else if (setting->getIdent() == "width") {
            pass->relativeWidth = GUIKIT::String::convertToNumber(setting->value);
        } else if (setting->getIdent() == "height") {
            pass->relativeHeight = GUIKIT::String::convertToNumber(setting->value);
        } else if (setting->getIdent() == "modulo") {
            pass->modulo = setting->uValue;
        } else if (setting->getIdent() == "vertex") {
            pass->vertex = loadShader(path, setting->value, pass);
        } else if (setting->getIdent() == "fragment") {
            pass->fragment = loadShader(path, setting->value, pass);
        } else if (setting->getIdent() == "geometry") {
            pass->geometry = loadShader(path, setting->value, pass);
        }
    }
}

auto Shader::loadShader(std::string path, std::string shaderFile, ShaderPass* pass) -> std::string {
	
	if (shaderFile.empty())
		return "";
	
	path += shaderFile;
    GUIKIT::File file(path);
    if (!file.open()) {
		pass->error = "error";
        loadErrors.push_back(path);
        return "";
    }
    std::string s;
    s.assign((char*) file.read(), file.getSize());
    return s;
}

auto Shader::addActiveShader(std::string shader) -> void {

    std::string shaderList = "";
    
    for (auto& activeShader : getActiveShaders())
        shaderList += activeShader + "###";    
    
    shaderList += shader;            
    
    vManager->settings->set<std::string>("shader", shaderList);
    
	bool error = !loadExternal(); 
	    
    if (activeVideoManager == vManager)
		error |= !sendToDriver();
    
	if (error) 
		view->updateShader();
}

auto Shader::removeActiveShader(std::string shader) -> void {
    std::string shaderList = "";

    for (auto& activeShader : getActiveShaders()) {
        if (shader != activeShader) {
            
            if (!shaderList.empty())
                shaderList += "###";
                
            shaderList += activeShader;
        }
    }
    
    vManager->settings->set<std::string>("shader", shaderList);
    
	bool error = !loadExternal(); 
	    
    if (activeVideoManager == vManager)
		error |= !sendToDriver();
    
	if (error) 
		view->updateShader();	    
}

auto Shader::getActiveShaders() -> std::vector<std::string> {
    auto activeShaders = vManager->settings->get<std::string>("shader", "");
    return GUIKIT::String::explode(activeShaders, "###");
}

auto Shader::clean(std::vector<ShaderPass*>& passes) -> void {
    for (auto pass : passes)
        delete pass;

    passes.clear();
}

auto Shader::transferDataToShader() -> void {
    transferOutputEncoding();
    transferLumaLatency();
    transferNoise();
    transferDelayLine();        
    transferGammaAndScanlines();          
    transferRadialDistortion();
	transferMaskTexture();
    transferMask();
    transferLuminance();
    transferRandomLine();
	transferBloom();
}

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

auto Shader::transferGammaAndScanlines() -> void {
    
    if (vManager->scanlines && !lace) {
        videoDriver->setShaderAttribute("scanlines", "gammaWithShade", &vManager->preCalcScanlineF[0], 512 * 3);    
        videoDriver->setShaderAttribute("scanlines", "gamma", &vManager->preCalcF[0], 256 * 3);
               
    } else {
        videoDriver->setShaderAttribute("gamma", "gamma", &vManager->preCalcF[0], 256 * 3);
    }
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

auto Shader::transferMask() -> void {
	// 1 inch = 25.4 mm
	// x dpi -> x dots = 25.4 mm
	// 1 dot = y mm (how much mm takes one dot)
	// y = 25.4 mm / x dots	
	float oneDotWidth = 25.4f / (float)vManager->maskDpi;
	
	// dot pitch means distance between two red holes in mask   
	float scaleX = ((float)(vManager->emulator->cropWidth() ) * oneDotWidth) / vManager->maskPitch;
	float scaleY;
    
    switch(vManager->maskType) {
		
		default:
		case VideoManager::MaskType::Aperture:
        case VideoManager::MaskType::SlotMask: {    
     
            scaleY = ((float)(vManager->emulator->cropWidth() << 1) * scaleX) / ((float)(vManager->emulator->cropHeight() ));
        } break;
        case VideoManager::MaskType::ShadowMask: {
             
            scaleY = ((float)(vManager->emulator->cropWidth()) * scaleX) / (float)(vManager->emulator->cropHeight() );
        } break;                    
    }
    
	setAttribute( "crtMask", "maskLevel", vManager->maskLevel );
	setAttribute( "crtMask", "maskScaleX", scaleX );
	setAttribute( "crtMask", "maskScaleY", scaleY );
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

Shader::Shader(VideoManager* vManager) {
    this->vManager = vManager;
	buildMaskTexture();
}

Shader::~Shader() {
    clean(externalPasses);
    clean(internalPasses);
    clean(internalPassesPost);
}
