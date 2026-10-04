# Technical notes

[简体中文](TECHNICAL.md)

## Target behavior

The original engine couples text advance with fading or stopping the current character voice. Voice Continue separates those events:

```text
advance + no new voice  -> keep current voice
advance + new voice     -> stop current voice, play new voice
```

A blanket hook on `IDirectSoundBuffer::Stop` is therefore incorrect. The game legitimately calls `Stop` when a new voice replaces the previous one.

## Two patch sites

In the analyzed 2002 PC KID engine, two code paths enter the voice fade-out logic when text or input advances.

Baseline virtual addresses:

```text
VA 0x00413E7D
VA 0x0041D43C
```

The original internal test build patched those exact locations with:

```text
E9 40 00 00 00
E9 3A 00 00 00
```

The public build does not rely on those fixed virtual addresses or file offsets.

## Signature scanning

The patcher parses the PE section table and scans only sections marked `IMAGE_SCN_MEM_EXECUTE`.

Absolute global addresses and `CALL rel32` operands are wildcarded. Fixed opcodes, constants, and surrounding control-flow structure provide specificity.

### Address relationship check

The original prefix is:

```text
A1 xx xx xx xx    ; mov eax, [voice_timing_global]
```

Later in the same block:

```text
0F AF 05 yy yy yy yy
```

The baseline engine satisfies:

```text
xx_addr == yy_addr + 0x0C
```

The scanner requires this relationship. The `/restore` path uses the same relationship to reconstruct the original five-byte instruction.

## Compatibility model

The matcher is expected to survive:

- translation, string, and PE resource changes;
- EXE size or PE timestamp changes;
- section raw-offset changes;
- code relocation with the same instruction layout;
- changed image/global absolute addresses;
- changed `CALL rel32` operands.

The patcher refuses to modify a file when:

- either signature is missing;
- either signature matches more than once;
- only one patch site is already modified;
- the low-level voice-control code has been recompiled or restructured enough that equivalence cannot be established.

A binary patcher should fail closed when it cannot prove the target.
