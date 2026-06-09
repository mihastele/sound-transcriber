Project scaffolded and CMake configures successfully. Here's what was built:

**Architecture:**
- **CMake** build system with FetchContent (auto-downloads all deps)
- **PortAudio** for cross-platform audio device enumeration and capture
- **whisper.cpp** for local Whisper inference (all model sizes: tiny-large)
- **Dear ImGui + SDL2 + OpenGL** for the GUI
- **Lock-free ring buffer** for thread-safe audio passing

**Files created:**
| File | Purpose |
|---|---|
| `CMakeLists.txt` | Build system, fetches SDL2, PortAudio, whisper.cpp, ImGui |
| `src/ring_buffer.h` | Lock-free SPSC ring buffer |
| `src/audio_device.h/cpp` | PortAudio device enumeration |
| `src/audio_capture.h/cpp` | Audio stream capture with callback |
| `src/whisper_engine.h/cpp` | whisper.cpp wrapper with resampling |
| `src/transcriber.h/cpp` | Real-time + record-then-transcribe orchestration |
| `src/gui.h/cpp` | ImGui panels: devices, controls, transcript |
| `src/app.h/cpp` | Main app loop tying everything together |
| `src/main.cpp` | Entry point |

**To build:**
```bash
cmake -B build -G "Visual Studio 17 2022"   # already done
cmake --build build --config Release
```

**To use:** Download a Whisper model (e.g. `ggml-base.bin`) into a `models/` folder, launch the app, load the model via the GUI, select your audio device, pick a language, and start listening.

**Note on output device capture:** Input devices (mics) work everywhere. Capturing system audio output requires WASAPI loopback on Windows, PulseAudio monitors on Linux, or a virtual audio device (BlackHole) on macOS.