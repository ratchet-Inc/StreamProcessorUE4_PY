#build the modules

from distutils.core import setup, Extension

setup(name='StreamProcessor_PY', version='1.0',  \
      ext_modules=[Extension('StreamProcessor_PY', ['dllmain.cpp'])])

# build command: python setup.py build or install