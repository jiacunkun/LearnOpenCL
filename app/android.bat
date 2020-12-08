adb root
adb remount

adb shell "rm -rf /data/local/tmp/test/"

adb shell "mkdir -p /data/local/tmp/test/"

adb push IMG_20201116104745_0_4000x3000_iso_466_crop_1011_697_2012_1509.nv12 /data/local/tmp/test/

adb push ../src/nlmOCL/PyramidNLM.cl /data/local/tmp/test/

adb logcat -c

adb push ../cmake-build-release/bin/test_ocl /data/local/tmp/test/
adb shell "chmod 777 /data/local/tmp/test/test_ocl"
adb shell "LD_LIBRARY_PATH=/data/local/tmp/test:$LD_LIBRARY_PATH  /data/local/tmp/test/test_ocl"
adb pull /data/local/tmp/test/IMG_20201116104745_0_4000x3000_iso_466_crop_1011_697_2012_1509_res_android.nv12 ./out/
adb pull /data/local/tmp/test/binary_source.bin ./out/
adb pull /data/local/tmp/test/program_binary.bin ./out/


adb logcat > ./log/12.8.2.txt

pause