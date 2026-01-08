#define NOMINMAX
#include <windows.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>

#include "ShaderManager.h"
#include "../camera/Camera.h"
#include "../scene/Mesh.h"
#include "../loaders/GLTFLoader.h"
#include "../effects/PostProcessing.h"
#include "../camera/InputHandler.h"
#include "../scene/Light.h"
#include "Shaders.h"

extern "C" {
    extern void* windowPtr;
}

// Screen dimensions
static int g_screenWidth = 1920;
static int g_screenHeight = 1080;

// Model transform state
static glm::vec3 g_modelPosition(0.0f, 0.0f, 0.0f);
static glm::vec3 g_modelScale(1.0f, 1.0f, 1.0f);
static float g_modelYaw = 0.0f;
static float g_modelPitch = 0.0f;

// Shader programs
static GLuint g_pbrShaderProgram = 0;
static GLuint g_pixelShaderProgram = 0;
static GLuint g_bloomShaderProgram = 0;
static GLuint g_blurShaderProgram = 0;
static GLuint g_compositeShaderProgram = 0;

// System components
static Karma::ShaderManager* g_shaderManager = nullptr;
static Karma::Camera* g_camera = nullptr;
static Karma::GLTFLoader* g_gltfLoader = nullptr;
static Karma::PostProcessing* g_postProcessing = nullptr;
static Karma::InputHandler* g_inputHandler = nullptr;

// Meshes, Materials, and Lights
static std::vector<Karma::Mesh*> g_meshes;
static std::vector<Karma::Material*> g_materials;
static std::vector<Karma::Light*> g_lights;

// ========== Exported C functions ==========
extern "C" {
    // Forward declaration for audio cleanup
    void CleanupBackgroundAudio();

int InitRenderer() {
    std::cout << "[OpenGL] InitRenderer called\n";

    GLFWwindow* window = static_cast<GLFWwindow*>(windowPtr);
    if (!window) {
        std::cerr << "[OpenGL] No window!\n";
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "[OpenGL] Failed to initialize GLAD\n";
        return -1;
    }

    std::cout << "[OpenGL] Version: " << glGetString(GL_VERSION) << "\n";
    std::cout << "[OpenGL] Renderer: " << glGetString(GL_RENDERER) << "\n";

    glfwGetFramebufferSize(window, &g_screenWidth, &g_screenHeight);
    glViewport(0, 0, g_screenWidth, g_screenHeight);

    // Clear to black and present immediately so user sees something while loading
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwSwapBuffers(window);
    glfwPollEvents();

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Enable back-face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Enable alpha blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    // Initialize components
    g_shaderManager = new Karma::ShaderManager();
    g_camera = new Karma::Camera(glm::vec3(0.0f, 5.0f, 12.5f), glm::vec3(0.0f, 0.0f, 0.0f));
    g_gltfLoader = new Karma::GLTFLoader();
    g_postProcessing = new Karma::PostProcessing();
    g_inputHandler = new Karma::InputHandler();

    // Create shaders
    g_pbrShaderProgram = g_shaderManager->CreateShaderProgram(Karma::pbrVertexShader, Karma::pbrFragmentShader);
    g_pixelShaderProgram = g_shaderManager->CreateShaderProgram(Karma::pixelVertexShader, Karma::pixelFragmentShader);
    g_bloomShaderProgram = g_shaderManager->CreateShaderProgram(Karma::pixelVertexShader, Karma::bloomExtractFragmentShader);
    g_blurShaderProgram = g_shaderManager->CreateShaderProgram(Karma::pixelVertexShader, Karma::blurFragmentShader);
    g_compositeShaderProgram = g_shaderManager->CreateShaderProgram(Karma::pixelVertexShader, Karma::compositeFragmentShader);

    if (!g_pbrShaderProgram || !g_pixelShaderProgram ||
        !g_bloomShaderProgram || !g_blurShaderProgram || !g_compositeShaderProgram) {
        std::cerr << "[OpenGL] Failed to create shaders\n";
        return -1;
    }

    std::cout << "[OpenGL] PBR shader program created successfully\n";
    std::cout << "[OpenGL] Post-processing shaders created successfully\n";

    // Initialize post-processing system
    if (!g_postProcessing->Initialize(g_screenWidth, g_screenHeight, g_pixelShaderProgram,
                                      g_bloomShaderProgram, g_blurShaderProgram, g_compositeShaderProgram)) {
        std::cerr << "[OpenGL] Failed to initialize post-processing\n";
        return -1;
    }

    // Set default exposure to match Blender
    g_postProcessing->SetExposure(1.0f);

    // Register input callbacks
    g_inputHandler->RegisterCallbacks(window);
    std::cout << "[OpenGL] WASD camera controls enabled\n";

    std::cout << "[OpenGL] Renderer initialized successfully\n";
    return 0;
}

bool LoadGLTF(const char* path) {
    // Delete old loader to force complete memory cleanup
    if (g_gltfLoader) {
        std::cout << "[OpenGL] Recreating GLTFLoader to force memory deallocation\n";
        delete g_gltfLoader;
    }

    // Create new loader instance
    g_gltfLoader = new Karma::GLTFLoader();

    return g_gltfLoader->Load(path, g_meshes, g_materials, g_lights);
}

void SetModelTransform(float posX, float posY, float posZ,
                       float scaleX, float scaleY, float scaleZ,
                       float yaw, float pitch) {
    g_modelPosition = glm::vec3(posX, posY, posZ);
    g_modelScale = glm::vec3(scaleX, scaleY, scaleZ);
    g_modelYaw = yaw;
    g_modelPitch = pitch;
}

void SetCameraTransform(float camPosX, float camPosY, float camPosZ,
                        float focusX, float focusY, float focusZ) {
    g_camera->SetTransform(glm::vec3(camPosX, camPosY, camPosZ), glm::vec3(focusX, focusY, focusZ));
}

void SetPixelationAmount(float scale) {
    g_postProcessing->SetPixelationAmount(scale);
}

void DisablePixelation() {
    g_postProcessing->Disable();
}

void RenderFrame() {
    static bool firstFrame = true;
    if (firstFrame) {
        glm::vec3 camPos = g_camera->GetPosition();
        glm::vec3 camTarget = g_camera->GetTarget();
        std::cout << "[OpenGL] First frame - Camera at (" << camPos.x << ", "
                  << camPos.y << ", " << camPos.z << "), "
                  << "looking at (" << camTarget.x << ", " << camTarget.y << ", "
                  << camTarget.z << "), " << g_meshes.size() << " meshes to render\n";
        firstFrame = false;
    }

    // Handle camera movement
    bool forward, backward, left, right;
    g_inputHandler->GetMovementInput(forward, backward, left, right);
    g_camera->UpdateMovement(forward, backward, left, right, 1.0f);

    // Calculate matrices
    glm::mat4 view = g_camera->GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)g_screenWidth / (float)g_screenHeight, 0.01f, 1000.0f);

    // Global transform for the entire model (set via SetModelTransform)
    glm::mat4 globalModelTransform = glm::translate(glm::mat4(1.0f), g_modelPosition);
    globalModelTransform = glm::rotate(globalModelTransform, g_modelYaw, glm::vec3(0.0f, 1.0f, 0.0f));
    globalModelTransform = glm::rotate(globalModelTransform, g_modelPitch, glm::vec3(1.0f, 0.0f, 0.0f));
    globalModelTransform = glm::scale(globalModelTransform, g_modelScale);

    // ALWAYS render to offscreen FBO for post-processing (bloom, fog, tone mapping)
    glBindFramebuffer(GL_FRAMEBUFFER, g_postProcessing->GetOffscreenFBO());
    glViewport(0, 0, g_screenWidth, g_screenHeight);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Black background (no ambient light)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render scene with PBR
    glUseProgram(g_pbrShaderProgram);

    // Set view and projection (same for all meshes)
    glUniformMatrix4fv(glGetUniformLocation(g_pbrShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(g_pbrShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Set up lights from GLTF
    int numLights = std::min((int)g_lights.size(), 16);
    glUniform1i(glGetUniformLocation(g_pbrShaderProgram, "numLights"), numLights);
    glUniform3fv(glGetUniformLocation(g_pbrShaderProgram, "camPos"), 1, glm::value_ptr(g_camera->GetPosition()));

    if (numLights > 0) {
        glm::vec3 lightPositions[16];
        glm::vec3 lightDirections[16];
        glm::vec3 lightColors[16];
        float lightIntensities[16];
        float lightRanges[16];
        int lightTypes[16];
        float lightInnerCones[16];
        float lightOuterCones[16];

        for (int i = 0; i < numLights; i++) {
            const Karma::Light* light = g_lights[i];
            lightPositions[i] = light->GetPosition();
            lightDirections[i] = light->GetDirection();
            lightColors[i] = light->GetColor();
            lightIntensities[i] = light->GetIntensity();
            lightRanges[i] = light->GetRange();
            lightTypes[i] = static_cast<int>(light->GetType());
            lightInnerCones[i] = light->GetInnerConeAngle();
            lightOuterCones[i] = light->GetOuterConeAngle();
        }

        glUniform3fv(glGetUniformLocation(g_pbrShaderProgram, "lightPositions"), numLights, glm::value_ptr(lightPositions[0]));
        glUniform3fv(glGetUniformLocation(g_pbrShaderProgram, "lightDirections"), numLights, glm::value_ptr(lightDirections[0]));
        glUniform3fv(glGetUniformLocation(g_pbrShaderProgram, "lightColors"), numLights, glm::value_ptr(lightColors[0]));
        glUniform1fv(glGetUniformLocation(g_pbrShaderProgram, "lightIntensities"), numLights, lightIntensities);
        glUniform1fv(glGetUniformLocation(g_pbrShaderProgram, "lightRanges"), numLights, lightRanges);
        glUniform1iv(glGetUniformLocation(g_pbrShaderProgram, "lightTypes"), numLights, lightTypes);
        glUniform1fv(glGetUniformLocation(g_pbrShaderProgram, "lightInnerConeAngles"), numLights, lightInnerCones);
        glUniform1fv(glGetUniformLocation(g_pbrShaderProgram, "lightOuterConeAngles"), numLights, lightOuterCones);
    }

    // Render each mesh with its own transformation from the scene graph
    for (const auto* mesh : g_meshes) {
        // Combine global model transform with this mesh's node transform from GLTF
        glm::mat4 finalModelMatrix = globalModelTransform * mesh->GetNodeTransform();
        glUniformMatrix4fv(glGetUniformLocation(g_pbrShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(finalModelMatrix));

        // Get material
        Karma::Material* material = mesh->GetMaterial();
        if (material) {
            // Handle double-sided materials
            if (material->IsDoubleSided()) {
                glDisable(GL_CULL_FACE);
            } else {
                glEnable(GL_CULL_FACE);
            }

            // Set material properties
            glUniform4fv(glGetUniformLocation(g_pbrShaderProgram, "baseColorFactor"), 1, glm::value_ptr(material->GetBaseColorFactor()));
            glUniform1f(glGetUniformLocation(g_pbrShaderProgram, "metallicFactor"), material->GetMetallicFactor());
            glUniform1f(glGetUniformLocation(g_pbrShaderProgram, "roughnessFactor"), material->GetRoughnessFactor());
            glUniform3fv(glGetUniformLocation(g_pbrShaderProgram, "emissiveFactor"), 1, glm::value_ptr(material->GetEmissiveFactor()));
            glUniform1f(glGetUniformLocation(g_pbrShaderProgram, "alphaCutoff"), material->GetAlphaCutoff());

            // Set alpha mode (0=Opaque, 1=Mask, 2=Blend)
            int alphaModeInt = 0;
            if (material->GetAlphaMode() == Karma::AlphaMode::Mask) alphaModeInt = 1;
            else if (material->GetAlphaMode() == Karma::AlphaMode::Blend) alphaModeInt = 2;
            glUniform1i(glGetUniformLocation(g_pbrShaderProgram, "alphaMode"), alphaModeInt);

            // Bind textures
            int textureUnit = 0;

            // Base Color Texture
            glUniform1i(glGetUniformLocation(g_pbrShaderProgram, "hasBaseColorTexture"), material->HasBaseColorTexture());
            if (material->HasBaseColorTexture()) {
                glActiveTexture(GL_TEXTURE0 + textureUnit);
                glBindTexture(GL_TEXTURE_2D, material->GetBaseColorTexture());
                glUniform1i(glGetUniformLocation(g_pbrShaderProgram, "baseColorTexture"), textureUnit);
                textureUnit++;
            }

            // Metallic-Roughness Texture
            glUniform1i(glGetUniformLocation(g_pbrShaderProgram, "hasMetallicRoughnessTexture"), material->HasMetallicRoughnessTexture());
            if (material->HasMetallicRoughnessTexture()) {
                glActiveTexture(GL_TEXTURE0 + textureUnit);
                glBindTexture(GL_TEXTURE_2D, material->GetMetallicRoughnessTexture());
                glUniform1i(glGetUniformLocation(g_pbrShaderProgram, "metallicRoughnessTexture"), textureUnit);
                textureUnit++;
            }

            // Normal Texture
            glUniform1i(glGetUniformLocation(g_pbrShaderProgram, "hasNormalTexture"), material->HasNormalTexture());
            if (material->HasNormalTexture()) {
                glActiveTexture(GL_TEXTURE0 + textureUnit);
                glBindTexture(GL_TEXTURE_2D, material->GetNormalTexture());
                glUniform1i(glGetUniformLocation(g_pbrShaderProgram, "normalTexture"), textureUnit);
                textureUnit++;
            }

            // Occlusion Texture
            glUniform1i(glGetUniformLocation(g_pbrShaderProgram, "hasOcclusionTexture"), material->HasOcclusionTexture());
            if (material->HasOcclusionTexture()) {
                glActiveTexture(GL_TEXTURE0 + textureUnit);
                glBindTexture(GL_TEXTURE_2D, material->GetOcclusionTexture());
                glUniform1i(glGetUniformLocation(g_pbrShaderProgram, "occlusionTexture"), textureUnit);
                textureUnit++;
            }

            // Emissive Texture
            glUniform1i(glGetUniformLocation(g_pbrShaderProgram, "hasEmissiveTexture"), material->HasEmissiveTexture());
            if (material->HasEmissiveTexture()) {
                glActiveTexture(GL_TEXTURE0 + textureUnit);
                glBindTexture(GL_TEXTURE_2D, material->GetEmissiveTexture());
                glUniform1i(glGetUniformLocation(g_pbrShaderProgram, "emissiveTexture"), textureUnit);
                textureUnit++;
            }
        }

        mesh->Render();
    }

    // ALWAYS apply post-processing (bloom, volumetric fog, tone mapping, gamma correction)
    g_postProcessing->Apply(g_lights, g_camera);

    // Swap buffers
    GLFWwindow* window = static_cast<GLFWwindow*>(windowPtr);
    if (window) {
        glfwSwapBuffers(window);
    }
}

void CleanupRenderer() {
    std::cout << "[OpenGL] Cleaning up...\n";

    // Cleanup background audio and free VLC resources
    CleanupBackgroundAudio();

    // Cleanup meshes
    for (auto* mesh : g_meshes) {
        delete mesh;
    }
    g_meshes.clear();

    // Cleanup materials
    for (auto* material : g_materials) {
        delete material;
    }
    g_materials.clear();

    // Cleanup lights
    for (auto* light : g_lights) {
        delete light;
    }
    g_lights.clear();

    // Cleanup components
    if (g_postProcessing) {
        delete g_postProcessing;
        g_postProcessing = nullptr;
    }

    if (g_shaderManager) {
        g_shaderManager->DeleteProgram(g_pbrShaderProgram);
        g_shaderManager->DeleteProgram(g_pixelShaderProgram);
        g_shaderManager->DeleteProgram(g_bloomShaderProgram);
        g_shaderManager->DeleteProgram(g_blurShaderProgram);
        g_shaderManager->DeleteProgram(g_compositeShaderProgram);
        delete g_shaderManager;
        g_shaderManager = nullptr;
    }

    if (g_gltfLoader) {
        delete g_gltfLoader;
        g_gltfLoader = nullptr;
    }

    if (g_camera) {
        delete g_camera;
        g_camera = nullptr;
    }

    if (g_inputHandler) {
        delete g_inputHandler;
        g_inputHandler = nullptr;
    }

    std::cout << "[OpenGL] Cleanup complete\n";
}

} // extern "C"
