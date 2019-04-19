#ifndef SCREENRECORDER
#define SCREENRECORDER

typedef enum
{
    STATUS_OK = 0,
    NO_SUCH_PROCESS_ERROR = 100,
    RECORDING_ALREADY_RUN_ERROR,
    RECORDING_THREAD_ERROR,
    RECORDING_THREAD_IS_NOT_RUNNING_ERROR,
    INVALID_ARGUMENTS_ERROR,
    SESSION_NOT_CREATED_ERROR,
    PREPARE_DESK_DUPL_ERROR,
    CREATE_TEXTURE_ERROR,
    DDA_CAPTURE_ERROR,
    FIND_WINDOW_ERROR,
    DDA_LOST_ACCESS_ERROR,
    DDA_TIMEOUT_ERROR,
    SYNC_TIMEOUT_ERROR,
    GENERAL_ERROR,
} ExitCodes;

extern "C" {
    __declspec(dllexport) int InitResources (int pid);
    __declspec(dllexport) int GetScreenShot (int maxAttempts, unsigned char *frameBuffer, int *width, int *height);
    __declspec(dllexport) int FreeResources ();
    __declspec(dllexport) int SetLogLevel (int level);
    __declspec(dllexport) int GetPID (int *pid);

    __declspec(dllexport) int StartVideoRecording (const char *outputFileName, int frameRate, int bitRate, bool useHardwareTransform);
    __declspec(dllexport) int StopVideoRecording ();
}

#endif
