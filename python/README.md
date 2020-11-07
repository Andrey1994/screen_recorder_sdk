# Screen Recorder SDK
Library to take screenshots and record videos

I use [Desktop Duplication API](https://docs.microsoft.com/en-us/windows/desktop/direct3ddxgi/desktop-dup-api) to capture desktop and [Media Foundation API](https://docs.microsoft.com/en-us/windows/desktop/medfound/media-foundation-platform-apis) to record video.

For screenshots it cuts process window from desktop while for videos it captures full display without cutting for better performance

## System Requirements

* Windows >= 10, it may work on Windows 8.1 and Windows Server 2012, but we don't ensure it
* DirectX, you can install it from [Microsoft Website](https://www.microsoft.com/en-us/download/details.aspx?id=17431)
* Media Feature Pack, download it [here](https://www.microsoft.com/en-us/software-download/mediafeaturepack)
* 64 bits Java or Python, we don't provide x86 libs

## Installation

First option is:
```
git clone https://github.com/Andrey1994/screen_recorder_sdk
cd screen_recorder_sdk
cd python
pip install .
```
ALso you can install it from PYPI:
```
pip install screen_recorder_sdk
```

## Simple Sample

```
import sys
import time
import numpy

from screen_recorder_sdk import screen_recorder


def main ():
    screen_recorder.enable_dev_log ()
    pid = int (sys.argv[1]) # pid == 0 means capture full screen
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
```

[More samples](https://github.com/Andrey1994/screen_recorder_sdk/tree/master/python/example)
