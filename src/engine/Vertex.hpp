#pragma once

struct VertexPCN {
  float px, py, pz;
  float nx, ny, nz;
  float r, g, b;
};

static_assert(sizeof(VertexPCN) == 36, "VertexPCN must be 36 bytes");
static_assert(offsetof(VertexPCN, px) == 0, "pos offset");
static_assert(offsetof(VertexPCN, nx) == 12, "nrm offset");
static_assert(offsetof(VertexPCN, r) == 24, "col offset");
