
#pragma once

struct CropPass {
    unsigned top = 0;
    unsigned left = 0;
    unsigned bottom = 0;
    unsigned right = 0;

    bool active = false;

    auto release() -> void {
        top = left = bottom = right = 0;
        active = false;
    }

    auto set(const CropPass& crop) -> void {
        top = crop.top;
        left = crop.left;
        bottom = crop.bottom;
        right = crop.right;
        active = top || left || bottom || right;
    }
};

struct ShaderPreset {
    enum WrapMode { WRAP_BORDER = 0, WRAP_EDGE, WRAP_REPEAT, WRAP_MIRRORED_REPEAT };
    enum Filter { FILTER_UNSPEC = 0, FILTER_LINEAR, FILTER_NEAREST };
    enum BufferType { BUFFER_UNORM = 0, BUFFER_SRGB, BUFFER_FP };
    enum ScaleType { SCALE_NONE = -1, SCALE_INPUT = 0, SCALE_ABSOLUTE, SCALE_VIEWPORT };

    int feedback;
    bool lumaChroma; // format of incoming frame data (uses floating point for YUV/YIC)

    struct Pass {
        std::string src;
        std::string code;
        Filter filter;
        WrapMode wrap;
        unsigned frameModulo;
        BufferType bufferType;
        bool mipmap;
        std::string alias;
        bool inUse;
        CropPass crop;
        std::string error;

        ScaleType scaleTypeX;
        ScaleType scaleTypeY;
        float scaleX;
        float scaleY;
        unsigned absX;
        unsigned absY;
    };
    std::vector<Pass> passes;

    struct Lut {
        Filter filter;
        WrapMode wrap;
        std::string id;
        std::string path;
        bool mipmap;

        uint8_t* data; // alternate
        unsigned width;
        unsigned height;
    };
    std::vector<Lut> luts;

    struct Param {
        int pass;
        float value;
        float minimum;
        float maximum;
        float initial;
        float step;
        std::string id;
        std::string desc;

        auto isDescriptor() -> bool {
            return (minimum == maximum) || ((maximum == step) && (step <= 0.01));
        }
    };
    std::vector<Param> params;

    auto clear() {
        params.clear();
        luts.clear();
        passes.clear();
        feedback = -1;
        lumaChroma = false;
    }
};

struct ShaderPass {	

    CropPass crop;
    
	bool primary = false;
    bool internalFormatMatchesData = false;
    bool external = true;
    bool mipmap = false;
	
	std::string fragment = "";	//fragment, pixel or as effect (any shader)
	std::string vertex = "";	//vertex
	std::string geometry = "";	//geometry

	std::string filter = "";
	std::string wrap = "";
	std::string format = "";

	unsigned relativeWidth = 0;
	unsigned relativeHeight = 0;
	unsigned modulo = 0;    

    std::string error = "";
    std::string ident = "";
    unsigned program = 0;
    
    ShaderPass() {}
    
    ShaderPass(const ShaderPass& source) {
        operator=(source);
    }

    ShaderPass& operator=(const ShaderPass& source) {
        if(this == &source)
            return *this;

        primary = source.primary;     
        internalFormatMatchesData = source.internalFormatMatchesData;
        fragment = source.fragment;
        vertex = source.vertex;
        geometry = source.geometry;
        filter = source.filter;
        wrap = source.wrap;                
        format = source.format;
        relativeHeight = source.relativeHeight;
        relativeWidth = source.relativeWidth;
        modulo = source.modulo;
        error = source.error;
        ident = source.ident;
        program = source.program;
        crop = source.crop;
        external = source.external;
        mipmap = source.mipmap;

        return *this;
    }
    
};
