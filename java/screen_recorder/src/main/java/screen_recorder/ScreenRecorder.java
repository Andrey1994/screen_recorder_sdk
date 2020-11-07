package screen_recorder;

import java.awt.Color;
import java.awt.Image;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;

import org.apache.commons.lang3.SystemUtils;

import com.sun.jna.Library;
import com.sun.jna.Native;

/**
 * Class to record video and take screenshots
 */
public class ScreenRecorder
{

    private interface DllInterface extends Library
    {
        int InitResources (String params);

        int GetScreenShot (int max_attempts, byte[] framebuffer, int[] width, int[] height);

        int StartVideoRecording (String filename, int frameRate, int bitRate, int useHardwareTransform);

        int StopVideoRecording ();

        int FreeResources ();

        int GetPID (int[] pid);

        int SetLogLevel (int log_level);

        int SetLogFile (String log_file);
    }

    private static DllInterface instance;

    static
    {

        String lib_name = "ScreenRecorder.dll";
        // need to extract libraries from jar
        unpack_from_jar (lib_name);
        instance = (DllInterface) Native.loadLibrary (lib_name, DllInterface.class);
    }

    private static Path unpack_from_jar (String lib_name)
    {
        try
        {
            File file = new File (lib_name);
            if (file.exists ())
                file.delete ();
            InputStream link = (ScreenRecorder.class.getResourceAsStream (lib_name));
            Files.copy (link, file.getAbsoluteFile ().toPath ());
            return file.getAbsoluteFile ().toPath ();
        } catch (Exception io)
        {
            System.err.println ("file: " + lib_name + " is not found in jar file");
            return null;
        }
    }

    /**
     * enable logger with level INFO
     */
    public static void enable_log () throws RecorderError
    {
        set_log_level (2);
    }

    /**
     * enable logger with level TRACE
     */
    public static void enable_dev_log () throws RecorderError
    {
        set_log_level (0);
    }

    /**
     * disable logger
     */
    public static void disable_log () throws RecorderError
    {
        set_log_level (6);
    }

    /**
     * redirect logger from stderr to a file
     */
    public static void set_log_file (String log_file) throws RecorderError
    {
        int ec = instance.SetLogFile (log_file);
        if (ec != ExitCode.STATUS_OK.get_code ())
        {
            throw new RecorderError ("Error in SetLogFile", ec);
        }
    }

    /**
     * set log level
     */
    public static void set_log_level (int log_level) throws RecorderError
    {
        int ec = instance.SetLogLevel (log_level);
        if (ec != ExitCode.STATUS_OK.get_code ())
        {
            throw new RecorderError ("Error in set_log_level", ec);
        }
    }

    private String input_json;

    /**
     * Create ScreenRecorder object
     */
    public ScreenRecorder (RecorderParams params)
    {
        this.input_json = params.to_json ();
    }

    /**
     * prepare required resources, creates worker thread
     */
    public void init_resources () throws RecorderError
    {
        int ec = instance.InitResources (input_json);
        if (ec != ExitCode.STATUS_OK.get_code ())
        {
            throw new RecorderError ("Error in init_resources", ec);
        }
        // workaround black screenshot if called right after init_resources, dont see
        // race cond in C++ and can not reproduce in python
        try
        {
            Thread.sleep (100);
        } catch (InterruptedException e)
        {
        }
    }

    /**
     * free all resources
     */
    public void free_resources () throws RecorderError
    {
        int ec = instance.FreeResources ();
        if (ec != ExitCode.STATUS_OK.get_code ())
        {
            throw new RecorderError ("Error in free_resources", ec);
        }
    }

    /**
     * get pid
     */
    public int get_pid () throws RecorderError
    {
        int[] res = new int[1];
        int ec = instance.GetPID (res);
        if (ec != ExitCode.STATUS_OK.get_code ())
        {
            throw new RecorderError ("Error in get_pid", ec);
        }
        return res[0];
    }

    /**
     * start video recording
     */
    public void start_video_recording (String filename, int frame_rate, int bit_rate, boolean use_hw_transfowrms)
            throws RecorderError
    {
        int ec = instance.StartVideoRecording (filename, frame_rate, bit_rate, use_hw_transfowrms ? 1 : 0);
        if (ec != ExitCode.STATUS_OK.get_code ())
        {
            throw new RecorderError ("Error in start_video_recording", ec);
        }
    }

    /**
     * start video recording with default args
     */
    public void start_video_recording (String filename) throws RecorderError
    {
        start_video_recording (filename, 30, 8000000, true);
    }

    /**
     * stop video recording
     */
    public void stop_video_recording () throws RecorderError
    {
        int ec = instance.StopVideoRecording ();
        if (ec != ExitCode.STATUS_OK.get_code ())
        {
            throw new RecorderError ("Error in stop_video_recording", ec);
        }
    }

    /**
     * get screenshot
     */
    public BufferedImage get_screenshot (int max_attempts) throws RecorderError
    {
        int max_width = 4096;
        int max_height = 4096;
        int max_pixels = max_width * max_height;
        byte[] framebuffer = new byte[max_pixels];
        int[] width = new int[1];
        int[] height = new int[1];
        int ec = instance.GetScreenShot (max_attempts, framebuffer, width, height);
        if (ec != ExitCode.STATUS_OK.get_code ())
        {
            throw new RecorderError ("Error in start_video_recording", ec);
        }

        BufferedImage img = new BufferedImage (width[0], height[0], BufferedImage.TYPE_INT_ARGB);
        int index = 0;
        for (int r = 0; r < height[0]; r++)
        {
            for (int c = 0; c < width[0]; c++)
            {
                int red = ((int) framebuffer[index + 2]) & 0xFF;
                int green = ((int) framebuffer[index + 1]) & 0xFF;
                int blue = ((int) framebuffer[index]) & 0xFF;
                Color color = new Color (red, green, blue);
                img.setRGB (c, r, color.getRGB ());
                index += 4;
            }
        }
        return img;
    }
}
