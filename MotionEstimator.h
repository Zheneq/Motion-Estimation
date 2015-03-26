#ifndef MOTIONESTIMATION_MOTIONESTIMATOR_H_
#define MOTIONESTIMATION_MOTIONESTIMATOR_H_

#include "types.h"

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
};

#endif
