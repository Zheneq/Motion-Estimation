#ifndef MOTIONESTIMATION_HALFPIXEL_H_
#define MOTIONESTIMATION_HALFPIXEL_H_

#include "types.h"

// Deinterlace functionality
void HalfpixelShift(BYTE *field, int width, int height, bool shift_up);
void HalfpixelShift(short *field, int width, int height, bool shift_up);
void HalfpixelShift(Pixel32 *field, int width, int height, bool shift_up);
void HalfpixelShiftHorz(BYTE *field, int width, int height, bool shift_up);
void HalfpixelShiftHorz(short *field, int width, int height, bool shift_up);
void HalfpixelShiftHorz(Pixel32 *field, int width, int height, bool shift_up);

#endif
