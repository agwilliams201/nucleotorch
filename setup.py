from setuptools import setup, Extension

module = Extension(
    "nucleotorch",
    sources=["seq2torch.c"],
)

setup(
    name="nucleotorch",
    version="1.0",
    description="Convert fastq reads to binary PyTorch tensors!",
    ext_modules=[module],
)
