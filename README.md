# nucleotorch
Convert FASTQ and FASTA reads to binary PyTorch tensors!

Nucleotorch is a lightweight, fast tool for converting raw NGS data in the form of FASTQ or FASTA files into numerical 
PyTorch tensors for genomic machine learning applications.

With the rise of models such as DNABERT, it's become clear that transformers have the capability to completely upend the 
field of genetics. More recently, encoder-only models such as InstaDeep's Nucleotide Transformer have started to appear.
Models such as these have the capacity to learn realtionships between genes in an unsupervised manner, allowing for the
use of unaligned NGS reads as input data. 

One problem with using FASTQ/FASTA data, though, is its prohbitively large size. Nucleotorch solves this issue by 
converting the FASTQ/FASTA text sequences into packed bit array in C, and then turning this bitarray into a PyTorch 
tensor consisiting of unsigned integers. Not only does this do away with the memory-intensity and long runtimes that 
come with reading and parsing large text files in Python, it also allows for concise and frictionless generation of 
compact, unique input samples.

Nucleotorch requires Numpy and PyTorch. Nucleotorch only accepts valid FASTQ or FASTA files as arguments.

RELEASE NOTES:

Version 1.2.0 (2/16/2025) (Latest): perf improvements on unpacking algo, refactoring to isolate tokenization in anticipation of adding cuda kernels soon

Version 1.1.13 (2/14/2025): toml updates for venv

Version 1.1.11 (2/14/2025): linux support

Version 1.1.8 (2/13/2025): macOS intel support

Version 1.1.7 (2/13/2025): Distribution fixes

Version 1.1.0 (2/12/2025): FASTA and FASTQ support, new token_length parameter allows users to control the size
of the tokens in the tensor

Version 1.0.3 (2/11/2025): FASTQ support

DOCUMENTATION

```python
def process_fastq(filepath: str, token_length: int = 1) -> List[torch.Tensor]:
    """
    Processes FASTQ reads into PyTorch embedding vectors.

    Args:
        filepath (str): Path to the FASTQ file relative to the current working directory.
        token_length (int, optional): Number of desired nucleotides per token (ie, entry in the tensor). If not 
        specified, the bitarray is divided into uint8 chunks by default.

    Returns:
        List[torch.Tensor]: A list of PyTorch tensors, with each tensor being composed of uints (type of uint depends on token_length).
    """
```

Example usage:

```python
import nucleotorch
tensors = nucleotorch.process_fastq("path/to/example/FASTQ/file", 6)
```

```python
def process_fasta(filepath: str, token_length: int = 1) -> List[torch.Tensor]:
    """
    Processes FASTQ reads into PyTorch embedding vectors.

    Args:
        filepath (str): Path to the FASTA file relative to the current working directory.
        token_length (int, optional): Number of desired nucleotides per token (ie, entry in the tensor). If not 
        specified, the bitarray is divided into uint8 chunks by default.

    Returns:
        List[torch.Tensor]: A list of PyTorch tensors, with each tensor being composed of uints (type of uint depends on token_length).
    """
```

Example usage:

```python
import nucleotorch
tensors = nucleotorch.process_fasta("path/to/example/FASTA/file", 6)
```

KNOWN ISSUES

2/14/2025
In some cases on intel macOS or linux, venv installation fails due to not finding numpy via the toml. This can be fixed by first installing numpy in the venv, and doing a fresh reinstall of nucleotorch, or by globally installing nucleotorch outside of a venv (assuming you have numpy installed). This allows for a cached file to be built and used for venv installation. To be fixed in v1.2.1

Source Code:

https://github.com/agwilliams201/nucleotorch