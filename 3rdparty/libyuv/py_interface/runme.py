import libyuv
import cv2
import numpy as np
from numpy import *
import matplotlib.pyplot as plt


# rgb resize
"""
img = cv2.imread("./7.jpg", 1)
img2 = cv2.cvtColor(img, cv2.COLOR_BGR2BGRA)
sz = img2.shape
src_height = sz[0]
src_width = sz[1]
src_channel = sz[2]
src_stride = src_width * src_channel

dst_width = 200
dst_height = 200
dst_stride = 200 * src_channel
src = np.ravel(img2)
dst = np.arange(dst_stride * dst_height, dtype = uint8)
libyuv.ARGBScale_func(src, src_stride, src_width, src_height, dst, dst_stride, dst_width, dst_height, libyuv.kFilterBilinear)
r = dst.reshape(dst_height, dst_width, src_channel)
dst_img = cv2.cvtColor(r, cv2.COLOR_BGRA2BGR)
cv2.imwrite("./771.jpg", dst_img)
"""
#gray resize

img = cv2.imread("./7.jpg", 0)
sz = img.shape
print (sz)
src_height = sz[0]
src_width = sz[1]
src_channel = 1
src_stride = src_width * src_channel

dst_width = 200
dst_height = 200
dst_stride = 200 * src_channel
src = np.ravel(img)
print (src)
dst = np.arange(dst_stride * dst_height, dtype = uint8)
libyuv.GRAYScale_func(src, src_stride, src_width, src_height, dst, dst_stride, dst_width, dst_height, libyuv.kFilterBilinear)
r = dst.reshape(dst_height, dst_width)
cv2.imwrite("./66.jpg", r)
