#include <Python.h>
#include "8b10b.h"

static PyObject *truncated, *codeviolation;

static PyObject *moduleDecode (PyObject *self, PyObject *args) {
	/* decodeable length in bits */
	Py_ssize_t len;
    Py_buffer data;
	eightbtenbCtx decoder;
	eightbtenbInit (&decoder);

    if (!PyArg_ParseTuple (args, "y*n", &data, &len)) {
        return NULL;
	}
	assert (PyBuffer_IsContiguous (&data, 'C'));

	if (data.len < len/8) {
		/* truncated */
		PyErr_SetString (truncated, "Byte string too small");
		PyBuffer_Release (&data);
		return NULL;
	}

	char * const decoded = calloc (len/10, sizeof (*decoded));
	eightbtenbSetDest (&decoder, decoded);
	if (!eightbtenbDecode (&decoder, data.buf, len)) {
		/* codeviolation */
		PyErr_SetString (codeviolation, "Invalid encoded input");
		free (decoded);
		PyBuffer_Release (&data);
		return NULL;
	}
	PyObject * const res = PyBytes_FromStringAndSize (decoded, len/10);

	/* res is a copy of decoded now */
	free (decoded);
	PyBuffer_Release (&data);

    return res;
}

static PyObject *moduleEncode (PyObject *self, PyObject *args) {
	/* encoded data */
    Py_buffer data;
	eightbtenbCtx encoder;
	eightbtenbInit (&encoder);

    if (!PyArg_ParseTuple (args, "y*", &data)) {
        return NULL;
	}
	assert (PyBuffer_IsContiguous (&data, 'C'));

	/* XXX: makes implementation easier */
	assert (data.len*10%8 == 0);
	const Py_ssize_t encodedLen = data.len*10/8;

	/* string must be zeroed */
	char * const encoded = calloc (encodedLen, sizeof (*encoded));
	eightbtenbSetDest (&encoder, encoded);
	eightbtenbEncode (&encoder, data.buf, data.len);
	PyObject * const res = PyBytes_FromStringAndSize (encoded, encodedLen);

	/* res is a copy of decoded now */
	free (encoded);
	PyBuffer_Release (&data);

    return res;
}

static PyMethodDef methods[] = {
    {"decode",  moduleDecode, METH_VARARGS, "Decode 8b10b encoded bytes."},
    {"encode",  moduleEncode, METH_VARARGS, "Encode bytes with 8b10b."},
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef module = {
   PyModuleDef_HEAD_INIT,
   "ceightbtenb",   /* name of module */
   NULL, /* module documentation, may be NULL */
   -1,       /* size of per-interpreter state of the module,
                or -1 if the module keeps state in global variables. */
   methods,
};

PyMODINIT_FUNC PyInit_ceightbtenb () {
	PyObject *m;

    if ((m = PyModule_Create (&module)) == NULL) {
		return m;
	}

	/* add decoder exceptions */
	truncated = PyErr_NewException("ceightbtenb.Truncated", NULL, NULL);
    Py_INCREF(truncated);
    PyModule_AddObject (m, "Truncated", truncated);

	codeviolation = PyErr_NewException("ceightbtenb.CodeViolation", NULL, NULL);
    Py_INCREF(codeviolation);
    PyModule_AddObject (m, "CodeViolation", codeviolation);

	return m;
}

