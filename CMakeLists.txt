cmake_minimum_required(VERSION 3.10)
project(evergreen)

set(CMAKE_CXX_STANDARD 17)

# --- Find dependencies ---
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# --- Gather all cpp files recursively ---
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    main.cpp
)

# --- Define your executable ---
add_executable(evergreen ${SOURCES})

# --- Include headers (optional but helpful) ---
target_include_directories(evergreen PRIVATE ${CMAKE_SOURCE_DIR}/src)

# --- Link libraries ---
target_link_libraries(evergreen
    PRIVATE
        glfw
        OpenGL::GL
        Threads::Threads
)
