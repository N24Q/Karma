#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

namespace Karma {

enum class AlphaMode {
    Opaque,
    Mask,
    Blend
};

class Material {
public:
    Material();
    ~Material();

    // PBR Metallic-Roughness properties
    void SetBaseColorFactor(const glm::vec4& color) { m_baseColorFactor = color; }
    void SetMetallicFactor(float metallic) { m_metallicFactor = metallic; }
    void SetRoughnessFactor(float roughness) { m_roughnessFactor = roughness; }
    void SetEmissiveFactor(const glm::vec3& emissive) { m_emissiveFactor = emissive; }

    // Textures
    void SetBaseColorTexture(GLuint texture) { m_baseColorTexture = texture; }
    void SetMetallicRoughnessTexture(GLuint texture) { m_metallicRoughnessTexture = texture; }
    void SetNormalTexture(GLuint texture) { m_normalTexture = texture; }
    void SetOcclusionTexture(GLuint texture) { m_occlusionTexture = texture; }
    void SetEmissiveTexture(GLuint texture) { m_emissiveTexture = texture; }

    // Alpha properties
    void SetAlphaMode(AlphaMode mode) { m_alphaMode = mode; }
    void SetAlphaCutoff(float cutoff) { m_alphaCutoff = cutoff; }
    void SetDoubleSided(bool doubleSided) { m_doubleSided = doubleSided; }

    // Texture scales
    void SetNormalScale(float scale) { m_normalScale = scale; }
    void SetOcclusionStrength(float strength) { m_occlusionStrength = strength; }

    // Getters
    glm::vec4 GetBaseColorFactor() const { return m_baseColorFactor; }
    float GetMetallicFactor() const { return m_metallicFactor; }
    float GetRoughnessFactor() const { return m_roughnessFactor; }
    glm::vec3 GetEmissiveFactor() const { return m_emissiveFactor; }

    GLuint GetBaseColorTexture() const { return m_baseColorTexture; }
    GLuint GetMetallicRoughnessTexture() const { return m_metallicRoughnessTexture; }
    GLuint GetNormalTexture() const { return m_normalTexture; }
    GLuint GetOcclusionTexture() const { return m_occlusionTexture; }
    GLuint GetEmissiveTexture() const { return m_emissiveTexture; }

    AlphaMode GetAlphaMode() const { return m_alphaMode; }
    float GetAlphaCutoff() const { return m_alphaCutoff; }
    bool IsDoubleSided() const { return m_doubleSided; }

    float GetNormalScale() const { return m_normalScale; }
    float GetOcclusionStrength() const { return m_occlusionStrength; }

    // Utility
    bool HasBaseColorTexture() const { return m_baseColorTexture != 0; }
    bool HasMetallicRoughnessTexture() const { return m_metallicRoughnessTexture != 0; }
    bool HasNormalTexture() const { return m_normalTexture != 0; }
    bool HasOcclusionTexture() const { return m_occlusionTexture != 0; }
    bool HasEmissiveTexture() const { return m_emissiveTexture != 0; }

    void Cleanup();

private:
    // PBR Metallic-Roughness
    glm::vec4 m_baseColorFactor;
    float m_metallicFactor;
    float m_roughnessFactor;
    glm::vec3 m_emissiveFactor;

    // Textures
    GLuint m_baseColorTexture;
    GLuint m_metallicRoughnessTexture;  // R=unused, G=roughness, B=metallic
    GLuint m_normalTexture;
    GLuint m_occlusionTexture;
    GLuint m_emissiveTexture;

    // Alpha
    AlphaMode m_alphaMode;
    float m_alphaCutoff;
    bool m_doubleSided;

    // Texture parameters
    float m_normalScale;
    float m_occlusionStrength;
};

} // namespace Karma
