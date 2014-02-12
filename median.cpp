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
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "median.h"

#define ERROR_PREFIX "Median: "


//////////////////////////////////////////////////////////////////////////////
// Constructor
//////////////////////////////////////////////////////////////////////////////
Median::Median(PClip _child, vector<PClip> _clips, bool _processchroma, IScriptEnvironment *env) :
  GenericVideoFilter(_child), clips(_clips), processchroma(_processchroma)
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

    if (!info[0].IsYUY2())
        env->ThrowError(ERROR_PREFIX "Only YUY2 is supported. Use ConvertToYUY2().");

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
    const unsigned char* srcp[MAX_DEPTH];

    for (unsigned int i = 0; i < depth; i++)
    {
        src[i] = clips[i]->GetFrame(n, env);
        srcp[i] = src[i]->GetReadPtr();
    }

    // Destination
    PVideoFrame dst = env->NewVideoFrame(vi);
    unsigned char* dstp = dst->GetWritePtr();

    // Loop over entire frame and calculate the resulting median
    for (int y = 0; y < info[0].height; ++y)
    {
        for (int x = 0; x < info[0].width; x++)
        {
            unsigned char luma[MAX_DEPTH];
            unsigned char chroma[MAX_DEPTH];

            for (unsigned int i = 0; i < depth; i++)
            {
                luma[i] = srcp[i][x * 2]; // Y#Y#Y#Y#...
                chroma[i] = srcp[i][x * 2 + 1]; // #U#V#U#V...
            }

            dstp[x * 2] = med(luma);

            if (processchroma)
                dstp[x * 2 + 1] = med(chroma);
            else
                dstp[x * 2 + 1] = chroma[0]; // Use chroma from first clip
        }

        // Advance to next line
        for (unsigned int i = 0; i < depth; i++)
            srcp[i] = srcp[i] + src[i]->GetPitch();

        dstp = dstp + dst->GetPitch();
    }

    return dst;
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

