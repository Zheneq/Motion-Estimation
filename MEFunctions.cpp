/*
********************************************************************
(c) MSU Video Group 2003-2010, http://compression.ru/video/
This source code is property of MSU Graphics and Media Lab

This code can be distributed and used only with WRITTEN PERMISSION.
********************************************************************
*/

#include "types.h"
#include <process.h>

void MEDiam(BYTE* cur_Y_frame, BYTE* prev_Y_frame, BYTE* prev_Y_frame_up, BYTE* prev_Y_frame_l, BYTE* prev_Y_frame_upl, int width, int height, MV *MVectors, BYTE quality, bool use_half_pixel);

void MEFunction(BYTE* cur_Y_frame, BYTE* prev_Y_frame, BYTE* prev_Y_frame_up, BYTE* prev_Y_frame_l, BYTE* prev_Y_frame_upl, int width, int height, MV *MVectors, BYTE quality, bool use_half_pixel)
{
	MEDiam(cur_Y_frame, prev_Y_frame, prev_Y_frame_up, prev_Y_frame_l, prev_Y_frame_upl, width, height, MVectors, quality, use_half_pixel);
}

void MEStart( int width, int height, BYTE quality )
{
	//this function is called once for filter chain startup
	//place here code for all memory allocations and initializations you want to do
}

void MEEnd()
{
	//this function is opposite to YourStart()
	//place here code to release resources if necessary
}


//========================================================
// 
//========================================================

struct pt { int x, y; };
/*struct pt VeryLargeDiamondPattern[] = { {0, -3}, {1, -2}, {2, -1}, {3, 0}, {2, 1}, {1, 2}, {0, 3},
{-1, 2}, {-2, 1}, {-3, 0}, {-2, -1}, {-1, -2} };//*/
struct pt LargeDiamondPattern[] = { {0, -2}, {1, -1}, {2, 0}, {1, 1}, {0, 2}, {-1, 1}, {-2, 0}, {-1, -1} };
struct pt SmallDiamondPattern[] = { {0, -1}, {1, 0}, {0, 1}, {-1, 0} };

struct MESearchParam
{
	MV        *prob_motion_vector;
	int        wext,
	           num_blocks_vert,
	           num_blocks_hor,
			   steps;
	BYTE      *cur_Y_frame,
	          *prev_Y_frame_s;
	Shift_Dir  dir;
};

unsigned int __stdcall MESearchBest(void *p)
{
	int i;
	int *pr = (int*)p;
	MV **pra = (MV **)p;
	MV *t1, *t2;
	for(i=0; i<pr[5]; ++i)
	{
		t1 = pra[1][i].error < pra[2][i].error ? pra[1] + i : pra[2] + i;
		t2 = pra[3][i].error < pra[4][i].error ? pra[3] + i : pra[4] + i;
		pra[0][i] = t1->error < t2->error ? *t1 : *t2;
	}
	return 0;
}

//========================================================
// ÐÎÌÁÎÂÛÉ ÏÎÈÑÊ
//========================================================

unsigned int __stdcall MEDiamSub(void *p)
{
	MESearchParam *pr = (MESearchParam*)p;
	int i, j, k, l, n, temp;
	int vert_offset, hor_offset, first_row_offset = pr->wext * BORDER + BORDER;
	long error, min_error;
	BYTE *cur, *prev;
	bool changed;
	int basex, basey;
	MV prob_motion_vector;

	for( i = 0; i < pr->num_blocks_vert; i++ )
	{
		vert_offset = ((i << BLOCKSIZELB) - 2) * pr->wext + first_row_offset;
		for( j = 0; j < pr->num_blocks_hor; j++ )
		{
			hor_offset  = (j << BLOCKSIZELB) - 2 ;
			basex = basey = 0;
			cur  = pr->cur_Y_frame    + vert_offset + hor_offset;
			prev = pr->prev_Y_frame_s + vert_offset + hor_offset;
			prob_motion_vector.dir = pr->dir;

			min_error   = GetError( cur, prev, pr->wext );
			prob_motion_vector.x = 0;
			prob_motion_vector.y = 0;
			prob_motion_vector.error = min_error;

			for( k = 0; k < pr->steps; k++ )
			{
				changed = false;
				for( l = 0; l < 12; l++ )
				{
					int tmpx = LargeDiamondPattern[l].x + basex, tmpy = LargeDiamondPattern[l].y + basey;
					error = GetError( cur, prev + (tmpy * pr->wext) + (tmpx), pr->wext );
					if( error < min_error )
					{
						changed = true;
						prob_motion_vector.x     = tmpx;
						prob_motion_vector.y     = tmpy;
						prob_motion_vector.error = error;
						min_error = error;
					}
				}
				if( !changed ) break;
				basex = prob_motion_vector.x;
				basey = prob_motion_vector.y;
			}
			for( k = 0; k < 2; k++ ) // èëè õâàòèò è îäíîãî?
			{
				changed = false;
				for( l = 0; l < 4; l++ )
				{
					error = GetError( cur, prev + (SmallDiamondPattern[l].y + basey) * pr->wext + (SmallDiamondPattern[l].x + basex), pr->wext );
					if( error < min_error )
					{
						changed = true;
						prob_motion_vector.x     = SmallDiamondPattern[l].x + basex;
						prob_motion_vector.y     = SmallDiamondPattern[l].y + basey;
						prob_motion_vector.error = error;
						min_error = error;
					}
				}
				if( !changed ) break;
				basex = prob_motion_vector.x;
				basey = prob_motion_vector.y;
			}
			pr->prob_motion_vector[i * pr->num_blocks_hor + j] = prob_motion_vector;
		}
	}
	return 0;
}

void MEDiam(BYTE* cur_Y_frame, BYTE* prev_Y_frame, BYTE* prev_Y_frame_up, BYTE* prev_Y_frame_l, BYTE* prev_Y_frame_upl, int width, int height, MV *MVectors, BYTE quality, bool use_half_pixel)
{
	int num_blocks_vert = (height + BLOCKSIZE-1) >> BLOCKSIZELB, num_blocks_hor = (width + BLOCKSIZE-1) >> BLOCKSIZELB;
	int wext = width + BORDER * 2;
	int i, j, k, l, temp;
	int vert_offset, hor_offset, first_row_offset = wext * BORDER + BORDER;
	long min_error, error;
	MV prob_motion_vector;
	MV *mv0, *mv1, *mv2, *mv3;
	BYTE *cur, *prev;
	int mm = MAX_MOTION;
	if(use_half_pixel)
	{
		temp = num_blocks_vert * num_blocks_hor;
		MESearchParam pr[4];
		mv0 = new MV[temp]; 
		mv1 = new MV[temp]; 
		mv2 = new MV[temp];
		mv3 = new MV[temp];
		pr[0].wext = wext;
	    pr[0].num_blocks_vert = num_blocks_vert;
	    pr[0].num_blocks_hor = num_blocks_hor;
		pr[0].cur_Y_frame = cur_Y_frame;
		pr[0].steps = quality / 10;
		memcpy(pr+1, pr, sizeof(MESearchParam));
		memcpy(pr+2, pr, 2*sizeof(MESearchParam));
	    pr[0].prev_Y_frame_s = prev_Y_frame;
		pr[0].dir = sd_none;
		pr[0].prob_motion_vector = mv0;
	    pr[1].prev_Y_frame_s = prev_Y_frame_up;
		pr[1].dir = sd_up;
		pr[1].prob_motion_vector = mv1;
	    pr[2].prev_Y_frame_s = prev_Y_frame_l;
		pr[2].dir = sd_l;
		pr[2].prob_motion_vector = mv2;
	    pr[3].prev_Y_frame_s = prev_Y_frame_upl;
		pr[3].dir = sd_upl;
		pr[3].prob_motion_vector = mv3;
		
		HANDLE hdls[3];
		hdls[0] = (HANDLE)_beginthreadex(NULL, 0, MEDiamSub, (void*)&pr[0], 0, NULL);
		hdls[1] = (HANDLE)_beginthreadex(NULL, 0, MEDiamSub, (void*)&pr[1], 0, NULL);
		hdls[2] = (HANDLE)_beginthreadex(NULL, 0, MEDiamSub, (void*)&pr[2], 0, NULL);
		MEDiamSub(&pr[3]);
		WaitForMultipleObjects(3, hdls, true, 0xFFFFFFFF);
		
		temp = (num_blocks_vert * num_blocks_hor) >> 1;
		void* tmp1[6] = {MVectors, mv0, mv1, mv2, mv3, (void*)(temp)};
		void* tmp2[6] = {MVectors+temp, mv0+temp, mv1+temp, mv2+temp, mv3+temp, (void*)(temp + (num_blocks_vert&1)|(num_blocks_hor&1))};
		hdls[0] = (HANDLE)_beginthreadex(NULL, 0, MESearchBest, (void*)tmp1, 0, NULL);
		MESearchBest(tmp2);
		WaitForSingleObject(hdls[0], 0xFFFFFFFF);
		delete[] mv0;
		delete[] mv1;
		delete[] mv2;
		delete[] mv3;
	}
	else
	{
		MESearchParam pr;
		pr.wext = wext;
	    pr.num_blocks_vert    = num_blocks_vert;
	    pr.num_blocks_hor     = num_blocks_hor;
		pr.cur_Y_frame        = cur_Y_frame;
	    pr.prev_Y_frame_s     = prev_Y_frame;
		pr.dir                = sd_none;
		pr.prob_motion_vector = MVectors;
		pr.steps              = quality / 10;
		MEDiamSub(&pr);
	}
}