#pragma once

#include <glm/glm.hpp>
#include <string>

namespace Karma {

enum class LightType {
    Directional,
    Point,
    Spot
};

class Light {
public:
    Light();
    ~Light();

    // Type
    void SetType(LightType type) { m_type = type; }
    LightType GetType() const { return m_type; }

    // Color and intensity
    void SetColor(const glm::vec3& color) { m_color = color; }
    void SetIntensity(float intensity) { m_intensity = intensity; }
    glm::vec3 GetColor() const { return m_color; }
    float GetIntensity() const { return m_intensity; }

    // Position (for point and spot lights)
    void SetPosition(const glm::vec3& position) { m_position = position; }
    glm::vec3 GetPosition() const { return m_position; }

    // Direction (for directional and spot lights)
    void SetDirection(const glm::vec3& direction) { m_direction = direction; }
    glm::vec3 GetDirection() const { return m_direction; }

    // Range (for point and spot lights, 0 = infinite)
    void SetRange(float range) { m_range = range; }
    float GetRange() const { return m_range; }

    // Spot light parameters
    void SetInnerConeAngle(float angle) { m_innerConeAngle = angle; }
    void SetOuterConeAngle(float angle) { m_outerConeAngle = angle; }
    float GetInnerConeAngle() const { return m_innerConeAngle; }
    float GetOuterConeAngle() const { return m_outerConeAngle; }

    // Name
    void SetName(const std::string& name) { m_name = name; }
    std::string GetName() const { return m_name; }

private:
    LightType m_type;
    glm::vec3 m_color;
    float m_intensity;
    glm::vec3 m_position;
    glm::vec3 m_direction;
    float m_range;
    float m_innerConeAngle;
    float m_outerConeAngle;
    std::string m_name;
};

} // namespace Karma
