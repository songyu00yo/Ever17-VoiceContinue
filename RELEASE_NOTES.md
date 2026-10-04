# v1.0.0

## 中文

首个公开版本。

- 为 2002 PC 版 Ever17 添加 Voice Continue（语音延续）。
- 翻到没有新语音的文本时，当前语音继续播放。
- 出现新语音时，旧语音立即停止，新语音立即播放。
- 保持原速，不排队，不重叠。
- 使用 PE 可执行节机器码特征扫描，不依赖固定 EXE 偏移。
- 写入前自动备份，并支持 `/restore`。
- 遇到未知或无法唯一识别的代码布局时拒绝修改。

下载 `Ever17_VoiceContinue.exe`，放到游戏 EXE 同目录后运行。

## English

First public release.

- Adds Voice Continue to the 2002 PC release of Ever17.
- The current voice keeps playing when the next line has no new voice.
- A new voice immediately replaces the previous voice.
- Original playback speed, with no queueing or overlap.
- Uses PE executable-section machine-code signatures instead of fixed EXE offsets.
- Creates a backup before writing and supports `/restore`.
- Refuses to patch unknown or ambiguous code layouts.

Download `Ever17_VoiceContinue.exe`, place it next to the game executable, and run it.
