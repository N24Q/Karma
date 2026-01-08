#include "Mesh.h"

namespace Karma {

Mesh::Mesh()
    : m_vao(0)
    , m_vbo(0)
    , m_ebo(0)
    , m_indexCount(0)
    , m_material(nullptr)
    , m_nodeTransform(1.0f)
{
}

Mesh::~Mesh() {
    Cleanup();
}

void Mesh::Create(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    m_indexCount = static_cast<int>(indices.size());

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    // TexCoord attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
}

void Mesh::Render() const {
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::Cleanup() {
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_ebo) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }
    // Material is managed externally, don't delete
    m_material = nullptr;
}

// Legacy compatibility methods
void Mesh::SetBaseColorTexture(GLuint texture) {
    if (m_material) {
        m_material->SetBaseColorTexture(texture);
    }
}

void Mesh::SetBaseColorFactor(const glm::vec4& factor) {
    if (m_material) {
        m_material->SetBaseColorFactor(factor);
    }
}

GLuint Mesh::GetBaseColorTexture() const {
    return m_material ? m_material->GetBaseColorTexture() : 0;
}

glm::vec4 Mesh::GetBaseColorFactor() const {
    return m_material ? m_material->GetBaseColorFactor() : glm::vec4(1.0f);
}

} // namespace Karma
