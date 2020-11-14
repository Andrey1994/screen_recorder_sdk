#include <d3d11.h>

#include "MFEncoder.h"
#include "Recorder.h"

#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "dxguid.lib")


MFEncoder::MFEncoder (int bitRate, int fps, const char *fileName, GUID videoInputFormat,
    int videoWidth, int videoHeight)
{
    this->pWriter = NULL;
    this->firstTime = 0;
    this->prevTime = 0;
    this->streamIndex = 0;
    this->bitRate = bitRate;
    this->fps = fps;
    this->fileName = fileName;
    this->videoInputFormat = videoInputFormat;
    this->videoHeight = videoHeight;
    this->videoWidth = videoWidth;
}

MFEncoder::~MFEncoder ()
{
    if (pWriter)
    {
        HRESULT hr = pWriter->Finalize ();
        pWriter->Release ();
        pWriter = NULL;
    }
    MFShutdown ();
}

HRESULT MFEncoder::InitializeSinkWriter (
    IMFDXGIDeviceManager *deviceManager, bool useHardwareTransform)
{
    IMFMediaType *pMediaTypeOut = NULL;
    IMFMediaType *pMediaTypeIn = NULL;
    IMFAttributes *pAttributes = NULL;

    HRESULT hr = MFStartup (MF_VERSION);
    if (useHardwareTransform)
    {
        const UINT32 cElements = 2;
        if (SUCCEEDED (hr))
        {
            Recorder::recordLogger->trace ("Before MFCreateAttributes");
            hr = MFCreateAttributes (&pAttributes, cElements);
        }
        if (SUCCEEDED (hr))
        {
            Recorder::recordLogger->trace ("Before set d3d manager");
            hr = pAttributes->SetUnknown (MF_SINK_WRITER_D3D_MANAGER, deviceManager);
        }
        if (SUCCEEDED (hr))
        {
            Recorder::recordLogger->trace ("Before enable hw transforms");
            hr = pAttributes->SetUINT32 (MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, 1);
        }
    }
    else
    {
        const UINT32 cElements = 1;
        if (SUCCEEDED (hr))
        {
            Recorder::recordLogger->trace ("Before MFCreateAttributes");
            hr = MFCreateAttributes (&pAttributes, cElements);
        }
        if (SUCCEEDED (hr))
        {
            Recorder::recordLogger->trace ("Before set d3d manager");
            hr = pAttributes->SetUnknown (MF_SINK_WRITER_D3D_MANAGER, deviceManager);
        }
    }

    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before MFCreateSinkWriterFromURL");
        std::wstring stemp = std::wstring (fileName.begin (), fileName.end ());
        LPCWSTR sw = stemp.c_str ();
        hr = MFCreateSinkWriterFromURL (sw, NULL, pAttributes, &pWriter);
    }
    // Set the output media type.
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before MFCreateMediaType");
        hr = MFCreateMediaType (&pMediaTypeOut);
    }
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before SetGUID MF_MT_MAJOR_TYPE");
        hr = pMediaTypeOut->SetGUID (MF_MT_MAJOR_TYPE, MFMediaType_Video);
    }
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before SetGUID MF_MT_SUBTYPE");
        hr = pMediaTypeOut->SetGUID (MF_MT_SUBTYPE, videoEncodingFormat);
    }
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before SetUINT32 MF_MT_AVG_BITRATE");
        hr = pMediaTypeOut->SetUINT32 (MF_MT_AVG_BITRATE, bitRate);
    }
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before SetUINT32 MF_MT_INTERLACE_MODE");
        hr = pMediaTypeOut->SetUINT32 (MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    }
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before MFSetAttributeSize MF_MT_FRAME_SIZE");
        hr = MFSetAttributeSize (pMediaTypeOut, MF_MT_FRAME_SIZE, videoWidth, videoHeight);
    }
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before MFSetAttributeRatio MF_MT_FRAME_RATE");
        hr = MFSetAttributeRatio (pMediaTypeOut, MF_MT_FRAME_RATE, fps, 1);
    }
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before MFSetAttributeRatio MF_MT_FRAME_RATE_RANGE_MAX");
        hr = MFSetAttributeRatio (pMediaTypeOut, MF_MT_FRAME_RATE_RANGE_MAX, 200, 1);
    }
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before MFSetAttributeRatio MF_MT_FRAME_RATE_RANGE_MIN");
        hr = MFSetAttributeRatio (pMediaTypeOut, MF_MT_FRAME_RATE_RANGE_MIN, 1, 1);
    }
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before MFSetAttributeRatio MF_MT_PIXEL_ASPECT_RATIO");
        hr = MFSetAttributeRatio (pMediaTypeOut, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    }
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before AddStream");
        hr = pWriter->AddStream (pMediaTypeOut, &streamIndex);
    }

    // Set the input media type.
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before MFCreateMediaType");
        hr = MFCreateMediaType (&pMediaTypeIn);
    }
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before SetGUID MF_MT_MAJOR_TYPE");
        hr = pMediaTypeIn->SetGUID (MF_MT_MAJOR_TYPE, MFMediaType_Video);
    }
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before SetGUID MF_MT_SUBTYPE");
        hr = pMediaTypeIn->SetGUID (MF_MT_SUBTYPE, videoInputFormat);
    }
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before SetUINT32 MF_MT_INTERLACE_MODE");
        hr = pMediaTypeIn->SetUINT32 (MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    }
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before SetUINT32 MF_SA_D3D11_AWARE");
        hr = pMediaTypeIn->SetUINT32 (MF_SA_D3D11_AWARE, 1);
    }
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before MFSetAttributeSize MF_MT_FRAME_SIZE");
        hr = MFSetAttributeSize (pMediaTypeIn, MF_MT_FRAME_SIZE, videoWidth, videoHeight);
    }
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before MFSetAttributeRatio MF_MT_FRAME_RATE");
        hr = MFSetAttributeRatio (pMediaTypeIn, MF_MT_FRAME_RATE, fps, 1);
    }
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before MFSetAttributeRatio MF_MT_PIXEL_ASPECT_RATIO");
        hr = MFSetAttributeRatio (pMediaTypeIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    }
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before SetInputMediaType streamIndex");
        hr = pWriter->SetInputMediaType (streamIndex, pMediaTypeIn, NULL);
    }
    if (SUCCEEDED (hr))
    {
        Recorder::recordLogger->trace ("Before BeginWriting");
        hr = pWriter->BeginWriting ();
    }

    if (pMediaTypeOut)
    {
        pMediaTypeOut->Release ();
        pMediaTypeOut = NULL;
    }
    if (pMediaTypeIn)
    {
        pMediaTypeIn->Release ();
        pMediaTypeIn = NULL;
    }

    return hr;
}

HRESULT MFEncoder::WriteFrame (IUnknown *texture)
{
    FILETIME ft;
    GetSystemTimePreciseAsFileTime (&ft);
    LONGLONG currTime = ((int64_t)ft.dwHighDateTime << 32L) | (int64_t)ft.dwLowDateTime;
    // skip first frame to measure frameduration properly
    if ((!firstTime) && (!prevTime))
    {
        firstTime = currTime;
        prevTime = currTime;
        return S_OK;
    }

    IMFSample *pSample = NULL;
    IMFMediaBuffer *pBuffer = NULL;

    HRESULT hr = MFCreateDXGISurfaceBuffer (IID_ID3D11Texture2D, texture, 0, FALSE, &pBuffer);
    if (SUCCEEDED (hr))
        hr = pBuffer->SetCurrentLength (4 * videoWidth * videoHeight);
    // Create a media sample and add the buffer to the sample.
    if (SUCCEEDED (hr))
        hr = MFCreateSample (&pSample);
    if (SUCCEEDED (hr))
        hr = pSample->AddBuffer (pBuffer);
    // Set the time stamp and the duration.
    if (SUCCEEDED (hr))
        hr = pSample->SetSampleTime (prevTime - firstTime);
    if (SUCCEEDED (hr))
        hr = pSample->SetSampleDuration (currTime - prevTime);
    // Send the sample to the Sink Writer.
    if (SUCCEEDED (hr))
        hr = pWriter->WriteSample (streamIndex, pSample);
    if (SUCCEEDED (hr))
        prevTime = currTime;

    if (pSample)
    {
        pSample->Release ();
        pSample = NULL;
    }
    if (pBuffer)
    {
        pBuffer->Release ();
        pBuffer = NULL;
    }

    return hr;
}
