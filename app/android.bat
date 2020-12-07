adb root
adb remount

adb shell "rm -rf /data/local/tmp/test/"

adb shell "mkdir -p /data/local/tmp/test/"

adb push ISO00304_043_1_4624x3472_[0]_50-0-50-0-0-0-80-0-0-0-0-1-0-1-0-1-0-0-0-0-0-0-0--1-0-0-0-ISO=0-CamType=0-fZoomValue=1.000000_res.NV21 /data/local/tmp/test/

adb push ../src/nlmOCL/PyramidNLM.cl /data/local/tmp/test/

adb logcat -c

adb push ../cmake-build-release/bin/test_ocl /data/local/tmp/test/
adb shell "chmod 777 /data/local/tmp/test/test_ocl"
adb shell "LD_LIBRARY_PATH=/data/local/tmp/test:$LD_LIBRARY_PATH  /data/local/tmp/test/test_ocl"
adb pull /data/local/tmp/test/ISO00304_043_1_4624x3472_[0]_50-0-50-0-0-0-80-0-0-0-0-1-0-1-0-1-0-0-0-0-0-0-0--1-0-0-0-ISO=0-CamType=0-fZoomValue=1.000000_res_res.NV21 ./out/
adb pull /data/local/tmp/test/binary_source.bin ./out/
adb pull /data/local/tmp/test/program_binary.bin ./out/


adb logcat > ./log/12.7.txt

pause