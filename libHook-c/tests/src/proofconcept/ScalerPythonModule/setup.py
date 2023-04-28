from setuptools import Extension
from setuptools import setup


module = Extension("scaler", sources=["TestPythonModuleInterception.cpp"], extra_compile_args=["-Wall"])

setup(
    name="scaler",
    version="1.0.0",
    description="Scaler frame interception test",
    url="https://github.com/UTSASRG/Scaler.git",
    author="Steven Tang",
    author_email="jtang@umass.edu",
    license="MIT",
    classifiers=[
        "Programming Language :: Python :: 3",
    ],
    ext_modules=[module],
    # test_suite="tests",
)