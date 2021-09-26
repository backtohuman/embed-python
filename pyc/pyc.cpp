#include <iostream>
#include <filesystem>
#include <string>

#include <thread>
#include <chrono>

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>
#pragma comment(lib, "python39.lib")


__inline std::string py_obj_to_str(PyObject* pyobj)
{
	std::string s;

	// Return value: New reference. Returns the string representation on success, NULL on failure.
	PyObject* str = PyObject_Str(pyobj);
	if (str)
	{
		// Return value: New reference. Return NULL if an exception was raised by the codec.
		PyObject* bytes = PyUnicode_AsEncodedString(str, "utf-8", "~E~");
		if (bytes)
		{
			const char* must_not_be_modified = PyBytes_AS_STRING(bytes);
			s = must_not_be_modified;
			Py_DECREF(bytes);
		}
		Py_DECREF(str);
	}
	return s;
}


struct Py_Mob
{
	int hp, level;
	char* name;
};
struct Private_Py_Mob
{
	PyObject_HEAD;

	/* Type-specific fields go here. */
	struct Py_Mob base;
};

static void Py_mob_dealloc(Private_Py_Mob* self)
{
	if (self->base.name)
		free(self->base.name);
	Py_TYPE(self)->tp_free((PyObject*)self);
}
static PyObject* Py_mob_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	Private_Py_Mob* self = (Private_Py_Mob*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		self->base.name = nullptr;
		self->base.hp = 0;
		self->base.level = 0;
	}
	return (PyObject*)self;
}
static int Py_mob_init(Private_Py_Mob* self, PyObject* args, PyObject* kwds)
{
	const char* name = NULL;
	static const char* kwlist[] = { "hp", "level", "name", NULL };
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "ii|s", const_cast<char**>(kwlist), 
		&self->base.hp, &self->base.level, &name))
	{
		return -1;
	}

	if (name)
	{
		// is this right idea tho?
		const auto len = strlen(name);
		self->base.name = (char*)malloc(len + 1);
		strcpy_s(self->base.name, len + 1, name);
	}

	// Returns 0 on success, -1 and sets an exception on error.
	return 0;
}
static PyObject* Py_mob_tp_str(Private_Py_Mob* self)
{
	const char* name = self->base.name;
	if (!name)
		name = "noname";
	return PyUnicode_FromFormat("name:%s, level:%d, hp:%d", name, self->base.level, self->base.hp);
}

static PyMemberDef Py_mob_members[] = {
	{ "hp",			T_INT,	offsetof(Private_Py_Mob, base.hp),			0, "hp" },
	{ "level",		T_INT,	offsetof(Private_Py_Mob, base.level),		0, "level" },
	{ "name",		T_STRING,	offsetof(Private_Py_Mob, base.name),	0, "name" },
	{ NULL }  /* Sentinel */
};
static PyTypeObject Py_mob_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"mob.mob",       /* tp_name */
	sizeof(Private_Py_Mob),     /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)Py_mob_dealloc, /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_reserved */
	0,                         /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash  */
	0,                         /* tp_call */
	(reprfunc)Py_mob_tp_str,   /* tp_str */
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,	       /* tp_flags */
	"mob",            /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	0,                         /* tp_methods */
	Py_mob_members,	/* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)Py_mob_init,     /* tp_init */
	0,                         /* tp_alloc */
	Py_mob_new,                         /* tp_new */
};

static PyObject* Py_mob_func(PyObject* self, PyObject* args)
{
	long l = 0;

	PySys_WriteStdout("mymod.func called\n");

	Py_BEGIN_ALLOW_THREADS;

	// something heavy happening
	l = 1;
	std::this_thread::sleep_for(std::chrono::seconds(1));

	Py_END_ALLOW_THREADS;

	return PyLong_FromLong(l);
}
static PyObject* Py_mob_callback(PyObject* self, PyObject* arg)
{
	// Methods with a single object argument can be listed with the METH_O flag, instead of invoking PyArg_ParseTuple() with a "O" argument. 
	// They have the type PyCFunction, with the self parameter, and a PyObject* parameter representing the single argument.
	if (!PyCallable_Check(arg))
		return NULL;

	PyObject* pyTuple = PyTuple_New(2);
	if (pyTuple)
	{
		// PyTuple_SetItem steals reference
		PyTuple_SetItem(pyTuple, 0, PyLong_FromLong(0));
		PyTuple_SetItem(pyTuple, 1, PyUnicode_FromString("string"));

		// Return value: New reference.
		PyObject* pyResult = PyObject_CallObject(arg, pyTuple);
		Py_DECREF(pyTuple);
		if (pyResult && PyBool_Check(pyResult) && PyObject_IsTrue(pyResult))
		{
			PySys_WriteStdout("%s returned true\n", py_obj_to_str(arg).c_str());
		}
		else
		{
			PySys_WriteStdout("callback failed for whatever reason.\n");
		}
		Py_XDECREF(pyResult);
	}

	/* Macro for returning Py_None from a function */
	Py_RETURN_NONE;
}
static PyObject* PyInit_mob(void)
{
	static struct PyMethodDef methods[] = {
		{ "func",	Py_mob_func, METH_NOARGS, "whatever" },
		{ "callback",	Py_mob_callback, METH_O, "whatever" },
		{ NULL, NULL, 0, NULL }
	};

	static struct PyModuleDef modDef = {
		PyModuleDef_HEAD_INIT, "mob", NULL, -1, methods,
		NULL, NULL, NULL, NULL
	};

	// Finalize a type object. This should be called on all type objects to finish their initialization. 
	// This function is responsible for adding inherited slots from a type’s base class. 
	// Return 0 on success, or return -1 and sets an exception on error.
	if (PyType_Ready(&Py_mob_type) < 0)
		return NULL;

	PyObject* m = PyModule_Create(&modDef);
	if (!m)
		return NULL;

	Py_INCREF(&Py_mob_type);
	PyModule_AddObject(m, "mob", (PyObject*)&Py_mob_type);
	return m;
}


static PyThreadState* py_init(const std::filesystem::path& embed_path, const std::filesystem::path& workingdir)
{
	// This function should be called before Py_Initialize()
	Py_SetStandardStreamEncoding("utf-8", nullptr);

	std::filesystem::path zip_path = embed_path;
	zip_path /= "python39.zip";

	// Set the default module search path. If this function is called before Py_Initialize(), 
	// then Py_GetPath() won’t attempt to compute a default search path but uses the one provided instead.
	std::wstring path = embed_path;
	path += L";";
	path += zip_path;
	path += L";";
	path += workingdir;
	Py_SetPath(path.c_str());

	// This should be called before Py_Initialize().
	PyImport_AppendInittab("mob", PyInit_mob);

	// Initialize the Python interpreter. In an application embedding Python, 
	// this should be called before using any other Python/C API functions; 
	// with the exception of Py_SetProgramName(), Py_SetPythonHome() and Py_SetPath().
	Py_Initialize();

	// GIL を初期化し、獲得します。
	// バージョン 3.2 で変更: この関数は Py_Initialize() より前に呼び出すことができなくなりました。
#if (PY_MAJOR_VERSION < 3 || (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION < 2))
	PyEval_InitThreads();
#endif

	// Release the global interpreter lock (if it has been created) and reset the thread state to NULL
	return PyEval_SaveThread();
}

static void py_finalize(PyThreadState* tstate)
{
	// GIL を獲得して、現在のスレッド状態を tstate に設定します。
	PyEval_RestoreThread(tstate);

	// GIL解放
	//PyEval_ReleaseThread(tstate);

	// Undo all initializations made by Py_Initialize() and subsequent use of Python/C API functions, 
	// and destroy all sub-interpreters (see Py_NewInterpreter() below) that were created and not yet destroyed since the last call to Py_Initialize().
	Py_Finalize();
}

static void py_run(PyThreadState* tstate, const std::filesystem::path& file, bool t)
{
	// GIL を獲得し、現在のスレッド状態を tstate に設定します。
	PyEval_RestoreThread(tstate);

	/*
	Note On Windows, fp should be opened as binary mode (e.g. fopen(filename, "rb").
	Otherwise, Python may not handle script file with LF line ending correctly.
	*/
	FILE* fp = _Py_fopen(file.generic_string().c_str(), "rb");
	if (fp)
	{
		const int closeit = 1;
		if (PyRun_SimpleFileEx(fp, file.filename().generic_string().c_str(), closeit) < 0)
		{
			// doesnt work?
		}
		// flush?
	}

	// GIL解放
	PyEval_ReleaseThread(tstate);
}

static void py_run(PyThreadState* tstate, const std::string& str)
{
	// GIL を獲得し、現在のスレッド状態を tstate に設定します。
	PyEval_RestoreThread(tstate);

	if (PyRun_SimpleString(str.c_str()) < 0)
	{
		// doesnt work?
	}

	// GIL解放
	PyEval_ReleaseThread(tstate);
}

static void py_func()
{
}


int main(int argc, char* argv[])
{
	namespace fs = std::filesystem;

	const fs::path workingdir = fs::path(argv[0]).parent_path();
	fs::path embed_path = workingdir;
	embed_path /= "python-3.9.7-embed-amd64";

	auto tstate = py_init(embed_path, workingdir);

	py_run(tstate, R"(
import mob

def py_callback(n, s):
	print('py_callback gets called')
	return True

def somefunc():
	print('somefunc yoooooo')
	return True

result = mob.func()
print(result)

lmob = mob.mob(3, 3)
print(lmob)

mob.callback(py_callback)
)");

	// call from c
	PyEval_RestoreThread(tstate);

	// Return value: New reference.
	PyObject* mainstr = PyUnicode_FromString("__main__");
	if (mainstr)
	{
		// Return value: New reference.
		PyObject* main = PyImport_GetModule(mainstr);

		// Return value: New reference.
		PyObject* pFunc = PyObject_GetAttrString(main, "somefunc");
		if (pFunc && PyCallable_Check(pFunc))
		{
			PyObject* pValue = PyObject_CallNoArgs(pFunc);
			Py_XDECREF(pValue);
		}
		Py_XDECREF(pFunc);
		Py_DECREF(main);
		Py_DECREF(mainstr);
	}
	PyEval_ReleaseThread(tstate);

	py_finalize(tstate);
	std::cout << "final\n";

	return 0;
}