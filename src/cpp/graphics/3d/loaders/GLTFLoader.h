#pragma once

#include <tinygltf/tiny_gltf.h>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <vector>
#include <map>
#include "../scene/Mesh.h"
#include "../scene/Material.h"
#include "../scene/Light.h"

namespace Karma {

class GLTFLoader {
public:
    GLTFLoader() = default;
    ~GLTFLoader();

    bool Load(const char* path, std::vector<Mesh*>& meshes, std::vector<Material*>& materials, std::vector<Light*>& lights);
    void Clear();

private:
    GLuint LoadTexture(const tinygltf::Image& image, bool isSRGB = false);
    Material* LoadMaterial(const tinygltf::Material& gltfMaterial, const std::map<int, GLuint>& textureMap);
    void LoadLights(std::vector<Light*>& lights);
    glm::mat4 GetNodeTransform(const tinygltf::Node& node);
    void ProcessNode(const tinygltf::Model& model, int nodeIdx, const glm::mat4& parentTransform,
                     const std::map<int, GLuint>& textureMap,
                     const std::vector<Material*>& materials,
                     const std::vector<Light*>& sceneLights,
                     std::vector<Mesh*>& meshes,
                     std::vector<Light*>& lights);

    tinygltf::Model m_model;
    std::vector<GLuint> m_loadedTextures;
};

} // namespace Karma
