#pragma once

#include <stddef.h>

struct Vertex {
  float px, py, pz;     // Position
  float nx, ny, nz;     // Normal
  float tx, ty, tz, tw; // Tangent
  float ux, uy;         // Texture Coordinate
  float r, g, b;        // Color
};

struct VertexColor {
  float px, py, pz;
  float nx, ny, nz;
  float r, g, b;
};

static_assert(sizeof(VertexColor) == 36, "VertexColor must be 36 bytes");
static_assert(offsetof(VertexColor, px) == 0, "pos offset");
static_assert(offsetof(VertexColor, nx) == 12, "nrm offset");
static_assert(offsetof(VertexColor, r) == 24, "col offset");
