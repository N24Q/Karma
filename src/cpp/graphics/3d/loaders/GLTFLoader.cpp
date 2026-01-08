#include "GLTFLoader.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <chrono>

namespace Karma {

GLTFLoader::~GLTFLoader() {
    Clear();
}

bool GLTFLoader::Load(const char* path, std::vector<Mesh*>& meshes, std::vector<Material*>& materials, std::vector<Light*>& lights) {
    std::cout << "[OpenGL] Loading GLTF: " << path << "\n";

    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool ret = false;
    std::string pathStr(path);
    if (pathStr.find(".glb") != std::string::npos) {
        std::cout << "[GLTF] Detected binary format (.glb)\n";
        ret = loader.LoadBinaryFromFile(&m_model, &err, &warn, path);
    } else {
        std::cout << "[GLTF] Detected ASCII format (.gltf)\n";
        ret = loader.LoadASCIIFromFile(&m_model, &err, &warn, path);
    }

    if (!warn.empty()) {
        std::cout << "[GLTF] Warning: " << warn << "\n";
    }
    if (!err.empty()) {
        std::cerr << "[GLTF] Error: " << err << "\n";
        return false;
    }
    if (!ret) {
        std::cerr << "[GLTF] Failed to parse file\n";
        return false;
    }

    std::cout << "[GLTF] Model loaded: " << m_model.meshes.size() << " meshes, "
              << m_model.materials.size() << " materials, "
              << m_model.images.size() << " images\n";

    // Determine which textures need sRGB based on usage in materials
    std::map<int, bool> textureSRGBMap;  // imageIndex -> isSRGB
    for (const auto& mat : m_model.materials) {
        // Base color texture should be sRGB
        if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0 &&
            mat.pbrMetallicRoughness.baseColorTexture.index < m_model.textures.size()) {
            int imageIdx = m_model.textures[mat.pbrMetallicRoughness.baseColorTexture.index].source;
            textureSRGBMap[imageIdx] = true;
        }
        // Emissive texture should be sRGB
        auto emissiveIt = mat.additionalValues.find("emissiveTexture");
        if (emissiveIt != mat.additionalValues.end()) {
            int texIdx = emissiveIt->second.TextureIndex();
            if (texIdx >= 0 && texIdx < m_model.textures.size()) {
                int imageIdx = m_model.textures[texIdx].source;
                textureSRGBMap[imageIdx] = true;
            }
        }
    }

    // Load textures
    std::map<int, GLuint> textureMap;
    m_loadedTextures.clear();
    for (size_t i = 0; i < m_model.images.size(); i++) {
        bool isSRGB = textureSRGBMap.find(i) != textureSRGBMap.end() && textureSRGBMap[i];
        GLuint tex = LoadTexture(m_model.images[i], isSRGB);
        if (tex == 0) {
            std::cerr << "[GLTF] FATAL: Failed to load texture " << i << " of " << m_model.images.size() << "\n";
            std::cerr << "[GLTF] This is likely due to GPU memory exhaustion.\n";
            std::cerr << "[GLTF] Consider using smaller textures or texture compression.\n";
            Clear(); // Clean up already loaded textures
            return false;
        }
        textureMap[i] = tex;
        m_loadedTextures.push_back(tex);
    }

    // Load materials
    materials.clear();
    for (size_t i = 0; i < m_model.materials.size(); i++) {
        Material* mat = LoadMaterial(m_model.materials[i], textureMap);
        materials.push_back(mat);
    }

    // Create a default material if none exist
    if (materials.empty()) {
        Material* defaultMat = new Material();
        defaultMat->SetBaseColorFactor(glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
        defaultMat->SetMetallicFactor(0.0f);
        defaultMat->SetRoughnessFactor(0.5f);
        materials.push_back(defaultMat);
        std::cout << "[GLTF] Created default material\n";
    }

    // Load lights from KHR_lights_punctual extension
    lights.clear();
    LoadLights(lights);

    // Clear any existing meshes
    meshes.clear();

    // Process the default scene or first scene
    int sceneIdx = m_model.defaultScene >= 0 ? m_model.defaultScene : 0;
    if (sceneIdx >= 0 && sceneIdx < m_model.scenes.size()) {
        const tinygltf::Scene& scene = m_model.scenes[sceneIdx];
        std::cout << "[GLTF] Processing scene " << sceneIdx << " with " << scene.nodes.size() << " root nodes\n";

        glm::mat4 identity(1.0f);
        for (int nodeIdx : scene.nodes) {
            ProcessNode(m_model, nodeIdx, identity, textureMap, materials, lights, meshes, lights);
        }
    } else {
        std::cerr << "[GLTF] No valid scene found!\n";
        return false;
    }

    std::cout << "[OpenGL] Loaded " << meshes.size() << " total primitives from scene graph\n";

    // FREE CPU-SIDE TEXTURE DATA AFTER GPU UPLOAD
    // Textures are already uploaded to GPU, so we can free the CPU copies
    std::cout << "[GLTF] Freeing CPU texture data to save memory...\n";
    size_t freedTextureMemory = 0;
    for (auto& image : m_model.images) {
        freedTextureMemory += image.image.size();
        image.image.clear();
        image.image.shrink_to_fit();
    }

    // Also free vertex buffer data (already on GPU)
    size_t freedBufferMemory = 0;
    for (auto& buffer : m_model.buffers) {
        freedBufferMemory += buffer.data.size();
        buffer.data.clear();
        buffer.data.shrink_to_fit();
    }

    size_t totalFreed = freedTextureMemory + freedBufferMemory;
    std::cout << "[GLTF] Memory freed: Textures=" << (freedTextureMemory / 1024 / 1024)
              << "MB, Buffers=" << (freedBufferMemory / 1024 / 1024)
              << "MB, Total=" << (totalFreed / 1024 / 1024) << "MB\n";

    // Debug: Print bounding box info
    if (!m_model.meshes.empty() && !m_model.meshes[0].primitives.empty()) {
        auto& prim = m_model.meshes[0].primitives[0];
        if (prim.attributes.find("POSITION") != prim.attributes.end()) {
            const tinygltf::Accessor& posAccessor = m_model.accessors[prim.attributes.at("POSITION")];
            std::cout << "[GLTF] Bounds: min=[" << posAccessor.minValues[0] << ", "
                      << posAccessor.minValues[1] << ", " << posAccessor.minValues[2] << "], "
                      << "max=[" << posAccessor.maxValues[0] << ", "
                      << posAccessor.maxValues[1] << ", " << posAccessor.maxValues[2] << "]\n";
        }
    }

    return true;
}

void GLTFLoader::Clear() {
    std::cout << "[GLTF] Clearing loader and freeing model memory...\n";

    // Delete GPU textures
    for (GLuint tex : m_loadedTextures) {
        if (tex != 0) {
            glDeleteTextures(1, &tex);
        }
    }
    m_loadedTextures.clear();
    m_loadedTextures.shrink_to_fit();  // Free vector capacity

    // Explicitly clear and shrink all model vectors to free memory
    m_model.accessors.clear();
    m_model.accessors.shrink_to_fit();

    m_model.animations.clear();
    m_model.animations.shrink_to_fit();

    m_model.buffers.clear();
    m_model.buffers.shrink_to_fit();

    m_model.bufferViews.clear();
    m_model.bufferViews.shrink_to_fit();

    m_model.materials.clear();
    m_model.materials.shrink_to_fit();

    m_model.meshes.clear();
    m_model.meshes.shrink_to_fit();

    m_model.nodes.clear();
    m_model.nodes.shrink_to_fit();

    m_model.textures.clear();
    m_model.textures.shrink_to_fit();

    m_model.images.clear();
    m_model.images.shrink_to_fit();

    m_model.skins.clear();
    m_model.skins.shrink_to_fit();

    m_model.samplers.clear();
    m_model.samplers.shrink_to_fit();

    m_model.cameras.clear();
    m_model.cameras.shrink_to_fit();

    m_model.scenes.clear();
    m_model.scenes.shrink_to_fit();

    m_model.lights.clear();
    m_model.lights.shrink_to_fit();

    m_model.extensions.clear();

    m_model.extensionsUsed.clear();
    m_model.extensionsUsed.shrink_to_fit();

    m_model.extensionsRequired.clear();
    m_model.extensionsRequired.shrink_to_fit();

    // Reset to empty model (this may still leave some overhead)
    m_model = tinygltf::Model();

    std::cout << "[GLTF] Model memory cleared\n";
}

GLuint GLTFLoader::LoadTexture(const tinygltf::Image& image, bool isSRGB) {
    auto startTime = std::chrono::high_resolution_clock::now();

    GLuint textureID;
    glGenTextures(1, &textureID);

    // Check for OpenGL errors after texture generation
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "[GLTF] ERROR: glGenTextures failed with error code: 0x" << std::hex << err << std::dec << "\n";
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, textureID);

    // Determine format and internal format
    GLenum format = GL_RGBA;
    GLenum internalFormat = GL_RGBA8;

    if (image.component == 1) {
        format = GL_RED;
        internalFormat = GL_R8;
    } else if (image.component == 2) {
        format = GL_RG;
        internalFormat = GL_RG8;
    } else if (image.component == 3) {
        format = GL_RGB;
        internalFormat = isSRGB ? GL_SRGB8 : GL_RGB8;
    } else if (image.component == 4) {
        format = GL_RGBA;
        internalFormat = isSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
    }

    // Calculate approximate memory usage (including mipmaps = 1.33x base size)
    size_t baseMemoryMB = (image.width * image.height * image.component) / (1024 * 1024);
    size_t totalMemoryMB = (baseMemoryMB * 4) / 3; // Approximation with mipmaps

    std::cout << "[GLTF] Allocating texture: " << image.width << "x" << image.height
              << ", components=" << image.component
              << ", format=" << (isSRGB ? "sRGB" : "Linear")
              << ", estimated memory: ~" << totalMemoryMB << "MB\n";

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.width, image.height, 0, format, GL_UNSIGNED_BYTE, image.image.data());

    // Check for OpenGL errors after texture upload
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "[GLTF] ERROR: glTexImage2D failed with error code: 0x" << std::hex << err << std::dec;
        if (err == GL_OUT_OF_MEMORY) {
            std::cerr << " (GL_OUT_OF_MEMORY - GPU memory exhausted!)\n";
            std::cerr << "[GLTF] Try reducing texture sizes or loading fewer textures.\n";
        } else {
            std::cerr << "\n";
        }
        glDeleteTextures(1, &textureID);
        return 0;
    }

    glGenerateMipmap(GL_TEXTURE_2D);

    // Check for OpenGL errors after mipmap generation
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "[GLTF] ERROR: glGenerateMipmap failed with error code: 0x" << std::hex << err << std::dec << "\n";
        glDeleteTextures(1, &textureID);
        return 0;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    std::cout << "[GLTF] Successfully loaded texture in " << duration << "ms\n";

    return textureID;
}

Material* GLTFLoader::LoadMaterial(const tinygltf::Material& gltfMaterial, const std::map<int, GLuint>& textureMap) {
    Material* material = new Material();

    // Base Color
    auto bcfIt = gltfMaterial.values.find("baseColorFactor");
    if (bcfIt != gltfMaterial.values.end()) {
        const auto& factor = bcfIt->second.ColorFactor();
        material->SetBaseColorFactor(glm::vec4(factor[0], factor[1], factor[2], factor[3]));
    }

    auto bctIt = gltfMaterial.values.find("baseColorTexture");
    if (bctIt != gltfMaterial.values.end()) {
        int texIndex = bctIt->second.TextureIndex();
        if (texIndex >= 0 && texIndex < m_model.textures.size()) {
            int imageIndex = m_model.textures[texIndex].source;
            if (textureMap.find(imageIndex) != textureMap.end()) {
                material->SetBaseColorTexture(textureMap.at(imageIndex));
            }
        }
    }

    // Metallic-Roughness
    auto mfIt = gltfMaterial.values.find("metallicFactor");
    if (mfIt != gltfMaterial.values.end()) {
        material->SetMetallicFactor(mfIt->second.Factor());
    }

    auto rfIt = gltfMaterial.values.find("roughnessFactor");
    if (rfIt != gltfMaterial.values.end()) {
        material->SetRoughnessFactor(rfIt->second.Factor());
    }

    auto mrtIt = gltfMaterial.values.find("metallicRoughnessTexture");
    if (mrtIt != gltfMaterial.values.end()) {
        int texIndex = mrtIt->second.TextureIndex();
        if (texIndex >= 0 && texIndex < m_model.textures.size()) {
            int imageIndex = m_model.textures[texIndex].source;
            if (textureMap.find(imageIndex) != textureMap.end()) {
                material->SetMetallicRoughnessTexture(textureMap.at(imageIndex));
            }
        }
    }

    // Normal Texture
    auto ntIt = gltfMaterial.additionalValues.find("normalTexture");
    if (ntIt != gltfMaterial.additionalValues.end()) {
        int texIndex = ntIt->second.TextureIndex();
        if (texIndex >= 0 && texIndex < m_model.textures.size()) {
            int imageIndex = m_model.textures[texIndex].source;
            if (textureMap.find(imageIndex) != textureMap.end()) {
                material->SetNormalTexture(textureMap.at(imageIndex));
            }
        }
        // Normal scale
        auto scaleIt = ntIt->second.json_double_value.find("scale");
        if (scaleIt != ntIt->second.json_double_value.end()) {
            material->SetNormalScale(static_cast<float>(scaleIt->second));
        }
    }

    // Occlusion Texture
    auto otIt = gltfMaterial.additionalValues.find("occlusionTexture");
    if (otIt != gltfMaterial.additionalValues.end()) {
        int texIndex = otIt->second.TextureIndex();
        if (texIndex >= 0 && texIndex < m_model.textures.size()) {
            int imageIndex = m_model.textures[texIndex].source;
            if (textureMap.find(imageIndex) != textureMap.end()) {
                material->SetOcclusionTexture(textureMap.at(imageIndex));
            }
        }
        // Occlusion strength
        auto strengthIt = otIt->second.json_double_value.find("strength");
        if (strengthIt != otIt->second.json_double_value.end()) {
            material->SetOcclusionStrength(static_cast<float>(strengthIt->second));
        }
    }

    // Emissive
    auto efIt = gltfMaterial.additionalValues.find("emissiveFactor");
    if (efIt != gltfMaterial.additionalValues.end()) {
        const auto& factor = efIt->second.ColorFactor();
        material->SetEmissiveFactor(glm::vec3(factor[0], factor[1], factor[2]));
    }

    auto etIt = gltfMaterial.additionalValues.find("emissiveTexture");
    if (etIt != gltfMaterial.additionalValues.end()) {
        int texIndex = etIt->second.TextureIndex();
        if (texIndex >= 0 && texIndex < m_model.textures.size()) {
            int imageIndex = m_model.textures[texIndex].source;
            if (textureMap.find(imageIndex) != textureMap.end()) {
                material->SetEmissiveTexture(textureMap.at(imageIndex));
            }
        }
    }

    // Alpha mode
    if (gltfMaterial.alphaMode == "OPAQUE") {
        material->SetAlphaMode(AlphaMode::Opaque);
    } else if (gltfMaterial.alphaMode == "MASK") {
        material->SetAlphaMode(AlphaMode::Mask);
        material->SetAlphaCutoff(gltfMaterial.alphaCutoff);
    } else if (gltfMaterial.alphaMode == "BLEND") {
        material->SetAlphaMode(AlphaMode::Blend);
    }

    // Double sided
    material->SetDoubleSided(gltfMaterial.doubleSided);

    std::cout << "[GLTF] Loaded material: " << gltfMaterial.name
              << " (metallic=" << material->GetMetallicFactor()
              << ", roughness=" << material->GetRoughnessFactor() << ")\n";

    return material;
}

void GLTFLoader::LoadLights(std::vector<Light*>& lights) {
    // Check if the KHR_lights_punctual extension exists
    if (m_model.extensions.find("KHR_lights_punctual") == m_model.extensions.end()) {
        std::cout << "[GLTF] No KHR_lights_punctual extension found\n";
        return;
    }

    const auto& lightsExt = m_model.extensions.at("KHR_lights_punctual");
    if (!lightsExt.Has("lights")) {
        std::cout << "[GLTF] No lights array in KHR_lights_punctual extension\n";
        return;
    }

    const auto& lightsArray = lightsExt.Get("lights");
    if (!lightsArray.IsArray()) {
        return;
    }

    std::cout << "[GLTF] Loading " << lightsArray.ArrayLen() << " lights from KHR_lights_punctual\n";

    for (size_t i = 0; i < lightsArray.ArrayLen(); i++) {
        const auto& lightDef = lightsArray.Get(i);
        Light* light = new Light();

        // Name
        if (lightDef.Has("name")) {
            light->SetName(lightDef.Get("name").Get<std::string>());
        }

        // Color
        if (lightDef.Has("color")) {
            const auto& colorArray = lightDef.Get("color");
            if (colorArray.IsArray() && colorArray.ArrayLen() == 3) {
                glm::vec3 color(
                    colorArray.Get(0).GetNumberAsDouble(),
                    colorArray.Get(1).GetNumberAsDouble(),
                    colorArray.Get(2).GetNumberAsDouble()
                );
                light->SetColor(color);
            }
        }

        // Intensity
        if (lightDef.Has("intensity")) {
            light->SetIntensity(static_cast<float>(lightDef.Get("intensity").GetNumberAsDouble())/10);
        }

        // Type
        if (lightDef.Has("type")) {
            std::string typeStr = lightDef.Get("type").Get<std::string>();
            if (typeStr == "directional") {
                light->SetType(LightType::Directional);
            } else if (typeStr == "point") {
                light->SetType(LightType::Point);

                // Range for point light
                if (lightDef.Has("range")) {
                    light->SetRange(static_cast<float>(lightDef.Get("range").GetNumberAsDouble()));
                }
            } else if (typeStr == "spot") {
                light->SetType(LightType::Spot);

                // Range for spot light
                if (lightDef.Has("range")) {
                    light->SetRange(static_cast<float>(lightDef.Get("range").GetNumberAsDouble()));
                }

                // Spot parameters
                if (lightDef.Has("spot")) {
                    const auto& spotDef = lightDef.Get("spot");
                    if (spotDef.Has("innerConeAngle")) {
                        light->SetInnerConeAngle(static_cast<float>(spotDef.Get("innerConeAngle").GetNumberAsDouble()));
                    }
                    if (spotDef.Has("outerConeAngle")) {
                        light->SetOuterConeAngle(static_cast<float>(spotDef.Get("outerConeAngle").GetNumberAsDouble()));
                    }
                }
            }
        }

        lights.push_back(light);
        std::cout << "[GLTF] Loaded light: " << light->GetName()
                  << " (type=" << (light->GetType() == LightType::Point ? "point" :
                                    light->GetType() == LightType::Directional ? "directional" : "spot")
                  << ", intensity=" << light->GetIntensity() << ")\n";
    }
}

glm::mat4 GLTFLoader::GetNodeTransform(const tinygltf::Node& node) {
    glm::mat4 transform(1.0f);

    if (node.matrix.size() == 16) {
        transform = glm::make_mat4(node.matrix.data());
    } else {
        if (node.translation.size() == 3) {
            transform = glm::translate(transform, glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
        }
        if (node.rotation.size() == 4) {
            glm::quat q(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
            transform *= glm::mat4_cast(q);
        }
        if (node.scale.size() == 3) {
            transform = glm::scale(transform, glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
        }
    }

    return transform;
}

void GLTFLoader::ProcessNode(const tinygltf::Model& model, int nodeIdx, const glm::mat4& parentTransform,
                              const std::map<int, GLuint>& textureMap,
                              const std::vector<Material*>& materials,
                              const std::vector<Light*>& sceneLights,
                              std::vector<Mesh*>& meshes,
                              std::vector<Light*>& lights) {
    const tinygltf::Node& node = model.nodes[nodeIdx];

    glm::mat4 localTransform = GetNodeTransform(node);
    glm::mat4 globalTransform = parentTransform * localTransform;

    if (node.mesh >= 0) {
        const tinygltf::Mesh& gltfMesh = model.meshes[node.mesh];

        for (size_t primIdx = 0; primIdx < gltfMesh.primitives.size(); primIdx++) {
            const auto& primitive = gltfMesh.primitives[primIdx];
            Mesh* mesh = new Mesh();
            mesh->SetNodeTransform(globalTransform);

            // Assign material
            if (primitive.material >= 0 && primitive.material < materials.size()) {
                mesh->SetMaterial(materials[primitive.material]);
            } else if (!materials.empty()) {
                // Use default material
                mesh->SetMaterial(materials[0]);
            }

            // Get vertex data
            const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
            const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
            const tinygltf::Buffer& posBuffer = model.buffers[posView.buffer];
            const float* positions = reinterpret_cast<const float*>(&posBuffer.data[posView.byteOffset + posAccessor.byteOffset]);

            const float* normals = nullptr;
            if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.at("NORMAL")];
                const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
                const tinygltf::Buffer& normBuffer = model.buffers[normView.buffer];
                normals = reinterpret_cast<const float*>(&normBuffer.data[normView.byteOffset + normAccessor.byteOffset]);
            }

            const float* texcoords = nullptr;
            if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                const tinygltf::Accessor& texAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                const tinygltf::BufferView& texView = model.bufferViews[texAccessor.bufferView];
                const tinygltf::Buffer& texBuffer = model.buffers[texView.buffer];
                texcoords = reinterpret_cast<const float*>(&texBuffer.data[texView.byteOffset + texAccessor.byteOffset]);
            }

            // Interleave vertex data
            std::vector<float> vertices;
            for (size_t v = 0; v < posAccessor.count; v++) {
                vertices.push_back(positions[v * 3 + 0]);
                vertices.push_back(positions[v * 3 + 1]);
                vertices.push_back(positions[v * 3 + 2]);

                if (normals) {
                    vertices.push_back(normals[v * 3 + 0]);
                    vertices.push_back(normals[v * 3 + 1]);
                    vertices.push_back(normals[v * 3 + 2]);
                } else {
                    vertices.push_back(0.0f);
                    vertices.push_back(1.0f);
                    vertices.push_back(0.0f);
                }

                if (texcoords) {
                    vertices.push_back(texcoords[v * 2 + 0]);
                    vertices.push_back(texcoords[v * 2 + 1]);
                } else {
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                }
            }

            // Get indices
            const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
            const tinygltf::BufferView& indexView = model.bufferViews[indexAccessor.bufferView];
            const tinygltf::Buffer& indexBuffer = model.buffers[indexView.buffer];

            std::vector<unsigned int> indices;
            if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                const unsigned short* data = reinterpret_cast<const unsigned short*>(&indexBuffer.data[indexView.byteOffset + indexAccessor.byteOffset]);
                for (size_t i = 0; i < indexAccessor.count; i++) {
                    indices.push_back(data[i]);
                }
            } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                const unsigned int* data = reinterpret_cast<const unsigned int*>(&indexBuffer.data[indexView.byteOffset + indexAccessor.byteOffset]);
                for (size_t i = 0; i < indexAccessor.count; i++) {
                    indices.push_back(data[i]);
                }
            }

            mesh->Create(vertices, indices);
            meshes.push_back(mesh);
        }
    }

    // Check if this node has a light
    if (node.extensions.find("KHR_lights_punctual") != node.extensions.end()) {
        const auto& lightExt = node.extensions.at("KHR_lights_punctual");
        if (lightExt.Has("light")) {
            int lightIndex = lightExt.Get("light").GetNumberAsInt();
            if (lightIndex >= 0 && lightIndex < sceneLights.size()) {
                // Clone the light with this node's transform
                Light* instancedLight = new Light(*sceneLights[lightIndex]);

                // Extract position from transform
                glm::vec3 position = glm::vec3(globalTransform[3]);
                instancedLight->SetPosition(position);

                // Extract direction from transform
                // In glTF, lights point along -Z axis in local space
                // Extract the Z-axis from rotation matrix and negate it
                glm::vec3 direction = -glm::normalize(glm::vec3(globalTransform[2]));
                instancedLight->SetDirection(direction);

                lights.push_back(instancedLight);
                std::cout << "[GLTF] Instanced light '" << instancedLight->GetName()
                          << "' at position (" << position.x << ", " << position.y << ", " << position.z << ")\n";
            }
        }
    }

    // Process children recursively
    for (int childIdx : node.children) {
        ProcessNode(model, childIdx, globalTransform, textureMap, materials, sceneLights, meshes, lights);
    }
}

} // namespace Karma
