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

#ifndef __SNESRC_H__
#define __SNESRC_H__

#include "_types.h"


#define DEBUG_BUILD
#define VERSION "v0.01"

/*
Description of "MAP" data generated on first pass:
 Each byte references the same addresses byte in the ROM file.
 Each byte is a bitflag:
   bit-0,1: 0 = no access, 1 = executed as code, 2 = read as data, 3 = code & data (give warning)
   bit-2,3: 0 = no label, 1 = pc-relative label, 2 = local label, 3 = global label
   bit-4:   (first pass): 0 = normal, 1 = fix was attempted  (other passes): RESERVED
   bit-5:   (first pass): 0 = clean, 1 = dirty  (other passes): RESERVED

  with bit-0,1 == 1:
   bit-6:   0 = 16-bit Index Regs,  1 = 8-bit Index Regs
   bit-7:   0 = 16-bit Accumulator, 1 = 8-bit Accumulator

  with bit-0,1 == 2:
   bit-6,7: data accessed as: 0 = byte, 1 = word, 2 = pointer, 3 = long

  with bit-0,1 == 0 or 3:
   bit-6:   RESERVED
   bit-7:   RESERVED

 First pass runs an emulator logging [how all bytes are accessed] to the MAP data
   (Minus the 'extra' bytes in multi-byte instructions)
 Second pass uses the MAP data to disassemble the ROM, seperating code from data
   (After a MAP byte has been used, it will be cleared to zero)
 Third pass will quickly run through the entire MAP data and report any flags that are set.
   (Should be helpful for debugging!)
 Fourth pass creates a makefile

 What it does:
  General:
   Need to do something about arrays(!) and relocated assembly(!) / function pointers(!)
   Need to do something about graphics(!) and music(!)
  First pass:
      Dirty bit (bit-5) specifies newly-accessed data.
       When an error occurs, the dirty bits can be checked to erase all code within the erroneous
       frame. Clear all dirty bits when starting a new frame (recursive action) to indicate that all
       logged info is without error.
  Second pass:
   Create global defines before disassembling. (Create .inc [header] files? Would be much nicer!)
   Detect ASCII strings.
   Detect free space at the end of each bank.
   Properly set ROM header at the end of bank-0.
*/

extern int warning;
extern int error;

extern const u8 regsig[256];
extern const u8 opsize[4][256];


// Register signature defines
#define RS_X    1
#define RS_A    2


// Map defines
#define MAP_ISCODE          0x01 /* Byte is marked as code */
#define MAP_ISDATA          0x02 /* Byte is marked as data */
#define MAP_ISCODEDATA      0x03 /* Byte is marked as code or data */
#define MAPMASK_ACCESS      0x03 /* Code/data mask */

#define MAP_PCLABEL         0x04 /* Byte is marked as pc-relatively labeled */
#define MAP_LABEL           0x08 /* Byte is marked as locally labeled */
#define MAP_GLABEL          0x0C /* Byte is marked as globally labeled */
#define MAPMASK_LABEL       0x0C /* Label mask */

#define MAP_TRYFIX          0x10 /* Instruction size fix was attempted */
#define MAP_DIRTY           0x20 /* Byte is marked as dirty */

#define MAP_CODEX8          0x40 /* Code was accessed with 8-bit Index Registers */
#define MAP_CODEA8          0x80 /* Code was accessed with 8-bit Accumulator */
#define MAP_DATAWORD        0x40 /* Data was accessed as word */
#define MAP_DATAPTR         0x80 /* Data was accessed as pointer */
#define MAP_DATALONG        0xC0 /* Data was accessed as long */
#define MAP_EXTENDED        0xC0 /* Extended information mask */


// Context defines
#define FLAG_NONE           0x00000000
#define FLAG_FIXBRK         0x00000001
#define FLAG_FIXCOP         0x00000002
#define FLAG_FIXSTP         0x00000004
#define FLAG_FIXWDM         0x00000008


// The context structure is used to pseudo-emulate the 6502
typedef struct {
#ifdef DEBUG_BUILD
    FILE *fdbg;
#endif // DEDUG_BUILD
    u32 size;
    int type;
    int range;
    u32 flags;
    u8 *buffer;
    u8 *map;
    u32 pc;
    u16 p;
    u16 sp;
} CONTEXT;


// Context handling
int DeleteDirtyBytes(CONTEXT *context);
int CleanDirtyBits(CONTEXT *context);
int FindFirstDirty(CONTEXT *context);
void UpdateContext(CONTEXT *context, u32 pc, u16 p, u16 sp);
int CreateMap(CONTEXT *context);
int FlushMap(CONTEXT *context);
void CheckMap(CONTEXT *context);

// Map handling
int GetMapSequence(CONTEXT *context, int where);

// Miscellaneous
void FreeContext(CONTEXT *context);
void PrintUsage(char *str);

#endif // __SNESRC_H__
