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

#ifndef __EMULATE_H__
#define __EMULATE_H__

#include "_types.h"
#include "snesrc.h"
#include "snes.h"


// Stack push defines
#define PUSHB(a) { \
    ram[(s << 8) | stack] = (a); stack--; \
}

#define PUSHW(a) { \
    PUSHB((a)); \
    PUSHB((a) >> 8); \
}

#define PUSHP(a) { \
    PUSHB((a)); \
    PUSHB((a) >> 8); \
    PUSHB((a) >> 16); \
}

// Stack pull defines
#define PULLB(a) { \
    stack++; (a) = ram[(s << 8) | stack]; \
}

#define PULLW(a) { \
    stack++; (a) = (ram[(s << 8) | stack] << 8); \
    stack++; (a) |= ram[(s << 8) | stack]; \
}

#define PULLP(a) { \
    stack++; (a) =  (ram[(s << 8) | stack] << 16); \
    stack++; (a) |= (ram[(s << 8) | stack] << 8); \
    stack++; (a) |=  ram[(s << 8) | stack]; \
}

// Accumulator and Index Register size defines
#define a8    (p & 0x0020)
#define a16 (!(p & 0x0120))
#define x8    (p & 0x0010)
#define x16 (!(p & 0x0110))

// Emulator defines
#define RAMSIZE 0x0200


// Emulator functions
int GetOpSize(CONTEXT *context, u32 pc, u16 p);
int emulate(CONTEXT *context, u8 *ramcopy);

#endif // __EMULATE_H__
