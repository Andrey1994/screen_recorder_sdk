#ifndef RECORDERDDA
#define RECORDERDDA

// clang-format off
#include <windows.h>
#include <d3d11_4.h>
#include <dxgi1_2.h>

#include "ScreenRecorder.h"
#include "MFEncoder.h"
#include "Recorder.h"
// clang-format on

#define MAX_DISPLAYS 6


class RecorderDDA : public Recorder
{
public:
    RecorderDDA (struct RecorderParams recoderParams);
    ~RecorderDDA ();

    int InitResources ();
    int GetScreenShot (
        unsigned int maxAttempts, unsigned char *frameBuffer, int *width, int *height);
    int StartVideoRecording (
        const char *outputFileName, int frameRate, int bitRate, bool useHardwareTransform);
    int StopVideoRecording ();

private:
    int FreeResources ();
    int GetScreenShotOfDisplay (
        unsigned int maxAttempts, ID3D11Texture2D **desktopTexture, unsigned int displayNum);
    bool CheckWindowOnDisplay (const RECT &display, const RECT &processRect);
    int GetProcessWindow (RECT *rc, int *processDisplay);
    int GetImageFromTexture (ID3D11Texture2D *destImage, int displayNum, RECT rcWind,
        volatile unsigned char *frameBuffer, volatile int *width, volatile int *height);
    int PrepareSession ();
    bool CheckThreadAlive ();
    int ReleaseDDA ();

    ID3D11Device *device;
    ID3D11DeviceContext *immediateContext;
    IMFDXGIDeviceManager *deviceManager;
    DXGI_OUTPUT_DESC outputDescs[MAX_DISPLAYS];
    IDXGIOutputDuplication *deskDupls[MAX_DISPLAYS];
    D3D11_TEXTURE2D_DESC descs[MAX_DISPLAYS];
    MFEncoder *mfenc;
    int totalDisplays;

    volatile unsigned int screenMaxAttempts;
    volatile unsigned char *imageBuffer;
    volatile int *imageWidth;
    volatile int *imageBufferWidth;
    volatile int *imageHeight;
    volatile char videoFileName[1024];
    volatile int frameRate;
    volatile int bitRate;
    volatile bool useHardwareTransform;

    static DWORD WINAPI CaptureThreadProc (LPVOID lpParameter);
    void WorkerThread ();
    volatile HANDLE hThread;
    HANDLE hEventSessionReady;
    HANDLE hEventStart;

    volatile bool threadMustExit;
    volatile int threadRes;
    volatile bool needScreen;
    volatile bool needVideo;

    CONDITION_VARIABLE condVar;
    CRITICAL_SECTION critSect;

    const static D3D_DRIVER_TYPE driverTypes[];
    const static D3D_FEATURE_LEVEL featureLevels[];
};

#endif
