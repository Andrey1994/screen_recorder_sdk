package screen_recorder;

import java.util.HashMap;
import java.util.Map;

public enum ExitCode
{

    STATUS_OK (0),
    NO_SUCH_PROCESS_ERROR (100),
    RECORDING_ALREADY_RUN_ERROR (101),
    RECORDING_THREAD_ERROR (102),
    RECORDING_THREAD_IS_NOT_RUNNING_ERROR (103),
    INVALID_ARGUMENTS_ERROR (104),
    SESSION_NOT_CREATED_ERROR (105),
    PREPARE_DESK_DUPL_ERROR (106),
    CREATE_TEXTURE_ERROR (107),
    DDA_CAPTURE_ERROR (108),
    FIND_WINDOW_ERROR (109),
    DDA_LOST_ACCESS_ERROR (110),
    DDA_TIMEOUT_ERROR (111),
    SYNC_TIMEOUT_ERROR (112),
    GENERAL_ERROR (113);

    private final int exit_code;
    private static final Map<Integer, ExitCode> ec_map = new HashMap<Integer, ExitCode> ();

    public int get_code ()
    {
        return exit_code;
    }

    public static String string_from_code (final int code)
    {
        return from_code (code).name ();
    }

    public static ExitCode from_code (final int code)
    {
        final ExitCode element = ec_map.get (code);
        return element;
    }

    ExitCode (final int code)
    {
        exit_code = code;
    }

    static
    {
        for (final ExitCode ec : ExitCode.values ())
        {
            ec_map.put (ec.get_code (), ec);
        }
    }

}
