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

#ifndef __DASM_H__
#define __DASM_H__

#include "_types.h"


// Disassembler
char *BinToASM(int addr, u16 p, u8 *opcode);

#endif // __DASM_H__
