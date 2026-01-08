#include "Light.h"
#include <cmath>

namespace Karma {

Light::Light()
    : m_type(LightType::Point)
    , m_color(1.0f, 1.0f, 1.0f)
    , m_intensity(1.0f)
    , m_position(0.0f, 0.0f, 0.0f)
    , m_direction(0.0f, -1.0f, 0.0f)
    , m_range(0.0f)  // 0 = infinite
    , m_innerConeAngle(0.0f)
    , m_outerConeAngle(0.785398f)  // ~45 degrees
    , m_name("")
{
}

Light::~Light() {
}

} // namespace Karma
