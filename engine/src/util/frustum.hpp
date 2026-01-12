#pragma once
#include "glm/glm.hpp"

struct Plane {
  glm::vec3 normal;
  float distance;
};

struct Frustum {
  Plane top;
  Plane bottom;

  Plane left;
  Plane right;

  Plane near;
  Plane far;
};