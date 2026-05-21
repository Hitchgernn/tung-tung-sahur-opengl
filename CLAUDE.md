# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Development Commands

- Build the executable: `make` or `make all`
- Run the program: `make run` or `./tung_tung_sahur` (ensure `TungTungTungSahur.obj` and `texture.png` are in the same directory)
- Clean build artifacts: `make clean`

There are no automated tests or linting scripts in this project.

## Project Structure

- `src/main.cpp`: Single source file containing all application logic:
  - Custom math structures (`Vec2`, `Vec3`, `Mat4`)
  - OBJ model loading via `loadObj()`, including positions, normals, and UV coordinates
  - Texture loading via `loadTexture()` and `src/stb_image.h`
  - Vertex and fragment shaders (texture sampling, Phong lighting, rim lighting, and height-based pattern)
  - VAO/VBO setup for the model and a floor grid
  - GLFW-based input handling for camera orbit controls
  - Main rendering loop
- `TungTungTungSahur.obj`: The 3D model file (large binary, not tracked in diffs)
- `texture.png`: Surface texture loaded at runtime and sampled by the mesh shader
- `src/stb_image.h`: Header-only image loader used for `texture.png`
- `Makefile`: Simple build script using g++ with GLFW and GLEW linking
- `report.*`: LaTeX-generated documentation (academic assignment materials)

## Key Implementation Details

- OpenGL 3.3+ core profile with VAO/VBO for efficient rendering
- OBJ faces are expected to use `v/vt/vn`-style indices so UVs can be mapped to the texture
- Mesh vertices use three vertex attributes: position at location 0, normal at location 1, and UV at location 2
- Separate shader programs:
  - Mesh shader: Samples `texture.png`, then applies Phong lighting with ambient, diffuse, specular, rim lighting, and sinusoidal height-based diffuse
  - Grid shader: Simple colored grid for spatial reference
- Camera controls: orbit (left/right), pitch (up/down), zoom (in/out), reset
- Enabled OpenGL features: depth testing, face culling, multisampling, alpha blending
- Material presets switchable via keys 1-4 (editable in `src/main.cpp`)

## Notes

- The program expects the OBJ and texture PNG files in the working directory; adjust the paths in `src/main.cpp` if needed.
- No configuration files or external dependencies beyond GLFW, GLEW, OpenGL, and the vendored `src/stb_image.h`.
- The codebase is intentionally compact for educational purposes, demonstrating fundamental OpenGL concepts in a single file.
