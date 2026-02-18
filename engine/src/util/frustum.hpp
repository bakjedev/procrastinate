#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_access.hpp"

enum class PlaneType : uint8_t
{
  kLeft,
  kRight,
  kBottom,
  kTop,
  kNear,
  kFar
};
enum class Halfspace : uint8_t
{
  kInside,
  kOutside,
  kOn
};

struct Plane
{
  glm::vec3 normal;
  float distance;
};

struct Frustum
{
  Plane left;
  Plane right;

  Plane bottom;
  Plane top;

  // These have plane suffixed because *windows*...
  Plane near_plane;
  Plane far_plane;
};

inline Plane ExtractPlane(const glm::mat4& matrix, PlaneType type)
{
  const auto row1 = glm::row(matrix, 0);
  const auto row2 = glm::row(matrix, 1);
  const auto row3 = glm::row(matrix, 2);
  const auto row4 = glm::row(matrix, 3);

  glm::vec4 result{};
  switch (type)
  {
    case PlaneType::kLeft:
      result = row4 + row1;
      break;
    case PlaneType::kRight:
      result = row4 - row1;
      break;
    case PlaneType::kBottom:
      result = row4 + row2;
      break;
    case PlaneType::kTop:
      result = row4 - row2;
      break;
    case PlaneType::kNear:
      result = row4 + row3;
      break;
    case PlaneType::kFar:
      result = row4 - row3;
      break;
  }

  const auto normal = glm::vec3(result);
  const float length = glm::length(normal);
  const glm::vec3 normalized = normal / length;
  const float distance = result.w / length;
  return Plane{.normal = normalized, .distance = distance};
}

inline Frustum ExtractFrustum(const glm::mat4& matrix)
{
  return Frustum{.left = ExtractPlane(matrix, PlaneType::kLeft),
                 .right = ExtractPlane(matrix, PlaneType::kRight),
                 .bottom = ExtractPlane(matrix, PlaneType::kBottom),
                 .top = ExtractPlane(matrix, PlaneType::kTop),
                 .near_plane = ExtractPlane(matrix, PlaneType::kNear),
                 .far_plane = ExtractPlane(matrix, PlaneType::kFar)};
}
