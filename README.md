# Live2Desktop

<img src="logo.png">

**A desktop application for Live2D model management and interaction based on Qt.**

<a href="README_zh_CN.md">中文 README</a>

## Update Log

- 2024/02/14: Live2Desktop de-coupled Live2D official `glfw / glew` and migrated to Qt Platform (`QOpenGLWidget`);

- 2024/07/29: Updated Live2Desktop's Live2D Core to `5-r.1` version. May support moc3 as `5.0`, not tested.

- 2024/08/19: Updated `Framework/Rendering` of Live2Desktop, synchronization of Framework code interface with `5-r.1` completed, backup directory removed.

## Background

As we all know, Live2D model display and interaction on Windows platform relies on Live2DViewerEx paid application, while Linux platform is rarely known. This application aims to build a lightweight, cross-platform desktop application for displaying and interacting with Live2D models based on the official Live2D Cubism SDK and Qt framework.

In addition, the official Cubism SDK uses glew / glfw linked libraries, **not only is it not good for application platform migration, but also can't use the latest `QOpenGLWidget` in Qt framework (header file conflict), and it's difficult to realize a lot of features (e.g., background transparency)**, which may cause a lot of trouble to the developer.

This application starts from the perspective of **modifying the official SDK**, and does things like replacing glew with Qt's built-in `OpenGL` functions, which provides a new way to develop Live2D in the Qt framework.

## Functions

- Adding model: Please place the model directory in `Resources` directory by yourself (`Hiyori` is given as an example in the source code), and add the model name manually in `Resources/config.json`.

- Managing model expressions and actions: Managing model expressions and actions can be found in the `settings` graphical interface in the program tray.

- Model Geometry Adjustment: See `Geometry Edit` in the program tray for the size and position of the model display.

- Model interaction: For the time being, two types of actions are supported: dragging and tapping. Dragging the mouse in the geometry area of the program (which can be edited by `Geometry Edit`) can attract the model's attention; tapping the model will show random expressions and actions.


## Quick Start

You can download the compiled binaries from the Release or compile them yourself.

Before compiling, you need to prepare the following development environment:

- C++ development kit: Only `build-essential` is needed for Unix platform, and MSVC compiler and corresponding development environment are needed for Windows platform;

- CMake;

- Qt 5 development framework (Qt 5.15).


> Please note that the version of Qt should not be too low, and the `QOpenGLWidget` feature needs to be supported.


The compilation commands for Unix are as follows:

```bash
cmake -B build && cd build && make -j
# or
cmake -B build && cmake --build build
```

By default `CMAKE_BUILD_TYPE=Release`. You can pass in the `-DDEBUG=ON` macro to build the Debug version, for example:

```bash
cmake -B build -DDEBUG=ON
cd build
make -j
```

The `USE_SHARED_LIB` option is also provided, which allows you to choose whether to link a third-party dynamic or static library, with the default being static. For example:

```bash
cmake -B build -DUSE_SHARED_LIB=ON
cd build
make -j
```

The default program output is in the `${CMAKE_BINARY_DIR}/bin/` directory.

## Compatibility Test

- Windows: Windows 10, MSVC 19+, CMake 3.16, Qt 5.15

- Linux: Ubuntu 22.04, gcc/g++ 12, CMake 3.22.1, Qt 5.15.3


## Declaration

The source files (`*.h/*.hpp`) begin with the statement `Live2D Cubism Inc.` The source code is copyrighted by that company.

Statements `@Author SSRVodka` follow the license of this project.

