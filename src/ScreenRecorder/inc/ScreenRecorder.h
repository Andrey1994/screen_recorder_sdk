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
    JSON_ERROR
} ExitCodes;

extern "C"
{
    __declspec(dllexport) int __cdecl InitResources (char *paramsString);
    __declspec(dllexport) int __cdecl GetScreenShot (
        unsigned int maxAttempts, unsigned char *frameBuffer, int *width, int *height);
    __declspec(dllexport) int __cdecl FreeResources ();
    __declspec(dllexport) int __cdecl SetLogLevel (int level);
    __declspec(dllexport) int __cdecl SetLogFile (char *logFile);
    __declspec(dllexport) int __cdecl GetPID (int *pid);

    __declspec(dllexport) int __cdecl StartVideoRecording (
        const char *outputFileName, int frameRate, int bitRate, int useHardwareTransform);
    __declspec(dllexport) int __cdecl StopVideoRecording ();
}

#endif
