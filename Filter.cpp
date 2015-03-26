/*
********************************************************************
(c) MSU Video Group 2003-2010, http://compression.ru/video/
This source code is property of MSU Graphics and Media Lab

This code can be distributed and used only with WRITTEN PERMISSION.
********************************************************************
*/


#define FILTER_NAME        "_ME_Pedan"
#define FILTER_DESCRIPTION "ME task filter"
#define MAKER_NAME         "Eugene Pedan"

#include <memory>

#include "Filter.h"
#include "types.h"
#include "HalfPixel.h"
#include "MotionEstimator.h"

#include "stdio.h"
#include <time.h>
#include <windows.h>

struct MFD
{
    //specific
    LARGE_INTEGER tick1_process_timer;
    LARGE_INTEGER tick2_process_timer;
    LARGE_INTEGER freq_process_timer;
    double timer;

    bool show_vectors;
    bool show_res_after;
    bool show_res_before;
    bool show_MC;
    bool show_nothing;
    bool show_source;
    Method method;

    //controls output
    BYTE quality;
    bool use_half_pixel;
    bool work_in_RGB;

    /// Motion estimator
    MotionEstimator *motion_estimator;

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
    //dimensions of extended (with BORDERs) field
    int ext_w, ext_h, ext_size;
    int width, height, size;
    //size of frame in blocks
    int num_blocks_vert, num_blocks_hor;
};


#define USE_PROCESS_TIMER \
    QueryPerformanceFrequency(&mfd->freq_process_timer)


#define START_PROCESS_TIMER QueryPerformanceCounter(&mfd->tick1_process_timer)


#define STOP_PROCESS_TIMER(d) \
    QueryPerformanceCounter(&mfd->tick2_process_timer); \
    (d) = (1000.*((mfd->tick2_process_timer.LowPart + 4294967296.0*mfd->tick2_process_timer.HighPart)- \
    (mfd->tick1_process_timer.LowPart + 4294967296.0*mfd->tick1_process_timer.HighPart))/ \
    (mfd->freq_process_timer.LowPart + 4294967296.0*mfd->freq_process_timer.HighPart))

// Global variables
static FilterDefinition *fd;
char *MethodDesc[] = {"Not another ME method" };
char *MethodDescShort[] = {"ME" };

int initProc(FilterActivation *fa, const FilterFunctions *ff);

int startProc(FilterActivation *fa, const FilterFunctions*)
{
    MFD* mfd = (MFD*)fa->filter_data;
    USE_PROCESS_TIMER;
    //Allocating buffers of extra size (will need in motion estimation)
    mfd->width = fa->dst.w;
    mfd->height = fa->dst.h;
    mfd->size = mfd->width * mfd->height;
    mfd->ext_w = mfd->width + MotionEstimator::BORDER * 2;
    mfd->ext_h = mfd->height + MotionEstimator::BORDER * 2;
    mfd->ext_size = mfd->ext_h * mfd->ext_w;
    mfd->num_blocks_vert = (mfd->height + 15) / 16;
    mfd->num_blocks_hor = (mfd->width + 15) / 16;
    mfd->prev_Y = new BYTE[mfd->ext_size];
    mfd->prev_Y_up = new BYTE[mfd->ext_size];
    mfd->prev_Y_upl = new BYTE[mfd->ext_size];
    mfd->prev_Y_l = new BYTE[mfd->ext_size];
    mfd->prev_U = new short[mfd->width * mfd->height];
    mfd->prev_U_up = new short[mfd->width * mfd->height];
    mfd->prev_U_upl = new short[mfd->width * mfd->height];
    mfd->prev_U_l = new short[mfd->width * mfd->height];
    mfd->prev_U_MC = new short[mfd->width * mfd->height];
    mfd->prev_V = new short[mfd->width * mfd->height];
    mfd->prev_V_up = new short[mfd->width * mfd->height];
    mfd->prev_V_upl = new short[mfd->width * mfd->height];
    mfd->prev_V_l = new short[mfd->width * mfd->height];
    mfd->prev_V_MC = new short[mfd->width * mfd->height];
    mfd->prev_Y_MC = new BYTE[mfd->width * mfd->height];
    mfd->cur_U = new short[mfd->width * mfd->height];
    mfd->cur_V = new short[mfd->width * mfd->height];
    mfd->cur_Y = new BYTE[mfd->ext_size];
    mfd->MVectors = new MV[mfd->num_blocks_vert * mfd->num_blocks_hor];
    mfd->output_buf = new Pixel32[mfd->size];
    mfd->prev_RGB = new Pixel32[mfd->size];
    mfd->prev_RGB_up = new Pixel32[mfd->size];
    mfd->prev_RGB_upl = new Pixel32[mfd->size];
    mfd->prev_RGB_l = new Pixel32[mfd->size];
    mfd->frame_no = 0;
    mfd->offset = 0;
    mfd->noise = 0.0;
    mfd->motion_estimator = new MotionEstimator(mfd->width, mfd->height, mfd->quality);
    return 0;
}

inline
BYTE
GetYFromRGB(const Pixel32 &rgb)
{
    return static_cast<BYTE>(((rgb & 0xff0000) >> 16) * 0.299 +
                             ((rgb & 0xff00) >> 8) * 0.587 +
                              (rgb & 0xff) * 0.114  + 0.5);
}

void DrawLine(Pixel32* canvas, int width, int height, int line_offset, int x1, int y1, int x2, int y2)
{
    int x, y;
    Pixel32 *cur;
    bool origin;
    bool point = x1 == x2 && y1 == y2;
    if (x1 == x2)
    {
        for (y = min(y1, y2); y <= max(y2, y1); y++)
        {
            x = x1;
            if (x < 0 || x >= width || y < 0 || y >= height)
                continue;
            origin = y == y1;
            cur = (Pixel32*)((BYTE*)canvas + y * line_offset) + x;
            if (point)
            {
                *cur = 0x00ff00;
                return;
            }
            if (GetYFromRGB(*cur) < 128)
            {
                if (origin)
                {
                    *cur = 0xff0000;
                    origin = false;
                }
                else
                    *cur = 0xffffff;
            }
            else
            {
                if (origin)
                {
                    *cur = 0xff0000;
                    origin = false;
                }
                else
                    *cur = 0x00;
            }
        }
    }
    else if (abs(x2 - x1) >= abs(y2 - y1))
    {
        for (x = min(x1, x2); x <= max(x2, x1); x++)
        {
            if (x < 0 || x >= width)
                continue;
            origin = x == x1;
            y = (x - x1) * (y2 - y1) / (x2 - x1) + y1;
            if (y < 0 || y >= height)
                continue;
            cur = (Pixel32*)((BYTE*)canvas + y * line_offset) + x;
            if (point)
            {
                *cur = 0x00ff00;
                return;
            }
            if (GetYFromRGB(*cur) < 128)
            {
                if (origin)
                {
                    *cur = 0xff0000;
                    origin = false;
                }
                else
                    *cur = 0xffffff;
            }
            else
            {
                if (origin)
                {
                    *cur = 0xff0000;
                    origin = false;
                }
                else
                    *cur = 0x000000;
            }
        }
    }
    else
    {
        for (y = min(y1, y2); y <= max(y2, y1); y++)
        {
            origin = y == y1;
            x = (y - y1) * (x2 - x1) / (y2 - y1) + x1;
            if (x < 0 || x >= width || y < 0 || y >= height)
                continue;
            cur = (Pixel32*)((BYTE*)canvas + y * line_offset) + x;
            if (point)
            {
                *cur = 0x00ff00;
                return;
            }
            if (GetYFromRGB(*cur) < 128)
            {
                if (origin)
                {
                    *cur = 0xff0000;
                    origin = false;
                }
                else
                    *cur = 0xffffff;
            }
            else
            {
                if (origin)
                {
                    *cur = 0xff0000;
                    origin = false;
                }
                else
                    *cur = 0x000000;
            }
        }
    }
}

int MotionEstimate(const FilterActivation *fa, const FilterFunctions*)
{
    MFD *mfd = (MFD*)fa->filter_data;
    mfd->motion_estimator->Estimate(mfd->cur_Y,
                                    mfd->prev_Y,
                                    mfd->prev_Y_up,
                                    mfd->prev_Y_l,
                                    mfd->prev_Y_upl,
                                    mfd->MVectors,
                                    mfd->use_half_pixel,
									mfd->quality);
    return 0;
}

void DrawOutput(const FilterActivation *fa, MV*)
{
    MV mvector;
    MFD *mfd = (MFD*)fa->filter_data;
    Pixel32 *src = (Pixel32*)fa->src.data;
    Pixel32 *dst = (Pixel32*)fa->dst.data;
    int w = mfd->width, h = mfd->height, p = fa->src.pitch;
    int wext = mfd->ext_w;
    int first_row_offset = MotionEstimator::BORDER * wext;
    BYTE *cur, *prev;
    short *cur_U, *cur_V, *prev_U, *prev_V;
    Pixel32 *RGB_MC, src_pixel, mc_pixel;
    int i, j;
    int y, u, v, r, g, b;

    if (mfd->show_res_after)
    {
        dst = (Pixel32*)fa->dst.data;
        src = (Pixel32*)fa->src.data;
        cur = mfd->cur_Y + first_row_offset + MotionEstimator::BORDER;
        cur_U = mfd->cur_U;
        cur_V = mfd->cur_V;
        prev = mfd->prev_Y_MC;
        prev_U = mfd->prev_U_MC;
        prev_V = mfd->prev_V_MC;
        RGB_MC = mfd->output_buf;
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                if (!mfd->work_in_RGB)
                {
                    y = (cur[j] - prev[j]) * 3;
                    u = (cur_U[j] - prev_U[j]) * 3;
                    v = (cur_V[j] - prev_V[j]) * 3;
                    r = static_cast<int>(y + 1.14 * v + 0.5 + 128);
                    g = static_cast<int>(y - 0.395 * u - 0.581 * v + 0.5 + 128);
                    b = static_cast<int>(y + 2.032 * u + 0.5 + 128);
                }
                else
                {
                    src_pixel = src[j];
                    mc_pixel = RGB_MC[j];
                    r = ((src_pixel >> 16 & 0xff) - (mc_pixel >> 16 & 0xff)) * 3 + 128;
                    g = ((src_pixel >> 8 & 0xff) - (mc_pixel >> 8 & 0xff)) * 3 + 128;
                    b = ((src_pixel & 0xff) - (mc_pixel & 0xff)) * 3 + 128;
                }
                if (r > 255) r = 255; else if (r < 0) r = 0;
                if (g > 255) g = 255; else if (g < 0) g = 0;
                if (b > 255) b = 255; else if (b < 0) b = 0;
                dst[j] = (r << 16) | (g << 8) | b;
            }
            cur += wext;
            cur_U += w;
            cur_V += w;
            prev += w;
            prev_U += w;
            prev_V += w;
            dst = (Pixel32*)((BYTE*)dst + p);
            src = (Pixel32*)((BYTE*)src + p);
            RGB_MC += w;
        }
    }
    else if (mfd->show_MC)
    {
        dst = (Pixel32*)fa->dst.data;
        prev = mfd->prev_Y_MC;
        prev_U = mfd->prev_U_MC;
        prev_V = mfd->prev_V_MC;
        RGB_MC = mfd->output_buf;
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                if (!mfd->work_in_RGB)
                {
                    y = prev[j];
                    u = prev_U[j];
                    v = prev_V[j];
                    r = static_cast<int>(y + 1.14 * v + 0.5);
                    g = static_cast<int>(y - 0.395 * u - 0.581 * v + 0.5);
                    b = static_cast<int>(y + 2.032 * u + 0.5);
                }
                else
                {
                    src_pixel = src[j];
                    mc_pixel = RGB_MC[j];
                    r = (mc_pixel >> 16 & 0xff);
                    g = (mc_pixel >> 8 & 0xff);
                    b = (mc_pixel & 0xff);
                }
                if (r > 255) r = 255; else if (r < 0) r = 0;
                if (g > 255) g = 255; else if (g < 0) g = 0;
                if (b > 255) b = 255; else if (b < 0) b = 0;
                dst[j] = (r << 16) | (g << 8) | b;
            }
            prev += w;
            prev_U += w;
            prev_V += w;
            dst = (Pixel32*)((BYTE*)dst + p);
            src = (Pixel32*)((BYTE*)src + p);
            RGB_MC += w;
        }
    }
    if (mfd->show_res_before)
    {
        dst = (Pixel32*)fa->dst.data;
        src = (Pixel32*)fa->src.data;
        cur = mfd->cur_Y + first_row_offset + MotionEstimator::BORDER;
        cur_U = mfd->cur_U;
        cur_V = mfd->cur_V;
        prev = mfd->prev_Y + first_row_offset + MotionEstimator::BORDER;
        prev_U = mfd->prev_U;
        prev_V = mfd->prev_V;
        RGB_MC = mfd->prev_RGB;
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                if (!mfd->work_in_RGB)
                {
                    y = (cur[j] - prev[j]) * 3;
                    u = (cur_U[j] - prev_U[j]) * 3;
                    v = (cur_V[j] - prev_V[j]) * 3;
                    r = static_cast<int>(y + 1.14 * v + 0.5 + 128);
                    g = static_cast<int>(y - 0.395 * u - 0.581 * v + 0.5 + 128);
                    b = static_cast<int>(y + 2.032 * u + 0.5 + 128);
                }
                else
                {
                    src_pixel = src[j];
                    mc_pixel = RGB_MC[j];
                    r = ((src_pixel >> 16 & 0xff) - (mc_pixel >> 16 & 0xff)) * 3 + 128;
                    g = ((src_pixel >> 8 & 0xff) - (mc_pixel >> 8 & 0xff)) * 3 + 128;
                    b = ((src_pixel & 0xff) - (mc_pixel & 0xff)) * 3 + 128;
                }
                if (r > 255) r = 255; else if (r < 0) r = 0;
                if (g > 255) g = 255; else if (g < 0) g = 0;
                if (b > 255) b = 255; else if (b < 0) b = 0;
                dst[j] = (r << 16) | (g << 8) | b;
            }
            cur += wext;
            cur_U += w;
            cur_V += w;
            prev += wext;
            prev_U += w;
            prev_V += w;
            dst = (Pixel32*)((BYTE*)dst + p);
            src = (Pixel32*)((BYTE*)src + p);
            RGB_MC += w;
        }
    }
    if (mfd->show_vectors)
    {
        dst = (Pixel32*)fa->dst.data;
        for (i = 0; i < mfd->num_blocks_vert; i++)
        {
            for (j = 0; j < mfd->num_blocks_hor; j++)
            {
                mvector = mfd->MVectors[i * mfd->num_blocks_hor + j];
                if (!mvector.IsSplit())
                {
                    DrawLine(dst, w, h, p, (j << 4) + 8, ((i << 4) + 8), (j << 4) + mvector.x + 8, ((i << 4) + mvector.y + 8));
                }
                else
                {
                    for (int h = 0; h < 4; h++)
                    {
                        DrawLine(dst, w, h, p, (j << 4) + 8 + ((h & 1)? 4 : -4), ((i << 4) + 8 + ((h > 1)? 4 : -4)), (j << 4) + mvector.SubVector(h).x + 8 + ((h & 1)? 4 : -4), ((i << 4) + mvector.SubVector(h).y + 8 + ((h > 1)? 4 : -4)));
                    }
                }
            }
        }
    }
}

int SpatialNoiseLevel(const FilterActivation *fa, int _step, int _threshold, double *D_noise)
{
    BYTE *CurrMetering, *b_point;
    int src_pitch = fa->src.pitch , src_modulo = fa->src.modulo , next = src_pitch-7, b_step = 4*_step;
    int X_num_metering = fa->src.w/_step, Y_num_metering = fa->src.h/_step;
    int num_of_metering = X_num_metering*Y_num_metering, real_nop=num_of_metering;
    int sumR,sumG,sumB, R,G,B, r,g,b, count, semaphore;
    double temp;

    //////////////////////////////////////////////////////////////
    //

    CurrMetering = (BYTE*)fa->src.data + (_step/2-2)*src_pitch + _step*2;

    *D_noise = 0;
    count = semaphore = 0;
    while(count < num_of_metering)
    {
        sumR = sumG = sumB = 0;

        b_point = CurrMetering;
        sumR += *b_point++;    sumG += *b_point++;    sumB += *b_point++;
        ////////////////////////////////////////////
        b_point += next;
        R = *b_point++;        G = *b_point++;        B = *b_point++;

        b_point++;
        r = *b_point++;        g = *b_point++;        b = *b_point++;
        if ((R-r<-_threshold) || (R-r>_threshold) || (G-g<-_threshold) || (G-g>_threshold) || (B-b<-_threshold) || (B-b>_threshold) ||
            (sumR-r<-_threshold) || (sumR-r>_threshold) || (sumG-g<-_threshold) || (sumG-g>_threshold) || (sumB-b<-_threshold) || (sumB-b>_threshold)   )
        {
            real_nop--;
            goto lable;
        }
        sumR += R+r*2;    sumG += G+g*2;    sumB += B+b*2;

        b_point++;
        R = *b_point++;    G = *b_point++;    B = *b_point++;
        if ((R-r<-_threshold) || (R-r>_threshold) || (G-g<-_threshold) || (G-g>_threshold) || (B-b<-_threshold) || (B-b>_threshold))
        {
            real_nop--;
            goto lable;
        }
        sumR += R;    sumG += G;    sumB += B;
        ////////////////////////////////////////////
        b_point += next;
        R = *b_point++;    G = *b_point++;    B = *b_point++;
        if ((R-r<-_threshold) || (R-r>_threshold) || (G-g<-_threshold) || (G-g>_threshold) || (B-b<-_threshold) || (B-b>_threshold))
        {
            real_nop--;
            goto lable;
        }
        sumR += R;    sumG += G;    sumB += B;



        temp = 0.299*(sumR - r*6) + 0.587*(sumG - g*6) + 0.114*(sumB - b*6);
        *D_noise += temp*temp;


lable:
        CurrMetering += b_step;
        count++;    semaphore++;
        if (semaphore==X_num_metering)
        {
            semaphore = 0;
            CurrMetering = (BYTE*)CurrMetering + src_pitch*(_step-1) + src_modulo;
        }

    }

    *D_noise /= 36*(real_nop-1);

    return (100*real_nop/num_of_metering);
}

int runProc(const FilterActivation *fa, const FilterFunctions *ff)
{
    Pixel32 *src = (Pixel32*)fa->src.data, *dst = (Pixel32*)fa->dst.data;
    MFD *mfd = (MFD*) fa->filter_data;
    int i, j, block_id;
    MV mvector;
    BYTE *prev = mfd->prev_Y, *cur = mfd->cur_Y, *prev_MC = mfd->prev_Y_MC;
    short *cur_U = mfd->cur_U, *cur_V = mfd->cur_V;
    short *prev_U = mfd->prev_U, *prev_V = mfd->prev_V;
    short *prev_U_MC = mfd->prev_U_MC, *prev_V_MC = mfd->prev_V_MC;
    int w = mfd->width, h = mfd->height, p = fa->src.pitch;
    int num_blocks_hor = (w + 15) >> 4;
    int wext = mfd->ext_w;
    int first_row_offset = MotionEstimator::BORDER * wext;
    int after_last_row_offset = (MotionEstimator::BORDER + h) * wext;

    if ((mfd->frame_no & 0xf) == 0)
    {
        SpatialNoiseLevel(fa, 4, 20, &mfd->noise);
        mfd->noise = sqrt(mfd->noise);
    }

    src = fa->src.data;
    dst = fa->dst.data;
    cur = cur + first_row_offset;
    for (i = 0; i < h; i++)
    {
        //filling extra space to the left from picture
        memset(cur, GetYFromRGB(src[0]), MotionEstimator::BORDER + 1);
        cur_U[0] = (short)(-0.147*((src[0]&0xff0000)>>16)
                        -0.289*((src[0]&0xff00)>>8)
                        +0.436*(src[0]&0xff) + 0.5);
        cur_V[0] = (short)(0.615*((src[0]&0xff0000)>>16)
                        -0.515*((src[0]&0xff00)>>8)
                        -0.100*(src[0]&0xff) + 0.5);
        dst[0] = src[0];
        for (j = 1; j < w - 1; j++)
        {
            cur[j + MotionEstimator::BORDER] = GetYFromRGB(src[j]);
            cur_U[j] = (short)(-0.147*((src[j]&0xff0000)>>16)
                            -0.289*((src[j]&0xff00)>>8)
                            +0.436*(src[j]&0xff) + 0.5);
            cur_V[j] = (short)(0.615*((src[j]&0xff0000)>>16)
                            -0.515*((src[j]&0xff00)>>8)
                            -0.100*(src[j]&0xff) + 0.5);
            dst[j] = src[j];
        }
        //filling extra space to the right from picture
        memset(cur + MotionEstimator::BORDER + w - 1, GetYFromRGB(src[w - 1]), MotionEstimator::BORDER + 1);
        cur_U[w - 1] = (short)(-0.147*((src[w - 1]&0xff0000)>>16)
                        -0.289*((src[w - 1]&0xff00)>>8)
                        +0.436*(src[w - 1]&0xff) + 0.5);
        cur_V[w - 1] = (short)(0.615*((src[w - 1]&0xff0000)>>16)
                        -0.515*((src[w - 1]&0xff00)>>8)
                        -0.100*(src[w - 1]&0xff) + 0.5);
        dst[w - 1] = src[w - 1];
        src = (Pixel32*)((BYTE*)src + p);
        dst = (Pixel32*)((BYTE*)dst + p);
        cur += wext;
        cur_U += w;
        cur_V += w;
    }

    //It's only left now to fill extra space above and below the picture
    BYTE *first_cur = mfd->cur_Y + first_row_offset;
    cur = mfd->cur_Y;
    for (i = 0; i < MotionEstimator::BORDER; i++)
    {
        memcpy(cur, first_cur, wext);
        cur += wext;
    }
    cur = mfd->cur_Y + after_last_row_offset;
    first_cur = mfd->cur_Y + after_last_row_offset - wext;
    for (i = 0; i < MotionEstimator::BORDER; i++)
    {
        memcpy(cur, first_cur, wext);
        cur += wext;
    }

    if (mfd->frame_no == 0)
    {
        memcpy(mfd->prev_Y, mfd->cur_Y, mfd->ext_size);
        memcpy(mfd->prev_U, mfd->cur_U, mfd->size * sizeof(short));
        memcpy(mfd->prev_V, mfd->cur_V, mfd->size * sizeof(short));
        for (i = 0; i < h; i++)
        {
            memcpy((BYTE*)(mfd->prev_RGB) + i * 4 * w, (BYTE*)(fa->src.data) + i * p, w * 4);
        }
    }

    //halfpixel shift
    if (mfd->use_half_pixel)
    {
        memcpy(mfd->prev_Y_l, mfd->prev_Y, mfd->ext_size);
        memcpy(mfd->prev_Y_up, mfd->prev_Y, mfd->ext_size);
        memcpy(mfd->prev_Y_upl, mfd->prev_Y, mfd->ext_size);

        HalfpixelShiftHorz(mfd->prev_Y_l, mfd->ext_w, mfd->ext_h, mfd->shift_up);
        HalfpixelShift(mfd->prev_Y_up, mfd->ext_w, mfd->ext_h, mfd->shift_up);
        HalfpixelShift(mfd->prev_Y_upl, mfd->ext_w, mfd->ext_h, mfd->shift_up);
        HalfpixelShiftHorz(mfd->prev_Y_upl, mfd->ext_w, mfd->ext_h, mfd->shift_up);

        memcpy(mfd->prev_U_l, mfd->prev_U, mfd->size*sizeof(short));
        memcpy(mfd->prev_U_up, mfd->prev_U, mfd->size*sizeof(short));
        memcpy(mfd->prev_U_upl, mfd->prev_U, mfd->size*sizeof(short));

        HalfpixelShiftHorz(mfd->prev_U_l, mfd->width, mfd->height, mfd->shift_up);
        HalfpixelShift(mfd->prev_U_up, mfd->width, mfd->height, mfd->shift_up);
        HalfpixelShift(mfd->prev_U_upl, mfd->width, mfd->height, mfd->shift_up);
        HalfpixelShiftHorz(mfd->prev_U_upl, mfd->width, mfd->height, mfd->shift_up);

        memcpy(mfd->prev_V_l, mfd->prev_V, mfd->size*sizeof(short));
        memcpy(mfd->prev_V_up, mfd->prev_V, mfd->size*sizeof(short));
        memcpy(mfd->prev_V_upl, mfd->prev_V, mfd->size*sizeof(short));

        HalfpixelShiftHorz(mfd->prev_V_l, mfd->width, mfd->height, mfd->shift_up);
        HalfpixelShift(mfd->prev_V_up, mfd->width, mfd->height, mfd->shift_up);
        HalfpixelShift(mfd->prev_V_upl, mfd->width, mfd->height, mfd->shift_up);
        HalfpixelShiftHorz(mfd->prev_V_upl, mfd->width, mfd->height, mfd->shift_up);

        memcpy(mfd->prev_RGB_l, mfd->prev_RGB, mfd->size*sizeof(Pixel32));
        memcpy(mfd->prev_RGB_upl, mfd->prev_RGB, mfd->size*sizeof(Pixel32));
        memcpy(mfd->prev_RGB_up, mfd->prev_RGB, mfd->size*sizeof(Pixel32));

        HalfpixelShiftHorz(mfd->prev_RGB_l, mfd->width, mfd->height, mfd->shift_up);
        HalfpixelShift(mfd->prev_RGB_up, mfd->width, mfd->height, mfd->shift_up);
        HalfpixelShift(mfd->prev_RGB_upl, mfd->width, mfd->height, mfd->shift_up);
        HalfpixelShiftHorz(mfd->prev_RGB_upl, mfd->width, mfd->height, mfd->shift_up);
    }

    double ttime;
    START_PROCESS_TIMER;
    MotionEstimate(fa, ff);
    STOP_PROCESS_TIMER(ttime);
    mfd->timer += ttime;

    //constructing MC frames
    cur = mfd->cur_Y + first_row_offset + MotionEstimator::BORDER;
    cur_U = mfd->cur_U;
    cur_V = mfd->cur_V;
    Pixel32* rgb_MC = mfd->output_buf, *prev_RGB = mfd->prev_RGB;
    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
        {
            mvector = mfd->MVectors[(i >> 4) * num_blocks_hor + (j >> 4)];
            if (mvector.IsSplit())
            {
                block_id = (((i % 16) > 7)? 0x2 : 0x0) + (((j % 16) > 7)? 0x1 : 0x0);
                mvector = mvector.SubVector(block_id);
            }
            switch(mvector.dir)
            {
                case sd_none:
                    prev = mfd->prev_Y + first_row_offset + MotionEstimator::BORDER + i * wext + j;
                    prev_U = mfd->prev_U + i * w + j;
                    prev_V = mfd->prev_V + i * w + j;
                    prev_RGB = mfd->prev_RGB + i * w + j;
                    break;
                case sd_up:
                    prev = mfd->prev_Y_up + first_row_offset + MotionEstimator::BORDER + i * wext + j;
                    prev_U = mfd->prev_U_up + i * w + j;
                    prev_V = mfd->prev_V_up + i * w + j;
                    prev_RGB = mfd->prev_RGB_up + i * w + j;
                    break;
                case sd_l:
                    prev = mfd->prev_Y_l + first_row_offset + MotionEstimator::BORDER + i * wext + j;
                    prev_U = mfd->prev_U_l + i * w + j;
                    prev_V = mfd->prev_V_l + i * w + j;
                    prev_RGB = mfd->prev_RGB_l + i * w + j;
                    break;
                case sd_upl:
                    prev = mfd->prev_Y_upl + first_row_offset + MotionEstimator::BORDER + i * wext + j;
                    prev_U = mfd->prev_U_upl + i * w + j;
                    prev_V = mfd->prev_V_upl + i * w + j;
                    prev_RGB = mfd->prev_RGB_upl + i * w + j;
                    break;
            }
            int sh_y, sh_x;
            if (j + mvector.x >= 0 && j + mvector.x <= w - 1)
                sh_x = mvector.x;
            else if (j + mvector.x < 0)
                sh_x = -j;
            else
                sh_x = w - 1 - j;
            if (mvector.y + i >= 0 && mvector.y + i <= h - 1)
                sh_y = mvector.y;
            else if (mvector.y + i < 0)
                sh_y = -i;
            else
                sh_y = h - 1 - i;
            prev_MC[j] = prev[sh_y * wext + sh_x];
            prev_U_MC[j] = prev_U[sh_y * w + sh_x];
            prev_V_MC[j] = prev_V[sh_y * w + sh_x];
            rgb_MC[j] = prev_RGB[sh_y * w + sh_x];
        }
        prev_MC += w;
        prev_U_MC += w;
        prev_V_MC += w;
        rgb_MC += w;
    }

    if (!mfd->show_nothing)
        DrawOutput(fa, mfd->MVectors);

    //preparing to the next frame
    memcpy(mfd->prev_Y, mfd->cur_Y, mfd->ext_size);
    memcpy(mfd->prev_U, mfd->cur_U, mfd->size * sizeof(short));
    memcpy(mfd->prev_V, mfd->cur_V, mfd->size * sizeof(short));
    for (i = 0; i < h; i++)
    {
        memcpy((BYTE*)mfd->prev_RGB + i * 4 * w, (BYTE*)fa->src.data + i * p, w * 4);
    }

    mfd->frame_no++;
    return 0;
}

BOOL CALLBACK ConfigDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    MFD *mfd = (MFD *)GetWindowLong(hdlg, DWL_USER);
    static bool sem = false;
    int temp;

    switch(msg) {
        case WM_INITDIALOG:
            SetWindowLong(hdlg, DWL_USER, lParam);
            mfd = (MFD *)lParam;
            SendMessage(GetDlgItem(hdlg, IDC_QUALITYSLIDER), TBM_SETRANGEMIN, true, 0);
            SendMessage(GetDlgItem(hdlg, IDC_QUALITYSLIDER), TBM_SETRANGEMAX, true, 100);
            SendMessage(GetDlgItem(hdlg, IDC_QUALITYSLIDER), TBM_SETLINESIZE, 0, 1);
            SendMessage(GetDlgItem(hdlg, IDC_QUALITYSLIDER), TBM_SETPAGESIZE, 0, 5);
            SendMessage(GetDlgItem(hdlg, IDC_QUALITYSLIDER), TBM_SETTICFREQ, 5, 0);
            SendMessage(GetDlgItem(hdlg, IDC_QUALITYSPIN), UDM_SETRANGE32, 0, 100);
            CheckRadioButton(hdlg, IDC_SHOWSOURCE, IDC_SHOWMC, mfd->show_res_after?IDC_SHOWRESAFTER:(mfd->show_MC?IDC_SHOWMC:(mfd->show_res_before?IDC_SHOWRESBEFORE:IDC_SHOWSOURCE)));
            CheckDlgButton(hdlg, IDC_SHOWMV, mfd->show_vectors?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hdlg, IDC_RGB, mfd->work_in_RGB?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hdlg, IDC_DRAWNOTHING, mfd->show_nothing?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hdlg, IDC_HALFPIXEL, mfd->use_half_pixel?BST_CHECKED:BST_UNCHECKED);
            SetDlgItemInt(hdlg, IDC_QUALITY, mfd->quality, false);
            return TRUE;

        case WM_COMMAND:
            switch(LOWORD(wParam)) {
            case IDOK:
                mfd->show_res_after = !!IsDlgButtonChecked(hdlg, IDC_SHOWRESAFTER);
                mfd->show_res_before = !!IsDlgButtonChecked(hdlg, IDC_SHOWRESBEFORE);
                mfd->show_MC = !!IsDlgButtonChecked(hdlg, IDC_SHOWMC);
                mfd->show_source = !!IsDlgButtonChecked(hdlg, IDC_SHOWSOURCE);
                mfd->show_vectors = !!IsDlgButtonChecked(hdlg, IDC_SHOWMV);
                mfd->quality = static_cast<BYTE>(GetDlgItemInt(hdlg, IDC_QUALITY, NULL, false));
                mfd->work_in_RGB = !!IsDlgButtonChecked(hdlg, IDC_RGB);
                mfd->show_nothing = !!IsDlgButtonChecked(hdlg, IDC_DRAWNOTHING);
                mfd->use_half_pixel = !!IsDlgButtonChecked(hdlg, IDC_HALFPIXEL);
                EndDialog(hdlg, 0);
                return TRUE;
            case IDCANCEL:
                EndDialog(hdlg, 1);
                return FALSE;
            case IDC_QUALITY:
                if (HIWORD(wParam) == EN_UPDATE)
                {
                    temp = GetDlgItemInt(hdlg, IDC_QUALITY, NULL, false);
                    if (temp > 100)
                    {
                        temp = 100;
                        SetDlgItemInt(hdlg, IDC_QUALITY, temp, false);
                    }
                    else if (temp < 0)
                    {
                        temp = 0;
                        SetDlgItemInt(hdlg, IDC_QUALITY, temp, false);
                    }
                    sem = true;
                    SendMessage(GetDlgItem(hdlg, IDC_QUALITYSLIDER), TBM_SETPOS, true, GetDlgItemInt(hdlg, IDC_QUALITY, NULL, false));
                    sem = false;
                }
                return TRUE;
            default:
                return TRUE;
            }
            break;
        case WM_NOTIFY:
            if (LOWORD(wParam) == IDC_QUALITYSLIDER)
            {
                if (!sem)
                    SetDlgItemInt(hdlg, IDC_QUALITY, SendMessage(GetDlgItem(hdlg, IDC_QUALITYSLIDER), TBM_GETPOS, 0, 0), false);
            }
            break;
    }

    return FALSE;
}

int configProc(FilterActivation *fa, const FilterFunctions*, HWND hwnd) {
    return DialogBoxParam(fa->filter->module->hInstModule,
            MAKEINTRESOURCE(IDDIALOG), hwnd,
            ConfigDlgProc, (LPARAM)fa->filter_data);
}

void stringProc(const FilterActivation*, const FilterFunctions*, char *str)
{
    sprintf(str, "");
}

long paramProc(FilterActivation*, const FilterFunctions*)
{
    return FILTERPARAM_NEEDS_LAST | FILTERPARAM_SWAP_BUFFERS;
}

int endProc(FilterActivation *fa, const FilterFunctions*)
{
    MFD* mfd = (MFD*)fa->filter_data;

    delete mfd->motion_estimator;
    delete[] mfd->prev_Y;
    delete[] mfd->cur_Y;
    delete[] mfd->prev_Y_MC;
    delete[] mfd->MVectors;
    return 0;
}

// Job support
bool fssProc(FilterActivation *fa, const FilterFunctions*, char *buf, int buflen)
{
    MFD *mfd = (MFD *)fa->filter_data;

    _snprintf(buf, buflen, "Config(%d, %d, %d, %d, %d, %d, %d, %d, %d)",
                                mfd->quality,
                                mfd->use_half_pixel,
                                mfd->show_nothing,
                                mfd->work_in_RGB,
                                mfd->show_vectors,
                                mfd->show_source,
                                mfd->show_MC,
                                mfd->show_res_before,
                                mfd->show_res_after);
    return true;
}

void scriptConfig(IScriptInterpreter*, void *lpVoid, CScriptValue *argv, int)
{
    FilterActivation *fa = (FilterActivation *)lpVoid;
    MFD *mfd = (MFD *)fa->filter_data;

    mfd->quality = static_cast<BYTE>(argv[0].asInt());
    mfd->use_half_pixel = !!(argv[1].asInt());
    mfd->show_nothing = !!(argv[2].asInt());
    mfd->work_in_RGB = !!(argv[3].asInt());
    mfd->show_vectors = !!(argv[4].asInt());
    mfd->show_source = !!(argv[5].asInt());
    mfd->show_MC = !!(argv[6].asInt());
    mfd->show_res_before = !!(argv[7].asInt());
    mfd->show_res_after = !!(argv[8].asInt());
}

ScriptFunctionDef script_func_defs[]
{
    {(ScriptFunctionPtr)scriptConfig, "Config", "0iiiiiiiii"},
    {NULL},
};

CScriptObject script_obj
{
    NULL,
    script_func_defs
};

struct FilterDefinition filterDef = {

    NULL, NULL, NULL,               // next, prev, module
    FILTER_NAME,                    // name
    FILTER_DESCRIPTION,             // desc
    MAKER_NAME,                     // maker
    NULL,                           // private_data
    sizeof(MFD),                    // inst_data_size

    initProc,                       // initProc
    NULL,                           // deinitProc
    runProc,                        // runProc
    paramProc,                      // paramProc
    configProc,                     // configProc
    stringProc,                     // stringProc
    startProc,                      // startProc
    endProc,                        // endProc

    &script_obj,                    // script_obj
    fssProc,                        // fssProc
};

int initProc(FilterActivation *fa, const FilterFunctions*)
{
    MFD *mfd = (MFD*)fa->filter_data;
    mfd->show_res_after = true;
    mfd->show_vectors = false;
    mfd->show_nothing = false;
    mfd->show_res_before = false;
    mfd->show_source = false;
    mfd->show_MC = false;
    mfd->work_in_RGB = false;
    mfd->quality = 100;
    mfd->use_half_pixel = false;
    mfd->method = den;
    mfd->offset = 0;
    return 0;
}

extern "C" int __declspec(dllexport) __cdecl VirtualdubFilterModuleInit2(FilterModule *fm, const FilterFunctions *ff, int& vdfd_ver, int& vdfd_compat);
extern "C" void __declspec(dllexport) __cdecl VirtualdubFilterModuleDeinit(FilterModule *fm, const FilterFunctions *ff);


int
__declspec(dllexport) __cdecl
VirtualdubFilterModuleInit2(FilterModule *fm,
                            const FilterFunctions *ff,
                            int &vdfd_ver,
                            int &vdfd_compat)
{
    fd = ff->addFilter(fm, &filterDef, sizeof(FilterDefinition));
    if (!fd)
        return 1;

    vdfd_ver    = VIRTUALDUB_FILTERDEF_VERSION;
    vdfd_compat = VIRTUALDUB_FILTERDEF_COMPATIBLE;

    return 0;
}

void
__declspec(dllexport) __cdecl
VirtualdubFilterModuleDeinit(FilterModule*,
                             const FilterFunctions *ff)
{
    ff->removeFilter(fd);
}
