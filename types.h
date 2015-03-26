/*
********************************************************************
(c) MSU Video Group 2003-2010, http://compression.ru/video/
This source code is property of MSU Graphics and Media Lab

This code can be distributed and used only with WRITTEN PERMISSION.
********************************************************************
*/

#ifndef MOTIONESTIMATION_TYPES_H_
#define MOTIONESTIMATION_TYPES_H_

#include <memory>
#include <array>
#include <cassert>
#include <cstdint>

#include "Filter.h"
#include "ScriptInterpreter.h"
#include "ScriptError.h"
#include "ScriptValue.h"
#include "resource.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <commctrl.h>

#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>

enum Shift_Dir
{
    sd_none,
    sd_up,
    sd_l,
    sd_upl
};

struct MV
{
    /// Constructor
    MV(const int x = 0,
       const int y = 0,
       const Shift_Dir dir = sd_none,
       const long error = MAXLONG):
        x(x),
        y(y),
        dir(dir),
        error(error),
        subvectors_(nullptr)
    {}

    /// Copy constructor
    MV(const MV &other):
        x(other.x),
        y(other.y),
        dir(other.dir),
        error(other.error),
        subvectors_(nullptr)
    {
        if (other.subvectors_)
            subvectors_.reset(new SubVectors(*other.subvectors_));
    }

    /// Destructor
    ~MV() = default;

    /// Assignment operator
    MV&
    operator= (const MV &other)
    {
        x = other.x;
        y = other.y;
        dir = other.dir;
        error = other.error;
        if (other.subvectors_)
        {
            subvectors_.reset(new SubVectors(*other.subvectors_));
        }
        else
        {
            subvectors_.release();
        }
        return *this;
    }

    /// Split the motion vector in 4 subvectors
    void
    Split()
    {
        subvectors_.reset(new SubVectors);
    }

    /// Unsplit the motion vector
    void
    Unsplit()
    {
        subvectors_.release();
    }

    /// Check if a motion vector is split
    bool
    IsSplit() const
    {
        return (subvectors_ != nullptr);
    }

    /// Access a subvector
    MV&
    SubVector(const int id)
    {
        assert(subvectors_ && id >= 0 && id < 4);
        return (*subvectors_)[id];
    }

    int x;
    int y;
    Shift_Dir dir;
    long error;

private:
    typedef std::array<MV, 4> SubVectors;
    std::unique_ptr<SubVectors> subvectors_;
};

enum Method
{
    den
};

#endif
