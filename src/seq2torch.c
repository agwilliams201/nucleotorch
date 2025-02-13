#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <Python.h>
#include <numpy/arrayobject.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> 

typedef struct {
    //total number of bits. to get num elements, divide by 2
    size_t size;
    unsigned char *data;
} BitArray;

BitArray *bitarray_create(size_t n) {
    BitArray *b = malloc(sizeof(BitArray));
    if (!b) return NULL;
    b->size = n;
    //add 7 for ceil div-- always allocate enough bytes
    //mult by 3 because 3 bit elements
    size_t num_bytes = (n * 3 + 7) / 8;
    b->data = calloc(num_bytes, sizeof(unsigned char));
    if (!b->data) {
        free(b);
        return NULL;
    }
    return b;
}

void bitarray_set(BitArray *b, size_t i, int value) {
    //oob check
    if (i >= b->size) return;
    size_t bit_offset = i * 3;
    size_t byte_index = bit_offset / 8;
    size_t bit_index = bit_offset % 8;
    size_t total_bytes = (b->size * 3 + 7) / 8;

    //single byte case
    if (bit_index <= 5) {
        unsigned char current = b->data[byte_index];
        //clear bits at target
        current &= ~(0x07 << bit_index);
        //set new value for target pos
        current |= ((value & 0x07) << bit_index);
        b->data[byte_index] = current;
    } else {
        //cross byte case
        //need uint16_t to handle both bytes
        uint16_t combined = b->data[byte_index];
        //last byte case
        if (byte_index + 1 < total_bytes) {
            combined |= ((uint16_t)b->data[byte_index + 1]) << 8;
        }
        //clear target pos
        uint16_t mask = 0x07 << bit_index;
        combined &= ~mask;
        //set bits across both bytes at target pos
        combined |= ((value & 0x07) << bit_index);
        b->data[byte_index] = combined & 0xFF;
        //last byte case
        if (byte_index + 1 < total_bytes) {
            b->data[byte_index + 1] = (combined >> 8) & 0xFF;
        }
    }
}

int bitarray_get(BitArray *b, size_t i) {
    //oob check
    if (i >= b->size) return 0;
    size_t bit_offset = i * 3;
    size_t byte_index = bit_offset / 8;
    size_t bit_index = bit_offset % 8;
    size_t total_bytes = (b->size * 3 + 7) / 8;
    //single byte case
    if (bit_index <= 5) {
        unsigned char current = b->data[byte_index];
        return (current >> bit_index) & 0x07;
    } else {
       //cross byte case
        uint16_t combined = b->data[byte_index];
        if (byte_index + 1 < total_bytes) {
            combined |= ((uint16_t)b->data[byte_index + 1]) << 8;
        }
        return (combined >> bit_index) & 0x07;
    }
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
            //handle any other nucleotide as 'N' (eg, 4)
            bitarray_set(b, i, 4);
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

size_t get_length_of_seq_fasta(FILE *fp){
    long start = ftell(fp);
    if (start == -1) {
        perror("ftell");
        return 0;
    }
    int c;
    size_t length = 0;
    while ((c = fgetc(fp)) != EOF) {
        if (c == '>'){
            break;
        }
        if (c != '\n'){
            length++;
        }
    }
    if (fseek(fp, start, SEEK_SET) != 0) {
        perror("fseek");
        return 0;
    }
    return length;
}

void read_seq_into_buff_fasta(char* buff, size_t len, FILE *fp){
    size_t count = 0;
    int c;
    while (count < len && (c = fgetc(fp)) != EOF) {
        if (c == '\n' || c == '\r') {
            // Skip newlines and carriage returns.
            continue;
        }
        buff[count++] = c;
    }
    buff[count] = '\0';
    return;
}


void skip_line(FILE *fp){
    int c;
    while ((c = fgetc(fp)) != EOF && c != '\n'){
    }
}


//cpython stuff

// global static variable to hold torch.from_numpy
static PyObject *torch_from_numpy_func = NULL;

// global static variable to hold bitarray_to_tensor as pyobj
static PyObject* bitarray_to_tensor(BitArray *b, int token_length);

//for freeing bitarray struct
static void capsule_destructor(PyObject *capsule) {
    void *ptr = PyCapsule_GetPointer(capsule, NULL);
    free(ptr);
}

//main method
static PyObject* process_fastq(PyObject* self, PyObject* args){
    const char* filename;
    unsigned int token_length = 1;  // default: token_length==1 => uint8 output
    // required string arg, optional uint arg
    if (!PyArg_ParseTuple(args, "s|I", &filename, &token_length)) {
        return NULL;
    }
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        PyErr_Format(PyExc_IOError, "Could not open file: %s", filename);
        return NULL;
    }
    PyObject* tensor_list = PyList_New(0);
    if (!tensor_list) {
        fclose(fp);
        return NULL;
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
            PyErr_NoMemory();
            fclose(fp);
            Py_DECREF(tensor_list);
            return NULL;
        }
        size_t bytesRead = fread(buff, 1, seqlen, fp);
        if (bytesRead != (size_t)size) {
            free(buff);
            fclose(fp);
            PyErr_Format(PyExc_IOError, "Failed to read sequence from file");
            Py_DECREF(tensor_list);
            return NULL;
        }
        buff[size] = '\0';
        // printf("%s\n", buff);
        BitArray* encoded = bitarray_create(seqlen);
        if (!encoded){
            free(buff);
            fclose(fp);
            PyErr_NoMemory();
            Py_DECREF(tensor_list);
            return NULL;
        }
        set_according_to_buff(encoded, buff, size);
        // printf("%d\n", bitarray_get(encoded, 20));
        // printf("%c\n", buff[20]);
        PyObject* tensor = bitarray_to_tensor(encoded, token_length);
        if (!tensor) {
            fclose(fp);
            Py_DECREF(tensor_list);
            return NULL;
        }
        // bitarray_free(encoded);
        free(buff);
        if (PyList_Append(tensor_list, tensor) != 0) {
            Py_DECREF(tensor);
            fclose(fp);
            Py_DECREF(tensor_list);
            return NULL;
        }
        Py_DECREF(tensor);
        for (int i = 0; i < 2; i++){
            skip_line(fp);
        }
    }
    fclose(fp);
    return tensor_list;
}

static PyObject* process_fasta(PyObject* self, PyObject* args){
    const char* filename;
    unsigned int token_length = 1;  // default: token_length==1 => uint8 output
    // required string arg, optional uint arg
    if (!PyArg_ParseTuple(args, "s|I", &filename, &token_length)) {
        return NULL;
    }
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        PyErr_Format(PyExc_IOError, "Could not open file: %s", filename);
        return NULL;
    }
    PyObject* tensor_list = PyList_New(0);
    if (!tensor_list) {
        fclose(fp);
        return NULL;
    }
    int c;
    //skip the first header
    skip_line(fp);
    while ((c = fgetc(fp)) != EOF){
        size_t seqlen = get_length_of_seq_fasta(fp);
        // printf("%zu\n", seqlen);
        long size = sizeof(char) * seqlen;
        char* buff = malloc(size + 1);
        if (!buff) {
            PyErr_NoMemory();
            fclose(fp);
            Py_DECREF(tensor_list);
            return NULL;
        }
        //gotta iterate over manually to read into buff fuck me lads
        read_seq_into_buff_fasta(buff, seqlen, fp);
        //need double skipline for newline and header
        skip_line(fp);
        skip_line(fp);
        BitArray* encoded = bitarray_create(seqlen);
        if (!encoded){
            free(buff);
            fclose(fp);
            PyErr_NoMemory();
            Py_DECREF(tensor_list);
            return NULL;
        }
        set_according_to_buff(encoded, buff, size+1);
        PyObject* tensor = bitarray_to_tensor(encoded, token_length);
        if (!tensor) {
            fclose(fp);
            Py_DECREF(tensor_list);
            return NULL;
        }
        // printf("%d\n", bitarray_get(encoded, 5));
        // printf("%c\n", buff[5]);
        // bitarray_free(encoded);
        free(buff);
        if (PyList_Append(tensor_list, tensor) != 0) {
            Py_DECREF(tensor);
            fclose(fp);
            Py_DECREF(tensor_list);
            return NULL;
        }
        Py_DECREF(tensor);
    }
    fclose(fp);
    return tensor_list;
}


// deprecated old func

// PyObject* bitarray_to_tensor(BitArray *b) {
//     //ceil div to get num bytes needed
//     int total_bytes = (b->size * 3 + 7) / 8;
//     //initialize array dims accordingly
//     npy_intp dims[1] = { total_bytes };

//     //just group into 8 bit chunks to make the np array of uint8s
//     //this needs to be parallelized + vectorized, so might have to write some custom stuff to interact with numpy api...
//     PyObject *np_array = PyArray_SimpleNewFromData(1, dims, NPY_UINT8, (void*)b->data);
//     if (!np_array) {
//         return NULL;
//     }
    
//     //b->data now owned by array, only need to free the struct itself
//     PyObject *capsule = PyCapsule_New(b->data, NULL, capsule_destructor);
//     PyArray_SetBaseObject((PyArrayObject*)np_array, capsule);
    
//     //free struct
//     free(b);

//     //pull torch in
//     if (!torch_from_numpy_func) {
//         Py_DECREF(np_array);
//         return NULL;
//     }
    
//     //tuple pack
//     PyObject *args = PyTuple_Pack(1, np_array);
//     Py_DECREF(np_array);
//     if (!args) {
//         return NULL;
//     }
//     //convert np array to tensor
//     PyObject *tensor = PyObject_CallObject(torch_from_numpy_func, args);
//     Py_DECREF(args);
    
//     return tensor;
// }

PyObject* bitarray_to_tensor(BitArray *b, int token_length) {
    // default to uint8
    if (token_length <= 1) {
        int total_bytes = (b->size * 3 + 7) / 8;
        npy_intp dims[1] = { total_bytes };
        PyObject *np_array = PyArray_SimpleNewFromData(1, dims, NPY_UINT8, (void*)b->data);
        if (!np_array) {
            return NULL;
        }
        //np array now owns b->data
        PyObject *capsule = PyCapsule_New(b->data, NULL, capsule_destructor);
        PyArray_SetBaseObject((PyArrayObject*)np_array, capsule);
        free(b);
        
        if (!torch_from_numpy_func) {
            Py_DECREF(np_array);
            return NULL;
        }
        PyObject *args = PyTuple_Pack(1, np_array);
        Py_DECREF(np_array);
        if (!args) {
            return NULL;
        }
        PyObject *tensor = PyObject_CallObject(torch_from_numpy_func, args);
        Py_DECREF(args);
        return tensor;
    }
    
    //otherwise need to chunk
    int total_tokens = (b->size + token_length - 1) / token_length; // ceiling division
    int bits_per_token = token_length * 3;
    npy_intp dims[1] = { total_tokens };

    if (bits_per_token <= 8) {
        uint8_t *tokens = malloc(total_tokens * sizeof(uint8_t));
        if (!tokens) {
            PyErr_NoMemory();
            free(b->data);
            free(b);
            return NULL;
        }
        for (int t = 0; t < total_tokens; t++) {
            uint8_t token = 0;
            for (int j = 0; j < token_length; j++) {
                size_t idx = t * token_length + j;
                int nucleotide = (idx < b->size) ? bitarray_get(b, idx) : 4; // pad with N
                token = (token << 3) | (nucleotide & 0x07);
            }
            tokens[t] = token;
        }
        free(b->data);
        free(b);
        PyObject *np_array = PyArray_SimpleNewFromData(1, dims, NPY_UINT8, (void*)tokens);
        if (!np_array) {
            free(tokens);
            return NULL;
        }
        PyObject *capsule = PyCapsule_New(tokens, NULL, capsule_destructor);
        PyArray_SetBaseObject((PyArrayObject*)np_array, capsule);
        if (!torch_from_numpy_func) {
            Py_DECREF(np_array);
            return NULL;
        }
        PyObject *args = PyTuple_Pack(1, np_array);
        Py_DECREF(np_array);
        if (!args) return NULL;
        PyObject *tensor = PyObject_CallObject(torch_from_numpy_func, args);
        Py_DECREF(args);
        return tensor;
    } else if (bits_per_token <= 16) {
        uint16_t *tokens = malloc(total_tokens * sizeof(uint16_t));
        if (!tokens) {
            PyErr_NoMemory();
            free(b->data);
            free(b);
            return NULL;
        }
        for (int t = 0; t < total_tokens; t++) {
            uint16_t token = 0;
            for (int j = 0; j < token_length; j++) {
                size_t idx = t * token_length + j;
                int nucleotide = (idx < b->size) ? bitarray_get(b, idx) : 4;
                token = (token << 3) | (nucleotide & 0x07);
            }
            tokens[t] = token;
        }
        free(b->data);
        free(b);
        PyObject *np_array = PyArray_SimpleNewFromData(1, dims, NPY_UINT16, (void*)tokens);
        if (!np_array) {
            free(tokens);
            return NULL;
        }
        PyObject *capsule = PyCapsule_New(tokens, NULL, capsule_destructor);
        PyArray_SetBaseObject((PyArrayObject*)np_array, capsule);
        if (!torch_from_numpy_func) {
            Py_DECREF(np_array);
            return NULL;
        }
        PyObject *args = PyTuple_Pack(1, np_array);
        Py_DECREF(np_array);
        if (!args) return NULL;
        PyObject *tensor = PyObject_CallObject(torch_from_numpy_func, args);
        Py_DECREF(args);
        return tensor;
    } else if (bits_per_token <= 32) {
        uint32_t *tokens = malloc(total_tokens * sizeof(uint32_t));
        if (!tokens) {
            PyErr_NoMemory();
            free(b->data);
            free(b);
            return NULL;
        }
        for (int t = 0; t < total_tokens; t++) {
            uint32_t token = 0;
            for (int j = 0; j < token_length; j++) {
                size_t idx = t * token_length + j;
                int nucleotide = (idx < b->size) ? bitarray_get(b, idx) : 4;
                token = (token << 3) | (nucleotide & 0x07);
            }
            tokens[t] = token;
        }
        free(b->data);
        free(b);
        PyObject *np_array = PyArray_SimpleNewFromData(1, dims, NPY_UINT32, (void*)tokens);
        if (!np_array) {
            free(tokens);
            return NULL;
        }
        PyObject *capsule = PyCapsule_New(tokens, NULL, capsule_destructor);
        PyArray_SetBaseObject((PyArrayObject*)np_array, capsule);
        if (!torch_from_numpy_func) {
            Py_DECREF(np_array);
            return NULL;
        }
        PyObject *args = PyTuple_Pack(1, np_array);
        Py_DECREF(np_array);
        if (!args) return NULL;
        PyObject *tensor = PyObject_CallObject(torch_from_numpy_func, args);
        Py_DECREF(args);
        return tensor;
    } else if (bits_per_token <= 64) {
        uint64_t *tokens = malloc(total_tokens * sizeof(uint64_t));
        if (!tokens) {
            PyErr_NoMemory();
            free(b->data);
            free(b);
            return NULL;
        }
        for (int t = 0; t < total_tokens; t++) {
            uint64_t token = 0;
            for (int j = 0; j < token_length; j++) {
                size_t idx = t * token_length + j;
                int nucleotide = (idx < b->size) ? bitarray_get(b, idx) : 4;
                token = (token << 3) | (nucleotide & 0x07);
            }
            tokens[t] = token;
        }
        free(b->data);
        free(b);
        PyObject *np_array = PyArray_SimpleNewFromData(1, dims, NPY_UINT64, (void*)tokens);
        if (!np_array) {
            free(tokens);
            return NULL;
        }
        PyObject *capsule = PyCapsule_New(tokens, NULL, capsule_destructor);
        PyArray_SetBaseObject((PyArrayObject*)np_array, capsule);
        if (!torch_from_numpy_func) {
            Py_DECREF(np_array);
            return NULL;
        }
        PyObject *args = PyTuple_Pack(1, np_array);
        Py_DECREF(np_array);
        if (!args) return NULL;
        PyObject *tensor = PyObject_CallObject(torch_from_numpy_func, args);
        Py_DECREF(args);
        return tensor;
    } else {
        PyErr_SetString(PyExc_ValueError, "Token length too high; cannot pack into 64 bits");
        free(b->data);
        free(b);
        return NULL;
    }
}


static PyMethodDef NucleotorchMethods[] = {
    {"process_fastq", process_fastq, METH_VARARGS, "Process a FASTQ file and return a list of PyTorch tensors."},
    {"process_fasta", process_fasta, METH_VARARGS, "Process a FASTA file and return a list of PyTorch tensors."},
    //sentinel
    {NULL, NULL, 0, NULL}
};

// module def
static struct PyModuleDef nucleotorch = {
    PyModuleDef_HEAD_INIT,    
    "nucleotorch",               
    "Convert FASTQ and FASTA reads to binary PyTorch tensors!", 
    -1,                         
    NucleotorchMethods,
    NULL,   /* m_slots */
    NULL,   /* m_traverse */
    NULL,   /* m_clear */
    NULL    /* m_free */      
};

// mod init func
PyMODINIT_FUNC PyInit_nucleotorch(void) {
    PyObject *m = PyModule_Create(&nucleotorch);
    if (m == NULL)
        return NULL;
    import_array(); 

    PyObject *torch_module = PyImport_ImportModule("torch");
    if (!torch_module) {
        Py_DECREF(m);
        return NULL;
    }
    torch_from_numpy_func = PyObject_GetAttrString(torch_module, "from_numpy");
    Py_DECREF(torch_module);
    if (!torch_from_numpy_func) {
        Py_DECREF(m);
        return NULL;
    }
    PyModule_AddObject(m, "torch_from_numpy", torch_from_numpy_func);
    return m;
}

// #ifdef TEST_MAIN
// int main(){
//     //init py interpreter
//     Py_Initialize();
//     if (_import_array() < 0) {
//         PyErr_Print();
//         exit(1);
//     }

//     //simulate calling func
//     PyObject* args = Py_BuildValue("(s)", "../data/9_Swamp_S2B_rbcLa_2019_minq7.fastq");
//     PyObject* result = process_fastq(NULL, args);
//     Py_DECREF(args);
//     if (result) {
//         //print res
//         PyObject* repr = PyObject_Repr(result);
//         const char* str = PyUnicode_AsUTF8(repr);
//         printf("Result: %s\n", str);
//         Py_DECREF(repr);
//         Py_DECREF(result);
//     } else {
//         PyErr_Print();
//     }
//     Py_Finalize();
//     return 0;
// }
// #endif
