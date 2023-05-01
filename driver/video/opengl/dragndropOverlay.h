
struct GLDragndropOverlay : DragndropOverlay {

    GLDragndropOverlay() : DragndropOverlay() {}

    GLuint program = 0;
    GLuint vertex = 0;
    GLuint fragment = 0;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint tex = 0;

    auto show(Viewport& _viewport) -> void {
        update(_viewport);

        if (!buffer)
            return;

        updateAlpha();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);

        glUseProgram(program);
        glEnable(GL_BLEND);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        float screenx = 2.0f / _viewport.width, screeny = 2.0f / _viewport.height;

        float x = -1.0 + texX * screenx;
        float y = 1.0 - texY * screeny;

        float w = (float)texWidth * screenx;
        float h = (float)texHeight * screeny;

        GLfloat box[4][4] = {
                {x, y, 0, 0},
                {x + w, y, 1, 0},
                {x, y - h, 0, 1},
                {x + w, y - h, 1, 1}
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof box, box, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDisable(GL_BLEND);
    }

    auto updateBuffer() -> void {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer );
    }

    auto buildTexture(unsigned width, unsigned height) -> void {
        if (!initialized)
            return;

        if (tex)
            glDeleteTextures(1, &tex);

        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer);

        texWidth = width;
        texHeight = height;
    }

    auto term() -> void {
        if (vertex) {
            glDetachShader(program, vertex);
            glDeleteShader(vertex);
            vertex = 0;
        }

        if (fragment) {
            glDetachShader(program, fragment);
            glDeleteShader(fragment);
            fragment = 0;
        }

        if (program) {
            glDeleteProgram(program);
            program = 0;
        }

        if (vbo) {
            glDeleteBuffers(1, &vbo);
            vbo = 0;
        }

        if (vao) {
            glDeleteVertexArrays(1, &vao);
            vao = 0;
        }

        if (tex) {
            glDeleteTextures(1, &tex);
            tex = 0;
        }

        DragndropOverlay::term();
        initialized = false;
    }

    auto init() -> bool {
        term();

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        std::string error = "";
        program = glCreateProgram();
        vertex = _glCreateShader(program, GL_VERTEX_SHADER, OpenGLDragnDropVertexShader.c_str(), error);
        fragment = _glCreateShader(program, GL_FRAGMENT_SHADER, OpenGLDragnDropFragmentShader.c_str(), error);
        _glLinkProgram( nullptr, program, error );
        glUseProgram( program );
        if (!error.empty())
            return false;

        auto texCoords = glGetAttribLocation(program, "texCoords");
        glEnableVertexAttribArray(texCoords);
        glVertexAttribPointer(texCoords, 4, GL_FLOAT, GL_FALSE, 0, 0);

        return initialized = true;
    }
};