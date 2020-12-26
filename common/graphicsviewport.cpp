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
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Westwood 32 bit Library                  *
 *                                                                         *
 *                    File Name : GBUFFER.CPP                              *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow                           *
 *                                                                         *
 *                   Start Date : May 3, 1994                              *
 *                                                                         *
 *                  Last Update : October 9, 1995   []                     *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   VVPC::VirtualViewPort -- Default constructor for a virtual viewport   *
 *   VVPC:~VirtualViewPortClass -- Destructor for a virtual viewport       *
 *   VVPC::Clear -- Clears a graphic page to correct color                 *
 *   VBC::VideoBufferClass -- Lowlevel constructor for video buffer class  *
 *   GVPC::Change -- Changes position and size of a Graphic View Port      *
 *   VVPC::Change -- Changes position and size of a Video View Port        *
 *   Set_Logic_Page -- Sets LogicPage to new buffer                        *
 *   GBC::DD_Init -- Inits a direct draw surface for a GBC                 *
 *   GBC::Init -- Core function responsible for initing a GBC              *
 *   GBC::Lock -- Locks a Direct Draw Surface                              *
 *   GBC::Unlock -- Unlocks a direct draw surface                          *
 *   GBC::GraphicBufferClass -- Default constructor (requires explicit init)*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "graphicsviewport.h"

#include "buffer.h"
#include "drawbuff.h"
#include "rect.h"
#include "gbuffer.h"
#include "ww_win.h"

#include <string.h>

bool AllowHardwareBlitFills = true;
bool OverlappedVideoBlits = true; // Can video driver blit overlapped regions?

/*
** Function to call if we detect focus loss
*/
void (*Misc_Focus_Loss_Function)(void) = nullptr;
void (*Misc_Focus_Restore_Function)(void) = nullptr;

/*=========================================================================*/
/* The following PRIVATE functions are in this file:                       */
/*=========================================================================*/

/*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/

/***************************************************************************
 * GVPC::GRAPHICVIEWPORTCLASS -- Constructor for basic view port class     *
 *                                                                         *
 * INPUT:      GraphicBufferClass * gbuffer - buffer to attach to          *
 *             int x                        - x offset into buffer         *
 *             int y                        - y offset into buffer         *
 *             int w                        - view port width in pixels    *
 *             int h                        - view port height in pixels   *
 *                                                                         *
 * OUTPUT:     Constructors may not have a return value                    *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/09/1994 PWG : Created.                                             *
 *=========================================================================*/
GraphicViewPortClass::GraphicViewPortClass(GraphicBufferClass* gbuffer, int x, int y, int w, int h)
    : Offset(0)
    , Width(0)
    , Height(0)
    , XAdd(0)
    , XPos(0)
    , YPos(0)
    , Pitch(0)
    , GraphicBuff(nullptr)
    , IsHardware(false)
    , LockCount(0)
{
    Attach(gbuffer, x, y, w, h);
}

/***************************************************************************
 * GVPC::GRAPHICVIEWPORTCLASS -- Default constructor for view port class   *
 *                                                                         *
 * INPUT:      none                                                        *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/09/1994 PWG : Created.                                             *
 *=========================================================================*/
GraphicViewPortClass::GraphicViewPortClass()
    : Offset(0)
    , Width(0)
    , Height(0)
    , XAdd(0)
    , XPos(0)
    , YPos(0)
    , Pitch(0)
    , GraphicBuff(nullptr)
    , IsHardware(false)
    , LockCount(0)
{
}

/***************************************************************************
 * GVPC::~GRAPHICVIEWPORTCLASS -- Destructor for GraphicViewPortClass      *
 *                                                                         *
 * INPUT:      none                                                        *
 *                                                                         *
 * OUTPUT:     A destructor may not return a value.                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/10/1994 PWG : Created.                                             *
 *=========================================================================*/
GraphicViewPortClass::~GraphicViewPortClass(void)
{
}

/***************************************************************************
 * GVPC::ATTACH -- Attaches a viewport to a buffer class                   *
 *                                                                         *
 * INPUT:    GraphicBufferClass *g_buff  -  pointer to gbuff to attach to  *
 *           int x                       - x position to attach to         *
 *           int y                       - y position to attach to         *
 *           int w                       - width of the view port          *
 *           int h                       - height of the view port         *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/10/1994 PWG : Created.                                             *
 *=========================================================================*/
void GraphicViewPortClass::Attach(GraphicBufferClass* gbuffer, int x, int y, int w, int h)
{
    /*======================================================================*/
    /* Can not attach a Graphic View Port if it is actually the physical    */
    /*     representation of a Graphic Buffer.                              */
    /*======================================================================*/
    if (this == Get_Graphic_Buffer()) {
        return;
    }

    /*======================================================================*/
    /* Verify that the x and y coordinates are valid and placed within the  */
    /*      physical buffer.                                                */
    /*======================================================================*/
    if (x < 0)                         // you cannot place view port off
        x = 0;                         //   the left edge of physical buf
    if (x >= gbuffer->Get_Width())     // you cannot place left edge off
        x = gbuffer->Get_Width() - 1;  //   the right edge of physical buf
    if (y < 0)                         // you cannot place view port off
        y = 0;                         //   the top edge of physical buf
    if (y >= gbuffer->Get_Height())    // you cannot place view port off
        y = gbuffer->Get_Height() - 1; //   bottom edge of physical buf

    /*======================================================================*/
    /* Adjust the width and height of necessary                             */
    /*======================================================================*/
    if (x + w > gbuffer->Get_Width()) // if the x plus width is larger
        w = gbuffer->Get_Width() - x; //    than physical, fix width

    if (y + h > gbuffer->Get_Height()) // if the y plus height is larger
        h = gbuffer->Get_Height() - y; //   than physical, fix height

    /*======================================================================*/
    /* Get a pointer to the top left edge of the buffer.                    */
    /*======================================================================*/
    Offset = gbuffer->Get_Offset() + ((gbuffer->Get_Width() + gbuffer->Get_Pitch()) * y) + x;

    /*======================================================================*/
    /* Copy over all of the variables that we need to store.                */
    /*======================================================================*/
    XPos = x;
    YPos = y;
    XAdd = gbuffer->Get_Width() - w;
    Width = w;
    Height = h;
    Pitch = gbuffer->Get_Pitch();
    GraphicBuff = gbuffer;
    IsHardware = gbuffer->IsHardware;
}

/***************************************************************************
 * GVPC::CHANGE -- Changes position and size of a Graphic View Port        *
 *                                                                         *
 * INPUT:       int the new x pixel position of the graphic view port      *
 *              int the new y pixel position of the graphic view port      *
 *              int the new width of the viewport in pixels                *
 *              int the new height of the viewport in pixels               *
 *                                                                         *
 * OUTPUT:      BOOL whether the Graphic View Port could be sucessfully    *
 *                      resized.                                           *
 *                                                                         *
 * WARNINGS:   You may not resize a Graphic View Port which is derived     *
 *                      from a Graphic View Port Buffer,                   *
 *                                                                         *
 * HISTORY:                                                                *
 *   09/14/1994 SKB : Created.                                             *
 *=========================================================================*/
bool GraphicViewPortClass::Change(int x, int y, int w, int h)
{
    /*======================================================================*/
    /* Can not change a Graphic View Port if it is actually the physical    */
    /*      representation of a Graphic Buffer.                             */
    /*======================================================================*/
    if (this == Get_Graphic_Buffer()) {
        return false;
    }

    /*======================================================================*/
    /* Since there is no allocated information, just re-attach it to the    */
    /*      existing graphic buffer as if we were creating the              */
    /*      GraphicViewPort.                                                */
    /*======================================================================*/
    Attach(Get_Graphic_Buffer(), x, y, w, h);
    return true;
}

/***************************************************************************
 * SET_LOGIC_PAGE -- Sets LogicPage to new buffer                          *
 *                                                                         *
 * INPUT:      GraphicBufferClass * the buffer we are going to set         *
 *                                                                         *
 * OUTPUT:     GraphicBufferClass * the previous buffer type               *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   02/23/1995 PWG : Created.                                             *
 *=========================================================================*/
GraphicViewPortClass* Set_Logic_Page(GraphicViewPortClass* ptr)
{
    GraphicViewPortClass* old = LogicPage;
    LogicPage = ptr;
    return (old);
}

/***************************************************************************
 * SET_LOGIC_PAGE -- Sets LogicPage to new buffer                          *
 *                                                                         *
 * INPUT:      GraphicBufferClass & the buffer we are going to set         *
 *                                                                         *
 * OUTPUT:     GraphicBufferClass * the previous buffer type               *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   02/23/1995 PWG : Created.                                             *
 *=========================================================================*/
GraphicViewPortClass* Set_Logic_Page(GraphicViewPortClass& ptr)
{
    GraphicViewPortClass* old = LogicPage;
    LogicPage = &ptr;
    return (old);
}

/***************************************************************************
 * GBC::LOCK -- Locks a Direct Draw Surface                                *
 *                                                                         *
 * INPUT:      none                                                        *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/09/1995     : Created.                                             *
 *   10/09/1995     : Code stolen from Steve Tall                          *
 *=========================================================================*/
extern void Colour_Debug(int call_number);

/***********************************************************************************************
 * GVPC::DD_Linear_Blit_To_Linear -- blit using the hardware blitter                           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    destination vvpc                                                                  *
 *           x coord to blit from                                                              *
 *           y coord to blit from                                                              *
 *           x coord to blit to                                                                *
 *           y coord to blit to                                                                *
 *           width to blit                                                                     *
 *           height to blit                                                                    *
 *                                                                                             *
 * OUTPUT:   DD_OK if successful                                                               *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09-22-95 11:05am ST : Created                                                             *
 *=============================================================================================*/

void GraphicViewPortClass::DD_Linear_Blit_To_Linear(GraphicViewPortClass& dest,
                                                    int source_x,
                                                    int source_y,
                                                    int dest_x,
                                                    int dest_y,
                                                    int width,
                                                    int height,
                                                    bool mask)

{
    Rect source_rectangle = {0};
    source_rectangle.X = source_x;
    source_rectangle.Y = source_y;
    source_rectangle.Width = width;
    source_rectangle.Height = height;

    Rect dest_rectangle = {0};
    dest_rectangle.X = dest_x;
    dest_rectangle.Y = dest_y;
    dest_rectangle.Width = width;
    dest_rectangle.Height = height;

    dest.GraphicBuff->Get_DD_Surface()->Blt(dest_rectangle, GraphicBuff->Get_DD_Surface(), source_rectangle, mask);
}

/***********************************************************************************************
 * GVPC::Lock -- lock the graphics buffer for reading or writing                               *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   TRUE if surface was successfully locked                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09-19-95 12:33pm ST : Created                                                             *
 *   10/09/1995     : Moved actually functionality to GraphicBuffer                            *
 *=============================================================================================*/
bool GraphicViewPortClass::Lock()
{
    bool lock = GraphicBuff->Lock();
    if (!lock) {
        return false;
    }

    if (this != GraphicBuff) {
        Attach(GraphicBuff, XPos, YPos, Width, Height);
    }
    return true;
}

/***********************************************************************************************
 * GVPC::Unlock -- unlock the video buffer                                                     *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   TRUE if surface was successfully unlocked                                         *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09-19-95 02:20pm ST : Created                                                             *
 *   10/09/1995     : Moved actually functionality to GraphicBuffer                            *
 *=============================================================================================*/
bool GraphicViewPortClass::Unlock()
{
    bool unlock = GraphicBuff->Unlock();
    if (!unlock) {
        return false;
    }
    if (this != GraphicBuff && IsHardware && !GraphicBuff->LockCount) {
        Offset = 0;
    }
    return true;
}

/***************************************************************************
 * GVPC::PUT_PIXEL -- stub to call curr graphic mode Put_Pixel					*
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
void GraphicViewPortClass::Put_Pixel(int x, int y, unsigned char color)
{
    if (Lock()) {
        Buffer_Put_Pixel(this, x, y, color);
        Unlock();
    }
}

/***************************************************************************
 * GVPC::GET_PIXEL -- stub to call curr graphic mode Get_Pixel             *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
int GraphicViewPortClass::Get_Pixel(int x, int y)
{
    int return_code = 0;

    if (Lock()) {
        return_code = (Buffer_Get_Pixel(this, x, y));
        Unlock();
    }
    return return_code;
}

/***************************************************************************
 * GVPC::CLEAR -- stub to call curr graphic mode Clear                     *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
void GraphicViewPortClass::Clear(unsigned char color)
{
    if (Lock()) {
        Buffer_Clear(this, color);
        Unlock();
    }
}

/***************************************************************************
 * GVPC::TO_BUFFER -- stub 1 to call curr graphic mode To_Buffer           *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
long GraphicViewPortClass::To_Buffer(int x, int y, int w, int h, void* buff, long size)
{
    long return_code = 0;
    if (Lock()) {
        return_code = (Buffer_To_Buffer(this, x, y, w, h, buff, size));
        Unlock();
    }
    return (return_code);
}

/***************************************************************************
 * GVPC::TO_BUFFER -- stub 2 to call curr graphic mode To_Buffer           *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
long GraphicViewPortClass::To_Buffer(int x, int y, int w, int h, BufferClass* buff)
{
    long return_code = 0;
    if (Lock()) {
        return_code = (Buffer_To_Buffer(this, x, y, w, h, buff->Get_Buffer(), buff->Get_Size()));
        Unlock();
    }
    return (return_code);
}

/***************************************************************************
 * GVPC::TO_BUFFER -- stub 3 to call curr graphic mode To_Buffer           *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
long GraphicViewPortClass::To_Buffer(BufferClass* buff)
{
    long return_code = 0;
    if (Lock()) {
        return_code = (Buffer_To_Buffer(this, 0, 0, Width, Height, buff->Get_Buffer(), buff->Get_Size()));
        Unlock();
    }
    return (return_code);
}

/***************************************************************************
 * GVPC::BLIT -- stub 1 to call curr graphic mode Blit to GVPC             *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
void GraphicViewPortClass::Blit(GraphicViewPortClass& dest,
                                int x_pixel,
                                int y_pixel,
                                int dx_pixel,
                                int dy_pixel,
                                int pixel_width,
                                int pixel_height,
                                bool trans)
{
    if (IsHardware && dest.IsHardware) {
        DD_Linear_Blit_To_Linear(dest,
                                 XPos + x_pixel,
                                 YPos + y_pixel,
                                 dest.Get_XPos() + dx_pixel,
                                 dest.Get_YPos() + dy_pixel,
                                 pixel_width,
                                 pixel_height,
                                 trans);
    } else {
        if (Lock()) {
            if (dest.Lock()) {
                Linear_Blit_To_Linear(&dest, x_pixel, y_pixel, dx_pixel, dy_pixel, pixel_width, pixel_height, trans);
                dest.Unlock();
            }
            Unlock();
        }
    }
}

/***************************************************************************
 * GVPC::BLIT -- Stub 2 to call curr graphic mode Blit to GVPC             *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
void GraphicViewPortClass::Blit(GraphicViewPortClass& dest, int dx, int dy, bool trans)
{
    if (IsHardware && dest.IsHardware) {
        DD_Linear_Blit_To_Linear(dest, XPos, YPos, dest.Get_XPos() + dx, dest.Get_YPos() + dy, Width, Height, trans);
    } else {
        if (Lock()) {
            if (dest.Lock()) {
                Linear_Blit_To_Linear(&dest, 0, 0, dx, dy, Width, Height, trans);
                dest.Unlock();
            }
            Unlock();
        }
    }
}

/***************************************************************************
 * GVPC::BLIT -- stub 3 to call curr graphic mode Blit to GVPC             *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
void GraphicViewPortClass::Blit(GraphicViewPortClass& dest, bool trans)
{
    if (IsHardware && dest.IsHardware) {
        DD_Linear_Blit_To_Linear(dest,
                                 XPos,
                                 YPos,
                                 dest.Get_XPos(),
                                 dest.Get_YPos(),
                                 MAX(Width, dest.Get_Width()),
                                 MAX(Height, dest.Get_Height()),
                                 trans);
    } else {
        if (Lock()) {
            if (dest.Lock()) {
                Linear_Blit_To_Linear(&dest, 0, 0, 0, 0, Width, Height, trans);
                dest.Unlock();
            }
            Unlock();
        }
    }
}

void GraphicViewPortClass::Blit(void* buffer, int x, int y, int w, int h)
{
    if (Lock()) {
        Buffer_To_Page(x, y, w, h, buffer, this);
        Unlock();
    }
}

/***************************************************************************
 * GVPC::SCALE -- stub 1 to call curr graphic mode Scale to GVPC           *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
void GraphicViewPortClass::Scale(GraphicViewPortClass& dest,
                                 int src_x,
                                 int src_y,
                                 int dst_x,
                                 int dst_y,
                                 int src_w,
                                 int src_h,
                                 int dst_w,
                                 int dst_h,
                                 bool trans,
                                 char* remap)
{
    if (Lock()) {
        if (dest.Lock()) {
            Linear_Scale_To_Linear(&dest, src_x, src_y, dst_x, dst_y, src_w, src_h, dst_w, dst_h, trans, remap);
            dest.Unlock();
        }
        Unlock();
    }
}

/***************************************************************************
 * GVPC::SCALE -- stub 2 to call curr graphic mode Scale to GVPC           *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
void GraphicViewPortClass::Scale(GraphicViewPortClass& dest,
                                 int src_x,
                                 int src_y,
                                 int dst_x,
                                 int dst_y,
                                 int src_w,
                                 int src_h,
                                 int dst_w,
                                 int dst_h,
                                 char* remap)
{
    if (Lock()) {
        if (dest.Lock()) {
            Linear_Scale_To_Linear(&dest, src_x, src_y, dst_x, dst_y, src_w, src_h, dst_w, dst_h, 0, remap);
            dest.Unlock();
        }
        Unlock();
    }
}

/***************************************************************************
 * GVPC::SCALE -- stub 3 to call curr graphic mode Scale to GVPC           *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
void GraphicViewPortClass::Scale(GraphicViewPortClass& dest, bool trans, char* remap)
{
    if (Lock()) {
        if (dest.Lock()) {
            Linear_Scale_To_Linear(&dest, 0, 0, 0, 0, Width, Height, dest.Get_Width(), dest.Get_Height(), trans, remap);
            dest.Unlock();
        }
        Unlock();
    }
}

/***************************************************************************
 * GVPC::SCALE -- stub 4 to call curr graphic mode Scale to GVPC           *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
void GraphicViewPortClass::Scale(GraphicViewPortClass& dest, char* remap)
{
    if (Lock()) {
        if (dest.Lock()) {
            Linear_Scale_To_Linear(&dest, 0, 0, 0, 0, Width, Height, dest.Get_Width(), dest.Get_Height(), false, remap);
            dest.Unlock();
        }
        Unlock();
    }
}

/***************************************************************************
 * GVPC::PRINT -- stub func to print a text string                         *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/17/1995 PWG : Created.                                             *
 *=========================================================================*/
unsigned long GraphicViewPortClass::Print(char const* str, int x, int y, int fcol, int bcol)
{
    unsigned long return_code = 0;
    if (Lock()) {
        return_code = (Buffer_Print(this, str, x, y, fcol, bcol));
        Unlock();
    }
    return (return_code);
}

#pragma warning(disable : 4996)

/***************************************************************************
 * GVPC::PRINT -- Stub function to print an integer                        *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *=========================================================================*/
unsigned long GraphicViewPortClass::Print(int num, int x, int y, int fcol, int bcol)
{
    char str[17];

    unsigned long return_code = 0;
    if (Lock()) {
        snprintf(str, sizeof(str), "%d", num);
        return_code = (Buffer_Print(this, str, x, y, fcol, bcol));
        Unlock();
    }
    return (return_code);
}

/***************************************************************************
 * GVPC::PRINT -- Stub function to print a short to a graphic viewport     *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *=========================================================================*/
unsigned long GraphicViewPortClass::Print(short num, int x, int y, int fcol, int bcol)
{
    char str[17];

    unsigned long return_code = 0;
    if (Lock()) {
        snprintf(str, sizeof(str), "%d", num);
        return_code = (Buffer_Print(this, str, x, y, fcol, bcol));
        Unlock();
    }
    return (return_code);
}

/***************************************************************************
 * GVPC::PRINT -- stub function to print a long on a graphic view port     *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *=========================================================================*/
unsigned long GraphicViewPortClass::Print(long num, int x, int y, int fcol, int bcol)
{
    char str[33];

    unsigned long return_code = 0;
    if (Lock()) {
        snprintf(str, sizeof(str), "%ld", num);
        return_code = (Buffer_Print(this, str, x, y, fcol, bcol));
        Unlock();
    }
    return (return_code);
}

/***************************************************************************
 * GVPC::DRAW_STAMP -- stub function to draw a tile on a graphic view port *
 *                     This version clips the tile to a window             *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *    07/31/1995 BWG : Created.                                            *
 *=========================================================================*/
void GraphicViewPortClass::Draw_Stamp(void const* icondata,
                                      int icon,
                                      int x_pixel,
                                      int y_pixel,
                                      void const* remap,
                                      int clip_window)
{
    if (Lock()) {
        Buffer_Draw_Stamp_Clip(this,
                               icondata,
                               icon,
                               x_pixel,
                               y_pixel,
                               remap,
                               WindowList[clip_window][WINDOWX],
                               WindowList[clip_window][WINDOWY],
                               WindowList[clip_window][WINDOWWIDTH],
                               WindowList[clip_window][WINDOWHEIGHT]);
    }
    Unlock();
}

/***************************************************************************
 * GVPC::DRAW_LINE -- Stub function to draw line in Graphic Viewport Class *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/16/1995 PWG : Created.                                             *
 *=========================================================================*/
void GraphicViewPortClass::Draw_Line(int sx, int sy, int dx, int dy, unsigned char color)
{
    if (Lock()) {
        Buffer_Draw_Line(this, sx, sy, dx, dy, color);
        Unlock();
    }
}

/***************************************************************************
 * GVPC::FILL_RECT -- Stub function to fill rectangle in a GVPC            *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/16/1995 PWG : Created.                                             *
 *=========================================================================*/
void GraphicViewPortClass::Fill_Rect(int sx, int sy, int dx, int dy, unsigned char color)
{
    if (AllowHardwareBlitFills && IsHardware && ((dx - sx) * (dy - sy) >= (32 * 32))
        && GraphicBuff->Get_DD_Surface()->IsReadyToBlit()) {
        Rect dest_rectangle(sx + XPos, sy + YPos, dx - sx, dy - sy);
        Rect self_rect(XPos, YPos, Width, Height);
        GraphicBuff->Get_DD_Surface()->FillRect(dest_rectangle.Intersect(self_rect), color);
    } else {
        if (Lock()) {
            Buffer_Fill_Rect(this, sx, sy, dx, dy, color);
            Unlock();
        }
    }
}

/***************************************************************************
 * GVPC::REMAP -- Stub function to remap a GVPC                            *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/16/1995 PWG : Created.                                             *
 *=========================================================================*/
void GraphicViewPortClass::Remap(int sx, int sy, int width, int height, void* remap)
{
    if (Lock()) {
        Buffer_Remap(this, sx, sy, width, height, remap);
        Unlock();
    }
}

/***************************************************************************
 * GVPC::REMAP -- Short form to remap an entire graphic view port          *
 *                                                                         *
 * INPUT:      BYTE * to the remap table to use                            *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/01/1994 PWG : Created.                                             *
 *=========================================================================*/
void GraphicViewPortClass::Remap(void* remap)
{
    if (Lock()) {
        Buffer_Remap(this, 0, 0, Width, Height, remap);
        Unlock();
    }
}

/***************************************************************************
 * Draw_Rect -- Draws a rectangle to the LogicPage.                        *
 *                                                                         *
 *    This routine will draw a rectangle to the LogicPage.  The rectangle  *
 *    doesn't have to be aligned on the vertical or horizontal axis.  In   *
 *    fact, it doesn't even have to be a rectangle.  The "square" can be   *
 *    skewed.                                                              *
 *                                                                         *
 * INPUT:   x1_pixel, y1_pixel   -- One corner.                            *
 *                                                                         *
 *          x2_pixel, y2_pixel   -- The other corner.                      *
 *                                                                         *
 *          color                -- The color to draw the lines.           *
 *                                                                         *
 * OUTPUT:  none                                                           *
 *                                                                         *
 * WARNINGS:   None, but the rectangle will be clipped to the current      *
 *             draw line clipping rectangle.                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   08/20/1993 JLB : Created.                                             *
 *=========================================================================*/
void GraphicViewPortClass::Draw_Rect(int x1_pixel, int y1_pixel, int x2_pixel, int y2_pixel, unsigned char color)
{
    Lock();
    Draw_Line(x1_pixel, y1_pixel, x2_pixel, y1_pixel, color);
    Draw_Line(x1_pixel, y2_pixel, x2_pixel, y2_pixel, color);
    Draw_Line(x1_pixel, y1_pixel, x1_pixel, y2_pixel, color);
    Draw_Line(x2_pixel, y1_pixel, x2_pixel, y2_pixel, color);
    Unlock();
}

void GraphicViewPortClass::Linear_Blit_To_Linear(GraphicViewPortClass* dst_vp,
                                                 int src_x,
                                                 int src_y,
                                                 int dst_x,
                                                 int dst_y,
                                                 int w,
                                                 int h,
                                                 int use_key)
{
    unsigned char* src = (unsigned char*)Offset;
    unsigned char* dst = (unsigned char*)(dst_vp->Offset);
    int src_pitch = (Pitch + XAdd + Width);
    int dst_pitch = (dst_vp->Pitch + dst_vp->XAdd + dst_vp->Width);

    if (src_x >= Width || src_y >= Height || dst_x >= dst_vp->Width || dst_y >= dst_vp->Height || h < 0 || w < 1) {
        return;
    }

    src_x = MAX(0, src_x);
    src_y = MAX(0, src_y);
    dst_x = MAX(0, dst_x);
    dst_y = MAX(0, dst_y);

    h = (dst_y + h) > dst_vp->Height ? dst_vp->Height - 1 - dst_y : h;
    w = (dst_x + w) > dst_vp->Width ? dst_vp->Width - 1 - dst_x : w;

    // move our pointers to the start locations
    src += src_x + src_y * src_pitch;
    dst += dst_x + dst_y * dst_pitch;

    // If src is before dst, we run the risk of overlapping memory regions so we
    // need to move src and dst to the last line and work backwards
    if (src < dst) {
        unsigned char* esrc = src + (h - 1) * src_pitch;
        unsigned char* edst = dst + (h - 1) * dst_pitch;
        if (use_key) {
            char key_colour = 0;
            while (h-- != 0) {
                // Can't optimize as we need to check every pixel against key colour :(
                for (int i = w - 1; i >= 0; --i) {
                    if (esrc[i] != key_colour) {
                        edst[i] = esrc[i];
                    }
                }

                edst -= dst_pitch;
                esrc -= src_pitch;
            }
        } else {
            while (h-- != 0) {
                memmove(edst, esrc, w);
                edst -= dst_pitch;
                esrc -= src_pitch;
            }
        }
    } else {
        if (use_key) {
            unsigned char key_colour = 0;
            while (h-- != 0) {
                // Can't optimize as we need to check every pixel against key colour :(
                for (int i = 0; i < w; ++i) {
                    if (src[i] != key_colour) {
                        dst[i] = src[i];
                    }
                }

                dst += dst_pitch;
                src += src_pitch;
            }
        } else {
            while (h-- != 0) {
                memmove(dst, src, w);
                dst += dst_pitch;
                src += src_pitch;
            }
        }
    }
}

// Note that this function does not generate pixel perfect results when compared to the ASM implementation
// It should not generate anything that is visibly different without doing side byt side comparisons however.
void GraphicViewPortClass::Linear_Scale_To_Linear(GraphicViewPortClass* dst_vp,
                                                  int src_x,
                                                  int src_y,
                                                  int dst_x,
                                                  int dst_y,
                                                  int src_width,
                                                  int src_height,
                                                  int dst_width,
                                                  int dst_height,
                                                  bool trans,
                                                  char* remap)
{
    // If there is nothing to scale, just return.
    if (src_width <= 0 || src_height <= 0 || dst_width <= 0 || dst_height <= 0) {
        return;
    }

    int src_x0 = src_x;
    int src_y0 = src_y;
    int dst_x0 = dst_x;
    int dst_y0 = dst_y;
    int dst_x1 = dst_width + dst_x;
    int dst_y1 = dst_height + dst_y;

    // These ifs are all for clipping purposes incase coords are outside
    // the expected area.
    if (src_x < 0) {
        src_x0 = 0;
        dst_x0 = dst_x + ((dst_width * -src_x) / src_width);
    }

    if (src_y < 0) {
        src_y0 = 0;
        dst_y0 = dst_y + ((dst_height * -src_y) / src_height);
    }

    if (src_x + src_width > Get_Width() + 1) {
        dst_x1 = dst_x + (dst_width * (Get_Width() - src_x) / src_width);
    }

    if (src_y + src_height > Get_Height() + 1) {
        dst_y1 = dst_y + (dst_height * (Get_Height() - src_y) / src_height);
    }

    if (dst_x0 < 0) {
        dst_x0 = 0;
        src_x0 = src_x + ((src_width * -dst_x) / dst_width);
    }

    if (dst_y0 < 0) {
        dst_y0 = 0;
        src_y0 = src_y + ((src_height * -dst_y) / dst_height);
    }

    if (dst_x1 > dst_vp->Get_Width() + 1) {
        dst_x1 = dst_vp->Get_Width();
    }

    if (dst_y1 > dst_vp->Get_Height() + 1) {
        dst_y1 = dst_vp->Get_Height();
    }

    if (dst_y0 > dst_y1 || dst_x0 > dst_x1) {
        return;
    }

    char* src = src_y0 * (Get_Pitch() + Get_XAdd() + Get_Width()) + src_x0 + reinterpret_cast<char*>(Get_Offset());
    char* dst = dst_y0 * (dst_vp->Get_Pitch() + dst_vp->Get_XAdd() + dst_vp->Get_Width()) + dst_x0
                + reinterpret_cast<char*>(dst_vp->Get_Offset());
    dst_x1 -= dst_x0;
    dst_y1 -= dst_y0;
    int x_ratio = ((src_width << 16) / dst_x1) + 1;
    int y_ratio = ((src_height << 16) / dst_y1) + 1;

    // trans basically means do we skip index 0 entries, thus treating them as
    // transparent?
    if (trans) {
        if (remap != nullptr) {
            for (int i = 0; i < dst_y1; ++i) {
                char* d = dst + i * (dst_vp->Get_Pitch() + dst_vp->Get_XAdd() + dst_vp->Get_Width());
                char* s = src + ((i * y_ratio) >> 16) * (Get_Pitch() + Get_XAdd() + Get_Width());
                int xrat = 0;

                for (int j = 0; j < dst_x1; ++j) {
                    char tmp = s[xrat >> 16];

                    if (tmp != 0) {
                        *d = (remap)[tmp];
                    }

                    ++d;
                    xrat += x_ratio;
                }
            }
        } else {
            for (int i = 0; i < dst_y1; ++i) {
                char* d = dst + i * (dst_vp->Get_Pitch() + dst_vp->Get_XAdd() + dst_vp->Get_Width());
                char* s = src + ((i * y_ratio) >> 16) * (Get_Pitch() + Get_XAdd() + Get_Width());
                int xrat = 0;

                for (int j = 0; j < dst_x1; ++j) {
                    char tmp = s[xrat >> 16];

                    if (tmp != 0) {
                        *d = tmp;
                    }

                    ++d;
                    xrat += x_ratio;
                }
            }
        }
    } else {
        if (remap != nullptr) {
            for (int i = 0; i < dst_y1; ++i) {
                char* d = dst + i * (dst_vp->Get_Pitch() + dst_vp->Get_XAdd() + dst_vp->Get_Width());
                char* s = src + ((i * y_ratio) >> 16) * (Get_Pitch() + Get_XAdd() + Get_Width());
                int xrat = 0;

                for (int j = 0; j < dst_x1; ++j) {
                    *d++ = (remap)[s[xrat >> 16]];
                    xrat += x_ratio;
                }
            }
        } else {
            for (int i = 0; i < dst_y1; ++i) {
                char* d = dst + i * (dst_vp->Get_Pitch() + dst_vp->Get_XAdd() + dst_vp->Get_Width());
                char* s = src + ((i * y_ratio) >> 16) * (Get_Pitch() + Get_XAdd() + Get_Width());
                int xrat = 0;

                for (int j = 0; j < dst_x1; ++j) {
                    *d++ = s[xrat >> 16];
                    xrat += x_ratio;
                }
            }
        }
    }
}

void GraphicViewPortClass::Fat_Put_Pixel(unsigned int x, unsigned int y, unsigned char color, unsigned int size)
{
    if (size == 0) {
        return;
    }

    if (y >= (unsigned)Height || x >= Width) {
        return;
    }

    int pitch = Get_Full_Pitch();
    char* buf = reinterpret_cast<char*>(x + pitch * y + Offset);
    int w = size;

    while (size--) {
        memset(buf, color, w);
        buf += pitch;
    }
}
