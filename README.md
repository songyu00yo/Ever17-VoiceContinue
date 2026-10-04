# Ever17 Voice Continue

[English](README.en.md)

为 2002 PC 版 `Ever17 -the out of infinity-` 添加 Voice Continue（语音延续）功能。

[![build](https://github.com/songyu00yo/Ever17-VoiceContinue/actions/workflows/build.yml/badge.svg)](https://github.com/songyu00yo/Ever17-VoiceContinue/actions/workflows/build.yml)
[![release](https://img.shields.io/github/v/release/songyu00yo/Ever17-VoiceContinue)](https://github.com/songyu00yo/Ever17-VoiceContinue/releases/latest)
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![platform](https://img.shields.io/badge/platform-Windows%20x86-lightgrey)

![C](https://img.shields.io/badge/language-C-A8B9CC?logo=c&logoColor=black)
![MSVC](https://img.shields.io/badge/toolchain-MSVC-5C2D91?logo=visualstudio&logoColor=white)
![Win32](https://img.shields.io/badge/API-Win32-0078D6)
![PE](https://img.shields.io/badge/target-PE%20patcher-grey)

## 下载

稳定版直接从 Releases 下载：

**[下载 Ever17_VoiceContinue.exe](https://github.com/songyu00yo/Ever17-VoiceContinue/releases/download/v1.0.0/Ever17_VoiceContinue.exe)**

Release 同时提供 SHA-256 校验文件。

## 功能

原版在翻到下一句文本时，会停止当前角色语音。补丁修改后的行为如下：

| 操作 | 结果 |
| --- | --- |
| 下一句没有新语音 | 当前语音继续播放 |
| 下一句有新语音 | 当前语音停止，新语音立即播放 |
| 语速 | 保持原速 |
| 多条语音 | 不排队，不重叠 |

项目不包含游戏本体、剧情脚本、语音或其他游戏资源。补丁只修改用户本地 EXE 中的两处语音控制逻辑。

## 工作原理

Ever17 老 PC 引擎把“翻页”和“停止当前语音”绑定在一起。

直接拦截 DirectSound 的 `Stop()` 会影响另一项正常行为：新语音开始时，游戏也需要停止上一条语音。拦截这次停止操作后，后续语音会进入队列。

本补丁只跳过两条“翻页触发语音停止”的代码路径。游戏原有的“新语音替换旧语音”逻辑保持不变。

```text
翻页，下一句无语音  -> 当前语音继续
翻页，下一句有语音  -> 停止当前语音，播放新语音
```

技术细节见 [docs/TECHNICAL.md](docs/TECHNICAL.md)。

## 兼容性

当前版本扫描 PE 可执行节中的机器码特征，不依赖固定文件偏移。

以下内容发生变化时，扫描仍可能正常工作：

- EXE 文件名和文件大小；
- SHA-256、PE 时间戳和资源；
- section 文件偏移；
- 代码中的绝对地址；
- `CALL rel32` 操作数。

只有两段目标代码都被唯一识别，并且状态一致时，程序才会写入。

因此，只要汉化版或整合版仍使用 2002 PC 版 KID 引擎的同一套语音控制代码，就有机会直接兼容。修改文本、图标或 PE 资源通常不会影响扫描结果。

目前已验证的基准 EXE：

```text
SHA-256
f90b13f6f4b6840c79ed338230f988b5617bab7786307e35445689a78f21471f
```

以下版本不在当前目标范围内：

- 2011 Xbox 360 重制版；
- 2025 版；
- 使用其他引擎的移植版；
- 重编译或重写了底层语音控制代码的 PC 修改版。

程序无法唯一确认目标代码时会停止，不会猜测地址并写入。

## 使用方法

1. 下载 `Ever17_VoiceContinue.exe`。
2. 把它放到游戏 EXE 所在目录。
3. 双击运行。
4. 程序扫描当前目录中的 x86 PE 文件。
5. 找到唯一符合特征的 Ever17 2002 PC EXE 后，程序检查两处目标代码。
6. 首次安装前，程序创建备份：

```text
游戏.exe.voicecontinue.bak
```

7. 写入补丁后，程序再次扫描并验证结果。
8. 验证通过后，程序启动游戏。

安装完成后，可以继续用补丁程序启动游戏，也可以直接运行已经修改过的游戏 EXE。

## 恢复原版行为

在命令行运行：

```bat
Ever17_VoiceContinue.exe /restore
```

恢复时，程序根据目标代码中保留的地址关系重建原始指令。

如果补丁之前临时禁用了音频代理 DLL，恢复过程也会尝试还原这些文件名。

## 与 AudioSpeedHack 共存

不要同时启用 ZeroInterrupt。

ZeroInterrupt 会拦截语音停止操作，并对语音做队列缓冲。它可能把“新语音出现时停止旧语音”的正常操作一起拦截，导致后续语音排队。

如果游戏目录存在以下文件：

```text
dsound.dll
MMDevAPI.dll
```

补丁会询问是否临时改名禁用。程序只改名，不删除文件。

## 安全检查

补丁写入前会检查：

- PE 是否为 x86；
- 目标代码是否位于可执行 section；
- 两条特征是否分别只命中一次；
- 两处代码是否同时处于原始状态或已打补丁状态；
- 当前目录是否存在多个符合特征的 EXE。

出现以下情况时，程序会停止：

- 只找到一处目标代码；
- 某条特征命中多次；
- 两处代码状态不一致；
- 底层代码结构与已知版本不同；
- 当前目录存在多个候选 EXE。

补丁不会修改存档、剧情脚本、`voice.dat`、BGM、SE 或语音文件。

## 自行构建

GitHub Actions 会在 Windows x86 环境中自动编译。

本地构建可使用 Visual Studio Developer Command Prompt：

```bat
cl /nologo /O2 /W4 /DUNICODE /D_UNICODE /MT src\patcher.c ^
  /Fe:Ever17_VoiceContinue.exe user32.lib kernel32.lib
```

## 兼容性反馈

如果程序无法识别你的版本，请提交 [兼容性报告](https://github.com/songyu00yo/Ever17-VoiceContinue/issues/new?template=compatibility.md)。

请提供：

- 游戏版本或汉化组；
- EXE 文件名；
- EXE 的 SHA-256；
- Windows 版本；
- 补丁程序显示的报错；
- 游戏目录中是否存在 `dsound.dll` 或 `MMDevAPI.dll`。

不要上传完整游戏、游戏 EXE、剧情文件或其他受版权保护的资源。

## 免责声明

Ever17 及相关名称、图像和游戏资源的版权归各自权利人所有。

本项目是非官方兼容性补丁，与 KID、MAGES.、Spike Chunsoft 或其他权利人无关联。

## License

源码使用 [MIT License](LICENSE)。
