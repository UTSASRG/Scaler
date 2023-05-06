from skbuild import setup  # This line replaces 'from setuptools import setup'

# Dependencies
# conda install -c anaconda cython
# conda install -c conda-forge pybind11 scikit-build pybind11
setup(
    name="scaler",
    version="1.0.0",
    description="Scaler frame interception test",
    url="https://github.com/UTSASRG/Scaler.git",
    author="Steven Tang",
    author_email="jtang@umass.edu",
    license="GPL",
    classifiers=[
        "Programming Language :: Python :: 3",
    ],
    packages=['scaler'],
    python_requires=">=3.7",
)