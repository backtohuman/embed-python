#include "pch.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>
//#pragma comment(lib, "python39.lib")

#include <zlib.h>
#pragma comment(lib, "zlibstat.lib")

static PyObject* s_inflate_error;
static PyObject* decompress(PyObject* self, PyObject* arg)
{
	int err = 0;
	z_stream strm;
	memset(&strm, 0, sizeof(z_stream));
	if (inflateInit(&strm) != Z_OK)
		return NULL;

	// input
	Py_buffer ibuf = { 0 };
	if (PyObject_GetBuffer(arg, &ibuf, 0) != 0)
	{
		inflateEnd(&strm);
		return NULL;
	}
	strm.next_in = (z_const Bytef*)ibuf.buf;
	strm.avail_in = ibuf.len;

	// output
	Py_ssize_t offset = 0;
	Py_ssize_t obuflen = 0x400000;
	PyObject* obuf = PyBytes_FromStringAndSize(0, obuflen);
	do
	{
		strm.next_out = (Byte*)PyBytes_AS_STRING(obuf) + offset;
		strm.avail_out = obuflen - offset;

		Py_BEGIN_ALLOW_THREADS;
		err = inflate(&strm, Z_FINISH);
		Py_END_ALLOW_THREADS;

		// reallocate
		if (err == Z_BUF_ERROR)
		{
			obuflen *= 2;
			if (_PyBytes_Resize(&obuf, obuflen) < 0)
				break;
		}

		offset += strm.total_out;
	} while (err == Z_OK || err == Z_BUF_ERROR);

	if (err != Z_STREAM_END || _PyBytes_Resize(&obuf, strm.total_out) < 0)
	{
		PyErr_Format(s_inflate_error, "Error %d while decompressing data", err);
		Py_XDECREF(obuf);
	}

	PyBuffer_Release(&ibuf);
	inflateEnd(&strm);
	return obuf;
}

PyMODINIT_FUNC
PyInit_pymodule(void)
{
	static struct PyMethodDef methods[] = {
		{ "decompress", decompress, METH_O, "" },
		{ NULL, NULL, 0, NULL }
	};

	static struct PyModuleDef modDef = {
		PyModuleDef_HEAD_INIT,
		"inflate",
		NULL,
		-1,
		methods,
		NULL, NULL, NULL, NULL
	};

	PyObject* m = PyModule_Create(&modDef);
	if (!m)
		return NULL;

	s_inflate_error = PyErr_NewException("inflate.error", NULL, NULL);
	if (s_inflate_error)
	{
		Py_INCREF(s_inflate_error);
		PyModule_AddObject(m, "error", s_inflate_error);
	}
	return m;
}