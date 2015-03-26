#include "HalfPixel.h"

void HalfpixelShift(BYTE *field, int width, int height, bool shift_up)
{
    int i, j, width2 = width * 2, temp;
    std::unique_ptr<BYTE[]> new_field(new BYTE[width*height]);
    BYTE *old_ptr = field, *new_ptr = new_field.get();
    old_ptr += width;
    for (j = 0; j < width; j++)
    {
        temp = (old_ptr[j-width] + old_ptr[j])>>1;
        if (temp < 255)
            if (temp >= 0);
            else
                temp = 0;
        else
            temp = 255;
        new_ptr[j] = static_cast<BYTE>(temp);
    }
    old_ptr += width;
    new_ptr += width;
    for (i = 2; i < height - 1; i++)
    {
        for (j = 0; j < width; j++)
        {
            temp = (5*(old_ptr[j-width] + old_ptr[j]) -
                (old_ptr[j-width2] + old_ptr[j+width]))>>3;
            if (temp < 255)
                if (temp >= 0);
                else
                    temp = 0;
            else
                temp = 255;
            new_ptr[j] = static_cast<BYTE>(temp);
        }
        old_ptr += width;
        new_ptr += width;
    }
    for (j = 0; j < width; j++)
    {
        temp = (old_ptr[j-width] + old_ptr[j])>>1;
        if (temp < 255)
            if (temp >= 0);
            else
                temp = 0;
        else
            temp = 255;
        new_ptr[j] = static_cast<BYTE>(temp);
    }
    if (shift_up)
    {
        temp = width;
    }
    else
    {
        temp = 0;
    }
    memcpy(field + temp, new_field.get(), width * (height - 1) * sizeof(BYTE));
}

void HalfpixelShiftHorz(BYTE *field, int width, int height, bool shift_right)
{
    int i, j, temp;
    std::unique_ptr<BYTE[]> new_field(new BYTE[width*height]);
    BYTE *old_ptr = field, *new_ptr = new_field.get();

    old_ptr += 1;
    for (j = 0; j < height; j++)
    {
        temp = (old_ptr[j*width - 1] + old_ptr[j*width])>>1;
        if (temp < 255)
            if (temp >= 0);
            else
                temp = 0;
        else
            temp = 255;
        new_ptr[j*width] = static_cast<BYTE>(temp);
    }
    old_ptr += 1;
    new_ptr += 1;
    for (i = 2; i < width - 1; i++)
    {
        for (j = 0; j < height; j++)
        {
            temp = (5*(old_ptr[j*width-1] + old_ptr[j*width]) -
                (old_ptr[j*width-2] + old_ptr[j*width+1]))>>3;
            if (temp < 255)
                if (temp >= 0);
                else
                    temp = 0;
            else
                temp = 255;
            new_ptr[j*width] = static_cast<BYTE>(temp);
        }
        old_ptr += 1;
        new_ptr += 1;
    }
    for (j = 0; j < height; j++)
    {
        temp = (old_ptr[j*width-1] + old_ptr[j*width])>>1;
        if (temp < 255)
            if (temp >= 0);
            else
                temp = 0;
        else
            temp = 255;
        new_ptr[j*width] = static_cast<BYTE>(temp);
    }
    if (shift_right)
    {
        new_ptr = new_field.get();
        old_ptr = field;
        for (j = 0; j < height; j++)
        {
            new_ptr[j*width] = old_ptr[j*width];
        }
    }
    else
    {
        new_ptr += 1;
        for (j = 0; j < height; j++)
        {
            new_ptr[j*width] = old_ptr[j*width];
        }
    }
    memcpy(field, new_field.get(), width * height * sizeof(BYTE));
}

void HalfpixelShift(Pixel32 *field, int width, int height, bool shift_up)
{
    int i, j, width2 = width * 2, temp;
    int r, g, b;
    std::unique_ptr<Pixel32[]> new_field(new Pixel32[width*height]);
    Pixel32 *old_ptr = field, *new_ptr = new_field.get();
    old_ptr += width;
    for (j = 0; j < width; j++)
    {
        r = (((old_ptr[j-width] & 0xff0000) + (old_ptr[j] & 0xff0000))>>17);
        g = (((old_ptr[j-width] & 0xff00) + (old_ptr[j] & 0xff00))>>9);
        b = (((old_ptr[j-width] & 0xff) + (old_ptr[j] & 0xff))>>1);
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;
        if (r < 0) r = 0;
        if (g < 0) g = 0;
        if (b < 0) b = 0;
        new_ptr[j] = (r<<16) | (g<<8) | b;
    }
    old_ptr += width;
    new_ptr += width;
    for (i = 2; i < height - 1; i++)
    {
        for (j = 0; j < width; j++)
        {
            r = (((old_ptr[j-width] & 0xff0000) + (old_ptr[j] & 0xff0000))>>16)*5;
            g = (((old_ptr[j-width] & 0xff00) + (old_ptr[j] & 0xff00))>>8)*5;
            b = ((old_ptr[j-width] & 0xff) + (old_ptr[j] & 0xff))*5;
            r -= (((old_ptr[j-width2] & 0xff0000) + (old_ptr[j+width] & 0xff0000))>>16);
            g -= (((old_ptr[j-width2] & 0xff00) + (old_ptr[j+width] & 0xff00))>>8);
            b -= ((old_ptr[j-width2] & 0xff) + (old_ptr[j+width] & 0xff));
            r >>= 3;
            g >>= 3;
            b >>= 3;
            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
            if (r < 0) r = 0;
            if (g < 0) g = 0;
            if (b < 0) b = 0;
            new_ptr[j] = (r<<16) | (g<<8) | b;
        }
        old_ptr += width;
        new_ptr += width;
    }
    for (j = 0; j < width; j++)
    {
        r = (((old_ptr[j-width] & 0xff0000) + (old_ptr[j] & 0xff0000))>>17);
        g = (((old_ptr[j-width] & 0xff00) + (old_ptr[j] & 0xff00))>>9);
        b = (((old_ptr[j-width] & 0xff) + (old_ptr[j] & 0xff))>>1);
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;
        if (r < 0) r = 0;
        if (g < 0) g = 0;
        if (b < 0) b = 0;
        new_ptr[j] = (r<<16) | (g<<8) | b;
    }
    if (shift_up)
    {
        temp = width * 4;
    }
    else
    {
        temp = 0;
    }
    memcpy(field + temp, new_field.get(), width * (height - 1) * sizeof(Pixel32));
}

void HalfpixelShiftHorz(Pixel32 *field, int width, int height, bool shift_right)
{
    int i, j;
    int r, g, b;
    std::unique_ptr<Pixel32[]> new_field(new Pixel32[width*height]);
    Pixel32 *old_ptr = field, *new_ptr = new_field.get();

    old_ptr += 1;
    for (j = 0; j < height; j++)
    {
        r = (((old_ptr[j*width-1] & 0xff0000) + (old_ptr[j*width] & 0xff0000))>>17);
        g = (((old_ptr[j*width-1] & 0xff00) + (old_ptr[j*width] & 0xff00))>>9);
        b = (((old_ptr[j*width-1] & 0xff) + (old_ptr[j*width] & 0xff))>>1);
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;
        if (r < 0) r = 0;
        if (g < 0) g = 0;
        if (b < 0) b = 0;
        new_ptr[j*width] = (r<<16) | (g<<8) | b;
    }
    old_ptr += 1;
    new_ptr += 1;
    for (i = 2; i < width - 1; i++)
    {
        for (j = 0; j < height; j++)
        {
            r = (((old_ptr[j*width-1] & 0xff0000) + (old_ptr[j*width] & 0xff0000))>>16)*5;
            g = (((old_ptr[j*width-1] & 0xff00) + (old_ptr[j*width] & 0xff00))>>8)*5;
            b = ((old_ptr[j*width-1] & 0xff) + (old_ptr[j*width] & 0xff))*5;
            r -= (((old_ptr[j*width-2] & 0xff0000) + (old_ptr[j*width+1] & 0xff0000))>>16);
            g -= (((old_ptr[j*width-2] & 0xff00) + (old_ptr[j*width+1] & 0xff00))>>8);
            b -= ((old_ptr[j*width-2] & 0xff) + (old_ptr[j*width+1] & 0xff));
            r >>= 3;
            g >>= 3;
            b >>= 3;
            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
            if (r < 0) r = 0;
            if (g < 0) g = 0;
            if (b < 0) b = 0;
            new_ptr[j*width] = (r<<16) | (g<<8) | b;
        }
        old_ptr += 1;
        new_ptr += 1;
    }
    for (j = 0; j < height; j++)
    {
        r = (((old_ptr[j*width-1] & 0xff0000) + (old_ptr[j*width] & 0xff0000))>>17);
        g = (((old_ptr[j*width-1] & 0xff00) + (old_ptr[j*width] & 0xff00))>>9);
        b = (((old_ptr[j*width-1] & 0xff) + (old_ptr[j*width] & 0xff))>>1);
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;
        if (r < 0) r = 0;
        if (g < 0) g = 0;
        if (b < 0) b = 0;
        new_ptr[j*width] = (r<<16) | (g<<8) | b;
    }
    if (shift_right)
    {
        new_ptr = new_field.get();
        old_ptr = field;
        for (j = 0; j < height; j++)
        {
            new_ptr[j*width] = old_ptr[j*width];
        }
    }
    else
    {
        new_ptr += 1;
        for (j = 0; j < height; j++)
        {
            new_ptr[j*width] = old_ptr[j*width];
        }
    }
    memcpy(field, new_field.get(), width * height * sizeof(Pixel32));
}

void HalfpixelShift(short *field, int width, int height, bool shift_up)
{
    int i, j, width2 = width * 2, temp;
    std::unique_ptr<short[]> new_field(new short[width*height]);
    short *old_ptr = field, *new_ptr = new_field.get();
    old_ptr += width;
    for (j = 0; j < width; j++)
    {
        temp = (old_ptr[j-width] + old_ptr[j])>>1;
        new_ptr[j] = static_cast<short>(temp);
    }
    old_ptr += width;
    new_ptr += width;
    for (i = 2; i < height - 1; i++)
    {
        for (j = 0; j < width; j++)
        {
            temp = (5*(old_ptr[j-width] + old_ptr[j]) -
                (old_ptr[j-width2] + old_ptr[j+width]))>>3;
            new_ptr[j] = static_cast<short>(temp);
        }
        old_ptr += width;
        new_ptr += width;
    }
    for (j = 0; j < width; j++)
    {
        temp = (old_ptr[j-width] + old_ptr[j])>>1;
        new_ptr[j] = static_cast<short>(temp);
    }
    if (shift_up)
    {
        temp = width;
    }
    else
    {
        temp = 0;
    }
    memcpy(field + temp, new_field.get(), width * (height - 1) * sizeof(short));
}

void HalfpixelShiftHorz(short *field, int width, int height, bool shift_right)
{
    int i, j, temp;
    std::unique_ptr<short[]> new_field(new short[width*height]);
    short *old_ptr = field, *new_ptr = new_field.get();

    old_ptr += 1;
    for (j = 0; j < height; j++)
    {
        temp = (old_ptr[j*width - 1] + old_ptr[j*width])>>1;
        new_ptr[j*width] = static_cast<short>(temp);
    }
    old_ptr += 1;
    new_ptr += 1;
    for (i = 2; i < width - 1; i++)
    {
        for (j = 0; j < height; j++)
        {
            temp = (5*(old_ptr[j*width-1] + old_ptr[j*width]) -
                (old_ptr[j*width-2] + old_ptr[j*width+1]))>>3;
            new_ptr[j*width] = static_cast<short>(temp);
        }
        old_ptr += 1;
        new_ptr += 1;
    }
    for (j = 0; j < height; j++)
    {
        temp = (old_ptr[j*width-1] + old_ptr[j*width])>>1;
        new_ptr[j*width] = static_cast<short>(temp);
    }
    if (shift_right)
    {
        new_ptr = new_field.get();
        old_ptr = field;
        for (j = 0; j < height; j++)
        {
            new_ptr[j*width] = old_ptr[j*width];
        }
    }
    else
    {
        new_ptr += 1;
        for (j = 0; j < height; j++)
        {
            new_ptr[j*width] = old_ptr[j*width];
        }
    }
    memcpy(field, new_field.get(), width * height * sizeof(short));
}
