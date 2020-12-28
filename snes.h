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

#ifndef __SNES_H__
#define __SNES_H__

#include "_types.h"


// SNES ROM Header definitions
//  use with snesGetHeaderAddr()
#define HDR_TITLE           0xFFC0 /* ROM Header, Game Title            - 21 bytes */
#define HDR_MAKEUP          0xFFD5 /* ROM Header, ROM Makeup            - 1 byte */
#define HDR_ROMTYPE         0xFFD6 /* ROM Header, ROM Type              - 1 byte */
#define HDR_SIZE            0xFFD7 /* ROM Header, ROM Size              - 1 byte */
#define HDR_SRAM            0xFFD8 /* ROM Header, SRAM Size             - 1 byte */
#define HDR_COUNTRY         0xFFD9 /* ROM Header, Country Code          - 1 byte */
#define HDR_LICENSE         0xFFDA /* ROM Header, License Code          - 1 byte */
#define HDR_VERSION         0xFFDB /* ROM Header, Game Version          - 1 byte */
#define HDR_COMPSUM         0xFFDC /* ROM Header, Checksum Complement   - 1 word */
#define HDR_CHECKSUM        0xFFDE /* ROM Header, Checksum              - 1 word */

// SNES ROM Vector definitions
//  use with snesGetVectorAddr()
#define VCT_COP             0xFFE4 /* ROM Header, Coprocessor IRQ Vector */
#define VCT_BRK             0xFFE6 /* ROM Header, 65816 BRK Vector */
#define VCT_ABORT           0xFFE8 /* ROM Header, 65816 Abort Vector */
#define VCT_NMI             0xFFEA /* ROM Header, NMI Vector */
#define VCT_RESET           0xFFEC /* ROM Header, Reset Vector */
#define VCT_IRQ             0xFFEE /* ROM Header, IRQ Vector */

// SNES Emulation definitions
#define SNESMODE_NATIVE     0
#define SNESMODE_EMU        1

// SNES ROM Mapping definitions
#define MAP_LOROM           0
#define MAP_HIROM           1

// function return definitions
#define SNES_SIZEERR        -3 /* SNES ROM size error */
#define SNES_HEADERR        -2 /* SNES ROM header size error */
#define SNES_ERROR          -1 /* Standard/general error */
#define SNES_SUCCESS        0  /* No error */


// Pointer abstraction routines
inline u32 snesFile2ROM(u32 addr);
inline u32 snesROM2File(u32 addr);

// Data read abstraction routines
inline u32 snesGetLong(u8 *buffer, u32 addr);
inline u32 snesGetPointer(u8 *buffer, u32 addr);
inline u16 snesGetWord(u8 *buffer, u32 addr);
inline u8 snesGetByte(u8 *buffer, u32 addr);

// Data write abstraction routines
inline void snesSetLong(u8 *buffer, u32 addr, u32 data);
inline void snesSetPointer(u8 *buffer, u32 addr, u32 data);
inline void snesSetWord(u8 *buffer, u32 addr, u16 data);
inline void snesSetByte(u8 *buffer, u32 addr, u8 data);

// Miscellaneous SNES abstraction routines
inline int snesGetBankAddr(u8 bank);
inline int snesGetBankSize();
inline int snesGetMaxBanks();
inline u32 snesGetVectorAddr(u32 addr, u8 mode);
inline u32 snesGetHeaderAddr(u32 addr);

inline void snesGenChecksum(u8 *buffer, int size);
inline int snesGetMappingType(u8 *buffer);
inline void snesSetMappingType(int type);
inline int snesInit(u8 *buffer, int size);

#endif // __SNES_H__
