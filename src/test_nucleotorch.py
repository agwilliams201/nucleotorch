#!/usr/bin/env python3
import nucleotorch

def main():
    #path to test file
    filename = "../data/9_Swamp_S2B_rbcLa_2019_minq7.fastq"
    
    #here goes nothing
    try:
        tensors = nucleotorch.process_fastq(filename)
    except Exception as e:
        print("Error processing FASTQ file:", e)
        return

    print(f"Processed {len(tensors)} tensors from {filename}")

    filename = "../data/SRR31676618.fasta"

    print(tensors[0])
    print(tensors[0].size())

    try:
        tensors = nucleotorch.process_fasta(filename)
    except Exception as e:
        print("Error processing FASTA file:", e)
        return
    
    print(f"Processed {len(tensors)} tensors from {filename}")

    # # for idx, tensor in enumerate(tensors):
    # #     print(f"Tensor {idx}:")
    # #     print(tensor)

if __name__ == '__main__':
    main()
