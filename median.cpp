//////////////////////////////////////////////////////////////////////////////
// Median filter for AviSynth
//
// This filter will take in a number of clips and calculate a pixel-by-pixel
// median out of them. This is useful for reducing noise and glitches in
// analog tape captures, but may have other uses as well.
//
// Author: antti.korhola@gmail.com
//
// License: Public domain. Credit would be nice, but do with this what you will.
//
// Forum thread: http://forum.doom9.org/showthread.php?t=170216
//
// History:
// 12-Feb-2014,  0.1: Initial release. YUY2 support only.
// 13-Feb-2014,  0.2: Added support for RGB and planar formats
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "median.h"

#define ERROR_PREFIX "Median: "


//////////////////////////////////////////////////////////////////////////////
// Constructor
//////////////////////////////////////////////////////////////////////////////
Median::Median(PClip _child, vector<PClip> _clips, bool _processchroma, IScriptEnvironment *env) :
GenericVideoFilter(_child), clips(_clips), processchroma(_processchroma), output(env->NewVideoFrame(vi))
{
    depth = clips.size();

    switch (depth)
    {
        case 3:
            med = opt_med3;
            break;
        case 5:
            med = opt_med5;
            break;
        case 7:
            med = opt_med7;
            break;
        case 9:
            med = opt_med9;
            break;
        default:
            env->ThrowError(ERROR_PREFIX "This should not happen, but somehow an unexpected number of clips ended up being processed.");
    }

    for (unsigned int i = 0; i < depth; i++)
        info.push_back(clips[i]->GetVideoInfo());

    for (unsigned int i = 1; i < depth; i++)
    {
        if (!info[i].IsSameColorspace(info[0]))
            env->ThrowError(ERROR_PREFIX "Format of all clips must match.");
    }

    for (unsigned int i = 1; i < depth; i++)
    {
        if (info[i].width != info[0].width || info[i].height != info[0].height)
            env->ThrowError(ERROR_PREFIX "Dimensions of all clips must match.");
    }
}


//////////////////////////////////////////////////////////////////////////////
// Destructor
//////////////////////////////////////////////////////////////////////////////
Median::~Median()
{
}


//////////////////////////////////////////////////////////////////////////////
// Actual image processing operations
//////////////////////////////////////////////////////////////////////////////
PVideoFrame __stdcall Median::GetFrame(int n, IScriptEnvironment* env)
{
    // Source
    PVideoFrame src[MAX_DEPTH];

    for (unsigned int i = 0; i < depth; i++)
        src[i] = clips[i]->GetFrame(n, env);

    // Select between planar and interleaved processing
    if (info[0].IsPlanar())
        ProcessPlanarFrame(src, output);
    else
        ProcessInterleavedFrame(src, output);

    return output;
}


//////////////////////////////////////////////////////////////////////////////
// Image processing for planar images
//////////////////////////////////////////////////////////////////////////////
void Median::ProcessPlanarFrame(PVideoFrame src[MAX_DEPTH], PVideoFrame& dst)
{
    // Luma
    ProcessPlane(PLANAR_Y, src, dst);

    // Chroma
    if (processchroma)
    {
        ProcessPlane(PLANAR_U, src, dst);
        ProcessPlane(PLANAR_V, src, dst);
    }
}


//////////////////////////////////////////////////////////////////////////////
// Processing of a single plane
//////////////////////////////////////////////////////////////////////////////
void Median::ProcessPlane(int plane, PVideoFrame src[MAX_DEPTH], PVideoFrame& dst)
{
    // Source
    const unsigned char* srcp[MAX_DEPTH];

    for (unsigned int i = 0; i < depth; i++)
        srcp[i] = src[i]->GetReadPtr(plane);
    
    // Destination
    unsigned char* dstp = dst->GetWritePtr(plane);

    // Dimensions
    const int width = src[0]->GetRowSize(plane);
    const int height = src[0]->GetHeight(plane);

    // Process
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            unsigned char values[MAX_DEPTH];

            for (unsigned int i = 0; i < depth; i++)
                values[i] = srcp[i][x];

            dstp[x] = med(values);
        }

        for (unsigned int i = 0; i < depth; i++)
            srcp[i] = srcp[i] + src[i]->GetPitch(plane);

        dstp = dstp + dst->GetPitch(plane);
    }
}


//////////////////////////////////////////////////////////////////////////////
// Image processing for interleaved images
//////////////////////////////////////////////////////////////////////////////
void Median::ProcessInterleavedFrame(PVideoFrame src[MAX_DEPTH], PVideoFrame& dst)
{
    // Source
    const unsigned char* srcp[MAX_DEPTH];

    for (unsigned int i = 0; i < depth; i++)
        srcp[i] = src[i]->GetReadPtr();

    // Destination
    unsigned char* dstp = dst->GetWritePtr();

    // Dimensions
    const int width = info[0].width;
    const int height = info[0].height;

    // Process
    if (info[0].IsYUY2())
    {
        //////////////////////////////////////////////////////////////////////
        // YUYV
        //////////////////////////////////////////////////////////////////////
        unsigned char luma[MAX_DEPTH];
        unsigned char chroma[MAX_DEPTH];

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; x++)
            {
                for (unsigned int i = 0; i < depth; i++)
                {
                    luma[i] = srcp[i][x * 2];
                    chroma[i] = srcp[i][x * 2 + 1];
                }

                dstp[x * 2] = med(luma);

                if (processchroma)
                    dstp[x * 2 + 1] = med(chroma);
                else
                    dstp[x * 2 + 1] = chroma[0]; // Use chroma from first clip
            }

            for (unsigned int i = 0; i < depth; i++)
                srcp[i] = srcp[i] + src[i]->GetPitch();

            dstp = dstp + dst->GetPitch();
        }
    }
    else if (info[0].IsRGB24())
    {
        //////////////////////////////////////////////////////////////////////
        // BGR
        //////////////////////////////////////////////////////////////////////
        unsigned char b[MAX_DEPTH];
        unsigned char g[MAX_DEPTH];
        unsigned char r[MAX_DEPTH];

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; x++)
            {
                for (unsigned int i = 0; i < depth; i++)
                {
                    b[i] = srcp[i][x * 3];
                    g[i] = srcp[i][x * 3 + 1];
                    r[i] = srcp[i][x * 3 + 2];
                }

                dstp[x * 3] = med(b);
                dstp[x * 3 + 1] = med(g);
                dstp[x * 3 + 2] = med(r);
            }

            for (unsigned int i = 0; i < depth; i++)
                srcp[i] = srcp[i] + src[i]->GetPitch();

            dstp = dstp + dst->GetPitch();
        }
    }
    else if (info[0].IsRGB32())
    {
        //////////////////////////////////////////////////////////////////////
        // BGRA
        //////////////////////////////////////////////////////////////////////
        unsigned char b[MAX_DEPTH];
        unsigned char g[MAX_DEPTH];
        unsigned char r[MAX_DEPTH];
        unsigned char a[MAX_DEPTH];

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; x++)
            {
                for (unsigned int i = 0; i < depth; i++)
                {
                    b[i] = srcp[i][x * 4];
                    g[i] = srcp[i][x * 4 + 1];
                    r[i] = srcp[i][x * 4 + 2];
                    a[i] = srcp[i][x * 4 + 3];
                }

                dstp[x * 4] = med(b);
                dstp[x * 4 + 1] = med(g);
                dstp[x * 4 + 2] = med(r);

                if (processchroma)
                    dstp[x * 4 + 3] = med(a);
                else
                    dstp[x * 4 + 3] = a[0]; // Use alpha from first clip
            }

            for (unsigned int i = 0; i < depth; i++)
                srcp[i] = srcp[i] + src[i]->GetPitch();

            dstp = dstp + dst->GetPitch();
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
// Create filter
//////////////////////////////////////////////////////////////////////////////
AVSValue __cdecl Create_Median(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    AVSValue array = args[0];
    int n = array.ArraySize();

    if (n != 3 && n != 5 && n != 7 && n != 9)
        env->ThrowError(ERROR_PREFIX "Need 3, 5, 7 or 9 clips.");

    vector<PClip> clips;

    for (int i = 0; i < n; i++)
        clips.push_back(array[i].AsClip());

    return new Median(clips[0], clips, args[1].AsBool(true), env);
}


//////////////////////////////////////////////////////////////////////////////
// Add filter
//////////////////////////////////////////////////////////////////////////////
extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
    env->AddFunction("Median", "c+[CHROMA]b", Create_Median, 0);

    return "Median of clips filter";
}

