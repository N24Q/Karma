#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Material.h"

namespace Karma {

class Mesh {
public:
    Mesh();
    ~Mesh();

    void Create(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
    void Render() const;
    void Cleanup();

    void SetMaterial(Material* material) { m_material = material; }
    void SetNodeTransform(const glm::mat4& transform) { m_nodeTransform = transform; }

    Material* GetMaterial() const { return m_material; }
    glm::mat4 GetNodeTransform() const { return m_nodeTransform; }
    int GetIndexCount() const { return m_indexCount; }

    // Legacy compatibility (deprecated - use Material instead)
    void SetBaseColorTexture(GLuint texture);
    void SetBaseColorFactor(const glm::vec4& factor);
    GLuint GetBaseColorTexture() const;
    glm::vec4 GetBaseColorFactor() const;

private:
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;
    int m_indexCount;
    Material* m_material;
    glm::mat4 m_nodeTransform;
};

} // namespace Karma
