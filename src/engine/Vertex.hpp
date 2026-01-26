#pragma once

struct Vertex {
  float px, py, pz;
  float nx, ny, nz;
  float ux, uy;
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
