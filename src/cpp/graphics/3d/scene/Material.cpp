#include "Material.h"

namespace Karma {

Material::Material()
    : m_baseColorFactor(1.0f, 1.0f, 1.0f, 1.0f)
    , m_metallicFactor(1.0f)
    , m_roughnessFactor(1.0f)
    , m_emissiveFactor(0.0f, 0.0f, 0.0f)
    , m_baseColorTexture(0)
    , m_metallicRoughnessTexture(0)
    , m_normalTexture(0)
    , m_occlusionTexture(0)
    , m_emissiveTexture(0)
    , m_alphaMode(AlphaMode::Opaque)
    , m_alphaCutoff(0.5f)
    , m_doubleSided(false)
    , m_normalScale(1.0f)
    , m_occlusionStrength(1.0f)
{
}

Material::~Material() {
    Cleanup();
}

void Material::Cleanup() {
    // Note: Textures are managed by the loader, don't delete here
    // Just reset handles
    m_baseColorTexture = 0;
    m_metallicRoughnessTexture = 0;
    m_normalTexture = 0;
    m_occlusionTexture = 0;
    m_emissiveTexture = 0;
}

} // namespace Karma
