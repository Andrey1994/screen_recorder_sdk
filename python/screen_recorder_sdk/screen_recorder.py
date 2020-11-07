import os
import json
import ctypes
import numpy
import enum
import platform
import struct
from numpy.ctypeslib import ndpointer
import pkg_resources
from PIL import Image
from screen_recorder_sdk.exit_codes import RecorderExitCodes


class RecorderParams (object):
    """ inputs parameters for init_resources method

    :param desktop_num: desktop num, counting from 0
    :type desktop_num: int
    :param pid: pid of process to capture
    :type pid: int
    """
    def __init__ (self, desktop_num = 0, pid = 0) -> None:
        self.pid = pid
        self.desktop_num = desktop_num

    def to_json (self) -> None :
        return json.dumps (self, default = lambda o: o.__dict__,
            sort_keys = True, indent = 4)


class RecorderError (Exception):
    """This exception is raised if non-zero exit code is returned from C code

    :param message: exception message
    :type message: str
    :param exit_code: exit code from low level API
    :type exit_code: int
    """
    def __init__ (self, message, exit_code):
        detailed_message = '%s:%d %s' % (RecorderExitCodes (exit_code).name, exit_code, message)
        super (RecorderError, self).__init__ (detailed_message)
        self.exit_code = exit_code


class ScreenRecorderDLL (object):

    __instance = None

    @classmethod
    def get_instance (cls):
        if cls.__instance is None:
            if platform.system () != 'Windows':
                raise Exception ("For now only Windows is supported, detected platform is %s" % platform.system ())
            if struct.calcsize ("P") * 8 != 64:
                raise Exception ("You need 64-bit python to use this library")
            cls.__instance = cls ()
        return cls.__instance

    def __init__ (self):

        self.lib = ctypes.cdll.LoadLibrary (pkg_resources.resource_filename (__name__, os.path.join ('lib', 'ScreenRecorder.dll')))

        self.InitResources = self.lib.InitResources
        self.InitResources.restype = ctypes.c_int
        self.InitResources.argtypes = [
            ctypes.c_char_p
        ]

        self.GetScreenShot = self.lib.GetScreenShot
        self.GetScreenShot.restype = ctypes.c_int
        self.GetScreenShot.argtypes = [
            ctypes.c_uint,
            ndpointer (ctypes.c_ubyte),
            ndpointer (ctypes.c_int64),
            ndpointer (ctypes.c_int64)
        ]

        self.FreeResources = self.lib.FreeResources
        self.FreeResources.restype = ctypes.c_int
        self.FreeResources.argtypes = []

        self.SetLogLevel = self.lib.SetLogLevel
        self.SetLogLevel.restype = ctypes.c_int
        self.SetLogLevel.argtypes = [
            ctypes.c_int64
        ]

        self.SetLogFile = self.lib.SetLogFile
        self.SetLogFile.restype = ctypes.c_int
        self.SetLogFile.argtypes = [
            ctypes.c_char_p
        ]

        self.StartVideoRecording = self.lib.StartVideoRecording
        self.StartVideoRecording.restype = ctypes.c_int
        self.StartVideoRecording.argtypes = [
            ctypes.c_char_p,
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_int
        ]

        self.StopVideoRecording = self.lib.StopVideoRecording
        self.StopVideoRecording.restype = ctypes.c_int
        self.StopVideoRecording.argtypes = []

        self.GetPID = self.lib.GetPID
        self.GetPID.restype = ctypes.c_int
        self.GetPID.argtypes = [
            ndpointer (ctypes.c_int64)
        ]


def init_resources (params):
    """ Init resources for recording

    :param params: params for recording
    :type params: RecorderParams
    :raises RecorderError: if non zero exit code returned from low level API
    """
    try:
        input_json = params.to_json ().encode ()
    except:
        input_json = params.to_json ()

    res = ScreenRecorderDLL.get_instance ().InitResources (input_json)
    if res != RecorderExitCodes.STATUS_OK.value:
        raise RecorderError ('unable to init resources', res)

def get_screenshot (max_attempts = 1):
    """ Get Screenshot

    :param max_attempts: max attempts to capture frame buffer
    :type max_attempts: int
    :return: Pillow Image
    :rtype: Pillow Image
    :raises RecorderError: if non zero exit code returned from low level API
    """
    max_width = 4096
    max_height = 4096
    max_pixels = max_width * max_height
    frame_buffer = numpy.zeros (max_pixels * 4).astype (numpy.uint8)
    width = numpy.zeros (1).astype (numpy.int64)
    height = numpy.zeros (1).astype (numpy.int64)

    res = ScreenRecorderDLL.get_instance().GetScreenShot (max_attempts, frame_buffer, width, height)
    if res != RecorderExitCodes.STATUS_OK.value:
        raise RecorderError ('unable to capture FrameBuffer', res)

    return ScreenShotConvertor (frame_buffer, width, height).get_image ()

def get_pid ():
    """ Get PID

    :rtype: int
    :return: PID
    :raises RecorderError: if non zero exit code returned from low level API
    """
    pid = numpy.zeros (1).astype (numpy.int64)
    res = ScreenRecorderDLL.get_instance ().GetPID (pid)
    if res != RecorderExitCodes.STATUS_OK.value:
        raise RecorderError ('unable to get pid', res)
    return pid[0]

def free_resources ():
    """ Free Resources

    :raises RecorderError: if non zero exit code returned from low level API
    """
    res = ScreenRecorderDLL.get_instance ().FreeResources ()
    if res != RecorderExitCodes.STATUS_OK.value:
        raise RecorderError ('unable to free capture resources', res)

def enable_log ():
    """ Enable Logger

    :raises RecorderError: if non zero exit code returned from low level API
    """
    res = ScreenRecorderDLL.get_instance ().SetLogLevel (1)
    if res != RecorderExitCodes.STATUS_OK.value:
        raise RecorderError ('unable to enable capture log', res)

def enable_dev_log ():
    """ Enable Dev Logger

    :raises RecorderError: if non zero exit code returned from low level API
    """
    res = ScreenRecorderDLL.get_instance ().SetLogLevel (0)
    if res != RecorderExitCodes.STATUS_OK.value:
        raise RecorderError ('unable to enable capture log', res)

def disable_log ():
    """ Disable Logger

    :raises RecorderError: if non zero exit code returned from low level API
    """
    res = ScreenRecorderDLL.get_instance ().SetLogLevel (6)
    if res != RecorderExitCodes.STATUS_OK.value:
        raise RecorderError ('unable to disable capture log', res)

def start_video_recording (filename, frame_rate = 30, bit_rate = 8000000, use_hw_transfowrms = True):
    """ Start Video Recording

    :param filename: filename to store video
    :type filename: str
    :param frame_rate: FPS
    :type frame_rate: int
    :param bit_rate: bit rate, set higher values for better quality
    :type bit_rate: int
    :param use_hw_transforms: if you have good GPU set this flag to True for better perf, if you see errors try to set it to false
    :type use_hw_transfowrms: bool
    :raises RecorderError: if non zero exit code returned from low level API
    """

    res = ScreenRecorderDLL.get_instance ().StartVideoRecording (filename.encode ('utf-8'),
        frame_rate, bit_rate, int (use_hw_transfowrms == True))
    if res != RecorderExitCodes.STATUS_OK.value:
        raise RecorderError ('unable to start recording video', res)

def stop_video_recording ():
    """ Stop video recording

    :raises RecorderError: if non zero exit code returned from low level API
    """
    res = ScreenRecorderDLL.get_instance ().StopVideoRecording ()
    if res != RecorderExitCodes.STATUS_OK.value:
        raise RecorderError ('unable to stop recording video', res)

def set_log_file (log_file):
    """redirect logger from stderr to file, can be called any time
    :param log_file: log file name
    :type log_file: str
    :raises RecorderError: if non zero exit code returned from low level API
    """
    try:
        file = log_file.encode ()
    except:
        file = log_file
    res = ScreenRecorderDLL.get_instance ().SetLogFile (file)
    if res != RecorderExitCodes.STATUS_OK.value:
        raise RecorderError ('unable to redirect logs to a file', res)


class ScreenShotConvertor (object):

    def __init__ (self, frame_buffer, width, height):
        self.frame_buffer = frame_buffer
        self.width = int (width[0])
        self.height = int (height[0])

    def get_bgr_array (self):
        reshaped = self.frame_buffer[0:self.width*self.height*4].reshape (self.height, self.width*4)
        reshaped = reshaped[:,:self.width*4]
        reshaped = reshaped.reshape (self.height, self.width, 4)
        return reshaped[:,:,:-1]

    def get_rgb_array (self):
        return self.get_bgr_array ()[:,:,::-1]

    def get_image (self):
        rgb = self.get_rgb_array ()
        return Image.fromarray (rgb)
