#ifndef MOTIONESTIMATION_MOTIONESTIMATOR_H_
#define MOTIONESTIMATION_MOTIONESTIMATOR_H_

#include "types.h"
#include <list>

inline float SqrDiff(const MV& a, const MV& b)
{
	return pow((float)(a.x - b.x), 2) + pow((float)(a.y - b.y), 2);
}

class MotionEstimator
{
public:
    /// Constructor
    MotionEstimator(const int width,
                    const int height,
                    const BYTE quality);

    /// Copy constructor (forbidden)
    MotionEstimator(const MotionEstimator&) = delete;

    /// Destructor
    ~MotionEstimator();

    /// Assignment operator (forbidden)
    MotionEstimator&
    operator= (const MotionEstimator&) = delete;

    /// Perform motion estimation between two frames
    void
    Estimate(const BYTE *cur_Y_frame,
             const BYTE *prev_Y_frame,
             const BYTE *prev_Y_frame_up,
             const BYTE *prev_Y_frame_l,
             const BYTE *prev_Y_frame_upl,
             MV *MVectors,
             const bool use_half_pixel);

	/// Subroutines

	void
	BruteForce(const BYTE *cur_Y_frame,
		const BYTE *prev_Y_frame,
		const BYTE *prev_Y_frame_up,
		const BYTE *prev_Y_frame_l,
		const BYTE *prev_Y_frame_upl,
		MV *MVectors,
		const bool use_half_pixel);

	void
	ARPS(const BYTE *cur_Y_frame,
		const BYTE *prev_Y_frame,
		const BYTE *prev_Y_frame_up,
		const BYTE *prev_Y_frame_l,
		const BYTE *prev_Y_frame_upl,
		MV *MVectors,
		const bool use_half_pixel);

	int Quota;
	inline long _GetError(const BYTE *block1, const BYTE *block2, const int stride);
	inline long  GetError(const BYTE *block1, const BYTE *block2, const int stride);

    /// Border size
    static const int BORDER;

private:
    /// Maximum motion vector length per axis
    static const int MAX_MOTION;

    /// Error threshold for motiov vector splitting
    static const int SPLIT_TRESH;

    /// Frame width (doesn't include borders)
    const int width;

    /// Frame height (doesn't include borders)
    const int height;

    /// Quality
    const BYTE quality;

    /// The number of blocks per X-axis
    const int num_blocks_hor;

    /// The number of blocks per Y-axis
    const int num_blocks_vert;

    /// Extended width of the frame (including borders)
    const int wext;

    /// The position of the first pixel of the frame in the extended frame
    const int first_row_offset;

	/// Predictions
	struct PD
	{
		const float PREDICTION_THRESHOLD = 4.0f;

		struct v
		{
			MV vector;
			float weight;

			v(const MV& _vector, float _weight) : vector(_vector), weight(_weight) {}
		};
		
		std::list<v> pred;

		void AddVector(const MV& vec, float weight = 1.0f)
		{
			auto BestMatch = pred.end();
			float BestDiff = INFINITY;
			for (auto it = pred.begin(); it != pred.end(); ++it)
			{
				float t;
				if ((t = SqrDiff(vec, it->vector)) < PREDICTION_THRESHOLD)
				{
					if (BestDiff > t)
					{
						BestMatch = it;
						BestDiff = t;
					}
				}
			}
			if (BestMatch != pred.end())
			{
				auto bm = *BestMatch;
				pred.erase(BestMatch);
				bm.vector.error = (bm.vector.error * bm.weight + vec.error * weight) / (bm.weight + weight);
				bm.weight += weight;
				auto it = pred.begin();
				for (; it != pred.end(); ++it)
				{
					if (it->weight <= bm.weight)
					{
						break;
					}
				}
				pred.insert(it, bm);
			}
			else
			{
				pred.push_back(v(vec, weight));
			}
		}

		PD()
		{
			AddVector(MV(0, 0, sd_none, 100), 0.7f);
		}
	};

	PD *Predictions, *PrevPredictions;
	
};

#endif
