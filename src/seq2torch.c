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

//cython shit

// global static variable to hold torch.from_numpy
static PyObject *torch_from_numpy_func = NULL;

// global static variable to hold bitarray_to_tensor as pyobj
static PyObject* bitarray_to_tensor(BitArray *b);

//for freeing bitarray struct
static void capsule_destructor(PyObject *capsule) {
    void *ptr = PyCapsule_GetPointer(capsule, NULL);
    free(ptr);
}

//main method
static PyObject* process_fastq(PyObject* self, PyObject* args){
    const char* filename;
    if (!PyArg_ParseTuple(args, "s", &filename)) {
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
        BitArray* encoded = bitarray_create(seqlen * 2);
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
        PyObject* tensor = bitarray_to_tensor(encoded);
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

PyObject* bitarray_to_tensor(BitArray *b) {
    //ceil div to get num bytes needed
    int total_bytes = (b->size * 2 + 7) / 8;
    //initialize array dims accordingly
    npy_intp dims[1] = { total_bytes };

    //just group into 8 bit chunks to make the np array of uint8s
    //this needs to be parallelized + vectorized, so might have to write some custom stuff to interact with numpy api...
    PyObject *np_array = PyArray_SimpleNewFromData(1, dims, NPY_UINT8, (void*)b->data);
    if (!np_array) {
        return NULL;
    }
    
    //b->data now owned by array, only need to free the struct itself
    PyObject *capsule = PyCapsule_New(b->data, NULL, capsule_destructor);
    PyArray_SetBaseObject((PyArrayObject*)np_array, capsule);
    
    //free struct
    free(b);

    //pull torch in
    if (!torch_from_numpy_func) {
        Py_DECREF(np_array);
        return NULL;
    }
    
    //tuple pack
    PyObject *args = PyTuple_Pack(1, np_array);
    Py_DECREF(np_array);
    if (!args) {
        return NULL;
    }
    //convert np array to tensor
    PyObject *tensor = PyObject_CallObject(torch_from_numpy_func, args);
    Py_DECREF(args);
    
    return tensor;
}

static PyMethodDef NucleotorchMethods[] = {
    {"process_fastq", process_fastq, METH_VARARGS, "Process a FASTQ file and return a list of PyTorch tensors."},
    //sentinel
    {NULL, NULL, 0, NULL}
};

// module def
static struct PyModuleDef nucleotorch = {
    PyModuleDef_HEAD_INIT,    
    "nucleotorch",               
    "Convert FASTQ reads to binary PyTorch tensors!", 
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
