/*
********************************************************************
(c) MSU Video Group 2003-2010, http://compression.ru/video/
This source code is property of MSU Graphics and Media Lab

This code can be distributed and used only with WRITTEN PERMISSION.
********************************************************************
*/

#include "types.h"


#define GetError GetErrorSAD_16x16

void MEFunction(BYTE* cur_Y_frame, BYTE* prev_Y_frame, BYTE* prev_Y_frame_up, BYTE* prev_Y_frame_l, BYTE* prev_Y_frame_upl, int width, int height, MV *MVectors, BYTE quality, bool use_half_pixel)
{
	int num_blocks_vert = (height + 15) >> 4, num_blocks_hor = (width + 15) >> 4;
	int wext = width + BORDER * 2;
	int i, j, k, l, temp;
	int vert_offset, hor_offset, first_row_offset = wext * BORDER + BORDER;
	long min_error, error;
	MV prob_motion_vector;
	BYTE *cur, *prev;
	int mm = MAX_MOTION;

	for( i = 0; i < num_blocks_vert; i++ )
	{
		for( j = 0; j < num_blocks_hor; j++ )
		{
			vert_offset = ( i << 4 ) * wext + first_row_offset;
			hor_offset = (j << 4);
			min_error = MAXLONG;
			mm = MAX_MOTION;
			cur = cur_Y_frame + vert_offset + hor_offset;
			prev = prev_Y_frame + vert_offset + hor_offset;

			// INSERT YOUR CODE HERE

			for( k = -mm; k < mm; k++ )
			{
				temp = k * wext;
				for( l = -mm; l < mm; l++ )
				{
					error = GetError( cur, prev + temp + l, wext );
					if( error < min_error )
					{
						prob_motion_vector.x = l;
						prob_motion_vector.y = k;
						prob_motion_vector.dir = sd_none;
						prob_motion_vector.error = error;
						min_error = error;
					}
				}
			}
			if(use_half_pixel)
			{
				prev = prev_Y_frame_up + vert_offset + hor_offset;
				//searching for the most probable motion vector
				for( k = -mm; k < mm; k++ )
				{
					temp = k * wext;
					for( l = -mm; l < mm; l++ )
					{
						error = GetError( cur, prev + temp + l, wext );
						if( error < min_error )
						{
							prob_motion_vector.x = l;
							prob_motion_vector.y = k;
							prob_motion_vector.dir = sd_up;
							prob_motion_vector.error = error;
							min_error = error;
						}
					}
				}
				prev = prev_Y_frame_l + vert_offset + hor_offset;
				for( k = -mm; k < mm; k++ )
				{
					temp = k * wext;
					for( l = -mm; l < mm; l++ )
					{
						error = GetError( cur, prev + temp + l, wext );
						if( error < min_error )
						{
							prob_motion_vector.x = l;
							prob_motion_vector.y = k;
							prob_motion_vector.dir = sd_l;
							prob_motion_vector.error = error;
							min_error = error;
						}
					}
				}
				prev = prev_Y_frame_upl + vert_offset + hor_offset;
				for( k = -mm; k < mm; k++ )
				{
					temp = k * wext;
					for( l = -mm; l < mm; l++ )
					{
						error = GetError( cur, prev + temp + l, wext );
						if( error < min_error )
						{
							prob_motion_vector.x = l;
							prob_motion_vector.y = k;
							prob_motion_vector.dir = sd_upl;
							prob_motion_vector.error = error;
							min_error = error;
						}
					}
				}
			} // halfpixel
			MVectors[ i * num_blocks_hor + j ] = prob_motion_vector;
			if(prob_motion_vector.error > SPLIT_TRESH )
			{
				MVectors[ i * num_blocks_hor + j ].splitted = true;
				for(int h = 0; h < 4; h++)
				{
					if(MVectors[ i * num_blocks_hor + j ].sub[h] == NULL)
					{
						MVectors[ i * num_blocks_hor + j ].sub[h] = new MV;
					}
					prob_motion_vector.x = 0;
					prob_motion_vector.y = 0;
					vert_offset = (( i << 4 ) + ((h > 1)? 8 : 0))* wext + first_row_offset;
					hor_offset = (j << 4) + ((h & 1)? 8 : 0);
					min_error = MAXLONG;
					cur = cur_Y_frame + vert_offset + hor_offset;

					prev = prev_Y_frame + vert_offset + hor_offset;
					for( k = -mm; k < mm; k++ )
					{
						temp = k * wext;
						for( l = -mm; l < mm; l++ )
						{
							error = GetErrorSAD_8x8( cur, prev + temp + l, wext );
							if( error < min_error )
							{
								prob_motion_vector.x = l;
								prob_motion_vector.y = k;
								prob_motion_vector.dir = sd_none;
								prob_motion_vector.error = error;
								min_error = error;
							}
						}
					}

					if(use_half_pixel)
					{
						prev = prev_Y_frame_up + vert_offset + hor_offset;
						for( k = -mm; k < mm; k++ )
						{
							temp = k * wext;
							for( l = -mm; l < mm; l++ )
							{
								error = GetErrorSAD_8x8( cur, prev + temp + l, wext );
								if( error < min_error )
								{
									prob_motion_vector.x = l;
									prob_motion_vector.y = k;
									prob_motion_vector.dir = sd_up;
									prob_motion_vector.error = error;
									min_error = error;
								}
							}
						}

						prev = prev_Y_frame_l + vert_offset + hor_offset;
						for( k = -mm; k < mm; k++ )
						{
							temp = k * wext;
							for( l = -mm; l < mm; l++ )
							{
								error = GetErrorSAD_8x8( cur, prev + temp + l, wext );
								if( error < min_error )
								{
									prob_motion_vector.x = l;
									prob_motion_vector.y = k;
									prob_motion_vector.dir = sd_l;
									prob_motion_vector.error = error;
									min_error = error;
								}
							}
						}

						prev = prev_Y_frame_upl + vert_offset + hor_offset;
						for( k = -mm; k < mm; k++ )
						{
							temp = k * wext;
							for( l = -mm; l < mm; l++ )
							{
								error = GetErrorSAD_8x8( cur, prev + temp + l, wext );
								if( error < min_error )
								{
									prob_motion_vector.x = l;
									prob_motion_vector.y = k;
									prob_motion_vector.dir = sd_upl;
									prob_motion_vector.error = error;
									min_error = error;
								}
							}
						}
					}

					MVectors[ i * num_blocks_hor + j ].sub[h]->x = prob_motion_vector.x;
					MVectors[ i * num_blocks_hor + j ].sub[h]->y = prob_motion_vector.y;
					MVectors[ i * num_blocks_hor + j ].sub[h]->dir = prob_motion_vector.dir;
					MVectors[ i * num_blocks_hor + j ].sub[h]->error = prob_motion_vector.error;
					MVectors[ i * num_blocks_hor + j ].sub[h]->splitted = false;
				}
				min_error = MVectors[ i * num_blocks_hor + j ].error;
				if((MVectors[ i * num_blocks_hor + j ].sub[0]->error
					+MVectors[ i * num_blocks_hor + j ].sub[1]->error
					+MVectors[ i * num_blocks_hor + j ].sub[2]->error
					+MVectors[ i * num_blocks_hor + j ].sub[3]->error > 
					MVectors[ i * num_blocks_hor + j ].error * 0.7))
				{
					MVectors[ i * num_blocks_hor + j ].splitted = false;
				}
			} // split

			// END OF MOTION VECTORS SEARCH CODE

		} // vector search
	}

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

