#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> 

typedef struct {
    size_t size;
    unsigned char *data;
} BitArray;

BitArray *bitarray_create(size_t n) {
    BitArray *b = malloc(sizeof(BitArray));
    if (!b) return NULL;
    b->size = n;
    size_t num_bytes = (n + 7) / 8;
    b->data = calloc(num_bytes, sizeof(unsigned char));
    if (!b->data) {
        free(b);
        return NULL;
    }
    return b;
}

void bitarray_set(BitArray *b, size_t i, int value) {
     if (i >= b->size) return;
    size_t bit_offset = i * 2;
    size_t byte_index = bit_offset / 8;
    size_t bit_index = bit_offset % 8;
    b->data[byte_index] &= ~(0x03 << bit_index);
    b->data[byte_index] |= ((value & 0x03) << bit_index);
}

int bitarray_get(BitArray *b, size_t i) {
    if (i >= b->size) return 0;
    size_t bit_offset = i * 2;
    size_t byte_index = bit_offset / 8;
    size_t bit_index = bit_offset % 8;
    return (b->data[byte_index] >> bit_index) & 0x03;
}

void set_according_to_buff(BitArray *b, char* buff, long bufflen){
    for (int i = 0; i < bufflen; i++){
        switch (buff[i]){
            case 'A':
            break;
            case 'T':
            bitarray_set(b, i, 1);
            break;
            case 'G':
            bitarray_set(b, i, 2);
            break;
            case 'C':
            bitarray_set(b, i, 3);
            break;
            default:
            break;
        }
    }
}

void bitarray_free(BitArray *b) {
    if (b) {
        free(b->data);
        free(b);
    }
}


size_t get_length_of_seq(FILE *fp){
    size_t start = ftell(fp);
    // if (start == -1) {
    //     perror("ftell");
    //     return -1;
    // }
    int c;
    size_t length = 0;
    while ((c = fgetc(fp)) != EOF) {
        length++;
        if (c == '\n')
            break;
    }
    if (fseek(fp, start, SEEK_SET) != 0) {
        perror("fseek");
        return -1;
    }
    return length;
}

void skip_line(FILE *fp){
    int c;
    while ((c = fgetc(fp)) != EOF && c != '\n'){
    }
}

int main(){
    char* filename = "../data/9_Swamp_S2B_rbcLa_2019_minq7.fastq";
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        return 0;
    }
    int c;
    int i = 0;
    while ((c = fgetc(fp)) != EOF){
        if (i % 2 == 0){
            i++;
            skip_line(fp);
            continue;
        }
        i++;
        size_t seqlen = get_length_of_seq(fp);
        long size = sizeof(char) * seqlen;
        char* buff = malloc(size + 1);
        if (!buff) {
            perror("malloc");
            fclose(fp);
            return 0;
        }
        size_t bytesRead = fread(buff, 1, seqlen, fp);
        if (bytesRead != (size_t)size) {
            perror("fread");
            free(buff);
            fclose(fp);
            return 0;
        }
        buff[size] = '\0';
        // printf("%s\n", buff);
        BitArray* encoded = bitarray_create(seqlen * 4);
        set_according_to_buff(encoded, buff, size+1);
        printf("%d\n", bitarray_get(encoded, 5));
        bitarray_free(encoded);
        free(buff);
        for (int i = 0; i < 2; i++){
            skip_line(fp);
        }
    }
    return 0;
}
