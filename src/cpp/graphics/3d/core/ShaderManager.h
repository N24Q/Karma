#pragma once

#include <glad/glad.h>
#include <iostream>

namespace Karma {

class ShaderManager {
public:
    ShaderManager() = default;
    ~ShaderManager() = default;

    GLuint CompileShader(const char* source, GLenum type);
    GLuint CreateShaderProgram(const char* vertexSrc, const char* fragmentSrc);
    void DeleteProgram(GLuint program);

private:
    bool CheckShaderCompilation(GLuint shader);
    bool CheckProgramLinking(GLuint program);
};

} // namespace Karma
