# Live2Desktop

<img src="logo.png">

**一个基于 Qt 框架的 Live2D 模型展示、互动的桌面应用。**


## **⚠️** 注意

- **当前分支正在开发，可能有稳定性问题。如果希望更稳定的版本，请回到 `stable` 分支**。

- <u>Release 中没有本分支的编译产物。如果想尝试本分支的程序，请自行编译</u>。


## 功能

- Live2D 展示和有限的互动；

    - 添加模型：请自行向 `Resources` 目录下放置模型目录（源码中给了 `Hiyori` 作为例子），在 `Resources/config.json` 中手动添加模型名称即可。

    - 模型的表情、动作管理：模型的表情、动作等信息管理见程序托盘 `settings` 图形界面。

    - 模型几何性质调整：模型展示的大小、位置参见程序托盘 `Geometry Edit`。

    - 模型互动：暂时支持拖动、轻点两种动作。在程序的几何区域内（可以通过 `Geometry Edit` 自行编辑）拖动鼠标可以吸引模型视线；轻点模型随机展示表情、动作。

- 与 Client 语音对话、AI Agent 输出转语音（需要配置，参见下文）；

- 大语言模型对话（需要配置，参见下文）；

- 无需打开对话框，全局热键录音（CTRL+SHIFT+R，再次按下停止并提交）、大模型输出语音回复。

- 支持流式对话、MCP 工具调用（OpenAI 标准格式）；

## 已知问题

- Windows 下需要在生成 Makefile 时指定静态链接库 `cmake -B build -DBUILD_SHARED_LIBS=OFF`，不能使用动态链接库。

## 正在支持/未来可能会支持的

- [x] 请参见 [TODO 文件](./TODO);


## 更新日志

- 2024/02/14：Live2Desktop 去除 Live2D 官方 `glfw / glew` 耦合，迁移至 Qt Platform（`QOpenGLWidget`）；
- 2024/07/29：更新 Live2Desktop 的 Live2D 内核为 `5-r.1` 版本。可能支持 moc3 为 `5.0` 的版本，并未进行测试。
- 2024/08/19：更新 Live2Desktop 的 `Framework/Rendering`，Framework 代码接口与 `5-r.1` 完成同步，移除备份目录；
- 2025/03/28：更新语音、聊天功能（仍在测试开发中）；
- 2025/04/08：更新 Live2Desktop 的 Live2D 内核为 `5-r.3` 版本、支持 LLM 大模型、语音输入输出（STT/TTS）；
- 2025/06/07：支持流式对话、MCP 工具调用；


## 背景

众所周知，Live2D 模型在 Windows 平台下的展示与互动依托于 Live2DViewerEx 收费应用，而 Linux 平台则鲜有闻之。本应用旨在依托于 Live2D Cubism 官方 SDK 和 Qt 框架，建立起一个轻量级、跨平台的 Live2D 模型展示、互动的桌面应用。

此外，官方 Cubism SDK 使用 glew / glfw 链接库，**不仅不利于应用平台迁移，在 Qt 框架下还无法使用最新的 `QOpenGLWidget`（头文件冲突），很多特性难以实现（例如背景透明）**，可能给开发者带来不少麻烦。

本应用从**修改官方 SDK 的角度**出发，进行诸如将 glew 替换为 Qt 内置 `OpenGL` 函数之类的操作，为 Live2D 在 Qt 框架下的开发提供一种全新的开发途径。


## 配置对话模型

任意支持 OpenAI API 的模型服务均可。请修改仓库目录下的 `config/module_config.json` 然后执行 `make update-config`；

对配置文件作出任何修改后记得都需要执行 `make update-config`。

由于项目正在开发，且个人时间不充足，因此暂时不支持图形化设置，请见谅。

## 配置模型对话时语音转文字、文字转语音

出于性能考虑，本项目使用 sense-voice 本地推理进行语音转文字。

由于本项目还在开发中，因此你需要手动执行 `./get_model.sh` 或者从 [huggingface](https://huggingface.co/lovemefan/sense-voice-gguf) 上手动下载任一模型，并放到 `build/bin/models` 中（如果没有则手动创建）。

然后在 `config/module_config.json` 中修改 `stt -> model` 中的文件名替换为你的模型文件名称。

现在就配置好了语音转文字功能。

本项目目前使用 web service 的方案支持文字转语音。你可以选择任意一个遵循 OpenAI API 的语音转文字的模型服务，将完整 URL 和 API 等信息填写入 `config/module_config.json` 的 `tts` 子项中。

> 例如：你可以使用 [Kokoro FastAPI](https://github.com/remsky/Kokoro-FastAPI.git) 然后用 docker 容器跑一个文字转语音的模型，并且将它的服务信息配置在上面的文件中。

现在就配置好了文字转语音功能。

对配置文件作出任何修改后记得 `make update-config`。

预计未来将以上两种统一处理，无需再手动操作。

## 配置 MCP 工具调用

理论上应该支持所有符合 MCP 标准的 servers（参见 [modelcontextprotocol/**servers** - github](https://github.com/modelcontextprotocol/servers)）；

配置方法类似上文，不再赘述。示例已经给出在仓库的配置文件中。注意，你需要保证 `command` 里面的指令能够在宿主机上正常运行。


## 快速开始

下拉本仓库及子模块：

```bash
git clone https://github.com/SSRVodka/Live2Desktop
git submodule sync && git submodule update --init --recursive
```

接下来，您可以从 Release 中下载已经编译好的二进制文件，也可以自行编译。

在编译前，您需要准备以下开发环境：

- C++ 开发套件： Unix 平台下仅需要 `build-essential`，Windows 平台需要安装 MSVC 编译器及对应开发环境；

- CMake；

- Qt 5 开发框架（Qt 5.15）。


> 请注意，Qt 的版本不宜过低，需要支持 `QOpenGLWidget` 特性。

#### 直接下载二进制程序

参见 Release。

#### 使用 CMake 编译

Unix 下的编译指令如下：

```bash
cmake -B build && cd build && make -j
# 或者
cmake -B build && cmake --build build
```

默认情况下 `CMAKE_BUILD_TYPE=Release`（Windows 需要在编译时额外添加 `--config Release`）。您可以传入 `-DDEBUG=ON` 的宏来编译 Debug 版本，加入了 GDB 调试符号。例如：

```bash
cmake -B build -DDEBUG=ON
cd build
make -j
```

> 除了上述编译指令，您还可以使用 Makefile 帮助您操作 cmake 以及参数。仓库里有默认的 Makefile，您只需：
>
> ```shell
> # 使用 make 脚本（如果需要修改 CMake Flags 可能需要自己手动更改根目录 Makefile）
> # 默认 flags：(cmake) -DGGML_BLAS=ON  (build) --config Release
> make clean && make build
> ```
>
> 如果您需要修改编译参数等等，则需要您自行修改 Makefile。

本项目引用了 ggml，如果您的机器支持 BLAS 加速，可以加入参数 `-DGGML_BLAS=ON`。

默认以动态链接库的形式编译依赖库。如果想要更换为静态链接库，请自行调试。

默认程序输出位置在 `${CMAKE_BINARY_DIR}/bin/` 目录下，链接库输出在 `${CMAKE_BINARY_DIR}/lib/` 目录下。


## 兼容性测试

- Windows: Windows 10, MSVC 19+, CMake 3.16, Qt 5.15

- Linux: Ubuntu 22.04, gcc/g++ 12, CMake 3.22.1, Qt 5.15.3

## 声明

源文件（`*.h/*.hpp`）开头注明 `Live2D Cubism Inc.` 的源码解释权归该公司所有。

注明 `@Author SSRVodka` 的遵循本项目许可证。

## 致谢

- [nlohmann/json.hpp](https://github.com/nlohmann/json)

- [cpp-httplib](https://github.com/yhirose/cpp-httplib)

- [ggml](https://github.com/ggml-org/ggml)

- [cpp-mcp](https://github.com/hkr04/cpp-mcp)
