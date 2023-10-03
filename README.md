# GLMMD

## Requirements

+ C++17

+ CMake 3.14+

+ OpenGL

## Build

### 1. Install bullet physics

#### Windows

```
git clone https://github.com/bulletphysics/bullet3.git
```

Then build and Install from source code.

Set environment variable `Bullet_ROOT` to the install directory, or add definition `-DGLMMD_BULLET_ROOT=<install directory>` to CMake when building glmmd.

#### Ubuntu

```shell
sudo apt install libbullet-dev
```

### 2. Build `glmmd` library and viewer

```shell
git clone --recursive https://github.com/ljghy/glmmd.git
cd glmmd
mkdir build
cd build
```

Create file `init.json` in `build`:

```json
{
    "MSAA": 4,
    "models": [
        {
            "filename": "model0.pmx"
        },
        {
            "filename": "model1.pmx"
        }
    ],
    "motions": [
        {
            "filename": "motion1.vmd",
            "model": 0
        },
        {
            "filename": "motion2.vmd",
            "model": 0,
            "loop": true
        },
        {
            "filename": "motion3.vmd",
            "model": 1
        }
    ]
}
```

If `execution` header file is not supported, add definition `-DGLMMD_DO_NOT_USE_STD_EXECUTION=ON` to CMake.

#### Windows MSVC

```shell
cmake ..
cmake --build . --config Release [--parallel <N>]
.\apps\viewer\Release\viewer.exe
```

#### Windows MinGW

```shell
cmake -DCMAKE_BUILD_TYPE=Release -G"MinGW Makefiles" ..
mingw32-make [-j<N>]
.\apps\viewer\viewer.exe
```

#### Linux GCC

```shell
cmake -DCMAKE_BUILD_TYPE=Release ..
make [-j<N>]
./apps/viewer/viewer
```

## Credits

[Saba](https://github.com/benikabocha/saba)

[Dear ImGui](https://github.com/ocornut/imgui)

[nlohmann json](https://github.com/nlohmann/json)

[stb image](https://github.com/nothings/stb)

[glm](https://github.com/g-truc/glm)

[GLFW](https://github.com/glfw/glfw)
