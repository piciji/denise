
static std::string OpenGLStockVertex = R"(
  #version 140

  in vec4 Position;
  in vec2 TexCoord;

  uniform mat4 MVP;

  out vec2 texCoordFrag;

  void main() {
    gl_Position = MVP * Position;
    texCoordFrag = TexCoord;
  }
)";

static std::string OpenGLStockFragment = R"(
  #version 140

  uniform sampler2D source;

  in vec2 texCoordFrag;

  out vec4 fragColor;

  void main() {
    fragColor = texture(source, texCoordFrag);
  }
)";

static std::string OpenGLTextVertexShader = R"(
  #version 140

  in vec4 fontCoords;

  uniform vec4 color;
  uniform vec4 bgColor;

  out vec4 fragColor;
  out vec4 fragBgColor;
  out vec2 fontTexCoords;

  void main() {
    fragColor = color;
    fragBgColor = bgColor;
    fontTexCoords = fontCoords.zw;

    gl_Position = vec4(fontCoords.xy, 0.0, 1.0);
}
)";

static std::string OpenGLTextFragmentShader = R"(
  #version 140

  in vec4 fragColor;
  in vec4 fragBgColor;
  in vec2 fontTexCoords;
  out vec4 glFragColor;

  uniform sampler2D tex;

  void main() {
    if(fragBgColor.a == 0.0)
      glFragColor = vec4(1.0, 1.0, 1.0, texture(tex, fontTexCoords).r) * fragColor;
    else
      glFragColor = vec4( mix(fragBgColor.rgb, fragColor.rgb, texture(tex, fontTexCoords).r), fragBgColor.a);
  }
)";

static std::string OpenGLDragnDropVertexShader = R"(
  #version 140

  in vec4 texCoords;
  uniform vec4 color;

  out vec2 texCoordsOut;

  void main() {
    texCoordsOut = texCoords.zw;

    gl_Position = vec4(texCoords.xy, 0.0, 1.0);
}
)";

static std::string OpenGLDragnDropFragmentShader = R"(
#version 140

in vec2 texCoordsOut;
out vec4 glFragColor;
uniform sampler2D source[];

void main() {

    glFragColor = texture(source[0], texCoordsOut);
}
)";

static std::string OpenGLProgressVertexShader = R"(
  #version 140

  in vec4 texCoords;
  out vec2 texCoordsOut;

  void main() {
    texCoordsOut = texCoords.zw;
    gl_Position = vec4(texCoords.xy, 0.0, 1.0);
}
)";

static std::string OpenGLProgressFragmentShader = R"(
#version 140

#define PI 3.14159265358979323846
in vec2 texCoordsOut;
out vec4 glFragColor;
uniform sampler2D source;
uniform int degree;

vec2 rotateUV(vec2 uv, vec2 pivot, float rotation) {
    mat2 rotation_matrix = mat2(
        vec2(sin(rotation), -cos(rotation)),
        vec2(cos(rotation), sin(rotation))
    );

    uv -= pivot;
    uv = uv * rotation_matrix;
    uv += pivot;

    return uv;
}

void main() {

    glFragColor = texture(source, rotateUV(texCoordsOut, vec2(0.5, 0.5), float(degree) * (PI / 180.0) ));
}
)";