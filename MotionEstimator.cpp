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

#define BLOCK_16x16

#ifdef BLOCK_16x16
	#define BLOCK_SIZE 16
#else
	#define BLOCK_SIZE 8
#endif


inline long MotionEstimator::_GetError(const BYTE *block1, const BYTE *block2, const int stride)
{
#ifdef BLOCK_16x16
	return GetErrorSAD_16x16(block1, block2, stride);
#else
	return GetErrorSAD_8x8(block1, block2, stride);
#endif
}

long MotionEstimator::GetError(const BYTE *block1, const BYTE *block2, const int stride)
{
	static std::unordered_map<const BYTE *, long> cache;
	static const BYTE *cached = NULL;
	
	if (block1 == cached)
	{
		for (auto it = cache.begin(); it != cache.end(); ++it)
		{
			if (it->first == block2)
				return it->second;
		}
	}
	else
	{
		cache.clear();
		cached = block1;
	}

	--Quota;
	return cache[block1] = _GetError(block1, block2, stride);
}

struct pt { int x, y; };
// Diamond Patterns
struct pt SDP[] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
struct pt LDP[] = { { 0, -2 }, { 1, -1 }, { 2, 0 }, { 1, 1 }, { 0, 2 }, { -1, 1 }, { -2, 0 }, { -1, -1 } };
struct pt VLDP[] = { { 0, -3 }, { 1, -2 }, { 2, -1 }, { 3, 0 }, { 2, 1 }, { 1, 2 }, { 0, 3 },
{ -1, 2 }, { -2, 1 }, { -3, 0 }, { -2, -1 }, { -1, -2 } };


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
	num_blocks_hor((width + BLOCK_SIZE - 1) / BLOCK_SIZE),
	num_blocks_vert((height + BLOCK_SIZE - 1) / BLOCK_SIZE),
    wext(width + BORDER * 2),
    first_row_offset(wext * BORDER + BORDER),
	PrevPredictions(NULL)
{
	Predictions = new PD[num_blocks_vert * num_blocks_hor];
}

/// Destructor
MotionEstimator::~MotionEstimator()
{
	if (Predictions) delete[] Predictions;
	if (PrevPredictions) delete[] PrevPredictions;
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
//	if(quality == 100)
//		BruteForce(cur_Y_frame, prev_Y_frame, prev_Y_frame_up, prev_Y_frame_l, prev_Y_frame_upl, MVectors, use_half_pixel);
//	else
		ARPS(cur_Y_frame, prev_Y_frame, prev_Y_frame_up, prev_Y_frame_l, prev_Y_frame_upl, MVectors, use_half_pixel);

}

void
MotionEstimator::BruteForce(const BYTE *cur_Y_frame,
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
		{ sd_up, prev_Y_frame_up },
		{ sd_l, prev_Y_frame_l },
		{ sd_upl, prev_Y_frame_upl }
	};

	// alias for MAX_MOTION
	const int mm = MAX_MOTION;

	// Estimate motion
	for (int i = 0; i < num_blocks_vert; i++)
	{
		for (int j = 0; j < num_blocks_hor; j++)
		{
			const int block_id = i * num_blocks_hor + j;
			int vert_offset = i * BLOCK_SIZE * wext + first_row_offset;
			int hor_offset = j * BLOCK_SIZE;
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

void
MotionEstimator::ARPS(const BYTE *cur_Y_frame,
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
		{ sd_up, prev_Y_frame_up },
		{ sd_l, prev_Y_frame_l },
		{ sd_upl, prev_Y_frame_upl }
	};


	// Moving predictions
	if (PrevPredictions) delete[] PrevPredictions;
	PrevPredictions = Predictions;
	Predictions = new PD[num_blocks_vert * num_blocks_hor];

	// Estimate motion
	for (int i = 0; i < num_blocks_vert; i++)
	{
		for (int j = 0; j < num_blocks_hor; j++)
		{
			const int block_id = i * num_blocks_hor + j;
			int vert_offset = i * BLOCK_SIZE * wext + first_row_offset;
			int hor_offset = j * BLOCK_SIZE;
			const BYTE *cur = cur_Y_frame + vert_offset + hor_offset;
			const BYTE *prev = prev_Y_frame + vert_offset + hor_offset;

			long min_error = MAXLONG;
			long error;
			MV prob_motion_vector;
			prob_motion_vector.dir = sd_none;

			Quota = 5 + quality / 2;

			// Checking predictions
			bool NoGoodPredictions = true;
/*
//			if (PrevPredictions)
//			{
				PD &CurPred = PrevPredictions[block_id];

				for (auto it = CurPred.pred.begin(); it != CurPred.pred.end(); ++it)
				{
					error = GetError(cur, prev + it->vector.y * wext + it->vector.x, wext);
					if (error < min_error)
					{
						prob_motion_vector = it->vector;
						prob_motion_vector.error = error;
						min_error = error;
					}
					if (NoGoodPredictions && error < 2 * it->vector.error)
						NoGoodPredictions = false;
				}
//			}
//*/
			if (NoGoodPredictions)
			{
				// Don't have predictions, go Diamond Search
				min_error = prob_motion_vector.error = GetError(cur, prev, wext);

				const int ERROR_THRESHOLD = 10;
/*
				for (int k = MAX_MOTION / 4; k > 3; k /= 2)
				for (int l = -k; l <= k; l += 2*k)
				for (int m = -k; m <= k; m += 2*k)
				{
					MV origin = prob_motion_vector;

					error = GetError(cur, prev + (l + origin.y) * wext + (m + origin.x), wext);
					if (error < min_error)
					{
						prob_motion_vector.x = origin.x + m;
						prob_motion_vector.y = origin.y + l;
						prob_motion_vector.error = error;
						min_error = error;
					}

				}
//*/
				bool ContinueDP = min_error > ERROR_THRESHOLD;
				while (ContinueDP && Quota >= ARRAYSIZE(SDP))
				{
					ContinueDP = false;
					MV origin = prob_motion_vector;
					for (pt &p : VLDP)
					{
						error = GetError(cur, prev + (p.y + origin.y) * wext + (p.x + origin.x), wext);
						if (error < min_error)
						{
							prob_motion_vector.x = origin.x + p.x;
							prob_motion_vector.y = origin.y + p.y;
							prob_motion_vector.error = error;
							min_error = error;
							ContinueDP = true;
						}
					}
				}

				ContinueDP = min_error > ERROR_THRESHOLD;
				while (ContinueDP)
				{
					ContinueDP = false;
					MV origin = prob_motion_vector;
					for (pt &p : LDP)
					{
						error = GetError(cur, prev + (p.y + origin.y) * wext + (p.x + origin.x), wext);
						if (error < min_error)
						{
							prob_motion_vector.x = origin.x + p.x;
							prob_motion_vector.y = origin.y + p.y;
							prob_motion_vector.error = error;
							min_error = error;
							ContinueDP = true;
						}
					}
				}
			}

			// Small diamond pattern
			bool ContinueSDP = true;
			while (ContinueSDP/* && Quota >= ARRAYSIZE(SDP)*/)
			{
				ContinueSDP = false;
				MV origin = prob_motion_vector;
				for (pt &p : SDP)
				{
					error = GetError(cur, prev + (p.y + origin.y) * wext + (p.x + origin.x), wext);
					if (error < min_error)
					{
						prob_motion_vector.x = origin.x + p.x;
						prob_motion_vector.y = origin.y + p.y;
						prob_motion_vector.error = error;
						min_error = error;
						ContinueSDP = true;
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

					error = GetError(cur, prev + prob_motion_vector.y * wext + prob_motion_vector.x, wext);
					if (error < min_error)
					{
						prob_motion_vector.dir = direction;
						prob_motion_vector.error = error;
						min_error = error;
					}
				}
			}
			MVectors[block_id] = prob_motion_vector;
			
			if (prob_motion_vector.error == MAXLONG)
			{
				printf("Bad!");
			}

			// Adjacent blocks
			if (j != num_blocks_hor - 1)
			{
				PrevPredictions[block_id + 1].AddVector(prob_motion_vector);
				if (i != num_blocks_vert - 1)
					PrevPredictions[block_id + num_blocks_hor + 1].AddVector(prob_motion_vector);
			}
			if (j != 0 && i != num_blocks_vert - 1)
			{
				PrevPredictions[block_id + num_blocks_hor - 1].AddVector(prob_motion_vector);
			}
			if (i != num_blocks_vert - 1)
				PrevPredictions[block_id + num_blocks_hor].AddVector(prob_motion_vector);

			// Future blocks
			int t;
			const float POINT_RADIUS = 0.1f;
			float dx = (float)prob_motion_vector.x / BLOCK_SIZE;
			float dy = (float)prob_motion_vector.y / BLOCK_SIZE;
			
			for (int i = -1; i <= 1; ++i)
			for (int j = -1; j <= 1; ++j)
			{
				if ((t = block_id + (int)(dy + POINT_RADIUS * i) * num_blocks_hor + (int)(dx + POINT_RADIUS)) < num_blocks_vert * num_blocks_hor && t >= 0)
					Predictions[t].AddVector(prob_motion_vector, 1.0f/6.0f);
			}
			
		}
	}
}