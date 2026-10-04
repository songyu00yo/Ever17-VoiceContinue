# Technical notes

## Target behavior

The original engine couples text advance with fading/stopping the current character voice. Voice Continue separates those two events:

```text
advance + no new voice  -> keep current voice
advance + new voice     -> stop current voice, play new voice
```

A blanket hook of `IDirectSoundBuffer::Stop` is therefore wrong: it also suppresses the legitimate stop that occurs when a new voice replaces the previous one.

## Two patch sites

In the analyzed 2002 PC KID engine, two code paths enter the voice fade-out logic when text/input advances.

Baseline locations:

```text
VA 0x00413E7D
VA 0x0041D43C
```

The original private build patched those exact locations with:

```text
E9 40 00 00 00
E9 3A 00 00 00
```

The public build does not rely on those fixed VAs or file offsets.

## Signature scanning

The patcher parses the PE section table and scans only sections marked `IMAGE_SCN_MEM_EXECUTE`.

Absolute global addresses and `CALL rel32` operands are wildcarded. Fixed opcodes, constants, and surrounding control-flow structure provide specificity.

### Safety relation

The original prefix is:

```text
A1 xx xx xx xx    ; mov eax, [voice_timing_global]
```

Later in the same block:

```text
0F AF 05 yy yy yy yy
```

For the baseline engine:

```text
xx_addr == yy_addr + 0x0C
```

The scanner requires this relationship. The same relationship is used to reconstruct the original five bytes during `/restore`.

## Compatibility model

Expected to survive:

- translation/resource/string changes;
- EXE size or PE timestamp changes;
- section raw-offset changes;
- moved code with unchanged instruction layout;
- changed image/global absolute addresses;
- changed `CALL rel32` operands.

The patcher intentionally refuses to modify a file when:

- either signature is missing;
- either signature matches more than once;
- only one patch site is already modified;
- the low-level voice-control routine has been recompiled/restructured enough that equivalence cannot be proven.

Binary patching should fail closed, not guess.
