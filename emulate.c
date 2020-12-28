/* snesrc - SNES Recompiler
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005, 2009 Jason Oster
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 To do:
  Emulate A,X,Y registers
  Emulate RAM read/write
  Map data accesses
  Stack tracing should solve some problems.
  Trace register sizing properly! (how?)
    (example: a subroutine changes the size, and does not 'restore' it)
    possible solution: attempt to correct register size, and
    re-analyze dirty instructions.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "emulate.h"
#include "dasm.h"


int GetOpSize(CONTEXT *context, u32 pc, u16 p) {
    if (!(p & 0x0100)) return opsize[(p >> 4) & 0x03][snesGetByte(context->buffer, pc)];
    return opsize[3][snesGetByte(context->buffer, pc)];
}

int emulate(CONTEXT *context, u8 *ramcopy) {
    u32 addr, pc, ea;
    u16 tmp, p;
    u8 s = 1, stack = 0xFF;
    u8 inst, *ram;
    int loop = 1, ret = 0;
    int range[2], lastrs = 0;
    int i;

    if (context->sp) {
        s = (context->sp >> 8);
        stack = context->sp;
        context->sp = 0;
    }
    if (!(ram = (u8*)malloc(RAMSIZE))) {
        // FIXME
        printf("FATAL ERROR!\n");
        printf("Error allocating %d bytes of memory\n", RAMSIZE);
        printf("snesrc will now die disgracefully!\n\n");
        exit(1);
    }
    if (ramcopy) memcpy(ram, ramcopy, RAMSIZE);
    else memset(ram, 0, RAMSIZE);

    pc = context->pc;
    p = context->p;

    while (loop) {
        // Verify pc is within ROM space
        addr = snesROM2File(pc);
        if (addr == SNES_ERROR) {
            error++;
#ifdef DEBUG_BUILD
            fprintf(context->fdbg, "ERROR:\n SNES ROM address, pc: 0x%08X\n", pc);
            fprintf(context->fdbg, "  deleted %d dirty bytes\n\n", DeleteDirtyBytes(context));
#else // DEDUG_BUILD
            DeleteDirtyBytes(context);
#endif // DEDUG_BUILD
            ret = 1;
            break;
        }

        // Verify instruction does not overflow into another bank
        tmp = (pc + GetOpSize(context, pc, p));
        if (tmp < (pc & 0x0000FFFF)) {
            error++;
#ifdef DEBUG_BUILD
            fprintf(context->fdbg, "ERROR:\n CPU instruction overflow, pc: 0x%08X\n", pc);
            fprintf(context->fdbg, "  deleted %d dirty bytes\n\n", DeleteDirtyBytes(context));
#else // DEDUG_BUILD
            DeleteDirtyBytes(context);
#endif // DEDUG_BUILD
            ret = 1;
            break;
        }

        // Check for pre-accessed code
        if (snesGetByte(context->map, pc) & MAPMASK_ACCESS) {
            CleanDirtyBits(context);
            break;
        }

#ifdef DEBUG_BUILD
        // Disassemble it
        fprintf(context->fdbg, "$%02X:%04X  %s\n", (pc >> 16), (pc & 0xFFFF), BinToASM(pc, p, &context->buffer[addr]));
#endif // DEDUG_BUILD


        // Get instruction
        inst = snesGetByte(context->buffer, pc);

        // Add to Map
        tmp = (snesGetByte(context->map, pc) | MAP_ISCODE | MAP_DIRTY | ((p << 2) & 0xC0));
        for (i = 0; i < GetOpSize(context, pc, p); i++) {
            // Executed as code
            snesSetByte(context->map, (pc + i), tmp);

            // Force 8-bit regs in emulation mode
            if (p & 0x0100) snesSetByte(context->map, (pc + i), (tmp | MAP_CODEA8 | MAP_CODEX8));
        }

        // Get register signature
        if (regsig[inst]) lastrs = regsig[inst];

        // General instruction emulation
        switch (inst) {

        // Push Instructions
            case 0x08: // PHP
                PUSHB(p)
                break;
            case 0x0B: // PHD
                // FIXME
                PUSHW(0)
                break;
            case 0x4B: // PHK
                PUSHB(pc >> 16)
                break;
            case 0x8B: // PHB
                // FIXME
                PUSHB(0)
                break;

            case 0x48: // PHA
                // FIXME
                if (a16) PUSHW(0)
                else PUSHB(0)
                break;
            case 0x5A: // PHY
                // FIXME
                if (x16) PUSHW(0)
                else PUSHB(0)
                break;
            case 0xDA: // PHX
                // FIXME
                if (x16) PUSHW(0)
                else PUSHB(0)
                break;

            case 0x62: // PER
                tmp = (pc + 3);
                tmp += snesGetWord(context->buffer, (pc + 1));
                PUSHW(tmp)
                break;
            case 0xD4: // PEI
                PUSHB(ram[snesGetByte(context->buffer, (pc + 1)) + 1])
                PUSHB(ram[snesGetByte(context->buffer, (pc + 1))])
                warning++;
#ifdef DEBUG_BUILD
                fprintf(context->fdbg, "WARNING:\n PEI instruction reached\n\n");
#endif // DEDUG_BUILD
                break;
            case 0xF4: // PEA
                PUSHW(snesGetWord(context->buffer, (pc + 1)));
                break;


        // Pull Instructions
            case 0x28: // PLP
                tmp = (p & 0x0100);
                PULLB(p)
                p |= tmp;
                break;
            case 0x2B: // PLD
                // FIXME
                PULLW(tmp)
                break;
            case 0xAB: // PLB
                // FIXME
                PULLB(tmp)
                break;

            case 0x68: // PLA
                // FIXME
                if (a16) PULLW(tmp)
                else PULLB(tmp)
                break;
            case 0x7A: // PLY
                // FIXME
                if (x16) PULLW(tmp)
                else PULLB(tmp)
                break;
            case 0xFA: // PLX
                // FIXME
                if (x16) PULLW(tmp)
                else PULLB(tmp)
                break;


        // Processor Status Instructions
            case 0x18: // CLC
                p &= ~0x01;
                break;
            case 0x38: // SEC
                p |= 0x01;
                break;
            case 0x58: // CLI
                p &= ~0x04;
                break;
            case 0x78: // SEI
                p |= 0x04;
                break;
            case 0xB8: // CLV
                p &= ~0x40;
                break;
            case 0xD8: // CLD
                p &= ~0x08;
                break;
            case 0xF8: // SED
                p |= 0x08;
                break;
            case 0xFB: // XCE
                p = ((p & 0x00FE) | ((p & 0x01) << 8) | ((p >> 8) & 0x01));
                break;
            case 0xC2: // REP
                p &= ~snesGetByte(context->buffer, (pc + 1)); // This should not clear the emulation bit
                break;
            case 0xE2: // SEP
                p |= snesGetByte(context->buffer, (pc + 1));
                break;


        // Soubroutine Return Instructions
            case 0x60: // RTS
                PULLW(tmp)
                pc = ((pc & 0x00FF0000) | (tmp + 1));
                break;
            case 0x6B: // RTL
                PULLP(pc)
                pc = ((pc & 0x00FF0000) | ((pc + 1) & 0x0000FFFF));
                break;
        }


        // pc-changing instructions (flow-control)
        switch (inst) {
            case 0x00: // BRK
                if (context->flags & FLAG_FIXBRK) {
                    if (snesGetByte(context->map, pc) & MAP_TRYFIX) {
                        error++;
#ifdef DEBUG_BUILD
                        fprintf(context->fdbg, "ERROR:\n BRK reached, attempt to fix dirty instructions failed!\n");
                        fprintf(context->fdbg, "  Deleting %d dirty bytes\n\n", DeleteDirtyBytes(context));
#else // DEBUG_BUILD
                        DeleteDirtyBytes(context);
#endif // DEBUG_BUILD
                        loop = 0;
                        ret = 1;
                        break;
                    }
                    warning++;
#ifdef DEBUG_BUILD
                    fprintf(context->fdbg, "WARNING:\n BRK reached, attempting to fix dirty instructions...\n");
                    fprintf(context->fdbg, "  Last Register Signature: %d\n", lastrs);
                    fprintf(context->fdbg, "  Processor Status changed.\n  old = %04X\n", p);
#endif // DEBUG_BUILD
                    p ^= (lastrs << 4);
                    ea = pc;
                    pc = snesFile2ROM(FindFirstDirty(context));
#ifdef DEBUG_BUILD
                    fprintf(context->fdbg, "  new: %04X\n", p);
                    fprintf(context->fdbg, "  PC changed: %06X\n", pc);
                    fprintf(context->fdbg, "  Deleting %d dirty bytes\n\n", DeleteDirtyBytes(context));
#else // DEBUG_BUILD
                    DeleteDirtyBytes(context);
#endif // DEBUG_BUILD
                    // Set MAP_TRYFIX after deleting dirty bytes
                    snesSetByte(context->map, ea, (snesGetByte(context->map, ea) | MAP_TRYFIX));
                }
                else {
                    UpdateContext(context, snesGetWord(context->buffer, snesGetVectorAddr(VCT_BRK, (p >> 8))), p, ((s << 8) | stack));
                    emulate(context, ram);
                    tmp = (pc + 2);
                    pc = ((pc & 0x00FF0000) | tmp);
                }
                break;

            case 0x02: // COP
                if (context->flags & FLAG_FIXCOP) {
                    //
                }
                else {
                    UpdateContext(context, snesGetWord(context->buffer, snesGetVectorAddr(VCT_COP, (p >> 8))), p, ((s << 8) | stack));
                    emulate(context, ram);
                    tmp = (pc + 2);
                    pc = ((pc & 0x00FF0000) | tmp);
                }
                break;

            case 0x10: case 0x30: case 0x50: case 0x70: // Branches
            case 0x90: case 0xB0: case 0xD0: case 0xF0:
                tmp = (pc + 2);
                if (snesGetByte(context->buffer, (pc + 1)) & 0x80) tmp -= ((snesGetByte(context->buffer, (pc + 1)) - 1) ^ 0xFF);
                else tmp += snesGetByte(context->buffer, (pc + 1));
                CleanDirtyBits(context);
                UpdateContext(context, ((pc & 0x00FF0000) | tmp), p, ((s << 8) | stack));
                emulate(context, ram);
                tmp = (pc + 2);
                pc = ((pc & 0x00FF0000) | tmp);
                break;
            case 0x80: // BRA
                tmp = (pc + 2);
                if (snesGetByte(context->buffer, (pc + 1)) & 0x80) tmp -= ((snesGetByte(context->buffer, (pc + 1)) - 1) ^ 0xFF);
                else tmp += snesGetByte(context->buffer, (pc + 1));
                pc = ((pc & 0x00FF0000) | tmp);
                break;
            case 0x82: // BRL
                tmp = (pc + 3);
                if (snesGetWord(context->buffer, (pc + 1)) & 0x8000) tmp -= ((snesGetWord(context->buffer, (pc + 1)) - 1) ^ 0xFFFF);
                else tmp += snesGetWord(context->buffer, (pc + 1));
                pc = ((pc & 0x00FF0000) | tmp);
                break;

            case 0x20: // JSR
                CleanDirtyBits(context);
                tmp = (pc + 2);
                PUSHW(tmp)
                tmp = snesGetWord(context->buffer, (pc + 1));
                UpdateContext(context, ((pc & 0x00FF0000) | tmp), p, ((s << 8) | stack));
                emulate(context, ram);
                PULLW(tmp)
                pc = ((pc & 0x00FF0000) | (tmp + 1));
                break;

            case 0x22: // JSL
                CleanDirtyBits(context);
                tmp = (pc + 3);
                pc = ((pc & 0x00FF0000) | tmp);
                PUSHP(pc)
                UpdateContext(context, snesGetPointer(context->buffer, (pc - 2)), p, ((s << 8) | stack));
                emulate(context, ram);
                PULLP(pc)
                pc = ((pc & 0x00FF0000) | ((pc & 0x0000FFFF) + 1));
                break;

            case 0x40: // RTI
            case 0xDB: // STP
                CleanDirtyBits(context);
                loop = 0;
                break;

            case 0x4C: // JMP
                pc = (snesGetWord(context->buffer, (pc + 1)) | (pc & 0x00FF0000));
                break;

            case 0x5C: // JML
                pc = snesGetPointer(context->buffer, (pc + 1));
                break;

            case 0x60: // RTS
            case 0x6B: // RTL
                // These are here only so the default case does not increment our pc
                break;

            case 0x6C: // JMP (indirect)
            case 0xDC: // JMP [indirect long]
                warning++;
#ifdef DEBUG_BUILD
                fprintf(context->fdbg, "WARNING:\n Indirect jump reached\n\n");
#endif // DEDUG_BUILD
                ret = 1;
                loop = 0;
                break;

            case 0x7C: // JMP (indirect,indexed)
                CleanDirtyBits(context);

                tmp = snesGetWord(context->buffer, (pc + 1));
                range[0] = 0;
                while ((ea = snesGetWord(context->buffer, ((pc & 0x00FF0000) | tmp))) != SNES_ERROR) {
                    range[1] = (ea >> 8);
                    ea = ((pc & 0x00FF0000) | ea);
                    addr = ((pc & 0x00FF0000) | tmp);
                    if ((snesROM2File(ea) == SNES_ERROR) || (snesGetByte(context->map, addr) & MAPMASK_ACCESS)) break;

                    // pointer table range check
                    if ((range[0]) && (((range[0] - range[1]) < -context->range) || ((range[0] - range[1]) > context->range))) break;
                    range[0] = range[1];

                    snesSetByte(context->map, addr, (snesGetByte(context->map, addr) | (MAP_ISDATA | MAP_DATAWORD)));

                    UpdateContext(context, ea, p, ((s << 8) | stack));
                    if (emulate(context, ram)) snesSetByte(context->map, addr, 0);
                    tmp += 2;
                }

                loop = 0;
                break;

            case 0xFC: // JSR (indirect,indexed)
                CleanDirtyBits(context);

                tmp = (pc + 2);
                PUSHW(tmp)

                tmp = snesGetWord(context->buffer, (pc + 1));
                range[0] = 0;
                while ((ea = snesGetWord(context->buffer, ((pc & 0x00FF0000) | tmp))) != SNES_ERROR) {
                    range[1] = (ea >> 8);
                    ea = ((pc & 0x00FF0000) | ea);
                    addr = ((pc & 0x00FF0000) | tmp);
                    if ((snesROM2File(ea) == SNES_ERROR) || (snesGetByte(context->map, addr) & MAPMASK_ACCESS)) break;

                    // pointer table range check
                    if ((range[0]) && (((range[0] - range[1]) < -context->range) || ((range[0] - range[1]) > context->range))) break;
                    range[0] = range[1];

                    snesSetByte(context->map, addr, (snesGetByte(context->map, addr) | (MAP_ISDATA | MAP_DATAWORD)));

                    UpdateContext(context, ea, p, ((s << 8) | stack));
                    if (emulate(context, ram)) snesSetByte(context->map, addr, 0);
                    tmp += 2;
                }

                PULLW(tmp)
                pc = ((pc & 0x00FF0000) | (tmp + 1));
                break;

            default:
                // advance to next instruction
                tmp = (pc + GetOpSize(context, pc, p));
                pc = ((pc & 0x00FF0000) | tmp);
                break;
        }
    }

    free(ram);

    return ret;
}
