from distutils.core import setup, Extension
import sys


libyuv_module = Extension('_libyuv',
                                                 library_dirs=['./../macOS/lib/'],
                                                 libraries=['yuv'],
                                                 sources=['libyuv_wrap.cxx'] )

setup (name = 'libyuv',
       version = '0.1',
       author      = "lb",
       description = """for test""",
       ext_modules = [libyuv_module],
       py_modules = ["libyuv"],)

