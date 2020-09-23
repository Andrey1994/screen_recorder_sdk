#include "Recorder.h"
#include "ScreenRecorder.h"

std::shared_ptr<spdlog::logger> Recorder::recordLogger = spdlog::stderr_logger_mt ("recordLogger");

int Recorder::SetLogLevel (int level)
{
    int log_level = level;
    if (level > 6)
        log_level = 6;
    if (level < 0)
        log_level = 0;
    Recorder::recordLogger->set_level (spdlog::level::level_enum (log_level));
    Recorder::recordLogger->flush_on (spdlog::level::level_enum (log_level));
    return STATUS_OK;
}

Recorder::Recorder (struct RecorderParams recorderParams) : params (recorderParams)
{
}
