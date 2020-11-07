#include "Recorder.h"
#include "ScreenRecorder.h"

#include "logger/sinks/null_sink.h"

#define LOGGER_NAME "recordLogger"

std::shared_ptr<spdlog::logger> Recorder::recordLogger = spdlog::stderr_logger_mt (LOGGER_NAME);

int Recorder::SetLogLevel (int level)
{
    int logLevel = level;
    if (level > 6)
        logLevel = 6;
    if (level < 0)
        logLevel = 0;
    Recorder::recordLogger->set_level (spdlog::level::level_enum (logLevel));
    Recorder::recordLogger->flush_on (spdlog::level::level_enum (logLevel));
    return STATUS_OK;
}

int Recorder::SetLogFile (char *logFile)
{
    try
    {
        spdlog::level::level_enum level = Recorder::recordLogger->level ();
        Recorder::recordLogger = spdlog::create<spdlog::sinks::null_sink_st> (
            "null_logger"); // to dont set logger to nullptr and avoid race condition
        spdlog::drop (LOGGER_NAME);
        Recorder::recordLogger = spdlog::basic_logger_mt (LOGGER_NAME, logFile);
        Recorder::recordLogger->set_level (level);
        Recorder::recordLogger->flush_on (level);
        spdlog::drop ("null_logger");
    }
    catch (...)
    {
        return GENERAL_ERROR;
    }
    return STATUS_OK;
}

Recorder::Recorder (struct RecorderParams recorderParams) : params (recorderParams)
{
}
