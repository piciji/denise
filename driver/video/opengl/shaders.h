
static std::string OpenGLOutputVertexShader = R"(
  #version 150

  uniform vec4 targetSize;
  uniform vec4 outputSize;

  in vec2 texCoord;

  out Vertex {
    vec2 texCoord;
  } vertexOut;

  void main() {
    //center image within output window
    if(gl_VertexID == 0 || gl_VertexID == 2) {
      gl_Position.x = -(targetSize.x / outputSize.x);
    } else {
      gl_Position.x = +(targetSize.x / outputSize.x);
    }

    //center and flip vertically (buffer[0, 0] = top-left; OpenGL[0, 0] = bottom-left)
    if(gl_VertexID == 0 || gl_VertexID == 1) {
      gl_Position.y = +(targetSize.y / outputSize.y);
    } else {
      gl_Position.y = -(targetSize.y / outputSize.y);
    }

    //align image to even pixel boundary to prevent aliasing
    vec2 align = fract((outputSize.xy + targetSize.xy) / 2.0) * 2.0;
    gl_Position.xy -= align / outputSize.xy;
    gl_Position.zw = vec2(0.0, 1.0);

    vertexOut.texCoord = texCoord;
  }
)";

static std::string OpenGLOutputVertexShaderLegacy = R"(
  #version 140

  uniform vec4 targetSize;
  uniform vec4 outputSize;

  in vec2 texCoord;

  out vec2 texCoordFrag;

  void main() {
    //center image within output window
    if(gl_VertexID == 0 || gl_VertexID == 2) {
      gl_Position.x = -(targetSize.x / outputSize.x);
    } else {
      gl_Position.x = +(targetSize.x / outputSize.x);
    }

    //center and flip vertically (buffer[0, 0] = top-left; OpenGL[0, 0] = bottom-left)
    if(gl_VertexID == 0 || gl_VertexID == 1) {
      gl_Position.y = +(targetSize.y / outputSize.y);
    } else {
      gl_Position.y = -(targetSize.y / outputSize.y);
    }

    //align image to even pixel boundary to prevent aliasing
    vec2 align = fract((outputSize.xy + targetSize.xy) / 2.0) * 2.0;
    gl_Position.xy -= align / outputSize.xy;
    gl_Position.zw = vec2(0.0, 1.0);

    texCoordFrag = texCoord;
  }
)";

static std::string OpenGLVertexShader = R"(
  #version 150

  in vec4 position;
  in vec2 texCoord;

  out Vertex {
    vec2 texCoord;
  } vertexOut;

  void main() {
    gl_Position = position;
    vertexOut.texCoord = texCoord;
  }
)";

static std::string OpenGLVertexShaderLegacy = R"(
  #version 140

  in vec4 position;
  in vec2 texCoord;

  out vec2 texCoordFrag;

  void main() {
    gl_Position = position;
    texCoordFrag = texCoord;
  }
)";

static std::string OpenGLGeometryShader = R"(
  #version 150

  layout(triangles) in;
  layout(triangle_strip, max_vertices = 3) out;

  in Vertex {
    vec2 texCoord;
  } vertexIn[];

  out Vertex {
    vec2 texCoord;
  };

  void main() {
    for(int i = 0; i < gl_in.length(); i++) {
      gl_Position = gl_in[i].gl_Position;
      texCoord = vertexIn[i].texCoord;
      EmitVertex();
    }
    EndPrimitive();
  }
)";

static std::string OpenGLFragmentShader = R"(
  #version 150

  uniform sampler2D source[];

  in Vertex {
    vec2 texCoord;
  };

  out vec4 fragColor;

  void main() {
    fragColor = texture(source[0], texCoord);
  }
)";

static std::string OpenGLFragmentShaderLegacy = R"(
  #version 140

  uniform sampler2D source[];

  in vec2 texCoordFrag;

  out vec4 fragColor;

  void main() {
    fragColor = texture(source[0], texCoordFrag);
  }
)";

static std::string OpenGLTextVertexShader = R"(
  #version 140

  in vec4 fontCoords;
 
  uniform vec4 color;
 
  out vec4 fragColor;
  out vec2 fontTexCoords;

  void main() {
    fragColor = color;
    fontTexCoords = fontCoords.zw;
     
    gl_Position = vec4(fontCoords.xy, 0.0, 1.0);
}
)";

static std::string OpenGLTextFragmentShader = R"(
#version 140

in vec4 fragColor; // fragment color
in vec2 fontTexCoords; // texture coords for th glyph
out vec4 glFragColor;
 
uniform sampler2D tex; // texture id
 
void main() {
    // draw pixel with fragColor's red, green and blue, and texture's alpha
    glFragColor = vec4(1.0, 1.0, 1.0, texture(tex, fontTexCoords).r) * fragColor;
}
)";

static std::string OpenGLDragnDropVertexShader = R"(
  #version 140

  in vec4 texCoords;
  uniform vec4 color;

  out vec4 fragColor;
  out vec2 texCoordsOut;

  void main() {
    fragColor = color;
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
