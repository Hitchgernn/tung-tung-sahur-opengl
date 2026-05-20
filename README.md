# Tung Tung Sahur OpenGL Visualization

Interactive C++ OpenGL viewer for `TungTungTungSahur.obj`.

## Build

```sh
make
```

## Run

```sh
make run
```

## Controls

- `A` / `D`: rotate camera
- `W` / `S`: tilt camera
- `Q` / `E`: zoom
- `1` - `4`: switch Phong material presets
- `R`: reset camera
- `Esc`: quit

## Implemented OpenGL Requirements

- OBJ model loading from `TungTungTungSahur.obj`
- VAO/VBO upload for mesh vertices and normals
- Separate model, view, and projection matrix setup
- Vertex and fragment shaders using the Phong lighting model
- Multiple material presets with ambient, diffuse, specular, and shininess values
