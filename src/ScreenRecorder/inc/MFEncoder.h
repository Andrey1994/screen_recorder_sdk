#ifndef MFENCODER
#define MFENCODER

// clang-format off
#include <string>
#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <mferror.h>
// clang-format on


class MFEncoder
{

public:
    MFEncoder (int bitRate, int fps, const char *fileName, GUID videoInputFormat, int videoWidth,
        int videoHeight);
    ~MFEncoder ();

    HRESULT InitializeSinkWriter (IMFDXGIDeviceManager *deviceManager, bool useHardwareTransform);
    HRESULT WriteFrame (IUnknown *punkTexture);

private:
    int bitRate;
    int fps;
    std::string fileName;
    GUID videoInputFormat;
    int videoWidth;
    int videoHeight;
    DWORD streamIndex;
    IMFSinkWriter *pWriter;
    LONGLONG firstTime;
    LONGLONG prevTime;

    const GUID videoEncodingFormat = MFVideoFormat_H264;
};

#endif
