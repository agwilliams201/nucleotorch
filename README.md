# nucleotorch
Convert FASTQ reads to binary PyTorch tensors!

Nucleotorch is a lightweight, fast tool for converting raw NGS data in the form of FASTQ files into numerical PyTorch tensors for genomic machine learning applications.

With the rise of models such as DNABERT, it's become clear that transformers have the capability to completely upend the field of genetics. More recently, encoder-only models such as InstaDeep's Nucleotide Transformer have started to appear. Models such as these have the capacity to learn realtionships between genes in an unsupervised manner, allowing for the use of unaligned NGS reads as input data. 

One problem with using FASTQ data, though, is its prohbitively large size. Nucleotorch solves this issue by converting the FASTQ text sequences into packed bit array in C, and then turning this bitarray into a PyTorch tensor consisiting of uint8s. Not only does this do away with the memory-intensity and long runtimes that come with reading and parsing large text files in Python, it also allows for concise and frictionless generation of compact, unique input samples.

Nucleotorch requires Numpy and PyTorch. Nucleotorch only accepts valid FASTQ files as arguments.

DOCUMENTATION

def process_fastq(filepath: str)-> List[torch.Tensor]:
    """ Processes FASTQ reads into PyTorch embedding vectors.

    Args:
        filepath (str): path to the FASTQ file relative to cwd
    
    Returns:
        List[torch.Tensor]: a list of PyTorch tensors, with each tensor being composed of uint8s

    """

Example usage:

import nucleotorch

tensors = nucleotorch.process_fastq("path/to/example/FASTQ/file")

Source Code:

https://github.com/agwilliams201/nucleotorch