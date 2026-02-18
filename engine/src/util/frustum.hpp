#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_access.hpp"

enum class PlaneType : uint8_t
{
  Left,
  Right,
  Bottom,
  Top,
  Near,
  Far
};
enum class Halfspace : uint8_t
{
  Inside,
  Outside,
  On
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
  Plane nearPlane;
  Plane farPlane;
};

inline Plane extractPlane(const glm::mat4& matrix, PlaneType type)
{
  const auto row1 = glm::row(matrix, 0);
  const auto row2 = glm::row(matrix, 1);
  const auto row3 = glm::row(matrix, 2);
  const auto row4 = glm::row(matrix, 3);

  glm::vec4 result{};
  switch (type)
  {
    case PlaneType::Left:
      result = row4 + row1;
      break;
    case PlaneType::Right:
      result = row4 - row1;
      break;
    case PlaneType::Bottom:
      result = row4 + row2;
      break;
    case PlaneType::Top:
      result = row4 - row2;
      break;
    case PlaneType::Near:
      result = row4 + row3;
      break;
    case PlaneType::Far:
      result = row4 - row3;
      break;
  }

  const auto normal = glm::vec3(result);
  const float length = glm::length(normal);
  const glm::vec3 normalized = normal / length;
  const float distance = result.w / length;
  return Plane{.normal = normalized, .distance = distance};
}

inline Frustum extractFrustum(const glm::mat4& matrix)
{
  return Frustum{.left = extractPlane(matrix, PlaneType::Left),
                 .right = extractPlane(matrix, PlaneType::Right),
                 .bottom = extractPlane(matrix, PlaneType::Bottom),
                 .top = extractPlane(matrix, PlaneType::Top),
                 .nearPlane = extractPlane(matrix, PlaneType::Near),
                 .farPlane = extractPlane(matrix, PlaneType::Far)};
}

inline Halfspace checkPointForPlane(const Plane& plane, const glm::vec3& point)
{
  const float dist = glm::dot(plane.normal, point) + plane.distance;

  if (dist < 0) return Halfspace::Inside;
  if (dist > 0) return Halfspace::Outside;
  return Halfspace::On;
}

inline bool checkPoint(const Frustum& frustum, const glm::vec3& point)
{
  return checkPointForPlane(frustum.left, point) == Halfspace::Inside &&
         checkPointForPlane(frustum.right, point) == Halfspace::Inside &&
         checkPointForPlane(frustum.bottom, point) == Halfspace::Inside &&
         checkPointForPlane(frustum.top, point) == Halfspace::Inside &&
         checkPointForPlane(frustum.nearPlane, point) == Halfspace::Inside &&
         checkPointForPlane(frustum.farPlane, point) == Halfspace::Inside;
}

inline glm::vec3 intersect3Planes(const Plane& plane1, const Plane& plane2, const Plane& plane3)
{
  const glm::mat3 matrix = glm::transpose(glm::mat3(plane1.normal, plane2.normal, plane3.normal));
  const glm::vec3 distances(-plane1.distance, -plane2.distance, -plane3.distance);
  return glm::inverse(matrix) * distances;
}
