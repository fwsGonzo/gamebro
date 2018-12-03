#### Lessons from GB emulation

Almost all DMG and CGB games drive game progress by waiting for a V-blank interrupt which happens once per frame after all the visible scanlines have been rendered. Specifically when LY turns 144, which is the first scanline outside of the screen. At that point you can only have MODE=1, which is V-blank.

Some ROMs select bogus RAM banks, so make sure to mask off bits based on the number of RAM banks in the cartridge header.

When writing to the lower bits of MBC1 ROM bank selection range, keep in mind that if *ROM/RAM mode select* is in *RAM mode* then writing to the lower ROM bank bits should *reset the upper bits*. This is *not* the case for RAM banks.

When RAM bank mode is selected in MBC1, when writing the upper 2 bits, you also must set the ROM bank as well. In short both get modified. If RAM bank mode is not selected, ROM bank is modified as expected.

When selecting/deselecting RTC (bit 7) in MBC3, the RAM bank bits are still in effect.

POP AF - The lower nibble of the FLAGS register is NOT writable. Since the lower nibble is always zero, you can mask off this value with `flags &= 0xF0`.

KEY1 - Many games need the bit at 0x80 to represent double speed mode. Make sure to not clobber it and set it only after enabling double speed.

Signed operations - `JR imm8`, `ADD SP, imm8` `LD HL, SP+imm8`. These instructions operate with a *signed* 8-bit integer.

Signed tile locations - Tiles at 0x9000 are retrieved by using a *signed* offset. The signed offset gives it access from 0x8800 to 0x97FF.

During instruction decoding the CPU reads one opcode from memory, during which time a hardware tick occurs. This is the case for all memory reads during instruction execution. This means that hardware ticks can occur mid-instruction for *all* non-trivial instructions. Some games rely on this timing-accurate behavior.

An HDMA operation operates on a block of 16 bytes at a time. This operation happens once during each H-blank...?

HDMA source and destination registers should always return 0xFF when read.

Interrupts are only executed for IE & IF (anded together). When executing an interrupt the CPU disables IME with no delay. Interrupts have priorities, with V-blank being the highest and so must be run first. The next interrupt can only run after IME is on again after IE instruction + a delay tick.

If you are creating a timing-accurate emulator you have to keep in mind that the CB opcode is not an instruction. It is an atomic read that LEADS to the actual instructions. If you execute a hardware tick between those bytes it could lead to a very subtle bug. For example an interrupt could end up executing and then returning into the middle of the two-byte opcode, or pass it altogether without executing it.

Double- and triple-check your helper functions. I had my is_hblank() function return true when LCDC mode == 3, which is not true. H-blank is mode 0.

VRAM attributes are in the same area tiles would be in, except VRAM bank is 1. That means the attributes are not in a fixed place. Use the same offset as for tiles.

And last, but not least: Don't despair if not every game works flawlessly. There are many emulators, and most only have support up to a point. Simply getting to the point where a GBC game runs the intro and you can safely get into the game is a long and arduous process. Give yourself some credit!
