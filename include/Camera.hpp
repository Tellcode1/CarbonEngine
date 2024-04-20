#ifndef __CAMERA__
#define __CAMERA__

#include "stdafx.hpp"

struct Camera {
    vec2 position = dvec2(0.0f, 0.0f);
    f32 zoom = 1.0f;

    Camera() = default;
    ~Camera() = default;
    Camera(dvec2 position, f64 zoom) : position(position), zoom(zoom) {}

    glm::mat4 GetViewMatrix() const {
        return glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(zoom, zoom, 1.0f));
    }
};

#endif