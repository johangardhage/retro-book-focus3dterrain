# retro-book-focus3dterrain

The demos from Focus on 3D Terrain Programming, brought back from the 90's.

![Screenshot](/screenshots/demo8_12.png "demo8_12")

## Prerequisites

To build the demo programs, you must first install the following tools:

- [Meson](https://mesonbuild.com/)
- [Ninja](https://ninja-build.org/)
- [GCC](https://gcc.gnu.org/) or [Clang](https://clang.llvm.org/)
- [SDL3](https://www.libsdl.org/)
- [OpenGL](https://www.opengl.org/)

### Install dependencies

#### openSUSE

`$ sudo zypper install meson ninja gcc-c++ SDL3-devel Mesa-libGL-devel glu-devel`

#### Ubuntu

`$ sudo apt install meson ninja-build g++ libsdl3-dev libgl1-mesa-dev libglu1-mesa-dev`

#### macOS

`$ brew install meson ninja pkg-config sdl3`

## Build instructions

To build the demo programs, run:

```
$ meson setup build
$ meson compile -C build
```

The `build` directory will contain the demo programs.

## Usage

```
Usage: demo [OPTION]...

Options:
 -h, --help         Display this text and exit
 -w, --window       Render in a window
     --fullwindow   Render in a fullscreen window
 -f, --fullscreen   Render in fullscreen
 -c, --showcursor   Show mouse cursor
     --nocursor     Hide mouse cursor
```

## License

Licensed under MIT license. See [LICENSE](LICENSE) for more information.

## Authors

Original code written by Trent Polack for [Focus on 3D Terrain Programming](https://www.amazon.com/Focus-Terrain-Programming-Game-Development/dp/1592000282/) (Course Technology).

The original sources also credit Mark Duchaineau (ROAM), Evan Pipho (TGA image loader), Jason Shankel (fractal terrain generation) and Stefan Röttger / Chris Cookson (quadtree LOD).

## Screenshots

![Screenshot](/screenshots/demo1_1.png "demo1_1")
![Screenshot](/screenshots/demo2_2.png "demo2_2")
![Screenshot](/screenshots/demo3_3.png "demo3_3")
![Screenshot](/screenshots/demo4_3.png "demo4_3")
![Screenshot](/screenshots/demo8_6.png "demo8_6")
![Screenshot](/screenshots/demo8_7.png "demo8_7")
