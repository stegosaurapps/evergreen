#pragma once

struct Dimensions {
  float width;
  float height;

  float ratio() const { return width / height; }
};
