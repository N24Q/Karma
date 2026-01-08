#include "PostProcessing.h"
#include "../scene/Light.h"
#include "../camera/Camera.h"
#include <algorithm>

namespace Karma {

PostProcessing::PostProcessing()
    : m_screenWidth(1920)
    , m_screenHeight(1080)
    , m_pixelScale(1.0f)
    , m_enabled(false)
    , m_bloomEnabled(true)
    , m_bloomIntensity(0.015f)      // Reduced from 0.0004 for more subtle bloom
    , m_bloomThreshold(2.5f)         // Increased threshold - only very bright areas bloom
    , m_bloomBlurPasses(8)           // Slightly fewer passes for tighter bloom
    , m_toneMappingMode(ToneMappingMode::ACES)  // ACES for film-like look matching Blender
    , m_volumetricFogEnabled(true)
    , m_fogDensity(0.005f)           // Significantly reduced fog density
    , m_godRaysEnabled(false)        // Disable god rays
    , m_godRaysDensity(0.8f)
    , m_godRaysWeight(0.5f)
    , m_godRaysDecay(0.95f)
    , m_godRaysSamples(100)
    , m_exposure(2.5f)  // Increase exposure to brighten the scene
    , m_autoExposureEnabled(false)
    , m_autoExposureSpeed(1.5f)
    , m_chromaticAberrationEnabled(false)
    , m_chromaticAberrationStrength(0.5f)
    , m_posterizationEnabled(true)
    , m_posterizationLevels(75)
    , m_filmGrainEnabled(false)
    , m_filmGrainStrength(0.01f)
    , m_contrast(1.3f)               // No contrast adjustment by default
    , m_redshiftEnabled(true)        // Enable slight redshift
    , m_redshiftStrength(0.05f)      // Subtle warm tint
    , m_offscreenFBO(0)
    , m_offscreenTexture(0)
    , m_offscreenDepthRBO(0)
    , m_downsampledFBO(0)
    , m_downsampledTexture(0)
    , m_brightFBO(0)
    , m_brightTexture(0)
    , m_bloomWidth(0)
    , m_bloomHeight(0)
    , m_godRaysFBO(0)
    , m_godRaysTexture(0)
    , m_luminanceFBO(0)
    , m_luminanceTexture(0)
    , m_currentExposure(0.5f)
    , m_pixelShaderProgram(0)
    , m_bloomShaderProgram(0)
    , m_blurShaderProgram(0)
    , m_compositeShaderProgram(0)
    , m_fullscreenVAO(0)
    , m_fullscreenVBO(0)
{
    m_blurFBO[0] = m_blurFBO[1] = 0;
    m_blurTexture[0] = m_blurTexture[1] = 0;
}

PostProcessing::~PostProcessing() {
    Cleanup();
}

bool PostProcessing::Initialize(int screenWidth, int screenHeight, GLuint pixelShaderProgram,
                                 GLuint bloomShaderProgram, GLuint blurShaderProgram, GLuint compositeShaderProgram) {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    m_pixelShaderProgram = pixelShaderProgram;
    m_bloomShaderProgram = bloomShaderProgram;
    m_blurShaderProgram = blurShaderProgram;
    m_compositeShaderProgram = compositeShaderProgram;

    std::cout << "[OpenGL] Initializing post-processing FBOs...\n";

    // Fullscreen quad for post-processing
    float quadVertices[] = {
        -1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f
    };

    glGenVertexArrays(1, &m_fullscreenVAO);
    glGenBuffers(1, &m_fullscreenVBO);
    glBindVertexArray(m_fullscreenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_fullscreenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // Offscreen FBO (full resolution) - stores HDR scene color
    glGenFramebuffers(1, &m_offscreenFBO);
    glGenTextures(1, &m_offscreenTexture);
    glGenRenderbuffers(1, &m_offscreenDepthRBO);

    glBindTexture(GL_TEXTURE_2D, m_offscreenTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_screenWidth, m_screenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindRenderbuffer(GL_RENDERBUFFER, m_offscreenDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_screenWidth, m_screenHeight);

    glBindFramebuffer(GL_FRAMEBUFFER, m_offscreenFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_offscreenTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_offscreenDepthRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[OpenGL] Offscreen framebuffer incomplete!\n";
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Downsampled FBO (for pixelation)
    glGenFramebuffers(1, &m_downsampledFBO);
    glGenTextures(1, &m_downsampledTexture);

    // Bloom FBOs (half resolution for better performance)
    m_bloomWidth = m_screenWidth / 2;
    m_bloomHeight = m_screenHeight / 2;

    // Bright extraction FBO
    glGenFramebuffers(1, &m_brightFBO);
    glGenTextures(1, &m_brightTexture);

    glBindTexture(GL_TEXTURE_2D, m_brightTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_bloomWidth, m_bloomHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, m_brightFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_brightTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[OpenGL] Bright extraction framebuffer incomplete!\n";
        return false;
    }

    // Blur FBOs (ping-pong, half resolution)
    for (int i = 0; i < 2; i++) {
        glGenFramebuffers(1, &m_blurFBO[i]);
        glGenTextures(1, &m_blurTexture[i]);

        glBindTexture(GL_TEXTURE_2D, m_blurTexture[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_bloomWidth, m_bloomHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, m_blurFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_blurTexture[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "[OpenGL] Blur framebuffer " << i << " incomplete!\n";
            return false;
        }
    }

    std::cout << "[OpenGL] Post-processing FBOs initialized\n";
    std::cout << "[OpenGL] Bloom: ENABLED, Volumetric Fog: ENABLED\n";
    return true;
}

void PostProcessing::SetPixelationAmount(float scale) {
    scale = std::clamp(scale, 0.01f, 1.0f);
    m_pixelScale = scale;
    m_enabled = true;

    UpdateDownsampledTexture();

    std::cout << "[OpenGL] Pixelation enabled: " << (scale * 100.0f) << "% ("
              << (int)(m_screenWidth * scale) << "x" << (int)(m_screenHeight * scale) << ")\n";
}

void PostProcessing::Disable() {
    m_enabled = false;
    m_pixelScale = 1.0f;
    std::cout << "[OpenGL] Pixelation disabled\n";
}

void PostProcessing::Apply(const std::vector<Light*>& lights, const Camera* camera) {
    glDisable(GL_DEPTH_TEST);

    // Step 1: Calculate auto-exposure if enabled
    if (m_autoExposureEnabled) {
        ApplyAutoExposure(0.016f); // Assuming ~60fps, use deltaTime if available
    }

    // Step 2: Extract bright pixels for bloom
    if (m_bloomEnabled) {
        ApplyBloom();
    }

    // Step 3: Final composite (scene + bloom + volumetric fog + tone mapping)
    ApplyFinalComposite();

    // Step 3: Optional pixelation effect
    if (m_enabled) {
        int downsampledWidth = (int)(m_screenWidth * m_pixelScale);
        int downsampledHeight = (int)(m_screenHeight * m_pixelScale);

        // Downsample to small texture
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_downsampledFBO);
        glBlitFramebuffer(0, 0, m_screenWidth, m_screenHeight,
                         0, 0, downsampledWidth, downsampledHeight,
                         GL_COLOR_BUFFER_BIT, GL_LINEAR);

        // Upsample to screen with nearest-neighbor
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, m_screenWidth, m_screenHeight);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(m_pixelShaderProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_downsampledTexture);
        glUniform1i(glGetUniformLocation(m_pixelShaderProgram, "screenTexture"), 0);

        glBindVertexArray(m_fullscreenVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    glEnable(GL_DEPTH_TEST);
}

void PostProcessing::ApplyBloom() {
    // Extract bright pixels (downsample to half resolution)
    glBindFramebuffer(GL_FRAMEBUFFER, m_brightFBO);
    glViewport(0, 0, m_bloomWidth, m_bloomHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_bloomShaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_offscreenTexture);
    glUniform1i(glGetUniformLocation(m_bloomShaderProgram, "sceneTexture"), 0);
    glUniform1f(glGetUniformLocation(m_bloomShaderProgram, "threshold"), m_bloomThreshold);

    glBindVertexArray(m_fullscreenVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Blur bright pixels (ping-pong between two buffers)
    bool horizontal = true;

    glUseProgram(m_blurShaderProgram);
    for (int i = 0; i < m_bloomBlurPasses; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_blurFBO[horizontal ? 1 : 0]);
        glViewport(0, 0, m_bloomWidth, m_bloomHeight);
        glUniform1i(glGetUniformLocation(m_blurShaderProgram, "horizontal"), horizontal);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, i == 0 ? m_brightTexture : m_blurTexture[horizontal ? 0 : 1]);
        glUniform1i(glGetUniformLocation(m_blurShaderProgram, "image"), 0);

        glBindVertexArray(m_fullscreenVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        horizontal = !horizontal;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_screenWidth, m_screenHeight);
}

void PostProcessing::ApplyAutoExposure(float deltaTime) {
    // Simple CPU-based auto-exposure by sampling scene luminance
    // Read center pixel from the offscreen texture (could be improved with downsampling)
    glBindFramebuffer(GL_FRAMEBUFFER, m_offscreenFBO);

    float pixel[4];
    glReadPixels(m_screenWidth / 2, m_screenHeight / 2, 1, 1, GL_RGBA, GL_FLOAT, pixel);

    // Calculate luminance
    float luminance = 0.2126f * pixel[0] + 0.7152f * pixel[1] + 0.0722f * pixel[2];
    luminance = std::max(luminance, 0.001f);

    // Target middle gray (18% reflectance = 0.18 in linear space)
    float targetExposure = 0.18f / luminance;

    // Clamp exposure to reasonable range
    targetExposure = std::clamp(targetExposure, 0.1f, 10.0f);

    // Smoothly interpolate to target exposure
    float adaptationSpeed = m_autoExposureSpeed * deltaTime;
    m_currentExposure = m_currentExposure + (targetExposure - m_currentExposure) * adaptationSpeed;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessing::ApplyFinalComposite() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_screenWidth, m_screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_compositeShaderProgram);

    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_offscreenTexture);
    glUniform1i(glGetUniformLocation(m_compositeShaderProgram, "sceneTexture"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_blurTexture[0]); // Final blurred bloom
    glUniform1i(glGetUniformLocation(m_compositeShaderProgram, "bloomTexture"), 1);

    // Calculate effective exposure (auto-exposure if enabled)
    float effectiveExposure = m_exposure;
    if (m_autoExposureEnabled) {
        effectiveExposure = m_currentExposure;
    }

    // Set uniforms
    glUniform1f(glGetUniformLocation(m_compositeShaderProgram, "bloomIntensity"), m_bloomIntensity);
    glUniform1f(glGetUniformLocation(m_compositeShaderProgram, "exposure"), effectiveExposure);
    glUniform1i(glGetUniformLocation(m_compositeShaderProgram, "bloomEnabled"), m_bloomEnabled);
    glUniform1i(glGetUniformLocation(m_compositeShaderProgram, "toneMappingMode"), static_cast<int>(m_toneMappingMode));
    glUniform1i(glGetUniformLocation(m_compositeShaderProgram, "chromaticAberrationEnabled"), m_chromaticAberrationEnabled);
    glUniform1f(glGetUniformLocation(m_compositeShaderProgram, "chromaticAberrationStrength"), m_chromaticAberrationStrength);

    // Color grading uniforms
    glUniform1i(glGetUniformLocation(m_compositeShaderProgram, "posterizationEnabled"), m_posterizationEnabled);
    glUniform1i(glGetUniformLocation(m_compositeShaderProgram, "posterizationLevels"), m_posterizationLevels);
    glUniform1i(glGetUniformLocation(m_compositeShaderProgram, "filmGrainEnabled"), m_filmGrainEnabled);
    glUniform1f(glGetUniformLocation(m_compositeShaderProgram, "filmGrainStrength"), m_filmGrainStrength);
    glUniform1f(glGetUniformLocation(m_compositeShaderProgram, "contrast"), m_contrast);
    glUniform1i(glGetUniformLocation(m_compositeShaderProgram, "redshiftEnabled"), m_redshiftEnabled);
    glUniform1f(glGetUniformLocation(m_compositeShaderProgram, "redshiftStrength"), m_redshiftStrength);

    // Time uniform for animated grain
    static float time = 0.0f;
    time += 0.016f; // Approximate frame time
    glUniform1f(glGetUniformLocation(m_compositeShaderProgram, "time"), time);

    glBindVertexArray(m_fullscreenVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void PostProcessing::Cleanup() {
    if (m_offscreenFBO) glDeleteFramebuffers(1, &m_offscreenFBO);
    if (m_offscreenTexture) glDeleteTextures(1, &m_offscreenTexture);
    if (m_offscreenDepthRBO) glDeleteRenderbuffers(1, &m_offscreenDepthRBO);
    if (m_downsampledFBO) glDeleteFramebuffers(1, &m_downsampledFBO);
    if (m_downsampledTexture) glDeleteTextures(1, &m_downsampledTexture);
    if (m_brightFBO) glDeleteFramebuffers(1, &m_brightFBO);
    if (m_brightTexture) glDeleteTextures(1, &m_brightTexture);
    if (m_fullscreenVAO) glDeleteVertexArrays(1, &m_fullscreenVAO);
    if (m_fullscreenVBO) glDeleteBuffers(1, &m_fullscreenVBO);

    for (int i = 0; i < 2; i++) {
        if (m_blurFBO[i]) glDeleteFramebuffers(1, &m_blurFBO[i]);
        if (m_blurTexture[i]) glDeleteTextures(1, &m_blurTexture[i]);
    }

    m_offscreenFBO = 0;
    m_offscreenTexture = 0;
    m_offscreenDepthRBO = 0;
    m_downsampledFBO = 0;
    m_downsampledTexture = 0;
    m_brightFBO = 0;
    m_brightTexture = 0;
    m_blurFBO[0] = m_blurFBO[1] = 0;
    m_blurTexture[0] = m_blurTexture[1] = 0;
    m_fullscreenVAO = 0;
    m_fullscreenVBO = 0;
}

void PostProcessing::UpdateDownsampledTexture() {
    int downsampledWidth = (int)(m_screenWidth * m_pixelScale);
    int downsampledHeight = (int)(m_screenHeight * m_pixelScale);

    glBindTexture(GL_TEXTURE_2D, m_downsampledTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, downsampledWidth, downsampledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, m_downsampledFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_downsampledTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[OpenGL] Downsampled framebuffer incomplete!\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace Karma
