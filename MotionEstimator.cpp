/*
********************************************************************
(c) MSU Video Group 2003-2010, http://compression.ru/video/
This source code is property of MSU Graphics and Media Lab

This code can be distributed and used only with WRITTEN PERMISSION.
********************************************************************
*/

#include <unordered_map>

#include "MotionEstimator.h"
#include "Metric.h"

/// Maximum motion vector length per axis
const int MotionEstimator::MAX_MOTION = 32;

/// Border size
const int MotionEstimator::BORDER = 48;

/// Error threshold for motiov vector splitting
const int MotionEstimator::SPLIT_TRESH = 1000;

/// Constructor
MotionEstimator::MotionEstimator(const int width,
                                 const int height,
                                 const BYTE quality):
    width(width),
    height(height),
    quality(quality),
    num_blocks_hor((width + 15) / 16),
    num_blocks_vert((height + 15) / 16),
    wext(width + BORDER * 2),
    first_row_offset(wext * BORDER + BORDER)
{
    // PUT YOUR CODE HERE
}

/// Destructor
MotionEstimator::~MotionEstimator()
{
    // PUT YOUR CODE HERE
}

void
MotionEstimator::Estimate(const BYTE *cur_Y_frame,
                          const BYTE *prev_Y_frame,
                          const BYTE *prev_Y_frame_up,
                          const BYTE *prev_Y_frame_l,
                          const BYTE *prev_Y_frame_upl,
                          MV *MVectors,
                          const bool use_half_pixel)
{
    // Map half-pixel shifts to shifted frames
    const std::unordered_map<Shift_Dir, const BYTE *> shifted_frames
    {
        {sd_up,  prev_Y_frame_up},
        {sd_l,   prev_Y_frame_l},
        {sd_upl, prev_Y_frame_upl}
    };

    // alias for MAX_MOTION
    const int mm = MAX_MOTION;

    // Estimate motion
	for (int i = 0; i < num_blocks_vert; i++)
	{
		for (int j = 0; j < num_blocks_hor; j++)
		{
            const int block_id = i * num_blocks_hor + j;
			int vert_offset = i * 16 * wext + first_row_offset;
			int hor_offset = j * 16;
            const BYTE *cur = cur_Y_frame + vert_offset + hor_offset;
            const BYTE *prev = prev_Y_frame + vert_offset + hor_offset;

            long min_error = MAXLONG;
            long error;
            MV prob_motion_vector;

			// PUT YOUR CODE HERE

            // Brute force
			for (int k = -mm; k < mm; k++)
			{
				for (int l = -mm; l < mm; l++)
				{
                    error = GetErrorSAD_16x16(cur, prev + k * wext + l, wext);
					if (error < min_error)
					{
						prob_motion_vector.x = l;
						prob_motion_vector.y = k;
						prob_motion_vector.dir = sd_none;
						prob_motion_vector.error = error;
						min_error = error;
					}
				}
			}

            // Use half-pixel precision
			if (use_half_pixel)
			{
                for (const auto dir_image : shifted_frames)
                {
                    const auto direction = dir_image.first;
                    prev = dir_image.second + vert_offset + hor_offset;
                    for (int k = -mm; k < mm; k++)
                    {
                        for (int l = -mm; l < mm; l++)
                        {
                            error = GetErrorSAD_16x16(cur, prev + k * wext + l, wext);
                            if (error < min_error)
                            {
                                prob_motion_vector.x = l;
                                prob_motion_vector.y = k;
                                prob_motion_vector.dir = direction;
                                prob_motion_vector.error = error;
                                min_error = error;
                            }
                        }
                    }
                }
			}
			MVectors[block_id] = prob_motion_vector;
		}
	}
}
