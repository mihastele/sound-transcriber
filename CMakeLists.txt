cmake_minimum_required(VERSION 3.24)
project(mictranscriber VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

include(FetchContent)

# ─── SDL2 ────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.30.9
    GIT_SHALLOW    TRUE
)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL2)

# ─── PortAudio ───────────────────────────────────────────────────────────────
FetchContent_Declare(
    portaudio
    GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
    GIT_TAG        v19.7.0
    GIT_SHALLOW    TRUE
)
set(PA_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(PA_BUILD_STATIC_LIBS ON  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(portaudio)

# ─── whisper.cpp ─────────────────────────────────────────────────────────────
FetchContent_Declare(
    whisper
    GIT_REPOSITORY https://github.com/ggerganov/whisper.cpp.git
    GIT_TAG        v1.7.3
    GIT_SHALLOW    TRUE
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(whisper)

# ─── Dear ImGui (manual build, no CMakeLists upstream) ───────────────────────
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.6
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC SDL2::SDL2-static)

find_package(OpenGL REQUIRED)
target_link_libraries(imgui PUBLIC OpenGL::GL)

# ─── Main application ────────────────────────────────────────────────────────
add_executable(mictranscriber
    src/main.cpp
    src/app.cpp
    src/audio_device.cpp
    src/audio_capture.cpp
    src/whisper_engine.cpp
    src/transcriber.cpp
    src/gui.cpp
)

target_include_directories(mictranscriber PRIVATE src)

target_link_libraries(mictranscriber PRIVATE
    SDL2::SDL2-static
    SDL2::SDL2main
    portaudio_static
    whisper
    imgui
    OpenGL::GL
)

if(WIN32)
    target_link_libraries(mictranscriber PRIVATE winmm ole32 uuid)
elseif(APPLE)
    find_library(COREAUDIO CoreAudio)
    find_library(AUDIOUNIT AudioUnit)
    find_library(COREFOUNDATION CoreFoundation)
    target_link_libraries(mictranscriber PRIVATE ${COREAUDIO} ${AUDIOUNIT} ${COREFOUNDATION})
else()
    find_package(Threads REQUIRED)
    target_link_libraries(mictranscriber PRIVATE Threads::Threads)
endif()

if(MSVC)
    target_compile_options(mictranscriber PRIVATE /W4)
else()
    target_compile_options(mictranscriber PRIVATE -Wall -Wextra -Wpedantic)
endif()
