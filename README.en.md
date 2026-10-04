# Ever17 Voice Continue

[简体中文](README.md)

Voice Continue patch for the 2002 PC release of `Ever17 -the out of infinity-`.

[![build](https://github.com/songyu00yo/Ever17-VoiceContinue/actions/workflows/build.yml/badge.svg)](https://github.com/songyu00yo/Ever17-VoiceContinue/actions/workflows/build.yml)
[![release](https://img.shields.io/github/v/release/songyu00yo/Ever17-VoiceContinue)](https://github.com/songyu00yo/Ever17-VoiceContinue/releases/latest)
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![platform](https://img.shields.io/badge/platform-Windows%20x86-lightgrey)

## Download

Download the stable build from Releases:

**[Download Ever17_VoiceContinue.exe](https://github.com/songyu00yo/Ever17-VoiceContinue/releases/download/v1.0.0/Ever17_VoiceContinue.exe)**

The release also includes a SHA-256 checksum file.

## Behavior

The original PC engine stops the current character voice when the player advances the text. This patch changes that behavior:

| Action | Result |
| --- | --- |
| Advance to a line with no new voice | Current voice keeps playing |
| Advance to a line with a new voice | Current voice stops and the new voice starts immediately |
| Playback speed | Unchanged |
| Multiple voice clips | No queueing and no overlap |

The project does not include the game, scenario files, voice data, or other game assets. It changes two voice-control paths in the user's local executable.

## How it works

The old Ever17 PC engine couples text advance with stopping or fading the current voice.

A global hook on DirectSound `Stop()` is not sufficient. The game also needs that call when a new voice replaces the previous one. Blocking it causes later voice clips to queue.

This patch skips only the two code paths that stop voice because text advanced. The original “new voice replaces old voice” path remains intact.

```text
advance + no new voice  -> keep current voice
advance + new voice     -> stop current voice, play new voice
```

See [docs/TECHNICAL.en.md](docs/TECHNICAL.en.md) for implementation details.

## Compatibility

The current build scans machine-code signatures inside executable PE sections. It does not depend on fixed file offsets.

The scanner can tolerate changes to:

- executable filename and file size;
- SHA-256, PE timestamp, and resources;
- section raw offsets;
- absolute addresses in the matched code;
- `CALL rel32` operands.

The patcher writes only when both target code blocks are found exactly once and have a consistent state.

Translation patches and repacks based on the same 2002 PC KID engine code may therefore work even when they change text, icons, or PE resources.

Verified baseline executable:

```text
SHA-256
f90b13f6f4b6840c79ed338230f988b5617bab7786307e35445689a78f21471f
```

The following releases are outside the current target:

- the 2011 Xbox 360 remake;
- the 2025 release;
- ports using another engine;
- PC modifications that recompile or rewrite the low-level voice-control routines.

If the patcher cannot identify the target code unambiguously, it stops without writing anything.

## Usage

1. Download `Ever17_VoiceContinue.exe`.
2. Place it next to the game executable.
3. Run it.
4. The patcher scans x86 PE files in the current directory.
5. It selects the only executable that matches the Ever17 2002 PC engine signatures.
6. Before the first modification, it creates:

```text
game.exe.voicecontinue.bak
```

7. It writes the patch and scans the executable again.
8. If verification succeeds, it launches the game.

After installation, you can keep using the patcher as a launcher or start the patched game executable directly.

## Restore

Run:

```bat
Ever17_VoiceContinue.exe /restore
```

The restore path reconstructs the original instruction from address relationships that remain in the matched engine code.

If the patcher previously renamed conflicting audio proxy DLLs, the restore command also attempts to restore their filenames.

## AudioSpeedHack compatibility

Do not use ZeroInterrupt at the same time.

ZeroInterrupt intercepts voice-stop operations and buffers voice clips in a queue. It can also intercept the legitimate stop used when a new voice replaces the previous one. The result is delayed, queued dialogue.

If the game directory contains:

```text
dsound.dll
MMDevAPI.dll
```

the patcher asks whether it should temporarily rename them. It does not delete the files.

## Safety checks

Before writing, the patcher verifies:

- the PE target is x86;
- matched code is inside executable sections;
- each signature has exactly one match;
- both patch sites are either original or already patched;
- only one candidate executable exists in the directory.

It stops when:

- only one target block is found;
- a signature matches more than once;
- the two sites have inconsistent states;
- the low-level code differs from the known structure;
- more than one candidate executable exists.

The patcher does not modify saves, scenario files, `voice.dat`, BGM, sound effects, or voice files.

## Build

GitHub Actions builds the project in a Windows x86 environment.

To build locally, use a Visual Studio Developer Command Prompt:

```bat
cl /nologo /O2 /W4 /DUNICODE /D_UNICODE /MT src\patcher.c ^
  /Fe:Ever17_VoiceContinue.exe user32.lib kernel32.lib
```

## Compatibility reports

If your executable is not recognized, open a [compatibility report](https://github.com/songyu00yo/Ever17-VoiceContinue/issues/new?template=compatibility-en.md).

Include:

- game release or translation patch;
- executable filename;
- executable SHA-256;
- Windows version;
- the patcher's error message;
- whether `dsound.dll` or `MMDevAPI.dll` is present.

Do not upload the full game, the game executable, scenario files, or other copyrighted game assets.

## Disclaimer

Ever17 and related names, images, and game assets belong to their respective rights holders.

This is an unofficial compatibility patch. It is not affiliated with KID, MAGES., Spike Chunsoft, or other rights holders.

## License

Source code is released under the [MIT License](LICENSE).
