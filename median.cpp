#include "stdafx.h"
#include "median.h"


//////////////////////////////////////////////////////////////////////////////
// Constructor
//////////////////////////////////////////////////////////////////////////////
Median::Median(PClip _child, vector<PClip> _clips, unsigned int _low, unsigned int _high, bool _processchroma, IScriptEnvironment *env) :
  GenericVideoFilter(_child), low(_low), high(_high), clips(_clips), processchroma(_processchroma)
{
    depth = clips.size();

	blend = depth - low - high;

    if (blend == 1 && low == high && depth < MAX_OPT)
        fastprocess = true;
    else
        fastprocess = false;

    //env->ThrowError("depth %d blend %d low %d high %d fast %d", depth, blend, low, high, (int)fastprocess);

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

	// Output
	PVideoFrame output = env->NewVideoFrame(vi);

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

            dstp[x] = ProcessPixel(values);
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

                dstp[x * 2] = ProcessPixel(luma);

                if (processchroma)
                    dstp[x * 2 + 1] = ProcessPixel(chroma);
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

                dstp[x * 3] = ProcessPixel(b);
                dstp[x * 3 + 1] = ProcessPixel(g);
                dstp[x * 3 + 2] = ProcessPixel(r);
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

                dstp[x * 4] = ProcessPixel(b);
                dstp[x * 4 + 1] = ProcessPixel(g);
                dstp[x * 4 + 2] = ProcessPixel(r);

                if (processchroma)
                    dstp[x * 4 + 3] = ProcessPixel(a);
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
// Processing of a stack of pixel values
//////////////////////////////////////////////////////////////////////////////
inline unsigned char Median::ProcessPixel(unsigned char* values) const
{
    unsigned char output;

    if (fastprocess) // Can use a fast method
    {
        output = med(values);
    }
    else // Have to sort the whole thing
    {
        std::sort(values, values + depth);

        if (blend > 1) // Need to average middle values
        {
            unsigned int sum = 0;

            for (unsigned int i = low; i < low + blend; i++)
                sum = sum + values[i];

            output = sum / blend;
        }
        else // Return a single value
        {
            output = values[low];
        }
    }

    return output;
}


