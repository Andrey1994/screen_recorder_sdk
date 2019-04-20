#include <windows.h>

#include "ScreenRecorder.h"
#include "Recorder.h"
#include "RecorderDDA.h"


Recorder *recorder = NULL;
HANDLE mutex = NULL;

int InitResources (int pid)
{
    if (!mutex)
    {
        mutex = CreateMutex (NULL, FALSE, NULL);
        if (!mutex)
        {
            return GENERAL_ERROR;
        }
    }
    if (recorder)
    {
        if (recorder->GetPID () == pid)
            return STATUS_OK;
        delete recorder;
        recorder = NULL;
    }

    recorder = new RecorderDDA (pid);
    WaitForSingleObject (mutex, INFINITE);
    int res = recorder->InitResources ();
    ReleaseMutex (mutex);
    if (res != STATUS_OK) {
        delete recorder;
        recorder = NULL;
        CloseHandle (mutex);
        mutex = NULL;
        return res;
    }

    return STATUS_OK;
}

int GetScreenShot (unsigned int maxAttempts, unsigned char *frameBuffer, int *width, int *height)
{
    if (!recorder)
        return SESSION_NOT_CREATED_ERROR;

    WaitForSingleObject (mutex, INFINITE);
    int res = recorder->GetScreenShot (maxAttempts, frameBuffer, width, height);
    ReleaseMutex (mutex);
    return res;
}

int FreeResources ()
{
    if (recorder) {
        if (mutex)
        {
            WaitForSingleObject (mutex, INFINITE);
            delete recorder;
            recorder = NULL;
            ReleaseMutex (mutex);
            CloseHandle (mutex);
            mutex = NULL;
        }
        else
        {
            return GENERAL_ERROR;
        }
    }

    return STATUS_OK;
}

int SetLogLevel (int level)
{
    return Recorder::SetLogLevel (level);
}

int StartVideoRecording (const char *outputFileName, int frameRate, int bitRate, bool useHardwareTransform)
{
    if (!recorder)
        return SESSION_NOT_CREATED_ERROR;

    WaitForSingleObject (mutex, INFINITE);
    int res = recorder->StartVideoRecording (outputFileName, frameRate, bitRate, useHardwareTransform);
    ReleaseMutex (mutex);
    return res;
}

int StopVideoRecording ()
{
    if (!recorder)
        return SESSION_NOT_CREATED_ERROR;

    WaitForSingleObject (mutex, INFINITE);
    int res = recorder->StopVideoRecording ();
    ReleaseMutex (mutex);
    return res;
}

int GetPID (int *pid)
{
    if (!recorder)
        return SESSION_NOT_CREATED_ERROR;

    *pid = recorder->GetPID ();
    return STATUS_OK;
}