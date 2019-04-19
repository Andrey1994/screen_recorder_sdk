import sys
import time
import numpy
import threading

from screen_recorder_sdk import screen_recorder

class ScreenShoter (threading.Thread):

    def __init__ (self):
        threading.Thread.__init__ (self)
        self.should_stop = False

    def run (self):
        i = 0
        while not self.should_stop:
            screen_recorder.get_screenshot (5).save ('test' + str (i) + '.png')
            time.sleep (1)
            i = i + 1


def main ():
    screen_recorder.enable_dev_log ()
    pid = int (sys.argv[1])
    screen_recorder.init_resources (pid)

    t = ScreenShoter ()
    t.start ()
    screen_recorder.start_video_recording ('video.mp4', 30, 8000000, True)
    time.sleep (20)
    screen_recorder.stop_video_recording ()
    t.should_stop = True
    t.join ()

    screen_recorder.free_resources ()

if __name__ == "__main__":
    main ()
