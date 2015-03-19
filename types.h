/*
********************************************************************
(c) MSU Video Group 2003-2010, http://compression.ru/video/
This source code is property of MSU Graphics and Media Lab

This code can be distributed and used only with WRITTEN PERMISSION.
********************************************************************
*/


#ifndef _TYPES_H_
#define _TYPES_H_

#include "filter.h"
#include "ScriptInterpreter.h"
#include "ScriptError.h"
#include "ScriptValue.h"
#include "resource.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <commctrl.h>

#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>


#define BORDER 48
#define MAX_MOTION 32
#define SPLIT_TRESH 1000


struct Point {
	int x, y;
};

typedef enum {sd_none, sd_up, sd_l, sd_upl} Shift_Dir;

struct MV {
	MV();
	int x, y;
	Shift_Dir dir;
	long error;
	bool splitted;
	MV* sub[4];
	MV operator += (MV vector);
	MV operator -= (MV vector);
	MV operator + (MV vector);
	MV operator - (MV vector);
	MV operator /= ( int );
	MV operator *= ( int );
};

//Deinterlace functionality
void HalfpixelShift(BYTE *field, int width, int height, bool shift_up);
void HalfpixelShift(short *field, int width, int height, bool shift_up);
void HalfpixelShift(Pixel32 *field, int width, int height, bool shift_up);
void HalfpixelShiftHorz(BYTE *field, int width, int height, bool shift_up);
void HalfpixelShiftHorz(short *field, int width, int height, bool shift_up);
void HalfpixelShiftHorz(Pixel32 *field, int width, int height, bool shift_up);

//ME functionality
void MEFunction(BYTE* cur_Y_frame, BYTE* prev_Y_frame, BYTE* prev_Y_frame_up, BYTE* prev_Y_frame_l, BYTE* prev_Y_frame_upl, int width, int height, MV *MVectors, BYTE quality, bool use_half_pixel);
void MEStart( int width, int heigth, BYTE quality );
void MEEnd();
long GetErrorSAD_16x16(const BYTE* block1, const BYTE* block2, const int stride);
long GetErrorSAD_8x8(const BYTE* block1, const BYTE* block2, const int stride);

typedef enum { den } Method;
typedef unsigned char BYTE;

struct MFD {
	//specific

	LARGE_INTEGER tick1_process_timer,tick2_process_timer,freq_process_timer;
	double timer;

	bool show_vectors;
	bool show_res_after;
	bool show_res_before;
	bool show_MC;
	bool show_nothing;
	bool show_source;
	Method method;
	bool log_need;
	char log_file[50];
	bool log_time_file;
	bool log_vectors;
	char log_vectors_file[50];
	bool read_motions;

	//controls output
	BYTE quality;
	bool use_half_pixel;
	bool work_in_RGB;

	//buffers
	BYTE* prev_Y;
	BYTE* prev_Y_up;
	BYTE* prev_Y_upl;
	BYTE* prev_Y_l;
	short* prev_U;
	short* prev_U_up;
	short* prev_U_upl;
	short* prev_U_l;
	short* prev_U_MC;
	short* prev_V;
	short* prev_V_up;
	short* prev_V_upl;
	short* prev_V_l;
	short* prev_V_MC;
	BYTE* prev_Y_MC;
	BYTE* cur_Y;
	short* cur_U;
	short* cur_V;
	Pixel32* output_buf;
	Pixel32* prev_RGB;
	Pixel32* prev_RGB_up;
	Pixel32* prev_RGB_upl;
	Pixel32* prev_RGB_l;

	int offset;
	double noise;
	int frame_no;
	//direction of the halfpixel shift
	bool shift_up;
	//motion vectors for current frame (not determined before special function call)
	MV *MVectors;
	//dimensions of extended ( with BORDERs ) field
	int ext_w, ext_h, ext_size;
	int width, height, size;
	//size of frame in blocks
	int num_blocks_vert, num_blocks_hor;
};


#endif