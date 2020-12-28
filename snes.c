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


// SNES Backend
//  This is mostly an abstraction layer for accessing SNES ROM images.
//  It contains plenty of error checking, so it's not written for speed.
//  The biggest advantage is the pointer handling, which enables you to
//  access synchronous data no matter which ROM mapping type is used.
//  For example, reading the 6502 reset vector can be done with only two
//  calls, rather than loading seperate pointers for each mapping type:
//   EG:  snesGetWord(buffer, snesGetHeaderAddr(VCT_RESET, SNESMODE_EMU));

#include <stdio.h>
#include <string.h>

#include "snes.h"


int header = 0;
int romtype = -1;
int romsize = 0;


inline u32 snesLoROM2File(u32 addr);
inline u32 snesFile2LoROM(u32 addr);

inline u32 snesHiROM2File(u32 addr);
inline u32 snesFile2HiROM(u32 addr);



inline u32 snesLoROM2File(u32 addr) {
    u8 bank;

    // Check for invalid LoROM address
    if (!(addr & 0x8000)) return SNES_ERROR;

    // Extract bank and address
    bank = (addr >> 16);
    addr &= 0x00007FFF;

    // Do conversion
    if ((romsize > 0x400000) && (bank <= 0x7D)) // ExLoROM
        return (((bank << 15) | addr) + 0x400000 + header);
    else if ((bank >= 0x70) && (bank <= 0x7F)) return SNES_ERROR;
    return ((((bank & 0x7F) << 15) | addr) + header);
}

inline u32 snesFile2LoROM(u32 addr) {
    addr -= header;

    if ((romsize > 0x400000) && (addr >= 0x400000)) // ExLoROM
        return ((((addr - 0x400000) << 1) & 0x7F0000) | (addr & 0x7FFF) | 0x8000);
    return (((addr << 1) & 0x7F0000) | (addr & 0x7FFF) | 0x808000);
}



inline u32 snesHiROM2File(u32 addr) {
    u8 bank;

    // Extract bank and address
    bank = (addr >> 16);
    addr &= 0x003FFFFF;

    // Check for invalid HiROM address
    if ((bank <= 0x77) && (!(addr & 0x8000))) return SNES_ERROR;
    if ((bank >= 0x7E) && (bank <= 0x7F)) return SNES_ERROR;

    // Do conversion
    if (romsize > 0x400000) { // ExHiROM
        if (bank >= 0xC0) return (addr + header);
        return (addr + 0x400000 + header);
    }
    return (addr + header);
}

inline u32 snesFile2HiROM(u32 addr) {
    addr -= header;

    if ((romsize > 0x400000) && (addr >= 0x400000)) {
        if ((addr >= 0x700000) && (addr <= 0x780000) && (!(addr & 0x8000))) return SNES_ERROR;
        return addr;
    }
    return (addr | 0xC00000);
}



inline u32 snesFile2ROM(u32 addr) {
    if (addr >= (romsize + header)) return SNES_ERROR;

    switch (romtype) {
        case MAP_LOROM: return snesFile2LoROM(addr);
        case MAP_HIROM: return snesFile2HiROM(addr);
    }

    return SNES_ERROR;
}

inline u32 snesROM2File(u32 addr) {
    switch (romtype) {
        case MAP_LOROM: addr = snesLoROM2File(addr); break;
        case MAP_HIROM: addr = snesHiROM2File(addr); break;
        default: return SNES_ERROR;
    }
    if (addr >= (romsize + header)) return SNES_ERROR;

    return addr;
}



inline u32 snesGetLong(u8 *buffer, u32 addr) {
    if (snesROM2File(addr + 1) == SNES_ERROR) return SNES_ERROR;
    if (snesROM2File(addr + 2) == SNES_ERROR) return SNES_ERROR;
    if (snesROM2File(addr + 3) == SNES_ERROR) return SNES_ERROR;
    addr = snesROM2File(addr);
    if (addr == SNES_ERROR) return SNES_ERROR;
    return ((buffer[addr + 3] << 24) |
            (buffer[addr + 2] << 16) |
            (buffer[addr + 1] << 8) |
            (buffer[addr + 0] << 0));
}

inline u32 snesGetPointer(u8 *buffer, u32 addr) {
    if (snesROM2File(addr + 1) == SNES_ERROR) return SNES_ERROR;
    if (snesROM2File(addr + 2) == SNES_ERROR) return SNES_ERROR;
    addr = snesROM2File(addr);
    if (addr == SNES_ERROR) return SNES_ERROR;
    return ((buffer[addr + 2] << 16) |
            (buffer[addr + 1] << 8) |
            (buffer[addr + 0] << 0));
}

inline u16 snesGetWord(u8 *buffer, u32 addr) {
    if (snesROM2File(addr + 1) == SNES_ERROR) return SNES_ERROR;
    addr = snesROM2File(addr);
    if (addr == SNES_ERROR) return SNES_ERROR;
    return ((buffer[addr + 1] << 8) |
            (buffer[addr + 0] << 0));
}

inline u8 snesGetByte(u8 *buffer, u32 addr) {
    addr = snesROM2File(addr);
    if (addr == SNES_ERROR) return SNES_ERROR;
    return (buffer[addr + 0] << 0);
}



inline void snesSetLong(u8 *buffer, u32 addr, u32 data) {
    if (snesROM2File(addr + 1) == SNES_ERROR) return;
    if (snesROM2File(addr + 2) == SNES_ERROR) return;
    if (snesROM2File(addr + 3) == SNES_ERROR) return;
    addr = snesROM2File(addr);
    if (addr == SNES_ERROR) return;
    buffer[addr + 3] = ((data >> 24) & 0xFF);
    buffer[addr + 2] = ((data >> 16) & 0xFF);
    buffer[addr + 1] = ((data >> 8) & 0xFF);
    buffer[addr + 0] = ((data >> 0) & 0xFF);
}

inline void snesSetPointer(u8 *buffer, u32 addr, u32 data) {
    if (snesROM2File(addr + 1) == SNES_ERROR) return;
    if (snesROM2File(addr + 2) == SNES_ERROR) return;
    addr = snesROM2File(addr);
    if (addr == SNES_ERROR) return;
    buffer[addr + 2] = ((data >> 16) & 0xFF);
    buffer[addr + 1] = ((data >> 8) & 0xFF);
    buffer[addr + 0] = ((data >> 0) & 0xFF);
}

inline void snesSetWord(u8 *buffer, u32 addr, u16 data) {
    if (snesROM2File(addr + 1) == SNES_ERROR) return;
    addr = snesROM2File(addr);
    if (addr == SNES_ERROR) return;
    buffer[addr + 1] = ((data >> 8) & 0xFF);
    buffer[addr + 0] = ((data >> 0) & 0xFF);
}

inline void snesSetByte(u8 *buffer, u32 addr, u8 data) {
    addr = snesROM2File(addr);
    if (addr == SNES_ERROR) return;
    buffer[addr + 0] = ((data >> 0) & 0xFF);
}



inline int snesGetBankAddr(u8 bank) {
    if (bank >= snesGetMaxBanks()) return SNES_ERROR;
    switch (romtype) {
        case MAP_LOROM:
            if (bank >= 0x80) return ((bank << 16) | 0x008000);
            return ((bank << 16) | 0x808000);
        case MAP_HIROM:
            if (bank >= 0x40) return ((bank << 16) | 0x800000);
            return ((bank << 16) | 0xC00000);
    }
    return SNES_ERROR;
}

inline int snesGetBankSize() {
    switch (romtype) {
        case MAP_LOROM: return 0x8000;  // 32KB
        case MAP_HIROM: return 0x10000; // 64KB
    }
    return SNES_ERROR;
}

inline int snesGetMaxBanks() {
    int size = snesGetBankSize();

    if (size == SNES_ERROR) return SNES_ERROR;
    return (romsize / size);
}

inline u32 snesGetVectorAddr(u32 addr, u8 mode) {
    if (mode > 1) return SNES_ERROR;

    return (snesGetHeaderAddr(addr) | (mode << 4));
}

inline u32 snesGetHeaderAddr(u32 addr) {
    switch (romtype) {
        case MAP_LOROM: return (addr | 0x800000);
        case MAP_HIROM: return (addr | 0xC00000);
    }

    return 0;
}



inline void snesGenChecksum(u8 *buffer, int size) {
    u32 ptr;
    u16 sum = 0, chk[16];
    int i;

    memset(chk, 0, (16 * sizeof(u16)));

    // clear old checksum
    snesSetWord(buffer, HDR_CHECKSUM, 0x0000);
    snesSetWord(buffer, HDR_COMPSUM, 0xFFFF);

    ptr = header;
    for (i = 0; i < size; i++) {
        chk[i >> 19] += buffer[ptr];
        ptr++;
    }
    if ((size - header) & 0x0007FFFF) {
        // uneven rom size
        printf("Warning: This ROM has an uneven size! Checksum generation will fail.\n");
    }
    for (i = 0; i < 16; i++) {
        sum += chk[i];
    }

    // set new checksum
    snesSetWord(buffer, HDR_CHECKSUM, sum);
    snesSetWord(buffer, HDR_COMPSUM, ~sum);
}

inline int snesGetMappingType(u8 *buffer) {
    int ptr, chk;

    // FIXME: Should not set romtype directly!

    romtype = MAP_LOROM;
    chk = (snesGetWord(buffer, snesGetHeaderAddr(HDR_CHECKSUM)) ^ snesGetWord(buffer, snesGetHeaderAddr(HDR_COMPSUM)));
    if ((chk == 0xFFFF) && ((snesGetByte(buffer, snesGetHeaderAddr(HDR_MAKEUP)) & 0x01) == romtype)) return romtype;

    romtype = MAP_HIROM;
    chk = (snesGetWord(buffer, snesGetHeaderAddr(HDR_CHECKSUM)) ^ snesGetWord(buffer, snesGetHeaderAddr(HDR_COMPSUM)));
    if ((chk == 0xFFFF) && ((snesGetByte(buffer, snesGetHeaderAddr(HDR_MAKEUP)) & 0x01) == romtype)) return romtype;

    return SNES_ERROR; // failed to identify ROM mapping type
}

inline void snesSetMappingType(int type) {
    romtype = type;
}

inline int snesInit(u8 *buffer, int size) {
    header = (size & 0x7FFF);
    romsize = (size - header);
    if ((header != 0x0000) && (header != 0x0200)) return SNES_HEADERR;

    romtype = snesGetMappingType(buffer);
    if ((romtype == MAP_LOROM) && (romsize > 0x7F0000)) return SNES_SIZEERR;
    else if ((romtype == MAP_HIROM) && (romsize > 0x800000)) return SNES_SIZEERR; // FIXME

    return romtype;
}
