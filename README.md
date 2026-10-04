# Ever17 Voice Continue

为 **2002 PC 版 `Ever17 -the out of infinity-`** 添加现代 GAL 常见的 **Voice Continue / 语音延续** 行为。

[![build](https://github.com/songyu00yo/Ever17-VoiceContinue/actions/workflows/build.yml/badge.svg)](https://github.com/songyu00yo/Ever17-VoiceContinue/actions/workflows/build.yml)
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![platform](https://img.shields.io/badge/platform-Windows%20x86-lightgrey)

> **不包含游戏本体、剧情脚本、语音或其他游戏资源。**  
> 本项目只修改用户自己持有的 Ever17 PC 可执行文件中的两处语音控制逻辑。

---

## 作用

Ever17 老 PC 引擎默认会在玩家翻到下一句文本时停止当前角色语音。

本补丁把行为改为：

| 情况 | 原版行为 | Voice Continue |
|---|---|---|
| 下一句没有新语音 | 当前语音被打断 | 当前语音继续播放 |
| 下一句有新语音 | 旧语音停止，新语音播放 | 旧语音停止，新语音播放 |
| 语速 | 原速 | 原速 |
| 多条语音排队 | 无 | 无 |
| 多条语音重叠 | 无 | 无 |

目标效果就是：

```text
翻页 + 没有新语音  -> 继续播放当前语音
翻页 + 出现新语音  -> 停止旧语音，立即播放新语音
```

不是倍速，也不是把所有 `Stop()` 全部屏蔽。

---

## 为什么不直接用 ZeroInterrupt

单纯拦截 DirectSound 的 `Stop()` 会产生副作用：

游戏在**新语音出现时**本来就需要停止旧语音。如果连这个 `Stop()` 也被拦截，后续语音就可能进入队列，表现为：

```text
语音 A 播完 -> 才开始语音 B -> 再开始语音 C
```

因此本项目不做全局音频 Hook，而是直接修改 Ever17 原引擎中：

**“文本推进触发语音淡出/停止”**

的两条代码路径，同时保留：

**“新语音替换旧语音”**

的原始逻辑。

---

## 兼容性

### 当前策略

早期测试版本使用固定文件偏移，只适用于一个具体 EXE。

当前版本改为 **PE executable section + machine-code signature scanning**：

- 不依赖 EXE 文件名；
- 不依赖固定文件大小；
- 不依赖固定 SHA-256；
- 不依赖固定文件偏移；
- 不依赖固定镜像地址；
- 对绝对地址、`CALL rel32` 等版本相关字节使用通配匹配；
- 只有两段目标代码都被**唯一且一致地识别**时才会修改。

因此，如果某个汉化版/整合版仍然基于 **2002 PC 版 KID 引擎的相同底层代码**，即使改过：

- 汉化文本；
- PE 资源；
- 图标；
- 时间戳；
- EXE 大小；
- section raw offset；
- 一部分外围代码；

也有机会直接兼容。

### 已验证

目前确认成功的基准 EXE：

```text
SHA-256:
f90b13f6f4b6840c79ed338230f988b5617bab7786307e35445689a78f21471f
```

这是一个 2002 PC 中文汉化/整合版本。

### 不保证兼容

以下版本不属于本项目的目标：

- 2011 Xbox 360 重制版；
- 2025 版；
- 完全不同的移植版；
- 对底层语音控制例程进行了重编译或重写的 PC 修改版。

> **本项目不会为了“兼容更多版本”猜地址硬写。**  
> 如果无法唯一证明目标代码位置，程序会直接拒绝修改。

如果你的版本无法识别，欢迎提交 [Compatibility report](../../issues/new?template=compatibility.md)。

---

## 下载 / 构建

### GitHub Actions

每次提交都会在 Windows x86 环境中自动编译。

进入：

**Actions → build → 最新成功任务 → Artifacts**

下载：

```text
Ever17_VoiceContinue-x86
```

### 自行编译

使用 Visual Studio Developer Command Prompt：

```bat
cl /nologo /O2 /W4 /DUNICODE /D_UNICODE /MT src\patcher.c ^
  /Fe:Ever17_VoiceContinue.exe user32.lib kernel32.lib
```

---

## 使用方法

1. 将 `Ever17_VoiceContinue.exe` 放进游戏 EXE 所在目录；
2. 双击运行；
3. 程序会扫描当前目录中的 x86 PE 文件；
4. 找到唯一符合 Ever17 2002 PC 引擎特征的 EXE 后进行校验；
5. 首次安装前自动创建备份：

```text
游戏.exe.voicecontinue.bak
```

6. 写入补丁后再次扫描验证；
7. 验证通过后启动游戏。

以后可以：

- 继续通过 `Ever17_VoiceContinue.exe` 启动；
- 或直接运行已经被修改过的游戏 EXE。

---

## 恢复原版行为

命令行运行：

```bat
Ever17_VoiceContinue.exe /restore
```

恢复逻辑不会依赖某个固定 EXE 的硬编码绝对地址，而是从仍保留的引擎代码关系中重建原始指令。

同时，如果之前由补丁临时禁用了兼容性冲突的音频代理 DLL，也会尝试恢复。

---

## 与 AudioSpeedHack 共存

### ZeroInterrupt

**不建议同时启用。**

如果当前目录存在：

```text
dsound.dll
MMDevAPI.dll
```

补丁会提示是否临时改名禁用。

原因是 ZeroInterrupt 可能拦截“新语音替换旧语音”时本来必须执行的停止操作，从而导致语音排队。

程序只会**改名**，不会删除 DLL。

---

## 安全机制

二进制 patcher 最重要的原则不是“尽量打上”，而是**无法确认就拒绝写入**。

当前实现包含：

- 只扫描 PE 中带 `IMAGE_SCN_MEM_EXECUTE` 标记的 section；
- 两条目标 signature 必须分别唯一命中；
- 两处代码必须同时处于：
  - 未打补丁状态，或
  - 已打补丁状态；
- 检测到部分修改状态时直接停止；
- 检测到多个候选 EXE 时直接停止；
- 写入前自动创建备份；
- 写入后重新扫描确认补丁状态；
- 不修改：
  - 存档；
  - 剧情脚本；
  - `voice.dat`；
  - BGM；
  - SE；
  - 语音文件本身。

---

## 技术说明

更详细的逆向与 patch 设计见：

[docs/TECHNICAL.md](docs/TECHNICAL.md)

核心思路是识别两条“文本推进导致语音淡出”的控制流路径，并跳过它们，而不是修改 DirectSound 的通用停止函数。

---

## 兼容性反馈

如果你的版本无法识别，请提交 Issue，并提供：

- 游戏版本 / 汉化组；
- EXE 文件名；
- EXE SHA-256；
- Windows 版本；
- patcher 的提示信息；
- 是否存在 `dsound.dll` / `MMDevAPI.dll`。

**不要上传完整游戏、游戏 EXE、剧情文件或其他受版权保护的资源。**

---

## Disclaimer

Ever17 及相关名称、图像和游戏资源的版权归其各自权利人所有。

本项目是非官方兼容性补丁，与 KID、MAGES.、Spike Chunsoft 或其他权利人无关联。

---

## License

本项目源码使用 [MIT License](LICENSE)。

补丁程序本身不包含 Ever17 游戏代码或资源。
