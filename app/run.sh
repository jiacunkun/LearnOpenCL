#!/usr/bin/env bash

#sh ./android_build.sh


adb shell "rm -rf /data/local/tmp/test/"
adb shell "mkdir -p /data/local/tmp/test/"



adb push ./test_4032x3024.NV21             /data/local/tmp/test/
adb push ./test_out_org.png                 /data/local/tmp/test/
adb push ../src/nlmOCL/PyramidNLM.cl        /data/local/tmp/test/
adb push ./libCheeseSymbols/android/armeabi-v7a/bin/libNLM_OCL.a    /data/local/tmp/test/


adb push ./libCheeseSymbols/android/armeabi-v7a/bin/test_ocl    /data/local/tmp/test/
adb shell "chmod 777 /data/local/tmp/test/test_ocl"
adb shell "LD_LIBRARY_PATH=/data/local/tmp/test:$LD_LIBRARY_PATH  /data/local/tmp/test/test_ocl"



adb pull /data/local/tmp/test/test_org.png   ./out/
adb pull /data/local/tmp/test/test_out.png   ./out/
adb pull /data/local/tmp/test/test_out.NV21   ./out/
#adb pull /data/local/tmp/test/binary_source.bin ./out/
#adb pull /data/local/tmp/test/program_binary.bin ./out/
