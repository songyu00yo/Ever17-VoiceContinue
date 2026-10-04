# Ever17 Voice Continue

为 2002 PC 版 `Ever17 -the out of infinity-` 添加 Voice Continue（语音延续）功能。

[![build](https://github.com/songyu00yo/Ever17-VoiceContinue/actions/workflows/build.yml/badge.svg)](https://github.com/songyu00yo/Ever17-VoiceContinue/actions/workflows/build.yml)
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![platform](https://img.shields.io/badge/platform-Windows%20x86-lightgrey)

原版在翻到下一句文本时，会停止当前角色语音。安装补丁后：

| 操作 | 结果 |
| --- | --- |
| 下一句没有新语音 | 当前语音继续播放 |
| 下一句有新语音 | 当前语音停止，新语音立即播放 |
| 语速 | 保持原速 |
| 多条语音 | 不排队，不重叠 |

项目不包含游戏本体、剧情脚本、语音或其他游戏资源。补丁只修改用户本地 EXE 中的两处语音控制逻辑。

## 原理

Ever17 的老 PC 引擎把“翻页”和“停止当前语音”绑定在一起。

如果直接拦截 DirectSound 的 `Stop()`，游戏在播放下一条语音时也无法正常停止上一条语音。结果通常是后续语音进入队列。

本补丁只跳过两条“翻页触发语音停止”的代码路径。游戏原有的“新语音替换旧语音”逻辑保持不变。

补丁后的行为如下：

```text
翻页，下一句无语音  -> 当前语音继续
翻页，下一句有语音  -> 停止当前语音，播放新语音
```

详细说明见 [docs/TECHNICAL.md](docs/TECHNICAL.md)。

## 兼容性

当前版本使用 PE 可执行节的机器码特征扫描，不依赖固定文件偏移。

扫描时不要求以下内容保持不变：

- EXE 文件名和文件大小；
- SHA-256、PE 时间戳和资源；
- section 的文件偏移；
- 代码中的绝对地址和 `CALL rel32` 操作数。

补丁只有在两段目标代码都被唯一识别，并且状态一致时才会写入。

因此，只要汉化版或整合版仍使用 2002 PC 版 KID 引擎的同一套语音控制代码，就有机会直接兼容。汉化组修改文本、图标或 PE 资源通常不会影响扫描结果。

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

从 GitHub Actions 下载编译结果，或自行编译 `src/patcher.c`。

将 `Ever17_VoiceContinue.exe` 放到游戏 EXE 所在目录，然后双击运行。

程序会执行以下步骤：

1. 扫描当前目录中的 x86 PE 文件。
2. 找到唯一符合特征的 Ever17 2002 PC EXE。
3. 检查两处目标代码的状态。
4. 首次安装前创建备份：
   ```text
   游戏.exe.voicecontinue.bak
   ```
5. 写入补丁后重新扫描验证。
6. 验证通过后启动游戏。

安装完成后，可以继续用补丁程序启动，也可以直接运行已经修改过的游戏 EXE。

## 恢复原版行为

在命令行运行：

```bat
Ever17_VoiceContinue.exe /restore
```

恢复时，程序会根据目标代码中保留的地址关系重建原始指令。

如果补丁之前临时禁用了音频代理 DLL，恢复过程也会尝试还原这些文件名。

## AudioSpeedHack

不要同时启用 ZeroInterrupt。

ZeroInterrupt 会拦截语音停止操作，并对语音做队列缓冲。这样可能把“新语音出现时停止旧语音”的正常操作一起拦截，导致后续语音排队。

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

## 下载和构建

### GitHub Actions

打开仓库的 **Actions** 页面，进入最新一次成功的 `build` 任务。

在 **Artifacts** 中下载：

```text
Ever17_VoiceContinue-x86
```

### 本地编译

使用 Visual Studio Developer Command Prompt：

```bat
cl /nologo /O2 /W4 /DUNICODE /D_UNICODE /MT src\patcher.c ^
  /Fe:Ever17_VoiceContinue.exe user32.lib kernel32.lib
```

## 兼容性反馈

如果程序无法识别你的版本，请提交 [Compatibility report](../../issues/new?template=compatibility.md)。

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
