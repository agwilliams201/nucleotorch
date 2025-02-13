from setuptools import setup, Extension
import numpy

module = Extension(
    "nucleotorch",
    sources=["seq2torch.c"],
    include_dirs=[numpy.get_include()],
    extra_compile_args=[],
    extra_link_args=["-undefined", "dynamic_lookup"],
)

setup(
    name="nucleotorch",
    version="1.1.0",
    author="Alex Williams",
    author_email="agwilliams200@gmail.com",
    description="Convert FASTQ and FASTA reads to binary PyTorch tensors!",
    long_description=open("../README.md", "r", encoding="utf-8").read(),
    long_description_content_type="text/markdown",
    url="https://github.com/agwilliams201/nucleotorch",
    classifiers=[
        "Programming Language :: Python :: 3",
        "Programming Language :: C",
        "License :: Public Domain",
        "Operating System :: OS Independent",
    ],
    ext_modules=[module],
    python_requires='>=3.6',
)

