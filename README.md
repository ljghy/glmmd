# GLMMD

## Requirements

+ C++17

+ CMake 3.14+

+ OpenGL

## Build

### 1. Install bullet physics

#### Windows

Install with vcpkg:

```shell
vcpkg install bullet3
```

Or build and install from source code:

```
git clone https://github.com/bulletphysics/bullet3.git
```

Then set environment variable `Bullet_ROOT` to the install directory.

#### Linux (Ubuntu)

```shell
sudo apt install libbullet-dev
```

### 2. Install glm

#### Windows

Install with vcpkg:

```shell
vcpkg install glm
```

Or build and install from source code:

```shell
git clone https://github.com/g-truc/glm.git
```

#### Linux (Ubuntu)

```shell
sudo apt install libglm-dev
```

### 3. Build `glmmd` library and viewer

```shell
git clone --recursive https://github.com/ljghy/glmmd.git
cd glmmd
mkdir build
cd build
```

Install [Intel oneTBB](https://github.com/oneapi-src/oneTBB) to enable parallel execution with `gcc` on Linux. Otherwise, add definition `-DGLMMD_DO_NOT_USE_STD_EXECUTION=ON` to CMake to disable the use of `std::execution::par`.

#### Windows MSVC

```shell
cmake -G"Visual Studio 17 2022" [-DCMAKE_TOOLCHAIN_FILE="your_vcpkg_cmake_toolchain_file"] ..
cmake --build . --config Release [--parallel <N>]
.\bin\Release\viewer.exe
```

#### Windows MinGW

```shell
cmake -DCMAKE_BUILD_TYPE=Release -G"MinGW Makefiles" ..
mingw32-make [-j<N>]
.\bin\viewer.exe
```

#### Linux GCC

```shell
cmake -DCMAKE_BUILD_TYPE=Release ..
make [-j<N>]
./bin/viewer
```

## Credits

[Saba](https://github.com/benikabocha/saba)

[glm](https://github.com/g-truc/glm)

[stb image](https://github.com/nothings/stb)

[Dear ImGui](https://github.com/ocornut/imgui)

[GLFW](https://github.com/glfw/glfw)

[Noto Sans CJK](https://github.com/notofonts/noto-cjk)
