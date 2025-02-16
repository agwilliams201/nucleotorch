#!/usr/bin/env python3
import nucleotorch
import datetime

def main():
    res = []
    filename = "../data/GCF_000005845.2_ASM584v2_genomic.fna"
    for i in range(2000):
        if i % 100 == 0:
            print(f"{i}/2000 iterations")
        start = datetime.datetime.now()
        try:
            tensors = nucleotorch.process_fasta(filename, 6)
        except Exception as e:
            print("Error processing FASTA file:", e)
        end = datetime.datetime.now()
        res.append((end-start).total_seconds())
    print(f"Average time per file: {sum(res)/len(res)} s")
    #path to test file
    # filename = "../data/9_Swamp_S2B_rbcLa_2019_minq7.fastq"
    
    # #here goes nothing
    # try:
    #     tensors = nucleotorch.process_fastq(filename)
    # except Exception as e:
    #     print("Error processing FASTQ file:", e)
    #     return

    # print(f"Processed {len(tensors)} tensors from {filename}")

    # filename = "../data/SRR31676618.fasta"

    # print(tensors[0])
    # print(tensors[0].size())

    # try:
    #     tensors = nucleotorch.process_fasta(filename)
    # except Exception as e:
    #     print("Error processing FASTA file:", e)
    #     return
    
    # print(f"Processed {len(tensors)} tensors from {filename}")

    # # for idx, tensor in enumerate(tensors):
    # #     print(f"Tensor {idx}:")
    # #     print(tensor)

if __name__ == '__main__':
    main()
