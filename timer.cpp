#include <Windows.h>

bool initTimer(void)
{
	TIMECAPS timecaps = { 0 };

	if (timeGetDevCaps(&timecaps, sizeof(timecaps)) == MMSYSERR_NOERROR)
	{
		if (timecaps.wPeriodMin == 1)
			return (timeBeginPeriod(1) == MMSYSERR_NOERROR);
	}

	return false;
}

void deinitTimer(void)
{
	timeEndPeriod(1);
}