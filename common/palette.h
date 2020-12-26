//
// Copyright 2020 Electronic Arts Inc.
//
// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
// in the hope that it will be useful, but with permitted additional restrictions
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
// distributed with this program. You should have received a copy of the
// GNU General Public License along with permitted additional restrictions
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection

/***************************************************************************
;**   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
;***************************************************************************
;*                                                                         *
;*                 Project Name : Palette 32bit Library.                   *
;*                                                                         *
;*                    File Name : PALETTE.H                                *
;*                                                                         *
;*                   Programmer : Scott K. Bowen                           *
;*                                                                         *
;*                   Start Date : April 25, 1994                           *
;*                                                                         *
;*                  Last Update : April 27, 1994 [BRR]                     *
;*                                                                         *
;* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef PALETTE_H
#define PALETTE_H

#define RGB_BYTES     3
#define PALETTE_SIZE  256
#define PALETTE_BYTES (RGB_BYTES * PALETTE_SIZE)

extern unsigned char CurrentPalette[PALETTE_BYTES];

void Set_Palette(void* palette);
void Fade_Palette_To(void* palette1, unsigned int delay, void (*callback)());

#endif // PALETTE_H
