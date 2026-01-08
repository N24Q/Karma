#pragma once

#include <glad/glad.h>
#include <iostream>
#include <glm/glm.hpp>
#include <vector>

namespace Karma {

// Forward declarations
class Light;
class Camera;

class PostProcessing {
public:
    // Tone mapping modes
    enum class ToneMappingMode {
        Exposure,
        Reinhard,
        ACES,
        Uncharted2
    };

    PostProcessing();
    ~PostProcessing();

    bool Initialize(int screenWidth, int screenHeight, GLuint pixelShaderProgram,
                    GLuint bloomShaderProgram, GLuint blurShaderProgram, GLuint compositeShaderProgram);
    void SetPixelationAmount(float scale);
    void SetBloomEnabled(bool enabled) { m_bloomEnabled = enabled; }
    void SetBloomIntensity(float intensity) { m_bloomIntensity = intensity; }
    void SetBloomThreshold(float threshold) { m_bloomThreshold = threshold; }
    void SetBloomBlurPasses(int passes) { m_bloomBlurPasses = std::max(2, std::min(passes, 20)); }

    void SetToneMappingMode(ToneMappingMode mode) { m_toneMappingMode = mode; }

    void SetVolumetricFogEnabled(bool enabled) { m_volumetricFogEnabled = enabled; }
    void SetFogDensity(float density) { m_fogDensity = density; }

    void SetGodRaysEnabled(bool enabled) { m_godRaysEnabled = enabled; }
    void SetGodRaysDensity(float density) { m_godRaysDensity = density; }
    void SetGodRaysWeight(float weight) { m_godRaysWeight = weight; }
    void SetGodRaysDecay(float decay) { m_godRaysDecay = decay; }
    void SetGodRaysSamples(int samples) { m_godRaysSamples = std::max(10, std::min(samples, 200)); }

    void SetExposure(float exposure) { m_exposure = exposure; }
    void SetAutoExposureEnabled(bool enabled) { m_autoExposureEnabled = enabled; }
    void SetAutoExposureSpeed(float speed) { m_autoExposureSpeed = speed; }

    void SetChromaticAberrationEnabled(bool enabled) { m_chromaticAberrationEnabled = enabled; }
    void SetChromaticAberrationStrength(float strength) { m_chromaticAberrationStrength = strength; }

    void SetPosterizationEnabled(bool enabled) { m_posterizationEnabled = enabled; }
    void SetPosterizationLevels(int levels) { m_posterizationLevels = std::max(2, std::min(levels, 256)); }

    void SetFilmGrainEnabled(bool enabled) { m_filmGrainEnabled = enabled; }
    void SetFilmGrainStrength(float strength) { m_filmGrainStrength = strength; }

    void SetContrast(float contrast) { m_contrast = contrast; }

    void SetRedshiftEnabled(bool enabled) { m_redshiftEnabled = enabled; }
    void SetRedshiftStrength(float strength) { m_redshiftStrength = strength; }

    void Disable();
    void Apply(const std::vector<Light*>& lights, const Camera* camera);
    void Cleanup();

    bool IsEnabled() const { return m_enabled; }
    GLuint GetOffscreenFBO() const { return m_offscreenFBO; }

private:
    void UpdateDownsampledTexture();
    void ApplyBloom();
    void ApplyVolumetricFog(const std::vector<Light*>& lights, const Camera* camera);
    void ApplyAutoExposure(float deltaTime);
    void ApplyFinalComposite();

    int m_screenWidth;
    int m_screenHeight;
    float m_pixelScale;
    bool m_enabled;

    // Bloom settings
    bool m_bloomEnabled;
    float m_bloomIntensity;
    float m_bloomThreshold;
    int m_bloomBlurPasses;

    // Tone mapping
    ToneMappingMode m_toneMappingMode;

    // Volumetric fog settings
    bool m_volumetricFogEnabled;
    float m_fogDensity;

    // God rays
    bool m_godRaysEnabled;
    float m_godRaysDensity;
    float m_godRaysWeight;
    float m_godRaysDecay;
    int m_godRaysSamples;

    // Exposure
    float m_exposure;
    bool m_autoExposureEnabled;
    float m_autoExposureSpeed;

    // Lens effects
    bool m_chromaticAberrationEnabled;
    float m_chromaticAberrationStrength;

    // Color grading effects
    bool m_posterizationEnabled;
    int m_posterizationLevels;
    bool m_filmGrainEnabled;
    float m_filmGrainStrength;
    float m_contrast;
    bool m_redshiftEnabled;
    float m_redshiftStrength;

    // Framebuffers and textures
    GLuint m_offscreenFBO;
    GLuint m_offscreenTexture;
    GLuint m_offscreenDepthRBO;
    GLuint m_downsampledFBO;
    GLuint m_downsampledTexture;

    // Bloom buffers (half resolution for performance)
    GLuint m_brightFBO;
    GLuint m_brightTexture;
    GLuint m_blurFBO[2];
    GLuint m_blurTexture[2];
    int m_bloomWidth;  // Half of screen width
    int m_bloomHeight; // Half of screen height

    // God rays buffer
    GLuint m_godRaysFBO;
    GLuint m_godRaysTexture;

    // Auto-exposure buffers
    GLuint m_luminanceFBO;
    GLuint m_luminanceTexture;
    float m_currentExposure;

    // Shader programs
    GLuint m_pixelShaderProgram;
    GLuint m_bloomShaderProgram;
    GLuint m_blurShaderProgram;
    GLuint m_compositeShaderProgram;

    GLuint m_fullscreenVAO;
    GLuint m_fullscreenVBO;
};

} // namespace Karma
