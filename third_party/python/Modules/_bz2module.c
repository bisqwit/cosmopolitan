/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:4;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=4 sts=4 sw=4 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Python 3                                                                     │
│ https://docs.python.org/3/license.html                                       │
╚─────────────────────────────────────────────────────────────────────────────*/
#define PY_SSIZE_T_CLEAN
#include "third_party/bzip2/bzlib.h"
#include "third_party/python/Include/abstract.h"
#include "third_party/python/Include/bytesobject.h"
#include "third_party/python/Include/ceval.h"
#include "third_party/python/Include/import.h"
#include "third_party/python/Include/modsupport.h"
#include "third_party/python/Include/object.h"
#include "third_party/python/Include/pyerrors.h"
#include "third_party/python/Include/pymacro.h"
#include "third_party/python/Include/pymem.h"
#include "third_party/python/Include/pythread.h"
#include "third_party/python/Include/structmember.h"
#include "third_party/python/Include/yoink.h"
/* clang-format off */

PYTHON_PROVIDE("_bz2");
PYTHON_PROVIDE("_bz2.BZ2Compressor");
PYTHON_PROVIDE("_bz2.BZ2Decompressor");

#ifdef WITH_THREAD
#define ACQUIRE_LOCK(obj) do { \
    if (!PyThread_acquire_lock((obj)->lock, 0)) { \
        Py_BEGIN_ALLOW_THREADS \
        PyThread_acquire_lock((obj)->lock, 1); \
        Py_END_ALLOW_THREADS \
    } } while (0)
#define RELEASE_LOCK(obj) PyThread_release_lock((obj)->lock)
#else
#define ACQUIRE_LOCK(obj)
#define RELEASE_LOCK(obj)
#endif

typedef struct {
    PyObject_HEAD
    bz_stream bzs;
    int flushed;
#ifdef WITH_THREAD
    PyThread_type_lock lock;
#endif
} BZ2Compressor;

typedef struct {
    PyObject_HEAD
    bz_stream bzs;
    char eof;           /* T_BOOL expects a char */
    PyObject *unused_data;
    char needs_input;
    char *input_buffer;
    size_t input_buffer_size;

    /* bzs->avail_in is only 32 bit, so we store the true length
       separately. Conversion and looping is encapsulated in
       decompress_buf() */
    size_t bzs_avail_in_real;
#ifdef WITH_THREAD
    PyThread_type_lock lock;
#endif
} BZ2Decompressor;

static PyTypeObject BZ2Compressor_Type;
static PyTypeObject BZ2Decompressor_Type;

/* Helper functions. */

static int
catch_bz2_error(int bzerror)
{
    switch(bzerror) {
        case BZ_OK:
        case BZ_RUN_OK:
        case BZ_FLUSH_OK:
        case BZ_FINISH_OK:
        case BZ_STREAM_END:
            return 0;
#ifdef BZ_CONFIG_ERROR
        case BZ_CONFIG_ERROR:
            PyErr_SetString(PyExc_SystemError,
                            "libbzip2 was not compiled correctly");
            return 1;
#endif
        case BZ_PARAM_ERROR:
            PyErr_SetString(PyExc_ValueError,
                            "Internal error - "
                            "invalid parameters passed to libbzip2");
            return 1;
        case BZ_MEM_ERROR:
            PyErr_NoMemory();
            return 1;
        case BZ_DATA_ERROR:
        case BZ_DATA_ERROR_MAGIC:
            PyErr_SetString(PyExc_IOError, "Invalid data stream");
            return 1;
        case BZ_IO_ERROR:
            PyErr_SetString(PyExc_IOError, "Unknown I/O error");
            return 1;
        case BZ_UNEXPECTED_EOF:
            PyErr_SetString(PyExc_EOFError,
                            "Compressed file ended before the logical "
                            "end-of-stream was detected");
            return 1;
        case BZ_SEQUENCE_ERROR:
            PyErr_SetString(PyExc_RuntimeError,
                            "Internal error - "
                            "Invalid sequence of commands sent to libbzip2");
            return 1;
        default:
            PyErr_Format(PyExc_IOError,
                         "Unrecognized error from libbzip2: %d", bzerror);
            return 1;
    }
}

#if BUFSIZ < 8192
#define INITIAL_BUFFER_SIZE 8192
#else
#define INITIAL_BUFFER_SIZE BUFSIZ
#endif

static int
grow_buffer(PyObject **buf, Py_ssize_t max_length)
{
    /* Expand the buffer by an amount proportional to the current size,
       giving us amortized linear-time behavior. Use a less-than-double
       growth factor to avoid excessive allocation. */
    size_t size = PyBytes_GET_SIZE(*buf);
    size_t new_size = size + (size >> 3) + 6;

    if (max_length > 0 && new_size > (size_t) max_length)
        new_size = (size_t) max_length;

    if (new_size > size) {
        return _PyBytes_Resize(buf, new_size);
    } else {  /* overflow */
        PyErr_SetString(PyExc_OverflowError,
                        "Unable to allocate buffer - output too large");
        return -1;
    }
}


/* BZ2Compressor class. */

static PyObject *
bz2_compress(BZ2Compressor *c, char *data, size_t len, int action)
{
    size_t data_size = 0;
    PyObject *result;

    result = PyBytes_FromStringAndSize(NULL, INITIAL_BUFFER_SIZE);
    if (result == NULL)
        return NULL;

    c->bzs.next_in = data;
    c->bzs.avail_in = 0;
    c->bzs.next_out = PyBytes_AS_STRING(result);
    c->bzs.avail_out = INITIAL_BUFFER_SIZE;
    for (;;) {
        char *this_out;
        int bzerror;

        /* On a 64-bit system, len might not fit in avail_in (an unsigned int).
           Do compression in chunks of no more than UINT_MAX bytes each. */
        if (c->bzs.avail_in == 0 && len > 0) {
            c->bzs.avail_in = (unsigned int)Py_MIN(len, UINT_MAX);
            len -= c->bzs.avail_in;
        }

        /* In regular compression mode, stop when input data is exhausted. */
        if (action == BZ_RUN && c->bzs.avail_in == 0)
            break;

        if (c->bzs.avail_out == 0) {
            size_t buffer_left = PyBytes_GET_SIZE(result) - data_size;
            if (buffer_left == 0) {
                if (grow_buffer(&result, -1) < 0)
                    goto error;
                c->bzs.next_out = PyBytes_AS_STRING(result) + data_size;
                buffer_left = PyBytes_GET_SIZE(result) - data_size;
            }
            c->bzs.avail_out = (unsigned int)Py_MIN(buffer_left, UINT_MAX);
        }

        Py_BEGIN_ALLOW_THREADS
        this_out = c->bzs.next_out;
        bzerror = BZ2_bzCompress(&c->bzs, action);
        data_size += c->bzs.next_out - this_out;
        Py_END_ALLOW_THREADS
        if (catch_bz2_error(bzerror))
            goto error;

        /* In flushing mode, stop when all buffered data has been flushed. */
        if (action == BZ_FINISH && bzerror == BZ_STREAM_END)
            break;
    }
    if (data_size != (size_t)PyBytes_GET_SIZE(result))
        if (_PyBytes_Resize(&result, data_size) < 0)
            goto error;
    return result;

error:
    Py_XDECREF(result);
    return NULL;
}

/*[clinic input]
module _bz2
class _bz2.BZ2Compressor "BZ2Compressor *" "&BZ2Compressor_Type"
class _bz2.BZ2Decompressor "BZ2Decompressor *" "&BZ2Decompressor_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=dc7d7992a79f9cb7]*/

#include "third_party/python/Modules/clinic/_bz2module.inc"

/*[clinic input]
_bz2.BZ2Compressor.compress

    data: Py_buffer
    /

Provide data to the compressor object.

Returns a chunk of compressed data if possible, or b'' otherwise.

When you have finished providing data to the compressor, call the
flush() method to finish the compression process.
[clinic start generated code]*/

static PyObject *
_bz2_BZ2Compressor_compress_impl(BZ2Compressor *self, Py_buffer *data)
/*[clinic end generated code: output=59365426e941fbcc input=85c963218070fc4c]*/
{
    PyObject *result = NULL;

    ACQUIRE_LOCK(self);
    if (self->flushed)
        PyErr_SetString(PyExc_ValueError, "Compressor has been flushed");
    else
        result = bz2_compress(self, data->buf, data->len, BZ_RUN);
    RELEASE_LOCK(self);
    return result;
}

/*[clinic input]
_bz2.BZ2Compressor.flush

Finish the compression process.

Returns the compressed data left in internal buffers.

The compressor object may not be used after this method is called.
[clinic start generated code]*/

static PyObject *
_bz2_BZ2Compressor_flush_impl(BZ2Compressor *self)
/*[clinic end generated code: output=3ef03fc1b092a701 input=d64405d3c6f76691]*/
{
    PyObject *result = NULL;

    ACQUIRE_LOCK(self);
    if (self->flushed)
        PyErr_SetString(PyExc_ValueError, "Repeated call to flush()");
    else {
        self->flushed = 1;
        result = bz2_compress(self, NULL, 0, BZ_FINISH);
    }
    RELEASE_LOCK(self);
    return result;
}

static PyObject *
BZ2Compressor_getstate(BZ2Compressor *self, PyObject *noargs)
{
    PyErr_Format(PyExc_TypeError, "cannot serialize '%s' object",
                 Py_TYPE(self)->tp_name);
    return NULL;
}

static void*
BZ2_Malloc(void* ctx, int items, int size)
{
    if (items < 0 || size < 0)
        return NULL;
    if (size != 0 && (size_t)items > (size_t)PY_SSIZE_T_MAX / (size_t)size)
        return NULL;
    /* PyMem_Malloc() cannot be used: compress() and decompress()
       release the GIL */
    return PyMem_RawMalloc((size_t)items * (size_t)size);
}

static void
BZ2_Free(void* ctx, void *ptr)
{
    PyMem_RawFree(ptr);
}

/*[clinic input]
_bz2.BZ2Compressor.__init__

    compresslevel: int = 9
        Compression level, as a number between 1 and 9.
    /

Create a compressor object for compressing data incrementally.

For one-shot compression, use the compress() function instead.
[clinic start generated code]*/

static int
_bz2_BZ2Compressor___init___impl(BZ2Compressor *self, int compresslevel)
/*[clinic end generated code: output=c4e6adfd02963827 input=4e1ff7b8394b6e9a]*/
{
    int bzerror;

    if (!(1 <= compresslevel && compresslevel <= 9)) {
        PyErr_SetString(PyExc_ValueError,
                        "compresslevel must be between 1 and 9");
        return -1;
    }

#ifdef WITH_THREAD
    self->lock = PyThread_allocate_lock();
    if (self->lock == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate lock");
        return -1;
    }
#endif

    self->bzs.opaque = NULL;
    self->bzs.bzalloc = BZ2_Malloc;
    self->bzs.bzfree = BZ2_Free;
    bzerror = BZ2_bzCompressInit(&self->bzs, compresslevel, 0, 0);
    if (catch_bz2_error(bzerror))
        goto error;

    return 0;

error:
#ifdef WITH_THREAD
    PyThread_free_lock(self->lock);
    self->lock = NULL;
#endif
    return -1;
}

static void
BZ2Compressor_dealloc(BZ2Compressor *self)
{
    BZ2_bzCompressEnd(&self->bzs);
#ifdef WITH_THREAD
    if (self->lock != NULL)
        PyThread_free_lock(self->lock);
#endif
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyMethodDef BZ2Compressor_methods[] = {
    _BZ2_BZ2COMPRESSOR_COMPRESS_METHODDEF
    _BZ2_BZ2COMPRESSOR_FLUSH_METHODDEF
    {"__getstate__", (PyCFunction)BZ2Compressor_getstate, METH_NOARGS},
    {NULL}
};


static PyTypeObject BZ2Compressor_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_bz2.BZ2Compressor",               /* tp_name */
    sizeof(BZ2Compressor),              /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor)BZ2Compressor_dealloc,  /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash  */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    _bz2_BZ2Compressor___init____doc__,  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    BZ2Compressor_methods,              /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    _bz2_BZ2Compressor___init__,        /* tp_init */
    0,                                  /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
};


/* BZ2Decompressor class. */

/* Decompress data of length d->bzs_avail_in_real in d->bzs.next_in.  The output
   buffer is allocated dynamically and returned.  At most max_length bytes are
   returned, so some of the input may not be consumed. d->bzs.next_in and
   d->bzs_avail_in_real are updated to reflect the consumed input. */
static PyObject*
decompress_buf(BZ2Decompressor *d, Py_ssize_t max_length)
{
    /* data_size is strictly positive, but because we repeatedly have to
       compare against max_length and PyBytes_GET_SIZE we declare it as
       signed */
    Py_ssize_t data_size = 0;
    PyObject *result;
    bz_stream *bzs = &d->bzs;

    if (max_length < 0 || max_length >= INITIAL_BUFFER_SIZE)
        result = PyBytes_FromStringAndSize(NULL, INITIAL_BUFFER_SIZE);
    else
        result = PyBytes_FromStringAndSize(NULL, max_length);
    if (result == NULL)
        return NULL;

    bzs->next_out = PyBytes_AS_STRING(result);
    for (;;) {
        int bzret;
        size_t avail;

        /* On a 64-bit system, buffer length might not fit in avail_out, so we
           do decompression in chunks of no more than UINT_MAX bytes
           each. Note that the expression for `avail` is guaranteed to be
           positive, so the cast is safe. */
        avail = (size_t) (PyBytes_GET_SIZE(result) - data_size);
        bzs->avail_out = (unsigned int)Py_MIN(avail, UINT_MAX);
        bzs->avail_in = (unsigned int)Py_MIN(d->bzs_avail_in_real, UINT_MAX);
        d->bzs_avail_in_real -= bzs->avail_in;

        Py_BEGIN_ALLOW_THREADS
        bzret = BZ2_bzDecompress(bzs);
        data_size = bzs->next_out - PyBytes_AS_STRING(result);
        d->bzs_avail_in_real += bzs->avail_in;
        Py_END_ALLOW_THREADS
        if (catch_bz2_error(bzret))
            goto error;
        if (bzret == BZ_STREAM_END) {
            d->eof = 1;
            break;
        } else if (d->bzs_avail_in_real == 0) {
            break;
        } else if (bzs->avail_out == 0) {
            if (data_size == max_length)
                break;
            if (data_size == PyBytes_GET_SIZE(result) &&
                grow_buffer(&result, max_length) == -1)
                goto error;
            bzs->next_out = PyBytes_AS_STRING(result) + data_size;
        }
    }
    if (data_size != PyBytes_GET_SIZE(result))
        if (_PyBytes_Resize(&result, data_size) == -1)
            goto error;

    return result;

error:
    Py_XDECREF(result);
    return NULL;
}


static PyObject *
decompress(BZ2Decompressor *d, char *data, size_t len, Py_ssize_t max_length)
{
    char input_buffer_in_use;
    PyObject *result;
    bz_stream *bzs = &d->bzs;

    /* Prepend unconsumed input if necessary */
    if (bzs->next_in != NULL) {
        size_t avail_now, avail_total;

        /* Number of bytes we can append to input buffer */
        avail_now = (d->input_buffer + d->input_buffer_size)
            - (bzs->next_in + d->bzs_avail_in_real);

        /* Number of bytes we can append if we move existing
           contents to beginning of buffer (overwriting
           consumed input) */
        avail_total = d->input_buffer_size - d->bzs_avail_in_real;

        if (avail_total < len) {
            size_t offset = bzs->next_in - d->input_buffer;
            char *tmp;
            size_t new_size = d->input_buffer_size + len - avail_now;

            /* Assign to temporary variable first, so we don't
               lose address of allocated buffer if realloc fails */
            tmp = PyMem_Realloc(d->input_buffer, new_size);
            if (tmp == NULL) {
                PyErr_SetNone(PyExc_MemoryError);
                return NULL;
            }
            d->input_buffer = tmp;
            d->input_buffer_size = new_size;

            bzs->next_in = d->input_buffer + offset;
        }
        else if (avail_now < len) {
            memmove(d->input_buffer, bzs->next_in,
                    d->bzs_avail_in_real);
            bzs->next_in = d->input_buffer;
        }
        memcpy((void*)(bzs->next_in + d->bzs_avail_in_real), data, len);
        d->bzs_avail_in_real += len;
        input_buffer_in_use = 1;
    }
    else {
        bzs->next_in = data;
        d->bzs_avail_in_real = len;
        input_buffer_in_use = 0;
    }

    result = decompress_buf(d, max_length);
    if(result == NULL) {
        bzs->next_in = NULL;
        return NULL;
    }

    if (d->eof) {
        d->needs_input = 0;
        if (d->bzs_avail_in_real > 0) {
            Py_XSETREF(d->unused_data,
                      PyBytes_FromStringAndSize(bzs->next_in, d->bzs_avail_in_real));
            if (d->unused_data == NULL)
                goto error;
        }
    }
    else if (d->bzs_avail_in_real == 0) {
        bzs->next_in = NULL;
        d->needs_input = 1;
    }
    else {
        d->needs_input = 0;

        /* If we did not use the input buffer, we now have
           to copy the tail from the caller's buffer into the
           input buffer */
        if (!input_buffer_in_use) {

            /* Discard buffer if it's too small
               (resizing it may needlessly copy the current contents) */
            if (d->input_buffer != NULL &&
                d->input_buffer_size < d->bzs_avail_in_real) {
                PyMem_Free(d->input_buffer);
                d->input_buffer = NULL;
            }

            /* Allocate if necessary */
            if (d->input_buffer == NULL) {
                d->input_buffer = PyMem_Malloc(d->bzs_avail_in_real);
                if (d->input_buffer == NULL) {
                    PyErr_SetNone(PyExc_MemoryError);
                    goto error;
                }
                d->input_buffer_size = d->bzs_avail_in_real;
            }

            /* Copy tail */
            memcpy(d->input_buffer, bzs->next_in, d->bzs_avail_in_real);
            bzs->next_in = d->input_buffer;
        }
    }

    return result;

error:
    Py_XDECREF(result);
    return NULL;
}

/*[clinic input]
_bz2.BZ2Decompressor.decompress

    data: Py_buffer
    max_length: Py_ssize_t=-1

Decompress *data*, returning uncompressed data as bytes.

If *max_length* is nonnegative, returns at most *max_length* bytes of
decompressed data. If this limit is reached and further output can be
produced, *self.needs_input* will be set to ``False``. In this case, the next
call to *decompress()* may provide *data* as b'' to obtain more of the output.

If all of the input data was decompressed and returned (either because this
was less than *max_length* bytes, or because *max_length* was negative),
*self.needs_input* will be set to True.

Attempting to decompress data after the end of stream is reached raises an
EOFError.  Any data found after the end of the stream is ignored and saved in
the unused_data attribute.
[clinic start generated code]*/

static PyObject *
_bz2_BZ2Decompressor_decompress_impl(BZ2Decompressor *self, Py_buffer *data,
                                     Py_ssize_t max_length)
/*[clinic end generated code: output=23e41045deb240a3 input=52e1ffc66a8ea624]*/
{
    PyObject *result = NULL;

    ACQUIRE_LOCK(self);
    if (self->eof)
        PyErr_SetString(PyExc_EOFError, "End of stream already reached");
    else
        result = decompress(self, data->buf, data->len, max_length);
    RELEASE_LOCK(self);
    return result;
}

static PyObject *
BZ2Decompressor_getstate(BZ2Decompressor *self, PyObject *noargs)
{
    PyErr_Format(PyExc_TypeError, "cannot serialize '%s' object",
                 Py_TYPE(self)->tp_name);
    return NULL;
}

/*[clinic input]
_bz2.BZ2Decompressor.__init__

Create a decompressor object for decompressing data incrementally.

For one-shot decompression, use the decompress() function instead.
[clinic start generated code]*/

static int
_bz2_BZ2Decompressor___init___impl(BZ2Decompressor *self)
/*[clinic end generated code: output=e4d2b9bb866ab8f1 input=95f6500dcda60088]*/
{
    int bzerror;

#ifdef WITH_THREAD
    PyThread_type_lock lock = PyThread_allocate_lock();
    if (lock == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate lock");
        return -1;
    }
    if (self->lock != NULL) {
        PyThread_free_lock(self->lock);
    }
    self->lock = lock;
#endif

    self->needs_input = 1;
    self->bzs_avail_in_real = 0;
    self->input_buffer = NULL;
    self->input_buffer_size = 0;
    Py_XSETREF(self->unused_data, PyBytes_FromStringAndSize(NULL, 0));
    if (self->unused_data == NULL)
        goto error;

    bzerror = BZ2_bzDecompressInit(&self->bzs, 0, 0);
    if (catch_bz2_error(bzerror))
        goto error;

    return 0;

error:
    Py_CLEAR(self->unused_data);
#ifdef WITH_THREAD
    PyThread_free_lock(self->lock);
    self->lock = NULL;
#endif
    return -1;
}

static void
BZ2Decompressor_dealloc(BZ2Decompressor *self)
{
    if(self->input_buffer != NULL)
        PyMem_Free(self->input_buffer);
    BZ2_bzDecompressEnd(&self->bzs);
    Py_CLEAR(self->unused_data);
#ifdef WITH_THREAD
    if (self->lock != NULL)
        PyThread_free_lock(self->lock);
#endif
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyMethodDef BZ2Decompressor_methods[] = {
    _BZ2_BZ2DECOMPRESSOR_DECOMPRESS_METHODDEF
    {"__getstate__", (PyCFunction)BZ2Decompressor_getstate, METH_NOARGS},
    {NULL}
};

PyDoc_STRVAR(BZ2Decompressor_eof__doc__,
"True if the end-of-stream marker has been reached.");

PyDoc_STRVAR(BZ2Decompressor_unused_data__doc__,
"Data found after the end of the compressed stream.");

PyDoc_STRVAR(BZ2Decompressor_needs_input_doc,
"True if more input is needed before more decompressed data can be produced.");

static PyMemberDef BZ2Decompressor_members[] = {
    {"eof", T_BOOL, offsetof(BZ2Decompressor, eof),
     READONLY, BZ2Decompressor_eof__doc__},
    {"unused_data", T_OBJECT_EX, offsetof(BZ2Decompressor, unused_data),
     READONLY, BZ2Decompressor_unused_data__doc__},
    {"needs_input", T_BOOL, offsetof(BZ2Decompressor, needs_input), READONLY,
     BZ2Decompressor_needs_input_doc},
    {NULL}
};

static PyTypeObject BZ2Decompressor_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_bz2.BZ2Decompressor",             /* tp_name */
    sizeof(BZ2Decompressor),            /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor)BZ2Decompressor_dealloc,/* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash  */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    _bz2_BZ2Decompressor___init____doc__,  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    BZ2Decompressor_methods,            /* tp_methods */
    BZ2Decompressor_members,            /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    _bz2_BZ2Decompressor___init__,      /* tp_init */
    0,                                  /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
};


/* Module initialization. */

static struct PyModuleDef _bz2module = {
    PyModuleDef_HEAD_INIT,
    "_bz2",
    NULL,
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__bz2(void)
{
    PyObject *m;

    if (PyType_Ready(&BZ2Compressor_Type) < 0)
        return NULL;
    if (PyType_Ready(&BZ2Decompressor_Type) < 0)
        return NULL;

    m = PyModule_Create(&_bz2module);
    if (m == NULL)
        return NULL;

    Py_INCREF(&BZ2Compressor_Type);
    PyModule_AddObject(m, "BZ2Compressor", (PyObject *)&BZ2Compressor_Type);

    Py_INCREF(&BZ2Decompressor_Type);
    PyModule_AddObject(m, "BZ2Decompressor",
                       (PyObject *)&BZ2Decompressor_Type);

    return m;
}

#ifdef __aarch64__
_Section(".rodata.pytab.1 //")
#else
_Section(".rodata.pytab.1")
#endif
 const struct _inittab _PyImport_Inittab__bz2 = {
    "_bz2",
    PyInit__bz2,
};
