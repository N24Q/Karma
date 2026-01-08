#pragma once

namespace Karma {

static const char* pixelVertexShader = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
out vec2 vTexCoord;
void main() {
    vTexCoord = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

static const char* pixelFragmentShader = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;
uniform sampler2D screenTexture;
void main() {
    FragColor = texture(screenTexture, vTexCoord);
}
)";

// PBR Vertex Shader
static const char* pbrVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * worldPos;
}
)";

// PBR Fragment Shader with Metallic-Roughness workflow
static const char* pbrFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

// Material properties
uniform vec4 baseColorFactor;
uniform float metallicFactor;
uniform float roughnessFactor;
uniform vec3 emissiveFactor;
uniform float alphaCutoff;
uniform int alphaMode;  // 0=Opaque, 1=Mask, 2=Blend

// Textures
uniform sampler2D baseColorTexture;
uniform sampler2D metallicRoughnessTexture; // G=roughness, B=metallic
uniform sampler2D normalTexture;
uniform sampler2D occlusionTexture;
uniform sampler2D emissiveTexture;

// Texture flags
uniform bool hasBaseColorTexture;
uniform bool hasMetallicRoughnessTexture;
uniform bool hasNormalTexture;
uniform bool hasOcclusionTexture;
uniform bool hasEmissiveTexture;

// Lighting (supports up to 16 lights)
uniform vec3 lightPositions[16];
uniform vec3 lightDirections[16];
uniform vec3 lightColors[16];
uniform float lightIntensities[16];
uniform float lightRanges[16];
uniform int lightTypes[16];  // 0=Directional, 1=Point, 2=Spot
uniform float lightInnerConeAngles[16];  // For spot lights
uniform float lightOuterConeAngles[16];  // For spot lights
uniform int numLights;
uniform vec3 camPos;

const float PI = 3.14159265359;

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

// Geometry Function (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Fresnel-Schlick approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    // Sample textures
    vec4 baseColor = baseColorFactor;
    if (hasBaseColorTexture) {
        baseColor *= texture(baseColorTexture, TexCoord);
    }

    // Handle alpha modes
    if (alphaMode == 1) {  // MASK mode
        if (baseColor.a < alphaCutoff) {
            discard;
        }
    }

    float metallic = metallicFactor;
    float roughness = roughnessFactor;
    if (hasMetallicRoughnessTexture) {
        vec3 mr = texture(metallicRoughnessTexture, TexCoord).rgb;
        roughness *= mr.g;
        metallic *= mr.b;
    }

    vec3 emissive = emissiveFactor;
    if (hasEmissiveTexture) {
        emissive *= texture(emissiveTexture, TexCoord).rgb;
    }

    float ao = 1.0;
    if (hasOcclusionTexture) {
        ao = texture(occlusionTexture, TexCoord).r;
    }

    // Normal mapping (simplified - just use vertex normal for now)
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - FragPos);

    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, baseColor.rgb, metallic);

    // Reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < numLights && i < 16; ++i) {
        vec3 L;
        float attenuation = 1.0;

        // Calculate light direction and attenuation based on light type
        if (lightTypes[i] == 0) {
            // Directional light
            L = normalize(-lightDirections[i]);
            attenuation = 1.0;
        } else if (lightTypes[i] == 1) {
            // Point light (may have been an area light in Blender)
            L = normalize(lightPositions[i] - FragPos);
            float distance = length(lightPositions[i] - FragPos);

            // GLTF spec: intensity in candelas, distance in meters
            // Attenuation = 1 / max(distance^2, 1)
            attenuation = 1.0 / max(distance * distance, 1.0);

            // Range cutoff (smooth falloff)
            if (lightRanges[i] > 0.0) {
                float range = lightRanges[i];
                float distanceRatio = distance / range;
                float smoothCutoff = clamp(1.0 - pow(distanceRatio, 4.0), 0.0, 1.0);
                attenuation *= smoothCutoff;
            }

            // Hemisphere directionality for converted area lights
            // If light has a direction set, apply hemisphere attenuation
            vec3 lightDir = lightDirections[i];
            if (length(lightDir) > 0.01) {
                // Light emits in hemisphere defined by direction
                float hemisphere = max(dot(-L, normalize(lightDir)), 0.0);
                attenuation *= hemisphere;
            }
        } else if (lightTypes[i] == 2) {
            // Spot light
            L = normalize(lightPositions[i] - FragPos);
            float distance = length(lightPositions[i] - FragPos);
            vec3 spotDir = normalize(lightDirections[i]);

            // Distance attenuation
            attenuation = 1.0 / max(distance * distance, 1.0);

            // Range cutoff
            if (lightRanges[i] > 0.0) {
                float range = lightRanges[i];
                float distanceRatio = distance / range;
                float smoothCutoff = clamp(1.0 - pow(distanceRatio, 4.0), 0.0, 1.0);
                attenuation *= smoothCutoff;
            }

            // Spot cone attenuation
            float theta = dot(L, -spotDir);
            float innerCutoff = cos(lightInnerConeAngles[i]);
            float outerCutoff = cos(lightOuterConeAngles[i]);
            float epsilon = max(innerCutoff - outerCutoff, 0.0001);
            float spotAttenuation = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);
            attenuation *= spotAttenuation;
        }

        vec3 H = normalize(V + L);

        // Scale intensity to match scene scale
        // Blender exports in candelas (cd), which assumes real-world scale (meters)
        // This scaling factor makes lights more reasonable for typical scenes
        float intensityScale = 0.01;  // Adjust this value as needed
        vec3 radiance = lightColors[i] * (lightIntensities[i] * intensityScale) * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * baseColor.rgb / PI + specular) * radiance * NdotL;
    }

    // Ambient lighting disabled - using only scene lights
    vec3 ambient = vec3(0.0);
    vec3 color = ambient + Lo + emissive;

    // Enhanced volumetric fog with Mie scattering phase function
    float fogDensity = 0.008;  // Reduced for subtler fog
    float fogDistance = length(camPos - FragPos);

    // Mie scattering phase function (approximation)
    // g is the anisotropy factor (-1 to 1, positive = forward scattering)
    float g = 0.65; // Slightly reduced forward scattering for more diffuse look

    vec3 viewDir = normalize(camPos - FragPos);
    for (int i = 0; i < numLights; i++) {
        vec3 lightToFrag = FragPos - lightPositions[i];
        float distToLight = length(lightToFrag);
        vec3 lightDir = normalize(lightToFrag);

        // Attenuation with realistic falloff
        float attenuation = 1.0 / max(distToLight * distToLight, 1.0);

        // Range-based fog contribution
        if (lightRanges[i] > 0.0) {
            float rangeFactor = clamp(1.0 - (distToLight / lightRanges[i]), 0.0, 1.0);
            attenuation *= rangeFactor;
        }

        // Mie phase function for more realistic scattering
        float cosTheta = dot(-viewDir, -lightDir);
        float g2 = g * g;
        float phase = (1.0 - g2) / (4.0 * 3.14159 * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));

        // Enhance forward scattering visibility but keep it subtle
        phase = mix(phase, pow(max(cosTheta, 0.0), 6.0), 0.2);

        // Intensity scaling - reduced significantly
        float intensityScale = 0.01;
        vec3 scatterColor = lightColors[i] * (lightIntensities[i] * intensityScale) * attenuation;

        // Exponential fog factor with distance
        float fogFactor = 1.0 - exp(-fogDistance * fogDensity);

        // Add volumetric scattering with phase function - significantly reduced intensity
        color += scatterColor * phase * fogFactor * 0.12;  // Reduced from 0.4 to 0.12
    }

    // Output HDR color (tone mapping will be done in post-processing)
    FragColor = vec4(color, baseColor.a);
}
)";

// Bloom Extract Shader (extracts bright pixels)
static const char* bloomExtractFragmentShader = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D sceneTexture;
uniform float threshold;

void main() {
    vec3 color = texture(sceneTexture, vTexCoord).rgb;

    // Extract bright pixels above threshold
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > threshold) {
        FragColor = vec4(color, 1.0);
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
)";

// Improved Gaussian Blur Shader (9-tap kernel for better quality)
static const char* blurFragmentShader = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D image;
uniform bool horizontal;

// 9-tap Gaussian kernel (sigma = 2.0)
const float weight[9] = float[](
    0.13298,  0.23227,  0.27324,  0.23227,  0.13298,
    0.06553,  0.02732,  0.00790,  0.00159
);

void main() {
    vec2 texOffset = 1.0 / textureSize(image, 0);
    vec3 result = texture(image, vTexCoord).rgb * weight[0];

    if (horizontal) {
        for(int i = 1; i < 9; ++i) {
            result += texture(image, vTexCoord + vec2(texOffset.x * float(i), 0.0)).rgb * weight[i];
            result += texture(image, vTexCoord - vec2(texOffset.x * float(i), 0.0)).rgb * weight[i];
        }
    } else {
        for(int i = 1; i < 9; ++i) {
            result += texture(image, vTexCoord + vec2(0.0, texOffset.y * float(i))).rgb * weight[i];
            result += texture(image, vTexCoord - vec2(0.0, texOffset.y * float(i))).rgb * weight[i];
        }
    }

    FragColor = vec4(result, 1.0);
}
)";

// Advanced Composite Shader (scene + bloom + advanced tone mapping + effects)
static const char* compositeFragmentShader = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D sceneTexture;
uniform sampler2D bloomTexture;
uniform float bloomIntensity;
uniform float exposure;
uniform bool bloomEnabled;
uniform int toneMappingMode; // 0=Exposure, 1=Reinhard, 2=ACES, 3=Uncharted2
uniform bool chromaticAberrationEnabled;
uniform float chromaticAberrationStrength;
uniform bool posterizationEnabled;
uniform int posterizationLevels;
uniform bool filmGrainEnabled;
uniform float filmGrainStrength;
uniform float contrast;
uniform bool redshiftEnabled;
uniform float redshiftStrength;
uniform float time; // For animated grain

// ACES Filmic Tone Mapping
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Uncharted 2 Tone Mapping
vec3 Uncharted2ToneMap(vec3 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 Uncharted2Filmic(vec3 color) {
    float W = 11.2;
    color = Uncharted2ToneMap(color * 2.0);
    vec3 whiteScale = 1.0 / Uncharted2ToneMap(vec3(W));
    return color * whiteScale;
}

// Reinhard Tone Mapping
vec3 Reinhard(vec3 color) {
    return color / (color + vec3(1.0));
}

// Chromatic Aberration
vec3 applyChromaticAberration(sampler2D tex, vec2 uv, float strength) {
    vec2 offset = (uv - 0.5) * strength * 0.01;
    float r = texture(tex, uv + offset).r;
    float g = texture(tex, uv).g;
    float b = texture(tex, uv - offset).b;
    return vec3(r, g, b);
}

// Film Grain - pseudo-random noise
float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

// Posterization
vec3 posterize(vec3 color, int levels) {
    float levelsFloat = float(levels);
    return floor(color * levelsFloat) / levelsFloat;
}

// Contrast adjustment - works in linear space
vec3 adjustContrast(vec3 color, float contrast) {
    // Apply contrast around middle gray
    return clamp((color - 0.5) * contrast + 0.5, 0.0, 1.0);
}

void main() {
    vec3 sceneColor;

    // Apply chromatic aberration if enabled
    if (chromaticAberrationEnabled) {
        sceneColor = applyChromaticAberration(sceneTexture, vTexCoord, chromaticAberrationStrength);
    } else {
        sceneColor = texture(sceneTexture, vTexCoord).rgb;
    }

    vec3 bloomColor = texture(bloomTexture, vTexCoord).rgb;

    // Add bloom
    vec3 color = sceneColor;
    if (bloomEnabled) {
        color += bloomColor * bloomIntensity;
    }

    // Apply exposure
    color *= exposure;

    // Apply contrast in linear space (before tone mapping)
    if (contrast != 1.0) {
        float avgLuminance = dot(color, vec3(0.299, 0.587, 0.114));
        color = mix(vec3(avgLuminance), color, contrast);
    }

    // Tone mapping
    if (toneMappingMode == 0) {
        // Exposure-based tone mapping
        color = vec3(1.0) - exp(-color);
    } else if (toneMappingMode == 1) {
        // Reinhard tone mapping
        color = Reinhard(color);
    } else if (toneMappingMode == 2) {
        // ACES Filmic tone mapping
        color = ACESFilm(color);
    } else if (toneMappingMode == 3) {
        // Uncharted 2 tone mapping
        color = Uncharted2Filmic(color);
    }

    // Gamma correction
    color = pow(color, vec3(1.0/2.2));

    // Apply redshift (warm tint)
    if (redshiftEnabled) {
        color.r += redshiftStrength * 0.2;  // Boost red channel
        color.g += redshiftStrength * 0.0;  // Slight boost to green
        color.b += redshiftStrength * 0.3;  // Slight reduction to blue
    }

    // Apply posterization
    if (posterizationEnabled) {
        color = posterize(color, posterizationLevels);
    }

    // Apply film grain
    if (filmGrainEnabled) {
        float grain = rand(vTexCoord + time) * 2.0 - 1.0;
        color += vec3(grain) * filmGrainStrength;
    }

    // Clamp final color
    color = clamp(color, 0.0, 1.0);

    FragColor = vec4(color, 1.0);
}
)";

// God Rays Shader (screen-space radial blur)
static const char* godRaysFragmentShader = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D sceneTexture;
uniform vec2 lightScreenPos; // Light position in screen space [0, 1]
uniform float density;
uniform float weight;
uniform float decay;
uniform int numSamples;

void main() {
    vec2 texCoord = vTexCoord;
    vec2 deltaTexCoord = texCoord - lightScreenPos;
    deltaTexCoord *= density / float(numSamples);

    vec3 color = texture(sceneTexture, texCoord).rgb;
    float illuminationDecay = 1.0;

    for (int i = 0; i < numSamples; i++) {
        texCoord -= deltaTexCoord;
        vec3 sample = texture(sceneTexture, texCoord).rgb;
        sample *= illuminationDecay * weight;
        color += sample;
        illuminationDecay *= decay;
    }

    FragColor = vec4(color, 1.0);
}
)";

// Luminance calculation shader (for auto-exposure)
static const char* luminanceFragmentShader = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D sceneTexture;

void main() {
    vec3 color = texture(sceneTexture, vTexCoord).rgb;

    // Calculate luminance using standard weights
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));

    // Store log luminance for better averaging
    float logLuminance = log(luminance + 0.001);

    FragColor = vec4(logLuminance, luminance, 0.0, 1.0);
}
)";

// Lens Flare Shader (procedural ghosts and halo)
static const char* lensFlareFragmentShader = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D sceneTexture;
uniform vec2 lightScreenPos; // Light position in screen space [0, 1]
uniform float flareIntensity;
uniform vec3 flareColor;

// Generate procedural lens flare ghosts
vec3 generateLensFlare(vec2 uv, vec2 lightPos) {
    vec3 flare = vec3(0.0);
    vec2 toLight = lightPos - uv;
    float distToLight = length(toLight);

    // Main halo
    float halo = 1.0 / (1.0 + distToLight * 30.0);
    halo = pow(halo, 3.0);

    // Create multiple ghosts along the flare axis
    vec2 flareVec = lightPos - vec2(0.5);
    vec2 flareAxis = -flareVec;

    for (int i = 0; i < 5; i++) {
        float t = float(i) * 0.25;
        vec2 ghostPos = vec2(0.5) + flareAxis * t;
        vec2 toGhost = ghostPos - uv;
        float ghostDist = length(toGhost);

        // Ghost size varies
        float ghostSize = 0.03 + float(i) * 0.01;
        float ghost = smoothstep(ghostSize, ghostSize * 0.5, ghostDist);

        // Color variation
        vec3 ghostColor = flareColor;
        if (i % 3 == 1) ghostColor *= vec3(0.8, 1.0, 0.9);
        if (i % 3 == 2) ghostColor *= vec3(1.0, 0.9, 0.8);

        flare += ghost * ghostColor * 0.3;
    }

    // Add halo
    flare += halo * flareColor * 0.5;

    // Starburst (simple version)
    float angle = atan(toLight.y, toLight.x);
    float starburst = abs(sin(angle * 6.0));
    starburst = pow(starburst, 10.0) / (1.0 + distToLight * 100.0);
    flare += starburst * flareColor;

    return flare;
}

void main() {
    vec3 sceneColor = texture(sceneTexture, vTexCoord).rgb;

    // Only generate flare if light is on screen
    if (lightScreenPos.x > 0.0 && lightScreenPos.x < 1.0 &&
        lightScreenPos.y > 0.0 && lightScreenPos.y < 1.0) {

        vec3 flare = generateLensFlare(vTexCoord, lightScreenPos);
        sceneColor += flare * flareIntensity;
    }

    FragColor = vec4(sceneColor, 1.0);
}
)";

} // namespace Karma
