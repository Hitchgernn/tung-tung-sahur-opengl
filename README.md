# Tung Tung Sahur OpenGL Visualization

An interactive C++ OpenGL program for viewing and manipulating the `TungTungTungSahur.obj` 3D model.
This project demonstrates core OpenGL concepts including model loading, VAO/VBO usage, matrix transformations,
and Phong shading with multiple material presets.
The model uses UV coordinates from the OBJ file and samples `texture.png` for its surface texture.

## Table of Contents
- [Features](#features)
- [Requirements](#requirements)
- [Getting Started](#getting-started)
  - [Building](#building)
  - [Running](#running)
- [Controls](#controls)
- [Implementation Details](#implementation-details)
- [Acknowledgments](#acknowledgments)
- [License](#license)

## Features

- Loads and displays a 3D model from an OBJ file (`TungTungTungSahur.obj`)
- Loads and applies `texture.png` using the OBJ texture coordinates
- Uses Vertex Array Objects (VAO) and Vertex Buffer Objects (VBO) for efficient GPU rendering
- Implements separate model, view, and projection matrix transformations
- Phong lighting model with ambient, diffuse, and specular components
- Four interchangeable material presets (editable in `src/main.cpp`)
- Interactive camera controls:
  - Rotate orbit (left/right)
  - Tilt pitch (up/down)
  - Zoom in/out
  - Reset to default view
- Floor grid visualization for spatial reference
- Depth testing, face culling, multisampling, and blending enabled

## Requirements

- A C++17 compatible compiler (e.g., GCC, Clang)
- OpenGL 3.3 or higher
- GLFW 3.x for window and input handling
- GLEW for OpenGL extension loading
- Make (for building via the provided Makefile)

On Ubuntu/Debian, install dependencies with:
```bash
sudo apt-get install libglfw3-dev libglew-dev
```

## Getting Started

### Building

Clone the repository and build the executable using the provided Makefile:

```bash
git clone <repository-url>
cd opengl/tung-tung-sahur
make
```

This will compile the source and produce an executable named `tung_tung_sahur`.

### Running

Run the executable from the project root directory, where `TungTungTungSahur.obj` and `texture.png` are located:

```bash
make run
# or directly:
./tung_tung_sahur
```

Ensure the OBJ and PNG texture files are in the same directory as the executable, or adjust the paths in `src/main.cpp`.

## Controls

| Key | Action |
|-----|--------|
| `A` | Rotate camera left (orbit) |
| `D` | Rotate camera right (orbit) |
| `W` | Tilt camera up |
| `S` | Tilt camera down |
| `Q` | Zoom out (increase distance) |
| `E` | Zoom in (decrease distance) |
| `1` | Material preset 1 (brown/orange) |
| `2` | Material preset 2 (yellow/gold) |
| `3` | Material preset 3 (blue/cyan) |
| `4` | Material preset 4 (gray/blue) |
| `5` | Cycle light position (right side) |
| `R` | Reset camera to default position |
| `Esc` | Exit the application |

## Implementation Details

### Core Components

- **Math Structures**: Custom `Vec2`, `Vec3`, and `Mat4` classes for vector and matrix operations.
- **Model Loading**: `loadObj()` function parses OBJ files, extracts vertices, normals, and texture coordinates, then computes model center and scale.
- **Texture Loading**: `loadTexture()` loads `texture.png` through stb_image and uploads it as an OpenGL 2D texture.
- **Shaders**:
  - Vertex Shader: Transforms vertices using model/view/projection matrices, passes world position, normal, UV, and height to fragment shader.
  - Fragment Shader: Implements Phong lighting with:
    - Ambient, diffuse, and specular components
    - Texture sampling from `texture.png`
    - Rim lighting effect
    - Height-based patterned diffuse (sinusoidal bands)
- **Rendering**:
  - VAO/VBO setup for both the model and a floor grid
  - Separate shader programs for mesh and grid rendering
  - Depth testing, face culling, multisampling, and alpha blending enabled
- **Input Handling**: GLFW-based camera orbit controls with pitch and distance clamping.

### Key Files

- `src/main.cpp`: Contains all application logic (model loading, shaders, rendering loop, input handling).
- `TungTungTungSahur.obj`: The 3D model file (over 1MB, not shown in repo if large).
- `texture.png`: Texture image sampled by the mesh shader.
- `src/stb_image.h`: Header-only image loader used for PNG loading.
- `Makefile`: Simple build script using g++ with GLFW and GLEW linking.
- `report.*`: LaTeX-generated documentation (likely from an academic assignment).

## Acknowledgments

This project appears to be based on an academic assignment for an OpenGL/computer graphics course.
The model `TungTungTungSahur.obj` is likely a reference to the "Tung Tung Sahur" internet meme.

## License

No explicit license is specified in the repository. Please check with the author for usage rights.
