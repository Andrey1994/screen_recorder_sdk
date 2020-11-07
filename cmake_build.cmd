rmdir /S /Q build
mkdir build

cd build

cmake -G "Visual Studio 14 2015 Win64" -DCMAKE_SYSTEM_VERSION=10.0.15063.0 ..
cmake --build . --config Release
cd ..

xcopy compiled\ScreenRecorder.dll python\screen_recorder_sdk\lib\ /s /Y
xcopy compiled\ScreenRecorder.dll java\screen_recorder\src\main\resources\ /s /Y