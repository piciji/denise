
#ifdef __APPLE__
    #include <OpenGL/gl3.h>
#elif _WIN32
    #include <GL/gl.h>
    #include <GL/glext.h>
    #define glGetProcAddress(name) wglGetProcAddress(name)
#else
    #include <GL/gl.h>
    #include <GL/glx.h>
    #define glGetProcAddress(name) (*glXGetProcAddress)((const GLubyte*)(name))
#endif

#include <cstring>
#ifdef DRV_FREETYPE
    #include <ft2build.h>
    #include FT_FREETYPE_H
	#include "../../tools/font.h"
#endif

#include "bind.h"
#include "../../tools/chronos.h"

namespace DRIVER {

#include "../../tools/tools.h"
#include "../../tools/shaderpass.h"
#include "shaders.h"
#include "utility.h"
#ifdef DRV_FREETYPE
    #include "text.h"
#endif

struct CustomTexture {        
    std::string attribute;
    GLuint texture;
    GLenum target;
};    
    
struct OpenGLTexture {
	auto getFormat() const -> GLuint;
	auto getType() const -> GLuint;

	GLuint texture = 0;
	unsigned width = 0;
	unsigned height = 0;
	GLuint format = GL_RGBA8;
	GLuint filter = GL_LINEAR;
	GLuint wrap = GL_CLAMP_TO_BORDER;
    bool internalFormatMatchesData = false;    
    CropPass crop;
    std::vector<CustomTexture> customTextures;
    bool mipmap = false;
};

struct OpenGLSurface : OpenGLTexture {
	auto allocate() -> void;
	auto size(unsigned w, unsigned h) -> bool;
	auto release() -> void;
	auto render(unsigned targetLeft, unsigned targetTop, unsigned targetWidth, unsigned targetHeight) -> void;
	auto deleteBuffer() -> void;
    auto getBuffer(RenderBuffer* renderBuffer = nullptr) -> void*;
    auto cropTexture(OpenGLSurface* src) -> void;
    auto resize(unsigned w, unsigned h) -> void;
    auto resize( RenderBuffer* renderBuffer, unsigned w, unsigned h) -> void;
    auto createTexture(RenderBuffer* renderBuffer = nullptr) -> void;
    auto updateTexture(RenderBuffer* renderBuffer = nullptr) -> void;

    struct {
        GLfloat modelView[16];
        GLfloat projection[16];
        GLfloat vertices[16];
        GLfloat texCoords[8];
        GLfloat modelViewProjection[4 * 4];
        GLfloat positions[4 * 4];
        unsigned width = 0;
        unsigned height = 0;
    } mvp;

	GLuint program = 0;
	GLuint framebuffer = 0;
	GLuint vao = 0;
	GLuint vbo[3] = {0, 0, 0};
	GLuint vertex = 0;
	GLuint geometry = 0;
	GLuint fragment = 0;
	uint32_t* buffer = nullptr;
    int32_t* bufferInt = nullptr;
    float* bufferFloat= nullptr;
};

struct OpenGLProgram : OpenGLSurface {
	auto bind(ShaderPass& pass) -> std::string;
	auto release() -> void;    

	unsigned phase = 0;
	unsigned modulo = 0;
	double relativeWidth = 0;
	double relativeHeight = 0;         
};

struct OpenGL : OpenGLProgram {
    
    struct {
        bool synchronize = false;
        bool hardSync = false;
        bool threaded = false;
        Video::Filter filter = Video::Filter::Nearest;
        std::vector<ShaderPass*> passes = {};

        float aspectWidth = 1.0;
        float aspectHeight = 1.0;
        bool integerScaling = false;

        bool vrr = false;
    } settings;

	auto shader(std::vector<ShaderPass*> passes) -> void;
    template<typename T> auto shaderAttribute( std::string _program, std::string attribute, T value ) -> void;
    auto shaderAttribute(std::string _program, std::string attribute, float* data, unsigned size) -> void;
    auto shaderAttribute(std::string _program, std::string attribute, uint32_t* data, unsigned _width, unsigned _height ) -> void;
    auto getProgram( std::string ident ) -> GLuint;
    auto lock(unsigned*& data, unsigned& pitch) -> bool;
    auto lock(float*& data, unsigned& pitch) -> bool;
    auto lock(int32_t*& data, unsigned& pitch) -> bool;
	auto clear() -> void;
	auto refresh(bool disallowShader = false) -> void;
	auto init() -> bool;
	auto term() -> void;
    auto hardSync(unsigned frames = 0) -> void;
    auto getCustomTexture( std::string _program, std::string attribute ) -> CustomTexture*;
    auto calcViewport() -> void;
    auto initVRR(float speed) -> void;
    auto waitVRR() -> void;
    
#ifdef DRV_FREETYPE    
    OpenGLText screenText;
#endif

	std::vector<OpenGLProgram> programs;
	unsigned outputWidth = 0;
	unsigned outputHeight = 0;
    int outputLeft = 0;
    int outputTop = 0;
	bool initialized = false;
    unsigned integerScalingHeight = 0;
    unsigned windowWidth = 0;
    unsigned windowHeight = 0;

    int64_t lastCapTime;
    int64_t minimumCapTime;

    GLsync fences[4];
    unsigned fenceCount = 0;  
};

#include "surface.h"
#include "program.h"
#include "main.h"
#include "viewport.h"
#include "vrr.h"

}

