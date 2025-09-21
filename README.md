<img src="app/src/main/assets/icon.bmp" width="256" alt="logo">

# 🎮 SENSE: The Game Customizer

# Project Compilation Guide

## Windows

### 64-bit build

``` bat
cmake -S app/src/main -B build64 -G "Visual Studio 17 2022" ^
  -A x64 ^
  -DBUILD_SHARED_LIBS=ON ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_INSTALL_PREFIX=build/release64 ^
  -D CMAKE_C_FLAGS="/D_WIN32_WINNT=0x0601 /DNTDDI_VERSION=0x06010000" ^
  -D CMAKE_CXX_FLAGS="/D_WIN32_WINNT=0x0601 /DNTDDI_VERSION=0x06010000"

cmake --build build64 --config Release
cmake --install build64
```

### 32-bit build

``` bat
cmake -S app/src/main -B build32 -G "Visual Studio 17 2022" ^
  -A Win32 ^
  -DBUILD_SHARED_LIBS=ON ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_INSTALL_PREFIX=build/release32 ^
  -D CMAKE_C_FLAGS="/D_WIN32_WINNT=0x0601 /DNTDDI_VERSION=0x06010000" ^
  -D CMAKE_CXX_FLAGS="/D_WIN32_WINNT=0x0601 /DNTDDI_VERSION=0x06010000"

cmake --build build32 --config Release
cmake --install build32
```

------------------------------------------------------------------------

## Linux

### 64-bit build

``` bash
cmake -S app/src/main -B build64   -DCMAKE_BUILD_TYPE=Release   -DBUILD_SHARED_LIBS=ON   -DCMAKE_INSTALL_PREFIX="/home/release64"   -DCMAKE_C_FLAGS="-m64"   -DCMAKE_CXX_FLAGS="-m64"

cmake --build build64 --config Release
cmake --install build64
```

### 32-bit build

``` bash
cmake -S app/src/main -B build32   -DCMAKE_BUILD_TYPE=Release   -DBUILD_SHARED_LIBS=ON   -DCMAKE_INSTALL_PREFIX="/home/release32"   -DCMAKE_C_FLAGS="-m32"   -DCMAKE_CXX_FLAGS="-m32"

cmake --build build32 --config Release
cmake --install build32
```

------------------------------------------------------------------------

## Android

For building on **Android**, please use **Android Studio**.

## 🧑‍💻 Authors

- [IPOleksenko](https://github.com/IPOleksenko) (owner) — Developer and creator of the idea.

## 📜 License

This project is licensed under the [MIT License][license].

[license]: ./LICENSE

---

> _"One must imagine Sisyphus happy."_ — Albert Camus
