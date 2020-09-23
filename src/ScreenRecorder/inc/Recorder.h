#ifndef RECORDER
#define RECORDER

#include "RecorderParams.h"

#include <logger\spdlog.h>

class Recorder
{
public:
    static std::shared_ptr<spdlog::logger> recordLogger;
    static int SetLogLevel (int level);

    Recorder (struct RecorderParams recorderParams);
    virtual ~Recorder ()
    {
    }

    virtual int InitResources () = 0;
    virtual int GetScreenShot (
        unsigned int maxAttempts, unsigned char *frameBuffer, int *width, int *height) = 0;

    virtual int StartVideoRecording (
        const char *outputFileName, int frameRate, int bitRate, bool useHardwareTransform) = 0;
    virtual int StopVideoRecording () = 0;

    int GetPID () const
    {
        return params.pid;
    }

protected:
    struct RecorderParams params;
};

#endif
