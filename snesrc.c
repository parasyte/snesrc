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
  Add "dead map" to log all deleted dirty bytes, use with pass 3
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

#include "snesrc.h"
#include "dasm.h"
#include "emulate.h"
#include "snes.h"



int error = 0;
int warning = 0;
char outdir[256];

// The regsig table is used to quickly grab the 'register signature' for
//  register-size-dependant instructions.
const u8 regsig[256] = {
/*0x00*/    0,   0,   0,0,0,0,0,0,0,RS_A,0,0,0,0,0,0,
/*0x10*/    0,   0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,0,
/*0x20*/    0,   0,   0,0,0,0,0,0,0,RS_A,0,0,0,0,0,0,
/*0x30*/    0,   0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,0,
/*0x40*/    0,   0,   0,0,0,0,0,0,0,RS_A,0,0,0,0,0,0,
/*0x50*/    0,   0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,0,
/*0x60*/    0,   0,   0,0,0,0,0,0,0,RS_A,0,0,0,0,0,0,
/*0x70*/    0,   0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,0,
/*0x80*/    0,   0,   0,0,0,0,0,0,0,RS_A,0,0,0,0,0,0,
/*0x90*/    0,   0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,0,
/*0xA0*/    RS_X,0,RS_X,0,0,0,0,0,0,RS_A,0,0,0,0,0,0,
/*0xB0*/    0,   0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,0,
/*0xC0*/    RS_X,0,   0,0,0,0,0,0,0,RS_A,0,0,0,0,0,0,
/*0xD0*/    0,   0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,0,
/*0xE0*/    RS_X,0,   0,0,0,0,0,0,0,RS_A,0,0,0,0,0,0,
/*0xF0*/    0,   0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,0
};

// The opsize table is used to quickly grab the 65816 instruction sizes (in bytes)
const u8 opsize[4][256] = {
    { // 16-bit accumulator, 16-bit index registers
    /*0x00*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x10*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x20*/    3,2,4,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x30*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x40*/    1,2,2,2,3,2,2,2,1,3,1,1,3,3,3,4,
    /*0x50*/    2,2,2,2,3,2,2,2,1,3,1,1,4,3,3,4,
    /*0x60*/    1,2,3,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x70*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x80*/    2,2,3,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x90*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0xA0*/    3,2,3,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0xB0*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0xC0*/    3,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0xD0*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,2,
    /*0xE0*/    3,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0xF0*/    2,2,2,2,3,2,2,2,1,3,1,1,3,3,3,4
    },
    { // 16-bit accumulator, 8-bit index registers
    /*0x00*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x10*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x20*/    3,2,4,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x30*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x40*/    1,2,2,2,3,2,2,2,1,3,1,1,3,3,3,4,
    /*0x50*/    2,2,2,2,3,2,2,2,1,3,1,1,4,3,3,4,
    /*0x60*/    1,2,3,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x70*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x80*/    2,2,3,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x90*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0xA0*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0xB0*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0xC0*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0xD0*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,2,
    /*0xE0*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0xF0*/    2,2,2,2,3,2,2,2,1,3,1,1,3,3,3,4
    },
    { // 8-bit accumulator, 16-bit index registers
    /*0x00*/    2,2,2,2,2,2,2,2,1,2,1,1,3,3,3,4,
    /*0x10*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x20*/    3,2,4,2,2,2,2,2,1,2,1,1,3,3,3,4,
    /*0x30*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x40*/    1,2,2,2,3,2,2,2,1,2,1,1,3,3,3,4,
    /*0x50*/    2,2,2,2,3,2,2,2,1,3,1,1,4,3,3,4,
    /*0x60*/    1,2,3,2,2,2,2,2,1,2,1,1,3,3,3,4,
    /*0x70*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x80*/    2,2,3,2,2,2,2,2,1,2,1,1,3,3,3,4,
    /*0x90*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0xA0*/    3,2,3,2,2,2,2,2,1,2,1,1,3,3,3,4,
    /*0xB0*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0xC0*/    3,2,2,2,2,2,2,2,1,2,1,1,3,3,3,4,
    /*0xD0*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,2,
    /*0xE0*/    3,2,2,2,2,2,2,2,1,2,1,1,3,3,3,4,
    /*0xF0*/    2,2,2,2,3,2,2,2,1,3,1,1,3,3,3,4
    },
    { // 8-bit accumulator, 8-bit index registers
    /*0x00*/    2,2,2,2,2,2,2,2,1,2,1,1,3,3,3,4,
    /*0x10*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x20*/    3,2,4,2,2,2,2,2,1,2,1,1,3,3,3,4,
    /*0x30*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x40*/    1,2,2,2,3,2,2,2,1,2,1,1,3,3,3,4,
    /*0x50*/    2,2,2,2,3,2,2,2,1,3,1,1,4,3,3,4,
    /*0x60*/    1,2,3,2,2,2,2,2,1,2,1,1,3,3,3,4,
    /*0x70*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0x80*/    2,2,3,2,2,2,2,2,1,2,1,1,3,3,3,4,
    /*0x90*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0xA0*/    2,2,2,2,2,2,2,2,1,2,1,1,3,3,3,4,
    /*0xB0*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,4,
    /*0xC0*/    2,2,2,2,2,2,2,2,1,2,1,1,3,3,3,4,
    /*0xD0*/    2,2,2,2,2,2,2,2,1,3,1,1,3,3,3,2,
    /*0xE0*/    2,2,2,2,2,2,2,2,1,2,1,1,3,3,3,4,
    /*0xF0*/    2,2,2,2,3,2,2,2,1,3,1,1,3,3,3,4
    }
};



// Context handling
int DeleteDirtyBytes(CONTEXT *context) {
    int i;
    int ret = 0;

    // Run through map deleting dirty data
    for (i = 0; i < context->size; i++) {
        if (context->map[i] & MAP_DIRTY) {
            context->map[i] = 0;
            ret++;
        }
    }

    return ret;
}

int CleanDirtyBits(CONTEXT *context) {
    int i;
    int ret = 0;

    // Run through map clearing the dirty bits
    for (i = 0; i < context->size; i++) {
        if (context->map[i] & MAP_DIRTY) {
            context->map[i] &= ~MAP_DIRTY;
            ret++;
        }
    }

    return ret;
}

int FindFirstDirty(CONTEXT *context) {
    int i;

    for (i = 0; i < context->size; i++) {
        if (context->map[i] & MAP_DIRTY) return i;
    }
    return -1;
}

void UpdateContext(CONTEXT *context, u32 pc, u16 p, u16 sp) {
    context->pc = pc;
    context->p = p;
    context->sp = sp;
}

int CreateMap(CONTEXT *context) {
    // For debugging
#ifdef DEBUG_BUILD
    char file[64];

    sprintf(file, "%s/pass1.log", outdir);
    if (!(context->fdbg = fopen(file, "wt"))) {
        printf("Error opening file \"%s\" in mode \"%s\"\n", file, "wt");
        return 1;
    }
#endif // DEDUG_BUILD

    // Emulate from specific hardware vectors
    printf("Pass 1...\n");

#ifdef DEBUG_BUILD
    fprintf(context->fdbg, "\nRESET (6502):\n");
#endif // DEDUG_BUILD
    context->pc = snesGetWord(context->buffer, snesGetVectorAddr(VCT_RESET, SNESMODE_EMU));
    context->p = 0x0134; // Note: E,M,X, and I bits are set on reset.
    context->sp = 0;
    emulate(context, 0);

#ifdef DEBUG_BUILD
    fprintf(context->fdbg, "\nIRQ (6502):\n");
#endif // DEDUG_BUILD
    context->pc = snesGetWord(context->buffer, snesGetVectorAddr(VCT_IRQ, SNESMODE_EMU));
    context->p = 0x0104;
    context->sp = 0;
    emulate(context, 0);

#ifdef DEBUG_BUILD
    fprintf(context->fdbg, "\nNMI (6502):\n");
#endif // DEDUG_BUILD
    context->pc = snesGetWord(context->buffer, snesGetVectorAddr(VCT_NMI, SNESMODE_EMU));
    context->p = 0x0104;
    context->sp = 0;
    emulate(context, 0);


#ifdef DEBUG_BUILD
    fprintf(context->fdbg, "\nIRQ (65816):\n");
#endif // DEDUG_BUILD
    context->pc = snesGetWord(context->buffer, snesGetVectorAddr(VCT_IRQ, SNESMODE_NATIVE));
    context->p = 0x0004;
    context->sp = 0;
    emulate(context, 0);

#ifdef DEBUG_BUILD
    fprintf(context->fdbg, "\nNMI (65816):\n");
#endif // DEDUG_BUILD
    context->pc = snesGetWord(context->buffer, snesGetVectorAddr(VCT_NMI, SNESMODE_NATIVE));
    context->p = 0x0004;
    context->sp = 0;
    emulate(context, 0);

#ifdef DEBUG_BUILD
    fclose(context->fdbg);
#endif // DEDUG_BUILD

    return 0;
}

int FlushMap(CONTEXT *context) {
    FILE *fout;
    char file[64], str[80];
    int i = 0, j;
    u32 pc, end;

    printf("Pass 2...\n");

    while (i < snesGetMaxBanks()) {
        sprintf(file, "%s/bank%02d.asm", outdir, i);
        if (!(fout = fopen(file, "wt"))) {
            printf("Error opening file \"%s\" in mode \"%s\"\n", file, "wt");
            return 1;
        }

        pc = snesGetBankAddr(i);
        end = (pc + snesGetBankSize());

        while (pc < end) {
            if ((i == 0) && (pc >= snesGetHeaderAddr(HDR_TITLE))) {
                // Clear Map within ROM header
                j = snesROM2File(snesGetHeaderAddr(HDR_TITLE));
                memset(&context->map[j], 0, 0x40);

                // Handle ROM header
                fprintf(fout, "\n\n.org 0x%04X\n\n; ROM Header\n", HDR_TITLE);
                for (j = 0; j < 21; j++) {
                    str[j] = snesGetByte(context->buffer, snesGetHeaderAddr(HDR_TITLE + j));
                }
                str[21] = 0;
                while (strlen(str) < 21) strcat(str, " ");
                fprintf(fout, ".db \"%s\" ; Title\n", str);
                fprintf(fout, ".db 0x%02X ; ROM Makeup\n",   snesGetByte(context->buffer, snesGetHeaderAddr(HDR_MAKEUP)));
                fprintf(fout, ".db 0x%02X ; ROM Type\n",     snesGetByte(context->buffer, snesGetHeaderAddr(HDR_ROMTYPE)));
                fprintf(fout, ".db 0x%02X ; ROM Size\n",     snesGetByte(context->buffer, snesGetHeaderAddr(HDR_SIZE)));
                fprintf(fout, ".db 0x%02X ; SRAM Size\n",    snesGetByte(context->buffer, snesGetHeaderAddr(HDR_SRAM)));
                fprintf(fout, ".db 0x%02X ; Country Code\n", snesGetByte(context->buffer, snesGetHeaderAddr(HDR_COUNTRY)));
                fprintf(fout, ".db 0x%02X ; License Code\n", snesGetByte(context->buffer, snesGetHeaderAddr(HDR_LICENSE)));
                fprintf(fout, ".db 0x%02X ; ROM Version\n",  snesGetByte(context->buffer, snesGetHeaderAddr(HDR_VERSION)));
                fprintf(fout, "\n");
                fprintf(fout, ".dw 0x%04X ; Checksum Complement\n", snesGetWord(context->buffer, snesGetHeaderAddr(HDR_COMPSUM)));
                fprintf(fout, ".dw 0x%04X ; Checksum\n",            snesGetWord(context->buffer, snesGetHeaderAddr(HDR_CHECKSUM)));

                pc += 0x40;
            }
            else {
                switch (snesGetByte(context->map, pc) & MAPMASK_ACCESS) {
                    case MAP_ISCODE:
                        fprintf(fout, "\t%s\n", BinToASM(pc, ((snesGetByte(context->map, pc) & 0xC0) >> 2), &context->buffer[snesROM2File(pc)]));
                        j = opsize[snesGetByte(context->map, pc) >> 6][snesGetByte(context->buffer, pc)];
                        while (j--) {
                            snesSetByte(context->map, pc, 0);
                            pc++;
                        }
                        break;

                    case MAP_ISDATA:
                        j = snesGetByte(context->map, pc);
                        snesSetByte(context->map, pc, 0);
                        if (j & MAP_DATAWORD) {
                            fprintf(fout, ".dw %04X\n", snesGetWord(context->buffer, pc));
                            snesSetByte(context->map, (pc + 1), 0);
                            pc += 2;
                        }
                        else {
                            fprintf(fout, ".db %02X\n", snesGetByte(context->buffer, pc));
                            pc++;
                        }
                        break;

                    default:
                        pc++;
                        break;
                }
            }
        }

        fclose(fout);

        // Delete empty files
        fout = fopen(file, "rb");
        fseek(fout, 0, SEEK_END);
        j = ftell(fout);
        fclose(fout);
        if (j == 0) remove(file);

        i++;
    }

    return 0;
}

void CheckMap(CONTEXT *context) {
#ifdef DEBUG_BUILD
    FILE *fout;
    char file[64];
#endif // DEBUG_BUILD
    int i;

    printf("Pass 3...\n");

#ifdef DEBUG_BUILD
    sprintf(file, "%s/pass3.log", outdir);
    if (!(fout = fopen(file, "wt"))) {
        printf("Error opening file \"%s\" in mode \"%s\"\n", file, "wt");
        return;
    }
#endif // DEBUG_BUILD

    for (i = 0; i < context->size; i++) {
        if (context->map[i]) {
            warning++;

#ifdef DEBUG_BUILD
            fprintf(fout, "WARNING: Unseen mapped data at pc %06X, data: %02X\n", snesFile2ROM(i), context->map[i]);
#endif // DEBUG_BUILD

        }
    }

#ifdef DEBUG_BUILD
    fclose(fout);
#endif // DEBUG_BUILD
}


int GetMapSequence(CONTEXT *context, int where) {
    /*
     To do:
      This function will return a value indicating the amount of 'sequenced'
      data at the specified address.
    */
    return 0;
}


// Miscellaneous
void FreeContext(CONTEXT *context) {
    free(context->buffer);
    free(context->map);
}


void PrintUsage(char *str) {
    char *chr;

    // Extract program file name
    if ((chr = strrchr(str, '\\'))) str = (chr + 1);
    else if ((chr = strrchr(str, '/'))) str = (chr + 1);

    printf("Usage:\n%s [options] <input file> <output dir>\n\n", str);
    printf("Options:\n");
    printf("\t-l:\tForce LoROM\n");
    printf("\t-h:\tForce HiROM\n");
    printf("\t-r<n>:\tSet pointer table range check size\n");
    printf("\t-fbrk:\tAttempt to fix code which reaches a BRK instruction\n");
    printf("\t-fcop:\tAttempt to fix code which reaches a COP instruction\n");
    printf("\t-fstp:\tAttempt to fix code which reaches a STP instruction\n");
    printf("\t-fwdm:\tAttempt to fix code which reaches a WDM instruction\n");
    printf("\n");
}

int main(int argc, char **argv) {
    FILE *fin;
    CONTEXT context;
    int i;
    int len;
    int forcetype = -1;

    printf("snesrc - The SNES Recompiler "VERSION"\nCopyright 2005 Parasyte\n\n");

    // Setup context
    context.range = 4;
    context.flags = FLAG_NONE;

    // Parse command line args
    if (argc < 3) {
        PrintUsage(argv[0]);
        return 1;
    }
    if (argc > 3) {
        for (i = 1; i < (argc - 2); i++) {
            if      (!(strcmp(argv[i], "-l"))) forcetype = 0;
            else if (!(strcmp(argv[i], "-h"))) forcetype = 1;
            else if (!(strncmp(argv[i], "-r", 2))) sscanf(&argv[i][2], "%d", &context.range);
            else if (!(strcmp(argv[i], "-fbrk"))) context.flags |= FLAG_FIXBRK;
            else if (!(strcmp(argv[i], "-fcop"))) context.flags |= FLAG_FIXCOP;
            else if (!(strcmp(argv[i], "-fstp"))) context.flags |= FLAG_FIXSTP;
            else if (!(strcmp(argv[i], "-fwdm"))) context.flags |= FLAG_FIXWDM;
        }
    }

    // Open file
    if (!(fin = fopen(argv[argc - 2], "rb"))) {
        printf("Error opening file \"%s\" in mode \"%s\"\n", argv[argc - 2], "rb");
        return 1;
    }

    // Create & clean output directory
    fclose(stderr);
    strcpy(outdir, argv[argc - 1]);
#ifdef __WIN32__
    if ((i = mkdir(outdir))) {
        printf("Unable to create directory \"%s\", mkdir() returned %d\nerrno = %d\n\n", outdir, i, errno);
        return 1;
    }
#else // __WIN32__
    if ((i = mkdir(outdir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))) { // mode 0755
        printf("Unable to create directory \"%s\", mkdir() returned %d\nerrno = %d\n\n", outdir, i, errno);
        return 1;
    }
#endif // __WIN32__

    // Get file size
    fseek(fin, 0, SEEK_END);
    context.size = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    // Allocate memory
    if (!(context.buffer = (u8*)malloc(context.size))) {
        printf("Error allocating %d bytes of memory\n", context.size);
        return 1;
    }
    if (!(context.map = (u8*)malloc(context.size))) {
        printf("Error allocating %d bytes of memory\n", context.size);
        free(context.buffer);
        return 1;
    }

    // Read file
    if ((len = fread(context.buffer, 1, context.size, fin)) != context.size) {
        printf("ERROR: fread() returned %d -- expected %d\n\n", len, context.size);
        FreeContext(&context);
        return 1;
    }
    fclose(fin);

    // Initialize
    context.type = snesInit(context.buffer, context.size);
    if (context.type == SNES_HEADERR) {
        printf("SNES ROM appears to contain an invalid header or uneven banks.\nAttempting to correct the problem...\n");
        // reallocate memory, aligned to next 32KB boundary
        len = (context.size & 0x7FFF);
        if (len) {
            len = (0x8000 - len);
            context.size = ((context.size + 0x7FFF) & ~0x7FFF);

            if (!(context.buffer = (u8*)realloc(context.buffer, context.size))) {
                printf("Error allocating %d bytes of memory\n", context.size);
                FreeContext(&context);
                return 1;
            }
            memset(&context.buffer[context.size - len], 0, len);

            if (!(context.map = (u8*)realloc(context.map, context.size))) {
                printf("Error allocating %d bytes of memory\n", context.size);
                FreeContext(&context);
                return 1;
            }
        }
        context.type = snesInit(context.buffer, context.size);
    }
    if (forcetype >= 0) {
        printf("snesInit() returned %d\nForcing mapping type...\n", context.type);
        context.type = forcetype;
        snesSetMappingType(forcetype);
    }
    else if (context.type < 0) {
        printf("ERROR: snesInit() returned %d\n\n", context.type);
        FreeContext(&context);
        return 1;
    }

    printf("ROM mapping type set to %d\n", context.type);
    printf("Pointer table range check set to %d\n", context.range);
    printf("Fix attempts installed:");
    if (context.flags == FLAG_NONE) printf(" NONE\n");
    else {
        if (context.flags & FLAG_FIXBRK) printf(" BRK");
        if (context.flags & FLAG_FIXCOP) printf(" COP");
        if (context.flags & FLAG_FIXSTP) printf(" STP");
        if (context.flags & FLAG_FIXWDM) printf(" WDM");
        printf("\n");
    }

    printf("\nNative mode vectors:\n");
    printf("   COP Vector:   0x%06X",     snesGetWord(context.buffer, snesGetVectorAddr(VCT_COP,   SNESMODE_NATIVE)));
    printf("   BRK Vector:   0x%06X",     snesGetWord(context.buffer, snesGetVectorAddr(VCT_BRK,   SNESMODE_NATIVE)));
    printf("   Abort Vector: 0x%06X\n",   snesGetWord(context.buffer, snesGetVectorAddr(VCT_ABORT, SNESMODE_NATIVE)));
    printf("   NMI Vector:   0x%06X",     snesGetWord(context.buffer, snesGetVectorAddr(VCT_NMI,   SNESMODE_NATIVE)));
    printf("  *Reset Vector: 0x%06X",     snesGetWord(context.buffer, snesGetVectorAddr(VCT_RESET, SNESMODE_NATIVE)));
    printf("   IRQ Vector:   0x%06X\n",   snesGetWord(context.buffer, snesGetVectorAddr(VCT_IRQ,   SNESMODE_NATIVE)));

    printf("\nEmulation mode vectors:\n");
    printf("   COP Vector:   0x%06X",     snesGetWord(context.buffer, snesGetVectorAddr(VCT_COP,   SNESMODE_EMU)));
    printf("  *BRK Vector:   0x%06X",     snesGetWord(context.buffer, snesGetVectorAddr(VCT_BRK,   SNESMODE_EMU)));
    printf("   Abort Vector: 0x%06X\n",   snesGetWord(context.buffer, snesGetVectorAddr(VCT_ABORT, SNESMODE_EMU)));
    printf("   NMI Vector:   0x%06X",     snesGetWord(context.buffer, snesGetVectorAddr(VCT_NMI,   SNESMODE_EMU)));
    printf("   Reset Vector: 0x%06X",     snesGetWord(context.buffer, snesGetVectorAddr(VCT_RESET, SNESMODE_EMU)));
    printf("   IRQ Vector:   0x%06X\n\n", snesGetWord(context.buffer, snesGetVectorAddr(VCT_IRQ,   SNESMODE_EMU)));

    // Clear map
    memset(context.map, 0, context.size);

    // Work some magic
    if (CreateMap(&context)) return 1;
    if (FlushMap(&context)) return 1;
    CheckMap(&context);
    printf("Errors: %d\nWarnings: %d\n", error, warning);

    // We're outta here!
    printf("\nDone!\n\n");
    FreeContext(&context);

    return 0;
}
