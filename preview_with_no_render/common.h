#ifndef COMMON_H
#define COMMON_H

#define FHD_30_BITRATE 1400000
#define HD_30_BITRATE 600000
#define MAXPATHLENGTH 1024

template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

enum CameraState
{
	State_Stop = 0,
	State_Start
};

#endif // COMMON