# 3D Chair — OBJ Import & Wireframe Renderer

A Blender-modeled chair imported into a custom OpenGL pipeline, rendered as an interactive wireframe with keyboard-driven transformations.

## Overview

This project models a piece of furniture in Blender to real-world dimensions, exports it as a Wavefront OBJ file, and renders it in a from-scratch OpenGL 3.3 Core Profile pipeline — no third-party model-loading library. A hand-written parser reads the OBJ format directly, and the renderer supports live translation, rotation, and scaling via the keyboard, with the model always rotating in place around its own center regardless of where its geometry sits in object space.

## Features

- **Custom OBJ parser** — reads vertex positions and face data directly from the Wavefront format, with no dependency on Assimp or tinyobjloader
- **Fan triangulation** — automatically converts non-triangular faces into triangles
- **Index validation** — bounds-checks every parsed index against the vertex list before rendering
- **Rotate-in-place** — computes the model's bounding-box center at load time and re-centers it in the transformation pipeline, so rotation and scaling always pivot around the object itself
- **Auto-fit scaling** — normalizes any loaded model to a consistent on-screen size regardless of its real-world units
- **Live keyboard controls** — translate, rotate, and scale the model in real time
- **Wireframe rendering** — pure edge rendering via `glPolygonMode`

## Tech Stack

- **Language:** C++
- **Graphics API:** OpenGL 3.3 (Core Profile)
- **Windowing / Input:** GLFW 3.5.1
- **Function loading:** GLEW 2.3.1
- **Math:** GLM
- **Modeling:** Blender

## Getting Started

### Prerequisites

- A C++ compiler (developed with MinGW-w64 / g++)
- [GLFW](https://www.glfw.org/) 3.5.1
- [GLEW](http://glew.sourceforge.net/) 2.3.1
- [GLM](https://github.com/g-truc/glm)

### Build

```bash
g++ main.cpp objloader.cpp -o Chair.exe -Wall -Wextra \
  -I"<path-to-glfw>/include" \
  -I"<path-to-glew>/include" \
  -I"<path-to-glm>" \
  -L"<path-to-glfw>/lib-mingw-w64" \
  -L"<path-to-glew>/lib/Release/x64" \
  -lglfw3 -lglew32 -lopengl32 -lgdi32
```

Adjust the include/library paths to match your local GLFW and GLEW installations.

### Run

`glew32.dll` must sit next to the executable at runtime.

```bash
./Chair.exe
```

## Usage

| Key | Action |
|---|---|
| `W` / `A` / `S` / `D` | Translate up / left / down / right |
| `Q` / `E` | Rotate counter-clockwise / clockwise |
| `R` / `F` | Scale up / down |
| `Esc` | Close the window |

## Project Structure

```
.
├── main.cpp        # Window/context setup, shaders, render loop, input handling
├── objloader.h      # OBJ loader interface
├── objloader.cpp    # OBJ parsing implementation
├── Chair.obj        # Exported model geometry
├── Chair.mtl        # Material file (unused by the wireframe renderer)
└── Chair.blend      # Source Blender file
```
