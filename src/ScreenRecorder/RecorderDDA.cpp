#include <dwmapi.h>
#include <algorithm>
#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <mferror.h>

#include "Recorder.h"
#include "RecorderDDA.h"
#include "ScreenRecorder.h"
#include "HandleD3DWindow.h"

#pragma comment (lib, "D3D11.lib")

#define MAX_DDA_INIT_TIME 20000


const D3D_DRIVER_TYPE RecorderDDA::driverTypes[] =
{
    D3D_DRIVER_TYPE_HARDWARE,
    D3D_DRIVER_TYPE_WARP,
    D3D_DRIVER_TYPE_REFERENCE,
};

// Feature levels supported
const D3D_FEATURE_LEVEL RecorderDDA::featureLevels[] =
{
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
    D3D_FEATURE_LEVEL_9_1
};

RecorderDDA::RecorderDDA (int pid) : Recorder (pid)
{
    this->threadRes = SYNC_TIMEOUT_ERROR;
    this->imageBuffer = NULL;
    this->imageWidth = NULL;
    this->imageHeight = NULL;
    this->needScreen = FALSE;
    this->needVideo = FALSE;
    this->screenMaxAttempts = 1;
    this->frameRate = 0;
    this->bitRate = 0;
    this->useHardwareTransform = FALSE;

    this->totalDisplays = 0;
    this->device = NULL;
    this->immediateContext = NULL;
    this->deviceManager = NULL;
    this->hThread = NULL;
    this->hEventSessionReady = NULL;
    this->hEventStart = NULL;
    this->mfenc = NULL;
    this->threadMustExit = FALSE;
    for (int i = 0; i < MAX_DISPLAYS; i++)
    {
        memset (&outputDescs[i], 0, sizeof (DXGI_OUTPUT_DESC));
        memset (&descs[i], 0, sizeof (D3D11_TEXTURE2D_DESC));
        deskDupls[i] = NULL;
    }

    InitializeCriticalSection (&critSect);
    InitializeConditionVariable (&condVar);
}

RecorderDDA::~RecorderDDA ()
{
    FreeResources ();
    DeleteCriticalSection (&critSect);
}

int RecorderDDA::InitResources ()
{
    if (!this->hThread)
    {
        FreeResources ();
        this->hEventSessionReady = CreateEvent (NULL, TRUE, FALSE, TEXT ("InitEvent"));
        if (this->hEventSessionReady == NULL)
        {
            Recorder::recordLogger->error ("Create Event InitEvent");
            return GENERAL_ERROR;
        }
        this->hEventStart = CreateEvent (NULL, TRUE, FALSE, TEXT ("StartEvent"));
        if (this->hEventStart == NULL)
        {
            Recorder::recordLogger->error ("Create Event StartEvent");
            return GENERAL_ERROR;
        }
        this->threadMustExit = FALSE;
        this->hThread = CreateThread (NULL, 0, CaptureThreadProc, this, 0, NULL);
        DWORD dwWait = WaitForSingleObject (this->hEventSessionReady, MAX_DDA_INIT_TIME);
        if (dwWait == WAIT_TIMEOUT)
        {
            Recorder::recordLogger->error ("wait fot init error");
            return SYNC_TIMEOUT_ERROR;
        }
        return threadRes;
    }
    else
    {
        Recorder::recordLogger->trace ("Resources already initialized");
        return RECORDING_ALREADY_RUN_ERROR;
    }
}

int RecorderDDA::FreeResources ()
{
    StopVideoRecording ();
    if (this->hThread)
    {
        EnterCriticalSection (&critSect);
        this->threadMustExit = TRUE;
        LeaveCriticalSection (&critSect);
        WaitForSingleObject (this->hThread, INFINITE);
        CloseHandle (hThread);
        CloseHandle (hEventSessionReady);
        CloseHandle (hEventStart);
        this->hEventSessionReady = NULL;
        this->hEventStart = NULL;
        this->hThread = NULL;
    }
    else
        return STATUS_OK;
}

int RecorderDDA::GetScreenShot (unsigned int maxAttempts, unsigned char *frameBuffer, int *width, int *height)
{
    if (CheckThreadAlive ())
    {
        if (!SetEvent (this->hEventStart))
        {
            Recorder::recordLogger->error ("Unable to set startevent");
            return GENERAL_ERROR;
        }
        EnterCriticalSection (&critSect);
        this->needScreen = TRUE;
        this->imageBuffer = frameBuffer;
        this->imageWidth = width;
        this->imageHeight = height;
        this->screenMaxAttempts = maxAttempts;
        SleepConditionVariableCS (&condVar, &critSect, maxAttempts * 4000); // 4 times bigger than capture timeout
        this->needScreen = FALSE;
        int res = this->threadRes;
        threadRes = SYNC_TIMEOUT_ERROR;
        LeaveCriticalSection (&critSect);
        return res;
    }
    else
        return threadRes;
}

int RecorderDDA::StartVideoRecording (const char *outputFileName, int frameRate, int bitRate, bool useHardwareTransform)
{
    if (CheckThreadAlive ())
    {
        Recorder::recordLogger->trace ("start video recording is called");
        if (!SetEvent (this->hEventStart))
        {
            Recorder::recordLogger->error ("unable to set startevent");
            return GENERAL_ERROR;
        }
        EnterCriticalSection (&critSect);
        this->needVideo = TRUE;
        strcpy ((char *)this->videoFileName, outputFileName);
        this->frameRate = frameRate;
        this->bitRate = bitRate;
        this->useHardwareTransform = useHardwareTransform;
        // sync StopVideoRecording and StartVideoRecording are called without delay
        bool waitRes = SleepConditionVariableCS (&condVar, &critSect, 2000);
        Recorder::recordLogger->trace ("wait res for start video is {}", waitRes);
        LeaveCriticalSection (&critSect);
        return STATUS_OK;
    }
    else
        return threadRes;
}

int RecorderDDA::StopVideoRecording () {
    if (CheckThreadAlive ())
    {
        Recorder::recordLogger->trace ("stop video recording is called");
        EnterCriticalSection (&critSect);
        this->needVideo = FALSE;
        this->frameRate = 0;
        this->bitRate = 0;
        this->useHardwareTransform = FALSE;
        // sync StopVideoRecording and StartVideoRecording are called without delay
        bool waitRes = SleepConditionVariableCS (&condVar, &critSect, 2000);
        Recorder::recordLogger->trace ("wait res for stop video is {}", waitRes);
        LeaveCriticalSection (&critSect);
        return STATUS_OK;
    }
    else
        return RECORDING_THREAD_IS_NOT_RUNNING_ERROR;
}

DWORD WINAPI RecorderDDA::CaptureThreadProc (LPVOID pRecorderDDA)
{
    ((RecorderDDA *) pRecorderDDA)->WorkerThread ();
    return 0;
}

void RecorderDDA::WorkerThread ()
{
    HRESULT hr = CoInitializeEx (NULL, COINIT_MULTITHREADED);
    if (FAILED (hr))
    {
        Recorder::recordLogger->error ("unable to Coinitialize");
        threadRes = RECORDING_THREAD_ERROR;
        return;
    }
    threadRes = PrepareSession ();
    if (!SetEvent (this->hEventSessionReady))
    {
        Recorder::recordLogger->error ("unable to setevent");
        threadRes = RECORDING_THREAD_ERROR;
        CoUninitialize ();
        return;
    }

    // wait for start
    WaitForSingleObject (this->hEventStart, INFINITE);

    int currentAttempt = 0;
    int maxAttempts = 7;
    int displayNum = 0;
    RECT rcWind;
    SetRect (&rcWind, 0, 0, 0, 0);
    int width = 0;
    int height = 0;
    bool isRecording = FALSE;
    ID3D11Texture2D *destImage = NULL;
    if (pid)
    {
        if (GetProcessWindow (&rcWind, &displayNum) != STATUS_OK)
        {
            Recorder::recordLogger->error ("Unable to get process window");
            threadRes = NO_SUCH_PROCESS_ERROR;
            CoUninitialize ();
            return;
        }
    }
    width = outputDescs[displayNum].DesktopCoordinates.right - outputDescs[displayNum].DesktopCoordinates.left;
    height = outputDescs[displayNum].DesktopCoordinates.bottom - outputDescs[displayNum].DesktopCoordinates.top;
    while (!threadMustExit)
    {
        if (destImage != NULL)
        {
            destImage->Release ();
            destImage = NULL;
        }

        int res = GetScreenShotOfDisplay (1, &destImage, displayNum);
        // retry if unable to capture
        if ((res != STATUS_OK) || (!destImage))
        {
            // unable to capture screen in maxattemps exit from loop
            if (currentAttempt >= maxAttempts)
            {
                Recorder::recordLogger->error ("Screen Capture Doesn't work");
                threadRes = res;
                break;
            }
            // handle lost access or create texture error
            if ((res == DDA_LOST_ACCESS_ERROR) || (res == CREATE_TEXTURE_ERROR))
            {
                if (destImage)
                {
                    destImage->Release ();
                    destImage = NULL;
                }
                res = ReleaseDDA ();
                if (res != STATUS_OK)
                {
                    Recorder::recordLogger->error ("Unable to free dda resources");
                    threadRes = res;
                    break;
                }
                Sleep (2000);
                res = PrepareSession ();
                if (res != STATUS_OK)
                {
                    Recorder::recordLogger->error ("Unable to reinit dda resources after DDA_LOST_ACCESS_ERROR or CREATE_TEXTURE_ERROR");
                    threadRes = res;
                    break;
                }
                if (pid)
                {
                    if (GetProcessWindow (&rcWind, &displayNum) != STATUS_OK)
                    {
                        Recorder::recordLogger->error ("Unable to get process window");
                        threadRes = NO_SUCH_PROCESS_ERROR;
                        break;
                    }
                }
                width = outputDescs[displayNum].DesktopCoordinates.right - outputDescs[displayNum].DesktopCoordinates.left;
                height = outputDescs[displayNum].DesktopCoordinates.bottom - outputDescs[displayNum].DesktopCoordinates.top;
                // looks like there is a raise condition inside API, call it twice instead Sleep
                // you will get a black image with correct exit code otherwise
                res = GetScreenShotOfDisplay (1, &destImage, displayNum);
                currentAttempt = 0;
                continue;
            }
            // all failures except timeout, timeout is handled in needScreen
            if (res != DDA_TIMEOUT_ERROR)
            {
                currentAttempt++;
                continue;
            }
        }
        // everything is ok at this point
        else
        {
            currentAttempt = 0;
        }
        if (this->needScreen)
        {
            if ((res == DDA_TIMEOUT_ERROR) && (currentAttempt < screenMaxAttempts))
            {
                currentAttempt++;
                Recorder::recordLogger->trace ("Unable to capture image for display {}, res {}", displayNum, res);
                continue;
            }
            EnterCriticalSection (&critSect);
            if (destImage)
                threadRes = GetImageFromTexture (destImage, displayNum, rcWind, imageBuffer, imageWidth, imageHeight);
            else
                Recorder::recordLogger->error ("Unable to capture image for display {}, res {}", displayNum, threadRes);
            currentAttempt = 0;
            needScreen = FALSE;
            LeaveCriticalSection (&critSect);
            WakeConditionVariable (&condVar);
        }
        if (this->needVideo)
        {
            // start recording video
            if (!isRecording)
            {
                WakeConditionVariable (&condVar);
                Recorder::recordLogger->trace ("starting video recording");
                if (mfenc != NULL)
                {
                    Recorder::recordLogger->error ("MFEncoder is not null");
                    delete mfenc;
                    mfenc = NULL;
                }
                EnterCriticalSection (&critSect);
                mfenc = new MFEncoder (this->bitRate, this->frameRate, (char *)this->videoFileName, MFVideoFormat_ARGB32, width, height);
                isRecording = TRUE;
                HRESULT hr = mfenc->InitializeSinkWriter (deviceManager, this->useHardwareTransform);
                if (FAILED (hr))
                {
                    Recorder::recordLogger->error ("Could not init InitializeSinkWriter");
                    isRecording = FALSE;
                }
                else
                {
                    Recorder::recordLogger->info ("Start recording video to {}", (char *)this->videoFileName);
                }
                LeaveCriticalSection (&critSect);
            }
            // add frame
            else
            {
                if (destImage)
                {
                    HRESULT hr = mfenc->WriteFrame (destImage);
                    if (FAILED (hr))
                    {
                        Recorder::recordLogger->trace ("Unable to write frame");
                    }
                }
                else
                {
                    Recorder::recordLogger->trace ("destImage is NULL");
                }
            }
        }
        else
        {
            // stop recording video
            if (isRecording)
            {
                WakeConditionVariable (&condVar);
                Recorder::recordLogger->info ("Stop recording video");
                delete mfenc;
                mfenc = NULL;
                isRecording = FALSE;
            }
        }
    }
    // sanity check
    if (mfenc)
    {
        WakeConditionVariable (&condVar);
        Recorder::recordLogger->info ("Stop recording video");
        delete mfenc;
        mfenc = NULL;
        isRecording = FALSE;
    }
    // free resources
    ReleaseDDA ();
    CoUninitialize ();
}

int RecorderDDA::PrepareSession ()
{
    if (pid != 0)
    {
        HANDLE processHandle = OpenProcess (PROCESS_QUERY_INFORMATION, 0, pid);
        if (!processHandle)
        {
            Recorder::recordLogger->error ("No such process {}, GetLastError is {}", pid, GetLastError());
            return NO_SUCH_PROCESS_ERROR;
        }
        CloseHandle (processHandle);
    }

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr (E_FAIL);

    for (UINT driverTypeIndex = 0; driverTypeIndex < ARRAYSIZE (driverTypes); driverTypeIndex++)
    {
        hr = D3D11CreateDevice (
            NULL,
            driverTypes[driverTypeIndex],
            NULL,
            D3D11_CREATE_DEVICE_VIDEO_SUPPORT,
            featureLevels,
            ARRAYSIZE (featureLevels),
            D3D11_SDK_VERSION,
            &device,
            &featureLevel,
            &immediateContext
        );

        if (SUCCEEDED (hr))
        {
            Recorder::recordLogger->info ("Created d3d11 device from {}", driverTypeIndex);
            break;
        }
        else
        {
            if (device)
            {
                device->Release ();
                device = NULL;
            }
            if (immediateContext)
            {
                immediateContext->Release ();
                immediateContext = NULL;
            }
        }
    }

    if ((FAILED (hr)) || (device == NULL))
    {
        Recorder::recordLogger->error ("unable to create d3d11 device, HRESULT is {}", hr);
        return PREPARE_DESK_DUPL_ERROR;
    }

    ID3D11Multithread *multiThreaded = NULL;
    hr = device->QueryInterface (IID_PPV_ARGS (&multiThreaded));
    if (FAILED (hr))
    {
        Recorder::recordLogger->error ("unable to get multithreaded interface");
        return PREPARE_DESK_DUPL_ERROR;
    }
    multiThreaded->SetMultithreadProtected (TRUE);

    UINT resetToken = NULL;

    hr = MFCreateDXGIDeviceManager (&resetToken, &deviceManager);
    if (FAILED (hr))
    {
        Recorder::recordLogger->error ("failed to create deviceManager");
        return PREPARE_DESK_DUPL_ERROR;
    }
    hr = deviceManager->ResetDevice (device, resetToken);
    if (FAILED (hr))
    {
        Recorder::recordLogger->error ("failed to create ResetDevice");
        return PREPARE_DESK_DUPL_ERROR;
    }
    // Get DXGI device
    IDXGIDevice *dxgiDevice = NULL;
    hr = device->QueryInterface (IID_PPV_ARGS (&dxgiDevice));
    if (FAILED (hr))
    {
        Recorder::recordLogger->error ("unable to get dxgidevice");
        return PREPARE_DESK_DUPL_ERROR;
    }

    // Get DXGI adapter
    IDXGIAdapter *dxgiAdapter = NULL;
    hr = dxgiDevice->GetParent (
        __uuidof (IDXGIAdapter),
        reinterpret_cast<void**>(&dxgiAdapter));
    dxgiDevice->Release ();
    if (FAILED (hr))
    {
        Recorder::recordLogger->error ("unable to get dxgiadapter");
        return PREPARE_DESK_DUPL_ERROR;
    }

    IDXGIOutput *dxgiOutputs[MAX_DISPLAYS];
    for (int i = 0; i < MAX_DISPLAYS; i++)
    {
        dxgiOutputs[i] = NULL;
    }
    for (int i = 0; i < MAX_DISPLAYS; i++)
    {
        if (dxgiAdapter->EnumOutputs (i, &dxgiOutputs[i]) != DXGI_ERROR_NOT_FOUND)
        {
            totalDisplays = i + 1;
        }
    }
    dxgiAdapter->Release ();
    if (totalDisplays == 0)
    {
        Recorder::recordLogger->error ("unable to get dxgioutput");
        return PREPARE_DESK_DUPL_ERROR;
    }
    else
    {
        Recorder::recordLogger->info ("found {} displays", totalDisplays);
    }

    // get output descriptions for all outputs
    DXGI_OUTPUT_DESC outputDesc;
    for (int i = 0; i < totalDisplays; i++)
    {
        hr = dxgiOutputs[i]->GetDesc (&outputDesc);
        if (FAILED (hr))
        {
            Recorder::recordLogger->error ("unable to get dxgioutputdesc");
            return PREPARE_DESK_DUPL_ERROR;
        }
        outputDescs[i] = outputDesc;
    }

    // get output1 for all outputs
    IDXGIOutput1 *dxgiOutput1s[MAX_DISPLAYS];
    for (int i = 0; i < MAX_DISPLAYS; i++)
    {
        dxgiOutput1s[i] = NULL;
    }
    for (int i = 0; i < totalDisplays; i++)
    {
        hr = dxgiOutputs[i]->QueryInterface (IID_PPV_ARGS (&dxgiOutput1s[i]));
        if (FAILED (hr))
        {
            break;
        }
    }
    for (int i = 0; i < totalDisplays; i++)
    {
        dxgiOutputs[i]->Release ();
    }
    if (FAILED (hr))
    {
        Recorder::recordLogger->error ("unable to get dxgiOutput1");
        return PREPARE_DESK_DUPL_ERROR;
    }

    // create deskDupls for all displays
    for (int i = 0; i < totalDisplays; i++)
    {
        hr = dxgiOutput1s[i]->DuplicateOutput (device, &deskDupls[i]);
        if (FAILED (hr))
        {
            break;
        }
    }
    for (int i = 0; i < totalDisplays; i++)
    {
        dxgiOutput1s[i]->Release ();
    }
    if (FAILED (hr))
    {
        switch (hr)
        {
            case E_INVALIDARG :
                Recorder::recordLogger->error ("invalid arg to create desktopdiplication");
                return PREPARE_DESK_DUPL_ERROR;
            case E_ACCESSDENIED :
                Recorder::recordLogger->error ("no priviligies to create desktopdiplication");
                return PREPARE_DESK_DUPL_ERROR;
            case DXGI_ERROR_UNSUPPORTED :
                Recorder::recordLogger->error ("desktopdiplication is unsupported on current desktop mode");
                return PREPARE_DESK_DUPL_ERROR;
            case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE :
                Recorder::recordLogger->error ("desktopdiplication is not available");
                return PREPARE_DESK_DUPL_ERROR;
            case DXGI_ERROR_SESSION_DISCONNECTED :
                Recorder::recordLogger->error ("desktopdiplication session is currently disconnected");
                return PREPARE_DESK_DUPL_ERROR;
            default :
                Recorder::recordLogger->error ("unable to create desktopduplication");
                return PREPARE_DESK_DUPL_ERROR;
        }
    }

    // Create GUI drawing texture
    DXGI_OUTDUPL_DESC outputDuplDesc;
    DXGI_OUTDUPL_DESC outputDuplDescs[MAX_DISPLAYS];
    for (int i = 0; i < totalDisplays; i++)
    {
        deskDupls[i]->GetDesc (&outputDuplDesc);
        outputDuplDescs[i] = outputDuplDesc;
    }
    for (int i = 0; i < totalDisplays; i++)
    {
        D3D11_TEXTURE2D_DESC desc;
        desc.Width = outputDuplDescs[i].ModeDesc.Width;
        desc.Height = outputDuplDescs[i].ModeDesc.Height;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.ArraySize = 1;
        desc.BindFlags = 0;
        desc.MiscFlags = 0;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.MipLevels = 1;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
        desc.Usage = D3D11_USAGE_STAGING;

        descs[i] = desc;
    }

    return STATUS_OK;
}

int RecorderDDA::GetScreenShotOfDisplay (unsigned int maxAttempts, ID3D11Texture2D **desktopTexture, unsigned int displayNum)
{
    HRESULT hr (E_FAIL);
    ID3D11Texture2D *acquiredImage = NULL;
    ID3D11Texture2D *destImage = NULL;

    D3D11_TEXTURE2D_DESC desc = descs[displayNum];

    hr = device->CreateTexture2D (&desc, NULL, &destImage);
    if ((FAILED (hr)) || (destImage == NULL))
    {
        Recorder::recordLogger->error ("unable to create texture with size {}x{} hr is {}", desc.Width, desc.Height, hr);
        return CREATE_TEXTURE_ERROR;
    }

    IDXGIResource *desktopResource = NULL;
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    int res = STATUS_OK;

    for (int i = 0; i < maxAttempts; i++)
    {
        hr = deskDupls[displayNum]->AcquireNextFrame (
            1000, // timeout
            &frameInfo,
            &desktopResource
        );

        if (SUCCEEDED (hr))
            break;

        if (hr == DXGI_ERROR_WAIT_TIMEOUT)
        {
            Recorder::recordLogger->info ("try to capture one more time {}/{}", i, maxAttempts);
            continue;
        }
        else
        {
            if (hr == DXGI_ERROR_INVALID_CALL)
            {
                Recorder::recordLogger->error ("DXGI_ERROR_INVALID_CALL previous frame is not released?");
                res = DDA_CAPTURE_ERROR;
                break;
            }
            else
            {
                if (hr == DXGI_ERROR_ACCESS_LOST)
                {
                    Recorder::recordLogger->error ("Access is lost, you should reinitialize obj");
                    res = DDA_LOST_ACCESS_ERROR;
                    break;
                }
                else
                {
                    if (FAILED (hr))
                    {
                        Recorder::recordLogger->error ("DDA capture error, HRESULT is {}", hr);
                        res = DDA_CAPTURE_ERROR;
                        break;
                    }
                }
            }
        }
    }

    if (FAILED (hr))
    {
        if (hr == DXGI_ERROR_WAIT_TIMEOUT)
        {
            Recorder::recordLogger->error ("no desktop updates for display {}", displayNum);
            res = DDA_TIMEOUT_ERROR;
        }
        else
        {
            Recorder::recordLogger->error ("DDA capture error, HRESULT is {}", hr);
            if (res == STATUS_OK)
            {
                res = DDA_CAPTURE_ERROR;
            }
        }
    }

    // QI for ID3D11Texture2D
    if (res == STATUS_OK)
    {
        hr = desktopResource->QueryInterface (IID_PPV_ARGS (&acquiredImage));
        if ((FAILED (hr)) || (acquiredImage == NULL))
        {
            Recorder::recordLogger->error ("QI for acquired Image error");
            res = DDA_CAPTURE_ERROR;
        }
        else
        {
            // Copy image into CPU access texture
            immediateContext->CopyResource (destImage, acquiredImage);
            acquiredImage->Release ();
            *desktopTexture = destImage;
        }
    }

    if (desktopResource)
    {
        desktopResource->Release ();
        desktopResource = NULL;
    }
    deskDupls[displayNum]->ReleaseFrame ();

    return res;
}

bool RecorderDDA::CheckWindowOnDisplay (const RECT& display, const RECT& processRect)
{
    int yCenter = processRect.top + (processRect.bottom - processRect.top) / 2;
    int xCenter = processRect.left + (processRect.right - processRect.left) / 2;

    Recorder::recordLogger->trace ("CheckWindowOnDisplay, window center is {}x{}, display coordinates are ({},{}), ({},{})",
                                    xCenter, yCenter, display.left, display.top, display.right, display.bottom);

    if ((yCenter < display.bottom) && (yCenter > display.top)
        && (xCenter > display.left) && (xCenter < display.right))
        return TRUE;
    else
        return FALSE;
}

int RecorderDDA::GetProcessWindow (RECT *rc, int *processDisplay)
{
    if (!pid)
    {
        Recorder::recordLogger->error ("PID is not set");
        return INVALID_ARGUMENTS_ERROR;
    }
    RECT rcWind;
    HWND hwndWind;
    int processDisplayNum = -1;
    hwndWind = GetMainWindow (pid);
    if (!hwndWind)
    {
        Recorder::recordLogger->error ("Unable to find window for pid {}", pid);
        return FIND_WINDOW_ERROR;
    }
    // IsWindowVisible return true even if window is hidden nevertheless keep this check
    if (!IsWindowVisible (hwndWind))
    {
        Recorder::recordLogger->error ("WindoW for pid {} is invisible", pid);
        return FIND_WINDOW_ERROR;
    }
    // check that window was minimized
    if (GetWindowLongA (hwndWind, GWL_STYLE) & WS_MINIMIZE)
    {
        Recorder::recordLogger->error ("WindoW for pid {} is minimized", pid);
        return FIND_WINDOW_ERROR;
    }

    if (DwmGetWindowAttribute (hwndWind, DWMWA_EXTENDED_FRAME_BOUNDS, (PVOID) &rcWind, sizeof (RECT)))
    {
        if (!GetWindowRect (hwndWind, &rcWind))
        {
            Recorder::recordLogger->error ("Unable to find rect for hwnd, pid {}", pid);
            return FIND_WINDOW_ERROR;
        }
    }
    Recorder::recordLogger->trace ("Window position from {},{} to {},{}", rcWind.left, rcWind.top, rcWind.right, rcWind.bottom);

    for (int i = 0; i < totalDisplays; i++)
    {
        DXGI_OUTPUT_DESC outputDesc = outputDescs[i];
        if (!CheckWindowOnDisplay (outputDesc.DesktopCoordinates, rcWind))
            continue;
        else
            processDisplayNum = i;
    }
    if (processDisplayNum == -1)
    {
        Recorder::recordLogger->error ("Unable to find display for process {}", pid);
        return FIND_WINDOW_ERROR;
    }
    Recorder::recordLogger->trace ("Process display is {}", processDisplayNum);

    *processDisplay = processDisplayNum;
    *rc = rcWind;
    return STATUS_OK;
}

int RecorderDDA::GetImageFromTexture (ID3D11Texture2D *destImage, int displayNum, RECT rcWind, volatile unsigned char *frameBuffer, volatile int *width, volatile int *height)
{
    D3D11_MAPPED_SUBRESOURCE resource;
    UINT subresource = D3D11CalcSubresource (0, 0, 0);
    HRESULT hr = immediateContext->Map (destImage, subresource, D3D11_MAP_READ_WRITE, 0, &resource);
    if (FAILED (hr))
    {
        Recorder::recordLogger->error ("Failed to Map subresource, hr is {}", hr);
        return DDA_CAPTURE_ERROR;
    }

    unsigned char* sptr = reinterpret_cast<unsigned char*>(resource.pData);
    UINT rowPitch = std::min<UINT>(descs[displayNum].Width * 4, resource.RowPitch);
    volatile unsigned char *dptr = frameBuffer;

    for (size_t h = 0; h < descs[displayNum].Height; h++)
    {
        memcpy ((void *)dptr, (void *)sptr, rowPitch);
        sptr += resource.RowPitch;
        dptr += rowPitch;
    }

    immediateContext->Unmap (destImage, subresource);

    int shiftLeft = std::min<int> (0, outputDescs[displayNum].DesktopCoordinates.left);
    int shiftTop = std::min<int> (0, outputDescs[displayNum].DesktopCoordinates.top);
    int displayWidth = outputDescs[displayNum].DesktopCoordinates.right - outputDescs[displayNum].DesktopCoordinates.left;
    int displayHeight = outputDescs[displayNum].DesktopCoordinates.bottom - outputDescs[displayNum].DesktopCoordinates.top;

    if (pid)
    {
        // window coordinates may be outside virtual desktop
        int newWindTop = std::max<int>(rcWind.top, outputDescs[displayNum].DesktopCoordinates.top);
        int newWindBottom = std::min<int>(rcWind.bottom, outputDescs[displayNum].DesktopCoordinates.bottom);
        int newWindLeft = std::max<int>(rcWind.left, outputDescs[displayNum].DesktopCoordinates.left);
        int newWindRight = std::min<int>(rcWind.right, outputDescs[displayNum].DesktopCoordinates.right);
        // move process window to framebuffer.begin
        int startPos = ((newWindTop - shiftTop) * displayWidth + newWindLeft - shiftLeft) * 4;
        volatile unsigned char *dptr = frameBuffer + startPos;
        int widthWin = newWindRight - newWindLeft;

        Recorder::recordLogger->trace ("startpose for copying is {} widthWin is {}", startPos, widthWin);
        for (int curH = newWindTop; curH < newWindBottom ; curH++)
        {
            int sourcePos = ((curH - shiftTop) * displayWidth + newWindLeft - shiftLeft) * 4;
            memmove ((void *)dptr, (void *)(frameBuffer + sourcePos), widthWin * 4);
            dptr += widthWin * 4;
        }
        memmove ((void *)frameBuffer, (void *)(frameBuffer + startPos), (newWindBottom - newWindTop) * widthWin * 4);

        *height = newWindBottom - newWindTop;
        *width = widthWin;
    }
    else
    {
        *height = displayHeight;
        *width = displayWidth;
    }
    Recorder::recordLogger->trace ("Captured desktop for display {}", displayNum);

    return STATUS_OK;
}

int RecorderDDA::ReleaseDDA ()
{
    Recorder::recordLogger->trace ("Releasing DDA capture resources");
    for (int i = 0; i < totalDisplays; i++)
    {
        memset (&outputDescs[i], 0, sizeof (DXGI_OUTPUT_DESC));
        memset (&descs[i], 0, sizeof (D3D11_TEXTURE2D_DESC));
        if (deskDupls[i])
        {
            deskDupls[i]->Release ();
            deskDupls[i] = NULL;
        }
    }
    if (immediateContext)
    {
        immediateContext->Release ();
        immediateContext = NULL;
    }
    if (device)
    {
        device->Release ();
        device = NULL;
    }
    totalDisplays = 0;
    return STATUS_OK;
}

bool RecorderDDA::CheckThreadAlive ()
{
    EnterCriticalSection (&critSect);
    if ((!this->hThread) && (threadMustExit))
    {
        LeaveCriticalSection (&critSect);
        return FALSE;
    }
    LeaveCriticalSection (&critSect);
    DWORD dwWait = WaitForSingleObject (hThread, 0);
    if (dwWait == WAIT_TIMEOUT)
        return TRUE;
    else
        return FALSE;
}