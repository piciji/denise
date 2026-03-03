
struct GLSplashScreen : SplashScreen {
    GLSplashScreen() : SplashScreen() {}

    ~GLSplashScreen() {
        term();
    }

    GLuint program = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint tex = 0;

    auto updateTex(Viewport& _viewport) -> bool {

        auto s = SplashScreen::update(_viewport);

        if (s == SplashScreen::FINISH) {
            return false;
        }

        if (!tex || (s == SplashScreen::TEXTURE_UPDATE)) {
            if (tex)
                glDeleteTextures(1, &tex);

            glActiveTexture(GL_TEXTURE0);
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            GLUtility::glParameters(GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap.scaledWidth, bitmap.scaledHeight, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, bitmap.scaledData);

        } else if (s == SplashScreen::DATA_UPDATE) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, bitmap.scaledWidth, bitmap.scaledHeight, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, bitmap.scaledData );
        }

        return true;
    }

    auto show(Viewport& _viewport) -> void {
        if (!tex)
            return;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);

        glUseProgram(program);
        glEnable(GL_BLEND);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        GLfloat box[4][4] = {
            {-1.0f,  1.0f, 0.0f, 0.0f},
            { 1.0f,  1.0f, 1.0f, 0.0f},
            {-1.0f, -1.0f, 0.0f, 1.0f},
            { 1.0f, -1.0f, 1.0f, 1.0f}
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof box, box, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDisable(GL_BLEND);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    auto term() -> void {
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

        initialized = false;
    }

    auto init() -> bool {
        std::string error = "";
        GLuint vertex = 0;
        GLuint fragment = 0;
        GLint texCoords;
        term();

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        vertex = GLUtility::createShader(GL_VERTEX_SHADER, OpenGLDragnDropVertexShader.c_str(), error);
        fragment = GLUtility::createShader(GL_FRAGMENT_SHADER, OpenGLDragnDropFragmentShader.c_str(), error);

        if (!vertex || !fragment)
            goto End;

        program = GLUtility::createProgram(vertex, fragment, error, true);

        if (!program)
            goto End;

        glUseProgram( program );

        texCoords = glGetAttribLocation(program, "texCoords");
        glEnableVertexAttribArray(texCoords);
        glVertexAttribPointer(texCoords, 4, GL_FLOAT, GL_FALSE, 0, 0);

        initialized = true;

        End:
        GLUtility::deleteShader(vertex);
        GLUtility::deleteShader(fragment);

        return initialized;
    }
};
