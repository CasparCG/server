// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the BLUETIMER_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// BLUETIMER_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef BLUETIMER_EXPORTS
#define BLUETIMER_API __declspec(dllexport)
#else
#define BLUETIMER_API __declspec(dllimport)
#endif


extern "C"
{
BLUETIMER_API int BlueBlackTimer(int deviceId, int blackFrames1, int bmpFrames, int blackFrames2);
}
