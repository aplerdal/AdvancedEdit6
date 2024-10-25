# Advanced Edit 6
A work-in-progress, cross platform track/graphics editor for Mario Kart: Super Circuit.
## Features
- [x] Editing Tilemaps
- [ ] Editing Palettes
- [ ] Editing tilesets
- [ ] Editing AI
- [ ] Saving modified game
- [ ] Quick track testing using savestates.
- [x] Techical documentation ([Mksc RE](https://github.com/aplerdal/MkscRE))
## Building
The linux build instructions are not fully tested, but should work.
### Windows
#### Dependecies
Make sure you have MSVC installed. It can be obtained by downloading visual studio and including `c/c++ Development Tools`.

Clone or download this repositiory into a local directory.

Run the commands
```sh
mkdir libs
mkdir external
git clone -b docking https://github.com/ocornut/imgui.git
```
Then, to install sdl, visit their releases page: [https://github.com/libsdl-org/SDL/releases](https://github.com/libsdl-org/SDL/releases) and install SDL3-devel-3.X.X-VC.zip and extract it into `AdvancedEdit6/libs/SDL`
#### Building
To build, run
```sh
mkdir build
cd build
cmake ..
cmake --build .
```
And if there are no errors the result should be in `build/Debug/`. You may need to manually copy the SDL3.dll file from `libs` for the project to run.
