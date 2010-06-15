/*
 * Copyright (c) 2005-2010 Slide, Inc
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of the author nor the names of other
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <Python.h>

#define INIT_BUFFER_LEN 0x1000
#define DEFAULT_MAX_DEPTH 0x10

#if PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION == 4
typedef int Py_ssize_t ;
#endif

static char info_doc[] =
"This module provides functions to assist in memory usage investigation\n\
";

static char obj_doc[] =
"obj(x) -> (int)\n\n\
Return information about the memory usage of the given object.\n\
The resulting tuple is dependent on the type of object examined.\n\n\
dict - five item tuple. (bytes in structure, bytes per entry, number\n\
       of active entries, (length) number of allocated entries,\n\
       boolean representing whether the allocated entries are\n\
       accounted for in the first element (bytes in structure))\n\
";

static char obj_tree[] =
"tree(x) -> int\n\n\
Return the (estimated) memory usage of the given object tree.\n\
Note:\n\
    Only the memory usage of the dict/list/tuple components of\n\
    the tree are counted.\n\
    The top level of the tree must be a tuple.\n\
";

static long mem_tree(PyObject *, long *, int);

static long _mem_dict(PyDictObject *input, long *size, int depth) {
	PyObject *value;
	PyObject *key;
	Py_ssize_t i = 0;
	int result = 0;

	*size += sizeof(PyDictObject);

	if (input->ma_table != input->ma_smalltable)
		*size += sizeof(PyDictEntry) * (input->ma_mask + 1);

	while (PyDict_Next((PyObject *)input, &i, &key, &value)) {
		result = mem_tree(key, size, depth - 1);
		if (result)
			return result;
		result = mem_tree(value, size, depth - 1);
		if (result)
			return result;
	}

	return result;
}

static long _mem_list(PyListObject *input, long *size, int depth) {
	int result = 0;
	int i;

	*size += sizeof(PyListObject);
	*size += sizeof(PyObject *) * input->allocated;

	for (i = 0; i < PyList_GET_SIZE(input); i++) {
		result = mem_tree(PyList_GET_ITEM(input, i), size, depth - 1);
		if (result)
			return result;
	}

	return result;
}

static long _mem_tuple(PyTupleObject *input, long *size, int depth) {
	int result = 0;
	int i;

	*size += sizeof(PyTupleObject);
	*size += sizeof(PyObject *) * input->ob_size;

	for (i = 0; i < PyTuple_GET_SIZE(input); i++) {
		result = mem_tree(PyTuple_GET_ITEM(input, i), size, depth - 1);
		if (result)
			return result;
	}

	return result;
}

static long mem_tree(PyObject *input, long *size, int depth) {
	if (!depth)
		return -1;

	if (PyList_Check(input))
		return _mem_list((PyListObject *)input, size, depth);

	if (PyDict_Check(input))
		return _mem_dict((PyDictObject *)input, size, depth);

	if (PyTuple_Check(input))
		return _mem_tuple((PyTupleObject *)input, size, depth);

	return 0;
}


static PyObject *obj_dict(PyDictObject *input) {
	PyObject *output;
	PyObject *value;
	int       result;

	output = PyTuple_New(5);
	if (!output)
		return NULL;

	/* 0 */
	value = PyInt_FromLong(sizeof(PyDictObject));
	if (!value)
		goto error;
		
	result = PyTuple_SetItem(output, 0, value);
	if (result)
		goto error;
	

	/* 1 */
	value = PyInt_FromLong(sizeof(PyDictEntry));
	if (!value)
		goto error;
		
	result = PyTuple_SetItem(output, 1, value);
	if (result)
		goto error;
	
	/* 2 */
	value = PyInt_FromLong(input->ma_used);
	if (!value)
		goto error;
		
	result = PyTuple_SetItem(output, 2, value);
	if (result)
		goto error;
	
	/* 3 */
	value = PyInt_FromLong(input->ma_mask);
	if (!value)
		goto error;
		
	result = PyTuple_SetItem(output, 3, value);
	if (result)
		goto error;
	
	/* 4 */
	if (input->ma_table == input->ma_smalltable)
		value = Py_True;
	else
		value = Py_False;

	Py_INCREF(value);

	result = PyTuple_SetItem(output, 4, value);
	if (result)
		goto error;
	
	/* done */
	return output;
error:
	Py_XDECREF(output);
	return NULL;
}

static PyObject *obj_list(PyListObject *input) {
	PyObject *output;
	PyObject *value;
	int       result;

	output = PyTuple_New(4);
	if (!output)
		return NULL;

	/* 0 */
	value = PyInt_FromLong(sizeof(PyListObject));
	if (!value)
		goto error;
		
	result = PyTuple_SetItem(output, 0, value);
	if (result)
		goto error;
	
	/* 1 */
	value = PyInt_FromLong(sizeof(PyObject *));
	if (!value)
		goto error;
		
	result = PyTuple_SetItem(output, 1, value);
	if (result)
		goto error;
	
	/* 2 */
	value = PyInt_FromLong(input->ob_size);
	if (!value)
		goto error;
		
	result = PyTuple_SetItem(output, 2, value);
	if (result)
		goto error;
	
	/* 3 */
	value = PyInt_FromLong(input->allocated);
	if (!value)
		goto error;
		
	result = PyTuple_SetItem(output, 3, value);
	if (result)
		goto error;
	
	/* done */
	return output;
error:
	Py_XDECREF(output);
	return NULL;
}

static PyObject *obj_tuple(PyTupleObject *input) {
	PyObject *output;
	PyObject *value;
	int       result;

	output = PyTuple_New(4);
	if (!output)
		return NULL;

	/* 0 */
	value = PyInt_FromLong(sizeof(PyTupleObject));
	if (!value)
		goto error;
		
	result = PyTuple_SetItem(output, 0, value);
	if (result)
		goto error;
	
	/* 1 */
	value = PyInt_FromLong(sizeof(PyObject *));
	if (!value)
		goto error;
		
	result = PyTuple_SetItem(output, 1, value);
	if (result)
		goto error;
	
	/* 2 */
	value = PyInt_FromLong(input->ob_size);
	if (!value)
		goto error;
		
	result = PyTuple_SetItem(output, 2, value);
	if (result)
		goto error;
	
	/* 3 */
	Py_INCREF(value);

	result = PyTuple_SetItem(output, 3, value);
	if (result)
		goto error;
	
	/* done */
	return output;
error:
	Py_XDECREF(output);
	return NULL;
}

static PyObject *py_obj(PyObject *self, PyObject *args)
{
	PyObject *input;
	int result;

	result = PyArg_ParseTuple(args, "O", &input);
	if (!result)
		return NULL;

	if (PyDict_Check(input))
		return obj_dict((PyDictObject *)input);
	if (PyList_Check(input))
		return obj_list((PyListObject *)input);
	if (PyTuple_Check(input))
		return obj_tuple((PyTupleObject *)input);

	PyErr_Format(PyExc_TypeError,
		     "Unhandled type: <%s>", 
		     input->ob_type->tp_name);
	return NULL;
}

static PyObject *py_tree(PyObject *self, PyObject *args)
{
	PyTupleObject *input;
	long value = 0;
	int depth = DEFAULT_MAX_DEPTH;
	int result;
	int i;

	result = PyArg_ParseTuple(args, "O!i", &PyTuple_Type, &input, &depth);
	if (!result)
		return NULL;

	for (i = 0; i < PyTuple_GET_SIZE(input); i++) {
		result = mem_tree(PyTuple_GET_ITEM(input, i), &value, depth);
		if (result) {
			PyErr_Format(PyExc_RuntimeError,
				     "Traversal depth exceeded: <%d>", 
				     depth);
			return NULL;
		}
	}

	return PyInt_FromLong(value);
}


static PyMethodDef _meminfo_methods[] = {
	{"obj",  py_obj,  METH_VARARGS, obj_doc},
	{"tree", py_tree, METH_VARARGS, obj_tree},
	{NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC init_meminfo(void)
{
	(void)Py_InitModule3("_meminfo", _meminfo_methods, info_doc);
}
/*
 * Local Variables:
 * c-file-style: "linux"
 * indent-tabs-mode: t
 * End:
 */
