#ifndef _MEDIAN_H
#define _MEDIAN_H

#include <vector>
using std::vector;

const unsigned int MAX_DEPTH = 25;
const unsigned int MAX_OPT = 9;

//////////////////////////////////////////////////////////////////////////////
// Class definition
//////////////////////////////////////////////////////////////////////////////
class Median : public GenericVideoFilter
{
public:
	Median(PClip _child, vector<PClip> _clips, unsigned int _low, unsigned int _high, bool _processchroma, IScriptEnvironment *env);
	~Median();

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

private:
    vector<PClip> clips;
    vector<VideoInfo> info;
    bool processchroma;
    unsigned int depth;
	unsigned int low;
	unsigned int high;
	unsigned int blend;
    bool fastprocess;
    unsigned char (*med)(unsigned char*);

    void ProcessPlane(int plane, PVideoFrame src[MAX_DEPTH], PVideoFrame& dst);
    void ProcessPlanarFrame(PVideoFrame src[MAX_DEPTH], PVideoFrame& dst);
    void ProcessInterleavedFrame(PVideoFrame src[MAX_DEPTH], PVideoFrame& dst);
    inline unsigned char ProcessPixel(unsigned char* values) const;
};


#endif // _MEDIAN_H
