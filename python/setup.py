import os
from setuptools import setup

dev_version = os.environ.get("SCREEN_RECORDER_SDK_VERSION")

setup(
  version=dev_version if dev_version else "0.0.1"
)
