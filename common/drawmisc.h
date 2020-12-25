#ifndef DRAWMISC_H
#define DRAWMISC_H

class GraphicViewPortClass;

extern unsigned char CurrentPalette[768];
extern unsigned char PaletteTable[1024];

void Buffer_Fill_Rect(void* thisptr, int sx, int sy, int dx, int dy, unsigned char color);
void Buffer_Clear(void* this_object, unsigned char color);

extern int LastIconset;
extern int StampPtr;
extern int IsTrans;
extern int MapPtr;
extern int IconWidth;
extern int IconHeight;
extern int IconSize;
extern int IconCount;

void Init_Stamps(unsigned int icondata);

void Fat_Put_Pixel(int x, int y, int value, int size, GraphicViewPortClass& gvp);

#endif
