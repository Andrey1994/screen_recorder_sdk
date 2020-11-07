package screen_recorder;

/**
 * RecorderError exception to notify about errors
 */
public class RecorderError extends Exception
{
    public String msg;
    /**
     * exit code returned from low level API
     */
    public int exit_code;

    public RecorderError (String message, int ec)
    {
        super (message + ":" + screen_recorder.ExitCode.string_from_code (ec));
        exit_code = ec;
        msg = message;
    }
}
