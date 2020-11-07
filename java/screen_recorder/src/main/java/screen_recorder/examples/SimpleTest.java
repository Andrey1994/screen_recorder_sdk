package screen_recorder.examples;

import java.awt.image.BufferedImage;
import java.io.File;

import javax.imageio.ImageIO;

import screen_recorder.RecorderParams;
import screen_recorder.ScreenRecorder;

public class SimpleTest
{

    public static void main (String[] args) throws Exception
    {
        ScreenRecorder.enable_dev_log ();

        RecorderParams params = new RecorderParams ();
        ScreenRecorder sr = new ScreenRecorder (params);
        sr.init_resources ();

        BufferedImage image = sr.get_screenshot (4);
        ImageIO.write (image, "png", new File ("before_video.png"));

        sr.start_video_recording ("video1.mp4", 30, 8000000, true);
        Thread.sleep (5000);
        BufferedImage image2 = sr.get_screenshot (4);
        Thread.sleep (5000);
        ImageIO.write (image2, "png", new File ("during_video.png"));
        sr.stop_video_recording ();

        BufferedImage image3 = sr.get_screenshot (4);
        ImageIO.write (image3, "png", new File ("after_video.png"));

        sr.free_resources ();
    }
}
