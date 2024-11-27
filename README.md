# Advanced Edit 6
[![Build](https://github.com/aplerdal/AdvancedEdit6/actions/workflows/build-win.yml/badge.svg)](https://github.com/aplerdal/AdvancedEdit6/actions/workflows/build-win.yml)

A work-in-progress, cross platform track/graphics editor for Mario Kart: Super Circuit.

For support, ask in `#antimattur` in the [Mksc Hacking Discord](https://discord.gg/C6dNp2EvGy) or open an issue.

## Features
- [x] Editing Tilemaps
- [ ] Editing Palettes
- [ ] Editing tilesets
- [ ] Editing AI
- [ ] Saving modified game
- [ ] Quick track testing using savestates
- [x] Techical documentation ([Mksc RE](https://github.com/aplerdal/MkscRE))
## Building
I don't update these instructions often atm. If you want to build these may not always work.

Clone or download this repositiory into a local directory including submodules using 
```
git clone --recurse-submodules https://github.com/aplerdal/AdvancedEdit6.git
```
and then follow the build steps for your platform.
### Windows
#### Dependecies
Make sure you have MSVC installed. It can be obtained by downloading visual studio and including `c/c++ Development Tools`.
Then, to install sdl, visit their releases page: [https://github.com/libsdl-org/SDL/releases](https://github.com/libsdl-org/SDL/releases) and install SDL3-devel-3.X.X-VC.zip and extract it into `AdvancedEdit6/SDL`
#### Building
To build, run
```sh
mkdir build
cd build
cmake ..
cmake --build .
```
And if there are no errors the result should be in `build/Debug/`.
### Linux
#### Dependencies
You must have g++ and SDL3-dev installed.
#### Building
```sh
mkdir build
cd build
cmake ..
cmake --build .
```
