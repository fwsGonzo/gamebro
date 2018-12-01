#### Lessons from GB emulation

Almost all DMG and CGB games drive game progress by waiting for a V-blank interrupt which happens once per frame after all the visible scanlines have been rendered. Specifically when LY turns 144, which is the first scanline outside of the screen. At that point you can only have MODE=1, which is V-blank.

Some ROMs select bogus RAM banks, so make sure to mask off bits based on the number of RAM banks in the cartridge header.

When writing to the lower bits of MBC1 ROM bank selection range, keep in mind that if *ROM/RAM mode select* is in *RAM mode* then writing to the lower ROM bank bits should *reset the upper bits*. This is *not* the case for RAM banks.

POP AF - The lower nibble of the FLAGS register is NOT writable. Since the lower nibble is always zero, you can mask off this value with `flags &= 0xF0`.

KEY1 - Many games need the bit at 0x80 to represent double speed mode. Make sure to not clobber it and set it only after enabling double speed.

Signed operations - `JR imm8`, `ADD SP, imm8` `LD HL, SP+imm8`. These instructions operate with a *signed* 8-bit integer.

Signed tile locations - Tiles at 0x9000 are retrieved by using a *signed* offset. The signed offset gives it access from 0x8800 to 0x97FF.

During instruction decoding the CPU reads one opcode from memory, during which time a hardware tick occurs. This is the case for all memory reads during instruction execution. This means that hardware ticks can occur mid-instruction for *all* non-trivial instructions. Some games rely on this timing-accurate behavior.

An HDMA operation operates on a block of 16 bytes at a time. This operation happens once during each H-blank...?
