// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

struct module_state {
    PyObject* error;
};

#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#define INITERROR return NULL

static PyObject* error_out(PyObject* m) {
    struct module_state* st = GETSTATE(m);
    PyErr_SetString(st->error, "something bad happened");
    return NULL;
}

//This is the function that is called from your python code
static PyObject* addList_add(PyObject* self, PyObject* args) {

    PyObject* listObj;

    //The input arguments come as a tuple, we parse the args to get the various variables
    //In this case it's only one list variable, which will now be referenced by listObj
    if (!PyArg_ParseTuple(args, "O", &listObj))
        return NULL;

    //length of the list
    unsigned long long length = PyList_Size(listObj);

    //iterate over all the elements
    unsigned long long i, sum = 0;
    for (i = 0; i < length; i++) {
        //get an element out of the list - the element is also a python objects
        PyObject* temp = PyList_GetItem(listObj, i);
        //we know that object represents an integer - so convert it into C long
        //long elem = PyInt_AsLong(temp);
        long elem = PyLong_AsLong(temp);
        sum += elem;
    }

    //value returned back to python code - another python object
    //build value here converts the C long to a python integer
    return Py_BuildValue("i", sum);
}

static PyObject* ChrToInt(PyObject* self, PyObject* args)
{
    PyObject* ch = nullptr;
    if (!PyArg_ParseTuple(args, "O", &ch)) {
        return Py_BuildValue("");
    }
    PyObject* val = PyLong_FromUnicodeObject(ch, 16);
    return val;
}

static PyObject* StrToInt(PyObject* self, PyObject* args)
{
    PyObject* data = nullptr;
    if (!PyArg_ParseTuple(args, "O", &data) || PySequence_Size(data) != 5+1) {
        return Py_BuildValue("");
    }
    PyObject* l = PyList_New(3);
    Py_INCREF(l);
    PyObject* R = PyLong_FromUnicodeObject(PySequence_GetSlice(data, 0, 2), 16);
    PyList_SetItem(l, 0, R);
    PyObject* G = PyLong_FromUnicodeObject(PySequence_GetSlice(data, 2, 4), 16);
    PyList_SetItem(l, 1, G);
    PyObject* B = PyLong_FromUnicodeObject(PySequence_GetSlice(data, 4, 6), 16);
    PyList_SetItem(l, 2, B);

    return l;
}

PyObject* ToHex(PyObject* xy)
{
    PyObject* l = PyList_New(3);
    Py_INCREF(l);
    PyList_SetItem(l, 0, PyLong_FromUnicodeObject(PySequence_GetSlice(xy, 0, 2), 16));
    PyList_SetItem(l, 1, PyLong_FromUnicodeObject(PySequence_GetSlice(xy, 2, 4), 16));
    PyList_SetItem(l, 2, PyLong_FromUnicodeObject(PySequence_GetSlice(xy, 4, 6), 16));
    return l;
}

static PyObject* ParseFrameList(PyObject* self, PyObject* args)
{
    PyObject* numpyArr = nullptr;
    PyObject* flatArr = nullptr;
    if (!PyArg_ParseTuple(args, "OO", &numpyArr, &flatArr)) {
        return Py_BuildValue("");
    }
    Py_INCREF(numpyArr);
    UINT64 width = PyList_Size(numpyArr);
    unsigned long long height = PyList_Size(PyList_GetItem(numpyArr, 0));
    unsigned int i, j, index = 0;
    for (i = 0; i < width; i++) {
        for (j = 0; j < height; j++) {
            PyObject* vals = ToHex(PySequence_GetSlice(flatArr, index+0, index+6));
            PyObject* temp = PyList_GetItem(numpyArr, i);
            Py_INCREF(temp);
            PyList_SetItem(temp, j, vals);
            Py_DECREF(temp);
            index += 6;
        }
    }

    return Py_BuildValue("i", index);
}

static PyObject* ParseFrame(PyObject* self, PyObject* args)
{
    PyArrayObject *numpyArr = nullptr;
    PyObject* flatArr = nullptr;
    if (!PyArg_ParseTuple(args, "OO!", &flatArr, &PyArray_Type, &numpyArr)) {
        return Py_BuildValue("");
    }
    Py_INCREF(numpyArr);
    UINT64 width = PyObject_Size((PyObject*)numpyArr);
    unsigned long long height = PyArray_DIM(numpyArr, 1);
    unsigned int i, j, index = 0;
    for (i = 0; i < width; i++) {
        for (j = 0; j < height; j++) {
            PyObject* vals = ToHex(PySequence_GetSlice(flatArr, index + 0, index + 6));
            npy_intp q = 0;
            void* temp = PyArray_GETPTR3(numpyArr, i, j, q);
            PyArray_SETITEM((PyArrayObject*)numpyArr, (char*)temp, PyList_GetItem(vals, 2));
            q++;
            temp = PyArray_GETPTR3(numpyArr, i, j, q);
            PyArray_SETITEM((PyArrayObject*)numpyArr, (char*)temp, PyList_GetItem(vals, 1));
            q++;
            temp = PyArray_GETPTR3(numpyArr, i, j, q);
            PyArray_SETITEM((PyArrayObject*)numpyArr, (char*)temp, PyList_GetItem(vals, 0));
            Py_DECREF(vals);
            index += 6;
        }
    }

    return Py_BuildValue("ii", width, height);
}

static int myextension_traverse(PyObject* m, visitproc visit, void* arg) {
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

static int myextension_clear(PyObject* m) {
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}

//This is the docstring that corresponds to our 'add' function.
static char addList_docs[] =
"add( ): add all elements of the list\n";

static char ParseData_docs[] =
"convert hex flat array data to numpy image data\nArg1 = numpy list,Arg2 = hex list,Arg3 = dict sink\n";

/* This table contains the relavent info mapping -
  <function-name in python module>, <actual-function>,
  <type-of-args the function expects>, <docstring associated with the function>
*/
static PyMethodDef addList_funcs[] = {
    {"add", (PyCFunction)addList_add, METH_VARARGS, addList_docs},
    {"ParseFrameData", (PyCFunction)ParseFrameList, METH_VARARGS, ParseData_docs},
    {"ParseFrame", (PyCFunction)ParseFrame, METH_VARARGS, "Note the re-ordering of parameters"},
    {"CharToInt", (PyCFunction)ChrToInt, METH_VARARGS, "Test Func"},
    {"StrToInt", (PyCFunction)StrToInt, METH_VARARGS, "Debug Func"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "StreamProcessorFAST",
        NULL,
        sizeof(struct module_state),
        addList_funcs,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC PyInit_StreamProcessor_PY(void)
{
    PyObject* module = PyModule_Create(&moduledef);
    if (module == NULL)
        INITERROR;
    struct module_state* st = GETSTATE(module);

    st->error = PyErr_NewException("myextension.Error", NULL, NULL);
    if (st->error == NULL) {
        Py_DECREF(module);
        INITERROR;
    }
    import_array();

    return module;
}
