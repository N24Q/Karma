#include "Camera.h"

namespace Karma {

Camera::Camera()
    : m_position(0.0f, 5.0f, 12.5f)
    , m_target(0.0f, 0.0f, 0.0f)
    , m_up(0.0f, 1.0f, 0.0f)
    , m_speed(0.1f)
{
}

Camera::Camera(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
    : m_position(position)
    , m_target(target)
    , m_up(up)
    , m_speed(0.1f)
{
}

void Camera::SetPosition(const glm::vec3& position) {
    m_position = position;
}

void Camera::SetTarget(const glm::vec3& target) {
    m_target = target;
}

void Camera::SetTransform(const glm::vec3& position, const glm::vec3& target) {
    m_position = position;
    m_target = target;
}

void Camera::Move(const glm::vec3& delta) {
    m_position += delta;
    m_target += delta;
}

void Camera::UpdateMovement(bool forward, bool backward, bool left, bool right, float deltaTime) {
    glm::vec3 movement(0.0f);

    if (forward) movement.z -= m_speed;
    if (backward) movement.z += m_speed;
    if (left) movement.x -= m_speed;
    if (right) movement.x += m_speed;

    if (glm::length(movement) > 0.0f) {
        Move(movement);
    }
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(m_position, m_target, m_up);
}

} // namespace Karma
