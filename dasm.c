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

#include <stdio.h>
#include <string.h>

#include "dasm.h"


#define relative(a) { \
    if (((a) = opcode[1]) & 0x80) (a) = (addr - (((a) - 1) ^ 0xFF)); \
    else (a) += addr; \
    (a) = ((addr & 0x00FF0000) | (((a) + 2) & 0xFFFF)); \
}

#define relativel(a) { \
    if (((a) = (opcode[1] | (opcode[2] << 8))) & 0x8000) (a) = (addr - (((a) - 1) ^ 0xFFFF)); \
    else (a) += addr; \
    (a) = ((addr & 0x00FF0000) | (((a) + 3) & 0xFFFF)); \
}

#define absolute(a) { \
    (a) = (opcode[1] | (opcode[2] << 8)); \
}

#define absolutel(a) { \
    (a) = (opcode[1] | (opcode[2] << 8) | (opcode[3] << 16)); \
}


// Disassembler
char *BinToASM(int addr, u16 p, u8 *opcode) {
    static char str[64] = { 0 }, chr[5] = { 0 };
    u32 tmp;

    switch (opcode[0]) {
        // Implied
        case 0x08: strcpy(str, "PHP");      break;
        case 0x0A: strcpy(str, "ASL");      break;
        case 0x0B: strcpy(str, "PHD");      break;
        case 0x18: strcpy(str, "CLC");      break;
        case 0x1A: strcpy(str, "INC");      break;
        case 0x1B: strcpy(str, "TCS");      break;
        case 0x28: strcpy(str, "PLP");      break;
        case 0x2A: strcpy(str, "ROL");      break;
        case 0x2B: strcpy(str, "PLD");      break;
        case 0x38: strcpy(str, "SEC");      break;
        case 0x3A: strcpy(str, "DEC");      break;
        case 0x3B: strcpy(str, "TSC");      break;
        case 0x40: strcpy(str, "RTI\n");    break;
        case 0x48: strcpy(str, "PHA");      break;
        case 0x4A: strcpy(str, "LSR");      break;
        case 0x4B: strcpy(str, "PHK");      break;
        case 0x58: strcpy(str, "CLI");      break;
        case 0x5A: strcpy(str, "PHY");      break;
        case 0x5B: strcpy(str, "TCD");      break;
        case 0x60: strcpy(str, "RTS\n");    break;
        case 0x68: strcpy(str, "PLA");      break;
        case 0x6A: strcpy(str, "ROR");      break;
        case 0x6B: strcpy(str, "RTL\n");    break;
        case 0x78: strcpy(str, "SEI");      break;
        case 0x7A: strcpy(str, "PLY");      break;
        case 0x7B: strcpy(str, "TDC");      break;
        case 0x88: strcpy(str, "DEY");      break;
        case 0x8A: strcpy(str, "TXA");      break;
        case 0x8B: strcpy(str, "PHB");      break;
        case 0x98: strcpy(str, "TYA");      break;
        case 0x9A: strcpy(str, "TXS");      break;
        case 0x9B: strcpy(str, "TXY");      break;
        case 0xA8: strcpy(str, "TAY");      break;
        case 0xAA: strcpy(str, "TAX");      break;
        case 0xAB: strcpy(str, "PLB");      break;
        case 0xB8: strcpy(str, "CLV");      break;
        case 0xBA: strcpy(str, "TSX");      break;
        case 0xBB: strcpy(str, "TYX");      break;
        case 0xC8: strcpy(str, "INY");      break;
        case 0xCA: strcpy(str, "DEX");      break;
        case 0xCB: strcpy(str, "WAI\n");    break;
        case 0xD8: strcpy(str, "CLD");      break;
        case 0xDA: strcpy(str, "PHX");      break;
        case 0xDB: strcpy(str, "STP\n");    break;
        case 0xE8: strcpy(str, "INX");      break;
        case 0xEA: strcpy(str, "NOP");      break;
        case 0xEB: strcpy(str, "XBA");      break;
        case 0xF8: strcpy(str, "SED");      break;
        case 0xFA: strcpy(str, "PLX");      break;
        case 0xFB: strcpy(str, "XCE");      break;

        // Move Block
        case 0x44: strcpy(chr, "MVP");      goto _moveblock;
        case 0x54: strcpy(chr, "MVN");      goto _moveblock;
        _moveblock:
            sprintf(str, "%s #$%02X,#$%02X", chr, opcode[1], opcode[2]);
            break;

        // Stack Relative,S
        case 0x03: strcpy(chr, "ORA");      goto _stackrel;
        case 0x23: strcpy(chr, "AND");      goto _stackrel;
        case 0x43: strcpy(chr, "EOR");      goto _stackrel;
        case 0x63: strcpy(chr, "ADC");      goto _stackrel;
        case 0x83: strcpy(chr, "EOR");      goto _stackrel;
        case 0xA3: strcpy(chr, "LDA");      goto _stackrel;
        case 0xC3: strcpy(chr, "CMP");      goto _stackrel;
        case 0xE3: strcpy(chr, "SBC");      goto _stackrel;
        _stackrel:
            sprintf(str, "%s $%02X,S", chr, opcode[1]);
            break;

        // (Stack Relative,S),Y
        case 0x13: strcpy(chr, "ORA");      goto _stackrely;
        case 0x33: strcpy(chr, "AND");      goto _stackrely;
        case 0x53: strcpy(chr, "EOR");      goto _stackrely;
        case 0x73: strcpy(chr, "ADC");      goto _stackrely;
        case 0x93: strcpy(chr, "STA");      goto _stackrely;
        case 0xB3: strcpy(chr, "LDA");      goto _stackrely;
        case 0xD3: strcpy(chr, "CMP");      goto _stackrely;
        case 0xF3: strcpy(chr, "SBC");      goto _stackrely;
        _stackrely:
            sprintf(str, "%s ($%02X,S),Y", chr, opcode[1]);
            break;

        // (Direct Page Indirect,X)
        case 0x01: strcpy(chr, "ORA");      goto _indirectx;
        case 0x21: strcpy(chr, "AND");      goto _indirectx;
        case 0x41: strcpy(chr, "EOR");      goto _indirectx;
        case 0x61: strcpy(chr, "ADC");      goto _indirectx;
        case 0x81: strcpy(chr, "STA");      goto _indirectx;
        case 0xA1: strcpy(chr, "LDA");      goto _indirectx;
        case 0xC1: strcpy(chr, "CMP");      goto _indirectx;
        case 0xE1: strcpy(chr, "SBC");      goto _indirectx;
        _indirectx:
            sprintf(str, "%s ($%02X,X)", chr, opcode[1]);
            break;

        // Direct Page
        case 0x04: strcpy(chr, "TSB");      goto _directpage;
        case 0x05: strcpy(chr, "ORA");      goto _directpage;
        case 0x06: strcpy(chr, "ASL");      goto _directpage;
        case 0x14: strcpy(chr, "TRB");      goto _directpage;
        case 0x24: strcpy(chr, "BIT");      goto _directpage;
        case 0x25: strcpy(chr, "AND");      goto _directpage;
        case 0x26: strcpy(chr, "ROL");      goto _directpage;
        case 0x45: strcpy(chr, "EOR");      goto _directpage;
        case 0x46: strcpy(chr, "LSR");      goto _directpage;
        case 0x64: strcpy(chr, "STZ");      goto _directpage;
        case 0x65: strcpy(chr, "ADC");      goto _directpage;
        case 0x66: strcpy(chr, "ROR");      goto _directpage;
        case 0x84: strcpy(chr, "STY");      goto _directpage;
        case 0x85: strcpy(chr, "STA");      goto _directpage;
        case 0x86: strcpy(chr, "STX");      goto _directpage;
        case 0xA4: strcpy(chr, "LDY");      goto _directpage;
        case 0xA5: strcpy(chr, "LDA");      goto _directpage;
        case 0xA6: strcpy(chr, "LDX");      goto _directpage;
        case 0xC4: strcpy(chr, "CPY");      goto _directpage;
        case 0xC5: strcpy(chr, "CMP");      goto _directpage;
        case 0xC6: strcpy(chr, "DEC");      goto _directpage;
        case 0xE4: strcpy(chr, "CPX");      goto _directpage;
        case 0xE5: strcpy(chr, "SBC");      goto _directpage;
        case 0xE6: strcpy(chr, "INC");      goto _directpage;
        _directpage:
            sprintf(str, "%s $%02X", chr, opcode[1]);
            break;

        // #Immediate
        case 0x00: strcpy(chr, "BRK");      goto _immediate;
        case 0x02: strcpy(chr, "COP");      goto _immediate;
        case 0x42: strcpy(chr, "WDM");      goto _immediate;
        case 0xC2: strcpy(chr, "REP");      goto _immediate;
        case 0xE2: strcpy(chr, "SEP");      goto _immediate;
        case 0x09: strcpy(chr, "ORA");      goto _immediatem;
        case 0x29: strcpy(chr, "AND");      goto _immediatem;
        case 0x49: strcpy(chr, "EOR");      goto _immediatem;
        case 0x69: strcpy(chr, "ADC");      goto _immediatem;
        case 0x89: strcpy(chr, "BIT");      goto _immediatem;
        case 0xA0: strcpy(chr, "LDY");      goto _immediatex;
        case 0xA2: strcpy(chr, "LDX");      goto _immediatex;
        case 0xA9: strcpy(chr, "LDA");      goto _immediatem;
        case 0xC0: strcpy(chr, "CPY");      goto _immediatex;
        case 0xC9: strcpy(chr, "CMP");      goto _immediatem;
        case 0xE0: strcpy(chr, "CPX");      goto _immediatex;
        case 0xE9: strcpy(chr, "SBC");      goto _immediatem;
        _immediate:
            sprintf(str, "%s #$%02X", chr, opcode[1]);
            break;
        _immediatem:
            if (p & 0x0120) sprintf(str, "%s #$%02X", chr, opcode[1]);
            else sprintf(str, "%s #$%02X%02X", chr, opcode[2], opcode[1]);
            break;
        _immediatex:
            if (p & 0x0110) sprintf(str, "%s #$%02X", chr, opcode[1]);
            else sprintf(str, "%s #$%02X%02X", chr, opcode[2], opcode[1]);
            break;

        // Absolute
        case 0x0C: strcpy(chr, "TSB");      goto _absolute;
        case 0x0D: strcpy(chr, "ORA");      goto _absolute;
        case 0x0E: strcpy(chr, "ASL");      goto _absolute;
        case 0x1C: strcpy(chr, "TRB");      goto _absolute;
        case 0x20: strcpy(chr, "JSR");      goto _absolute;
        case 0x2C: strcpy(chr, "BIT");      goto _absolute;
        case 0x2D: strcpy(chr, "AND");      goto _absolute;
        case 0x2E: strcpy(chr, "ROL");      goto _absolute;
        case 0x4C: strcpy(chr, "JMP");      goto _absolute;
        case 0x4D: strcpy(chr, "EOR");      goto _absolute;
        case 0x4E: strcpy(chr, "LSR");      goto _absolute;
        case 0x6D: strcpy(chr, "ADC");      goto _absolute;
        case 0x6E: strcpy(chr, "ROR");      goto _absolute;
        case 0x8C: strcpy(chr, "STY");      goto _absolute;
        case 0x8D: strcpy(chr, "STA");      goto _absolute;
        case 0x8E: strcpy(chr, "STX");      goto _absolute;
        case 0x9C: strcpy(chr, "STZ");      goto _absolute;
        case 0xAC: strcpy(chr, "LDY");      goto _absolute;
        case 0xAD: strcpy(chr, "LDA");      goto _absolute;
        case 0xAE: strcpy(chr, "LDX");      goto _absolute;
        case 0xCC: strcpy(chr, "CPY");      goto _absolute;
        case 0xCD: strcpy(chr, "CMP");      goto _absolute;
        case 0xCE: strcpy(chr, "DEC");      goto _absolute;
        case 0xEC: strcpy(chr, "CPX");      goto _absolute;
        case 0xED: strcpy(chr, "SBC");      goto _absolute;
        case 0xEE: strcpy(chr, "INC");      goto _absolute;
        case 0xF4: strcpy(chr, "PEA");      goto _absolute;
        _absolute:
            absolute(tmp);
            sprintf(str, "%s $%04X", chr, tmp);
            break;

        // Absolute Long
        case 0x0F: strcpy(chr, "ORA");      goto _absolutel;
        case 0x22: strcpy(chr, "JSL");      goto _absolutel;
        case 0x2F: strcpy(chr, "AND");      goto _absolutel;
        case 0x4F: strcpy(chr, "EOR");      goto _absolutel;
        case 0x5C: strcpy(chr, "JML");      goto _absolutel;
        case 0x6F: strcpy(chr, "ADC");      goto _absolutel;
        case 0x8F: strcpy(chr, "STA");      goto _absolutel;
        case 0xAF: strcpy(chr, "LDA");      goto _absolutel;
        case 0xCF: strcpy(chr, "CMP");      goto _absolutel;
        case 0xEF: strcpy(chr, "SBC");      goto _absolutel;
        _absolutel:
            absolutel(tmp);
            sprintf(str, "%s $%06X", chr, tmp);
            break;

        // branches
        case 0x10: strcpy(chr, "BPL");      goto _branch;
        case 0x30: strcpy(chr, "BMI");      goto _branch;
        case 0x50: strcpy(chr, "BVC");      goto _branch;
        case 0x70: strcpy(chr, "BVS");      goto _branch;
        case 0x80: strcpy(chr, "BRA");      goto _branch;
        case 0x90: strcpy(chr, "BCC");      goto _branch;
        case 0xB0: strcpy(chr, "BCS");      goto _branch;
        case 0xD0: strcpy(chr, "BNE");      goto _branch;
        case 0xF0: strcpy(chr, "BEQ");      goto _branch;
        _branch:
            relative(tmp);
            sprintf(str, "%s $%06X", chr, tmp);
            break;
        case 0x82:
            strcpy(chr, "BRL");
            relativel(tmp);
            sprintf(str, "%s $%06X", chr, tmp);
            break;

        // PER
        case 0x62:
            strcpy(chr, "PER");
            relativel(tmp);
            tmp += 3;
            tmp = ((addr & 0x00FF0000) | (tmp & 0xFFFF));
            sprintf(str, "%s $%06X", chr, tmp);
            break;

        // (Direct Page Indirect)
        case 0x12: strcpy(chr, "ORA");      goto _dpindirect;
        case 0x32: strcpy(chr, "AND");      goto _dpindirect;
        case 0x52: strcpy(chr, "EOR");      goto _dpindirect;
        case 0x72: strcpy(chr, "ADC");      goto _dpindirect;
        case 0x92: strcpy(chr, "STA");      goto _dpindirect;
        case 0xB2: strcpy(chr, "LDA");      goto _dpindirect;
        case 0xD2: strcpy(chr, "CMP");      goto _dpindirect;
        case 0xD4: strcpy(chr, "PEI");      goto _dpindirect;
        case 0xF2: strcpy(chr, "SBC");      goto _dpindirect;
        _dpindirect:
            sprintf(str, "%s ($%02X)", chr, opcode[1]);
            break;

        // (Direct Page Indirect),Y
        case 0x11: strcpy(chr, "ORA");      goto _indirecty;
        case 0x31: strcpy(chr, "AND");      goto _indirecty;
        case 0x51: strcpy(chr, "EOR");      goto _indirecty;
        case 0x71: strcpy(chr, "ADC");      goto _indirecty;
        case 0x91: strcpy(chr, "STA");      goto _indirecty;
        case 0xB1: strcpy(chr, "LDA");      goto _indirecty;
        case 0xD1: strcpy(chr, "CMP");      goto _indirecty;
        case 0xF1: strcpy(chr, "SBC");      goto _indirecty;
        _indirecty:
            sprintf(str, "%s ($%02X),Y", chr, opcode[1]);
            break;

        // [Direct Page Indirect Long]
        case 0x07: strcpy(chr, "ORA");      goto _directpagel;
        case 0x27: strcpy(chr, "AND");      goto _directpagel;
        case 0x47: strcpy(chr, "EOR");      goto _directpagel;
        case 0x67: strcpy(chr, "ADC");      goto _directpagel;
        case 0x87: strcpy(chr, "STA");      goto _directpagel;
        case 0xA7: strcpy(chr, "LDA");      goto _directpagel;
        case 0xC7: strcpy(chr, "CMP");      goto _directpagel;
        case 0xE7: strcpy(chr, "SBC");      goto _directpagel;
        _directpagel:
            sprintf(str, "%s [$%02X]", chr, opcode[1]);
            break;

        // [Direct Page Indirect Long],Y
        case 0x17: strcpy(chr, "ORA");      goto _indirectdpy;
        case 0x37: strcpy(chr, "AND");      goto _indirectdpy;
        case 0x57: strcpy(chr, "EOR");      goto _indirectdpy;
        case 0x77: strcpy(chr, "ADC");      goto _indirectdpy;
        case 0x97: strcpy(chr, "STA");      goto _indirectdpy;
        case 0xB7: strcpy(chr, "LDA");      goto _indirectdpy;
        case 0xD7: strcpy(chr, "CMP");      goto _indirectdpy;
        case 0xF7: strcpy(chr, "SBC");      goto _indirectdpy;
        _indirectdpy:
            sprintf(str, "%s [$%02X],Y", chr, opcode[1]);
            break;

        // Direct Page,X
        case 0x15: strcpy(chr, "ORA");      goto _directpagex;
        case 0x16: strcpy(chr, "ASL");      goto _directpagex;
        case 0x34: strcpy(chr, "BIT");      goto _directpagex;
        case 0x35: strcpy(chr, "AND");      goto _directpagex;
        case 0x36: strcpy(chr, "ROL");      goto _directpagex;
        case 0x55: strcpy(chr, "EOR");      goto _directpagex;
        case 0x56: strcpy(chr, "LSR");      goto _directpagex;
        case 0x74: strcpy(chr, "STZ");      goto _directpagex;
        case 0x75: strcpy(chr, "ADC");      goto _directpagex;
        case 0x76: strcpy(chr, "ROR");      goto _directpagex;
        case 0x94: strcpy(chr, "STY");      goto _directpagex;
        case 0x95: strcpy(chr, "STA");      goto _directpagex;
        case 0xB4: strcpy(chr, "LDY");      goto _directpagex;
        case 0xB5: strcpy(chr, "LDA");      goto _directpagex;
        case 0xD5: strcpy(chr, "CMP");      goto _directpagex;
        case 0xD6: strcpy(chr, "DEC");      goto _directpagex;
        case 0xF5: strcpy(chr, "SBC");      goto _directpagex;
        case 0xF6: strcpy(chr, "INC");      goto _directpagex;
        _directpagex:
            sprintf(str, "%s $%02X,X", chr, opcode[1]);
            break;

        // Absolute,Y
        case 0x19: strcpy(chr, "ORA");      goto _absolutey;
        case 0x39: strcpy(chr, "AND");      goto _absolutey;
        case 0x59: strcpy(chr, "EOR");      goto _absolutey;
        case 0x79: strcpy(chr, "ADC");      goto _absolutey;
        case 0x99: strcpy(chr, "STA");      goto _absolutey;
        case 0xB9: strcpy(chr, "LDA");      goto _absolutey;
        case 0xBE: strcpy(chr, "LDX");      goto _absolutey;
        case 0xD9: strcpy(chr, "CMP");      goto _absolutey;
        case 0xF9: strcpy(chr, "SBC");      goto _absolutey;
        _absolutey:
            absolute(tmp);
            sprintf(str, "%s $%04X,Y", chr, tmp);
            break;

        // Absolute,X
        case 0x1D: strcpy(chr, "ORA");      goto _absolutex;
        case 0x1E: strcpy(chr, "ASL");      goto _absolutex;
        case 0x3C: strcpy(chr, "BIT");      goto _absolutex;
        case 0x3D: strcpy(chr, "AND");      goto _absolutex;
        case 0x3E: strcpy(chr, "ROL");      goto _absolutex;
        case 0x5D: strcpy(chr, "EOR");      goto _absolutex;
        case 0x5E: strcpy(chr, "LSR");      goto _absolutex;
        case 0x7D: strcpy(chr, "ADC");      goto _absolutex;
        case 0x7E: strcpy(chr, "ROR");      goto _absolutex;
        case 0x9D: strcpy(chr, "STA");      goto _absolutex;
        case 0x9E: strcpy(chr, "STZ");      goto _absolutex;
        case 0xBC: strcpy(chr, "LDY");      goto _absolutex;
        case 0xBD: strcpy(chr, "LDA");      goto _absolutex;
        case 0xDD: strcpy(chr, "CMP");      goto _absolutex;
        case 0xDE: strcpy(chr, "DEC");      goto _absolutex;
        case 0xFD: strcpy(chr, "SBC");      goto _absolutex;
        case 0xFE: strcpy(chr, "INC");      goto _absolutex;
        _absolutex:
            absolute(tmp);
            sprintf(str, "%s $%04X,X", chr, tmp);
            break;

        // Absolute Long,X
        case 0x1F: strcpy(chr, "ORA");      goto _absolutelx;
        case 0x3F: strcpy(chr, "AND");      goto _absolutelx;
        case 0x5F: strcpy(chr, "EOR");      goto _absolutelx;
        case 0x7F: strcpy(chr, "ADC");      goto _absolutelx;
        case 0x9F: strcpy(chr, "STA");      goto _absolutelx;
        case 0xBF: strcpy(chr, "LDA");      goto _absolutelx;
        case 0xDF: strcpy(chr, "CMP");      goto _absolutelx;
        case 0xFF: strcpy(chr, "SBC");      goto _absolutelx;
        _absolutelx:
            absolutel(tmp);
            sprintf(str, "%s $%06X,X", chr, tmp);
            break;

        // (Absolute,X)
        case 0x7C: strcpy(chr, "JMP");      goto _absindirectx;
        case 0xFC: strcpy(chr, "JSR");      goto _absindirectx;
        _absindirectx:
            absolute(tmp);
            sprintf(str, "%s ($%04X,X)\n", chr, tmp);
            break;

        // jumps
        case 0x6C:
            absolute(tmp);
            sprintf(str, "JMP ($%04X)\n", tmp);
            break;
        case 0xDC:
            absolute(tmp);
            sprintf(str, "JMP [$%04X]\n", tmp);
            break;

        // Direct Page,Y
        case 0x96: strcpy(chr, "STX");      goto _directpagey;
        case 0xB6: strcpy(chr, "LDX");      goto _directpagey;
        _directpagey:
            sprintf(str, "%s $%02X,Y", chr, opcode[1]);
            break;
    }

    return str;
}
