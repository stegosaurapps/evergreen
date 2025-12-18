#pragma once

#include "Platform.hpp"

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>

#include <string>
#include <windows.h>

class Window {
public:
  Window() = default;
  ~Window();

  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;

  bool init(const char *title, int width, int height);
  void shutdown();

  // Pump all queued events; returns false if app should quit.
  bool pumpEvents();

  bool wasResized() const { return m_resized; }
  void clearResizedFlag() { m_resized = false; }

  int width() const { return m_width; }
  int height() const { return m_height; }

  SDL_Window *sdl() const { return m_window; }

  Win32WindowHandles win32Handles() const;

private:
  SDL_Window *m_window = nullptr;
  bool m_running = true;
  bool m_resized = false;
  int m_width = 0;
  int m_height = 0;
};
