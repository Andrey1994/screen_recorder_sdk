import sys
import time
import numpy

from screen_recorder_sdk import screen_recorder


def main ():
    screen_recorder.enable_dev_log ()
    pid = int (sys.argv[1])
    screen_recorder.init_resources (pid)

    screen_recorder.get_screenshot (5).save ('test_before.png')

    screen_recorder.start_video_recording ('video1.mp4', 30, 8000000, True)
    time.sleep (5)
    screen_recorder.get_screenshot (5).save ('test_during_video.png')
    time.sleep (5)
    screen_recorder.stop_video_recording ()

    screen_recorder.start_video_recording ('video2.mp4', 30, 8000000, True)
    time.sleep (5)
    screen_recorder.stop_video_recording ()

    screen_recorder.free_resources ()

if __name__ == "__main__":
    main ()
