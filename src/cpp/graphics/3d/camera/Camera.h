#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Karma {

class Camera {
public:
    Camera();
    Camera(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));

    void SetPosition(const glm::vec3& position);
    void SetTarget(const glm::vec3& target);
    void SetTransform(const glm::vec3& position, const glm::vec3& target);

    void Move(const glm::vec3& delta);
    void UpdateMovement(bool forward, bool backward, bool left, bool right, float deltaTime);

    glm::mat4 GetViewMatrix() const;
    glm::vec3 GetPosition() const { return m_position; }
    glm::vec3 GetTarget() const { return m_target; }

    void SetSpeed(float speed) { m_speed = speed; }
    float GetSpeed() const { return m_speed; }

private:
    glm::vec3 m_position;
    glm::vec3 m_target;
    glm::vec3 m_up;
    float m_speed;
};

} // namespace Karma
