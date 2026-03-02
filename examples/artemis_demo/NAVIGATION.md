# Artemis Demo — Navigation Tracking

## Purpose
Navigation log for the standalone Artemis L-System Tree Demo inside the
Nature Reality Engine repository.

---

## File Map

| File | Role |
|---|---|
| `examples/artemis_demo/main.cpp` | Demo entry point — runs all subsystems |
| `engine/nature/LSystem.h/cpp` | L-system rewriter (Oak, Pine, Willow) |
| `engine/nature/Mesh.h` | 3-D mesh data structures |
| `engine/nature/TreeRenderer.h/cpp` | Full tree renderer (Generate/Render/Wind/Season/Grow) |
| `engine/nature/WaterRenderer.h` | Water surface renderer stub |
| `engine/nature/VegetationSystem.h` | Instanced vegetation stub |
| `engine/nature/WeatherSystem.h` | Atmospheric weather stub |
| `engine/renderer/Renderer.h` | Abstract IRenderer (Vulkan hookable) |
| `CMakeLists.txt` | CMake build definition |

---

## Build Instructions

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/artemis_demo
```

---

## Integration Path

```
examples/artemis_demo/main.cpp
        │
        ▼
engine/nature/TreeRenderer
        │ uses
        ├── engine/nature/LSystem      (string rewriter)
        ├── engine/nature/Mesh         (vertex/index buffers)
        └── engine/renderer/IRenderer  (GPU backend interface)
                │ implemented by
                └── VulkanRenderer     (future Vulkan backend)
```

---

**Last Updated**: 2026-03-02  
**Status**: ✅ Complete
