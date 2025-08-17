# Chess 3D (C++ & OpenGL)
## Project Overview

This project is a C++/OpenGL 3D chess scene with lighting, reflections, shadows, a skybox (cubemap), and an in-world billboarding system for game messages. It builds with CMake and the included third-party libraries (GLFW, GLAD, stb).

![Preview](chess-3D-Preview.png)

## Features
- Lighting and Reflections (combined in `LightingAndReflection`)
- Shadow mapping
- Cubemap skybox
- Texture system (with caching)
- Billboarding for on-screen messages (greeting, check, checkmate, stalemate)
- Game logic helpers (check, checkmate, stalemate detection)
- LoadModel (`Chess/src/Object/Piece/*.obj`)
- MoveObject with key press
- CameraControl (orbit around the board)

## Project Structure
- `Chess/src/` – engine, features and game code
  - `Feature/basic/LightingAndReflection/` – shaders and utilities for lighting + reflections
  - `Feature/advanced/Shadow/` – shadow map creation and receivers
  - `Feature/basic/Cubemap/` – skybox loader (uses stb_image)
  - `Feature/intermediate/Billboarding/` – billboard text rendering for messages
    - `font/letter/*.png`, `font/number/*.png`, `font/symbol/{exclamation,question,space}.png`
  - `Object/Piece` - chess pieces in .obj
- `3rdParty/` – GLFW, GLAD, stb headers
- `CMakeLists.txt` – project build settings

## Tested Environment
- Windows 10/11, Visual Studio 2022 (MSVC v143+)
- CMake 3.26+
- GPU supporting OpenGL 3.3 Core Profile

## Build Types
- Debug: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`
- Release: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`

## Build (Windows)
Prerequisites: Visual Studio (MSVC), CMake, a recent Windows SDK.

1) Configure
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

2) Build
```
cmake --build build --config Debug
```

3) Run
- Launch the produced executable from the build directory (e.g., `build/Chess.exe`).
- The game starts in fullscreen on the primary monitor.

> Fullscreen is enabled by default. To run windowed, open `Chess/src/main.cpp` and change the fullscreen window creation to pass `NULL` for the monitor in `glfwCreateWindow`.

## Controls
- Camera: mouse to orbit, scroll to zoom, press `Caps Lock` to fix/lock camera
- Light control: press `L` to toggle light movement mode
  - Move: `W/A/S/D` (horizontal), 
  - Movment Speed: `Q/E`
  - Brightness: `T`/`G`
- Piece Movement: select a piece to move (highlighted in RED) using `arrow` keys
  - Help: `H`
  - Move: `Shift - hold` (remove piece from original square), 
          `Shift - release` (place piece on final square)

## Billboarding (Text)
- Text is rendered by composing per-character PNGs (with alpha) into a texture at runtime.
- Only uppercase glyphs are used; input is converted to uppercase automatically.
- Place font images under:
  - `Chess/src/Feature/intermediate/Billboarding/font/letter/A.png ... Z.png`
  - `Chess/src/Feature/intermediate/Billboarding/font/number/0.png ... 9.png`
  - `Chess/src/Feature/intermediate/Billboarding/font/symbol/{exclamation,question,space}.png`
- Greeting message appears for 10 seconds at startup. Check/stalemate/checkmate messages appear when detected.

## Resources
- 3D Model: https://free3d.com/3d-models/obj
- Cubemap: 
  - https://www.freepik.com/free-ai-image/shot-panoramic-composition-living-room_40572720.htm 
  - https://jaxry.github.io/panorama-to-cubemap/
- Font Text:
  - https://icons-for-free.com/ 
  - https://onlinepngtools.com/create-transparent-png

## Contribution
- Debby Miressa MIJENA

## Future Work
- **Sound**: Add sound effects (move, capture, check, UI feedback) with adjustable volume and mute toggle.
- **AI integration (single-player mode)**: Add a chess engine (e.g., Minimax with alpha-beta pruning) to allow human vs. AI games, with multiple difficulty levels.
- **Tutorials and hints**: Highlight recommended moves from an AI helper and interactive rules tutorial.
- **Timers and time controls**: Chess clock with configurable controls (blitz/rapid/classical), time increments (Fischer), and time warnings in the UI.
- **Online multiplayer**: Peer-to-peer or server-based matchmaking, ELO rating, and reconnection support.
- **Move history and PGN export**: Track move list with SAN notation, allow saving/loading games and exporting PGN.
- **Undo/redo and analysis mode**: Rollback moves, explore variations, evaluate positions with engine lines.
- **Animations and effects**: Smooth piece movement, capture animations, check/checkmate highlights, and sound effects.
- **Accessibility**: Keyboard-only play, color-blind friendly palette, scalable UI text, and high-contrast mode.
- **Customization**: Themes for boards/pieces (wood/marble/metal), toggles for reflections/shadows for performance.
