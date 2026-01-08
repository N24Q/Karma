#include "ShaderManager.h"

namespace Karma {

GLuint ShaderManager::CompileShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    if (!CheckShaderCompilation(shader)) {
        return 0;
    }
    return shader;
}

GLuint ShaderManager::CreateShaderProgram(const char* vertexSrc, const char* fragmentSrc) {
    GLuint vs = CompileShader(vertexSrc, GL_VERTEX_SHADER);
    GLuint fs = CompileShader(fragmentSrc, GL_FRAGMENT_SHADER);

    if (!vs || !fs) return 0;

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    if (!CheckProgramLinking(program)) {
        glDeleteShader(vs);
        glDeleteShader(fs);
        return 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

void ShaderManager::DeleteProgram(GLuint program) {
    if (program) {
        glDeleteProgram(program);
    }
}

bool ShaderManager::CheckShaderCompilation(GLuint shader) {
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "[Shader] Compilation failed:\n" << infoLog << "\n";
        return false;
    }
    return true;
}

bool ShaderManager::CheckProgramLinking(GLuint program) {
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "[Shader] Linking failed:\n" << infoLog << "\n";
        return false;
    }
    return true;
}

} // namespace Karma
