import os
from setuptools import setup, find_packages

this_directory = os.path.abspath (os.path.dirname (__file__))
with open (os.path.join (this_directory, 'README.md'), encoding = 'utf-8') as f:
    long_description = f.read ()

setup (
    name = 'screen_recorder_sdk',
    version = '1.2.1',
    description = 'Library to take screenshots and record video from desktop',
    long_description = long_description,
    long_description_content_type = 'text/markdown',
    url = 'https://github.com/Andrey1994/screen_recorder_sdk',
    author = 'Andrey Parfenov',
    author_email = 'a1994ndrey@gmail.com',
    packages = find_packages (),
    classifiers = [
        'Development Status :: 2 - Pre-Alpha',
        'Topic :: Utilities'
    ],
    install_requires = [
        'numpy', 'Pillow'
    ],
    package_data = {
        'screen_recorder_sdk': [os.path.join ('lib', 'ScreenRecorder.dll')]
    },
    zip_safe = True,
    python_requires = '>=3'
)
