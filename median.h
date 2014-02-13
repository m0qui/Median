#ifndef _MEDIAN_H
#define _MEDIAN_H

#include <vector>
using std::vector;

// This is tied to the functions available in opt_med.h
const unsigned int MAX_DEPTH = 9;

//////////////////////////////////////////////////////////////////////////////
// Class definition
//////////////////////////////////////////////////////////////////////////////
class Median : public GenericVideoFilter
{
public:
    Median(PClip _child, vector<PClip> _clips, bool _processchroma, IScriptEnvironment *env);
	~Median();

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

private:
    vector<PClip> clips;
    vector<VideoInfo> info;
    bool processchroma;
    unsigned int depth;
    unsigned char (*med)(unsigned char*);
    PVideoFrame output;

    void ProcessPlane(int plane, PVideoFrame src[MAX_DEPTH], PVideoFrame& dst);
    void ProcessPlanarFrame(PVideoFrame src[MAX_DEPTH], PVideoFrame& dst);
    void ProcessInterleavedFrame(PVideoFrame src[MAX_DEPTH], PVideoFrame& dst);
};


#endif // _MEDIAN_H
