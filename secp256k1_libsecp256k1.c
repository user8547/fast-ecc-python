#define USE_NUM_GMP
//#define USE_NUM_OPENSSL

#define USE_FIELD_5X52
#define USE_FIELD_5X52_ASM
//#define USE_FIELD_5X52_INT128
//#define USE_FIELD_GMP
//#define USE_FIELD_10X26

//#define USE_FIELD_INV_BUILTIN
#define USE_FIELD_INV_NUM

#define USE_ENDOMORPHISM

#include <Python.h>

#define HAVE___INT128
#define USE_SCALAR_4X64
//#define USE_SCALAR_8X32

//#define USE_SCALAR_INV_BUILTIN
#define USE_SCALAR_INV_NUM

//#define USE_ECMULT_STATIC_PRECOMPUTATION

#include "include/secp256k1.h"
#include "secp256k1.c"

#include "util.h"
#include "num.h"

//#include "num_openssl_impl.h"
#include "num_gmp_impl.h"
#include "field.h"
#include "field_impl.h"
#include "group.h"
#include "group_impl.h"
#include "scalar.h"
#include "scalar_impl.h"

#include "ecmult.h"
#include "ecmult_impl.h"
#include "ecmult_gen.h"
#include "ecmult_gen_impl.h"
#include "eckey_impl.h"
#include "eckey_impl.h"

static secp256k1_context *ctx = NULL;
static void secp256k1_fe_get_hex(char *, const secp256k1_fe *);
static int secp256k1_fe_set_hex(secp256k1_fe *, const char *);

static PyObject* mul(PyObject* self, PyObject* args)
{

	PyObject *value, *list;
	if (!PyArg_ParseTuple(args, "OO", &list, &value)) {
		return NULL;
	}

	if (!PyInt_Check(value) && !PyLong_Check(value)) {
		PyErr_SetString(PyExc_TypeError, "expected int or long");
		return NULL;
	}

	if (!PyList_Check(list)) {
		PyErr_SetString(PyExc_TypeError, "expected list");
		return NULL;
	}

        // load point
	PyObject *valuex, *valuey;
	valuex = PyList_GetItem(list, 0);
	valuey = PyList_GetItem(list, 1);

	// check if point at infinity received
	if (valuex==Py_None) {
		return Py_BuildValue("[O,O]", Py_None,Py_None);
	}

        // convert to hex string
        PyObject* fmt = PyString_FromString("%064x");
	PyObject* pyhex = PyString_Format(fmt, value);
	char* hex = PyString_AsString(pyhex);
	PyObject* pyhexPx = PyString_Format(fmt, valuex);
	char* hexPx = PyString_AsString(pyhexPx);
	PyObject* pyhexPy = PyString_Format(fmt, valuey);
	char* hexPy = PyString_AsString(pyhexPy);

	// load into bignum type
	secp256k1_fe x, y;
	secp256k1_fe_set_hex(&x, hexPx);
	secp256k1_fe_set_hex(&y, hexPy);


	// load scalar
	char hbuf[32];
	int b;
        for (int i=0; i < 32; i++) {
                sscanf(hex+2*i, "%02x", &b);
                hbuf[i] = b;
        }
	secp256k1_scalar a;
	secp256k1_scalar_set_b32(&a, hbuf, NULL);

	Py_DECREF(fmt);
	Py_DECREF(pyhex);
	Py_DECREF(pyhexPx);
	Py_DECREF(pyhexPy);


	// load x and y to affine struct
	secp256k1_ge p;
	secp256k1_ge_set_xy(&p, &x, &y);

	secp256k1_gej r;

	// use optimization if multiplication is performed on the base point
	if (secp256k1_fe_equal_var(&secp256k1_ge_const_g.x, &x) && secp256k1_fe_equal_var(&secp256k1_ge_const_g.y, &y)) {

		// perform scalar multiplication of G
		secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &r, &a);

	} else {

		// convert point from affine to jacobian coordinates
		secp256k1_gej_set_ge(&r, &p);

		// perform scalar multiplication
		secp256k1_scalar zero;
		secp256k1_scalar_set_int(&zero, 0);
		secp256k1_ecmult(&ctx->ecmult_ctx, &r, &r, &a, &zero);
	}

	// check if the result is point at infinity
	if (secp256k1_gej_is_infinity(&r)) {
		return Py_BuildValue("[O,O]", Py_None,Py_None);
	}

	// convert result from jacobian to affine coordinates
	secp256k1_ge t;
	secp256k1_ge_set_gej(&t, &r);

	// convert x and y to hex
	char cx[65];
	char cy[65];
	secp256k1_fe_get_hex(cx, &t.x);
	secp256k1_fe_get_hex(cy, &t.y);
        cx[64] = 0;
        cy[64] = 0;

	// convert hex string of x, y to python long
	PyObject* xr = PyLong_FromString(cx, NULL, 16);
	PyObject* yr = PyLong_FromString(cy, NULL, 16);

	return Py_BuildValue("[NN]", xr, yr);
}

static PyObject* add(PyObject* self, PyObject* args)
{


	PyObject *list1, *list2;
	if (!PyArg_ParseTuple(args, "OO", &list1, &list2)) {
		return NULL;
	}

	if (!PyList_Check(list1) || !PyList_Check(list2)) {
		PyErr_SetString(PyExc_TypeError, "expected list");
		return NULL;
	}

	PyObject *oAx, *oAy;
	oAx = PyList_GetItem(list1, 0);
	oAy = PyList_GetItem(list1, 1);

	PyObject *oBx, *oBy;
	oBx = PyList_GetItem(list2, 0);
	oBy = PyList_GetItem(list2, 1);

	// check if point at infinity received
	if (oAx==Py_None) {
		return Py_BuildValue("O", list2);
	}
	if (oBx==Py_None) {
		return Py_BuildValue("O", list1);
	}


	// convert to hex string
	PyObject* fmt = PyString_FromString("%064x");
	PyObject* pyhexAx = PyString_Format(fmt, oAx);
	char* hexAx = PyString_AsString(pyhexAx);
	PyObject* pyhexAy = PyString_Format(fmt, oAy);
	char* hexAy = PyString_AsString(pyhexAy);
	PyObject* pyhexBx = PyString_Format(fmt, oBx);
	char* hexBx = PyString_AsString(pyhexBx);
	PyObject* pyhexBy = PyString_Format(fmt, oBy);
	char* hexBy = PyString_AsString(pyhexBy);

	// load into bignum type
	secp256k1_fe Ax, Ay, Bx, By;
	secp256k1_fe_set_hex(&Ax, hexAx);
	secp256k1_fe_set_hex(&Ay, hexAy);
	secp256k1_fe_set_hex(&Bx, hexBx);
	secp256k1_fe_set_hex(&By, hexBy);

	Py_DECREF(fmt);
	Py_DECREF(pyhexAx);
	Py_DECREF(pyhexAy);
	Py_DECREF(pyhexBx);
	Py_DECREF(pyhexBy);

	secp256k1_gej r;

	// load point A in jacobian struct
	secp256k1_ge a;
	secp256k1_ge_set_xy(&a, &Ax, &Ay);
	secp256k1_gej A;
	secp256k1_gej_set_ge(&A, &a);

	// load point B in affine struct
	secp256k1_ge b;
	secp256k1_ge_set_xy(&b, &Bx, &By);

	// perform addition
	secp256k1_gej_add_ge_var(&r, &A, &b, NULL);

	// check if the result is point at infinity
	if (secp256k1_gej_is_infinity(&r)) {
		return Py_BuildValue("[O,O]", Py_None,Py_None);
	}

	// convert result from jacobian to affine coordinates
	secp256k1_ge t;
	secp256k1_ge_set_gej(&t, &r);

	// convert x and y to hex
	char cx[65];
	char cy[65];
	secp256k1_fe_get_hex(cx, &t.x);
	secp256k1_fe_get_hex(cy, &t.y);
        cx[64] = 0;
        cy[64] = 0;

	// convert hex string of x, y to python long
	PyObject* xr = PyLong_FromString(cx, NULL, 16);
	PyObject* yr = PyLong_FromString(cy, NULL, 16);

	return Py_BuildValue("[NN]", xr, yr);

}


static PyObject* inv(PyObject* self, PyObject* args)
{


	PyObject *list;
	if (!PyArg_ParseTuple(args, "O", &list)) {
		return NULL;
	}

	if (!PyList_Check(list)) {
		PyErr_SetString(PyExc_TypeError, "expected list");
		return NULL;
	}

	PyObject *Px, *Py;
	Px = PyList_GetItem(list, 0);
	Py = PyList_GetItem(list, 1);

	// check if point at infinity received
	if (Px==Py_None) {
		return Py_BuildValue("[O,O]", Py_None,Py_None);
	}

	// convert to hex string
	PyObject* fmt = PyString_FromString("%064x");
	PyObject* pyhexPx = PyString_Format(fmt, Px);
	char* hexPx = PyString_AsString(pyhexPx);
	PyObject* pyhexPy = PyString_Format(fmt, Py);
	char* hexPy = PyString_AsString(pyhexPy);

	// load into bignum type
	secp256k1_fe x, y;
	secp256k1_fe_set_hex(&x, hexPx);
	secp256k1_fe_set_hex(&y, hexPy);

	Py_DECREF(fmt);
	Py_DECREF(pyhexPx);
	Py_DECREF(pyhexPy);

	// load point in affine struct
	secp256k1_ge P;
	secp256k1_ge_set_xy(&P, &x, &y);

	// perform inverting
	secp256k1_ge t;
	secp256k1_ge_neg(&t, &P);

	// convert x and y to hex
	char cx[65];
	char cy[65];
	secp256k1_fe_get_hex(cx, &t.x);
	secp256k1_fe_get_hex(cy, &t.y);
        cx[64] = 0;
        cy[64] = 0;

	// convert hex string of x, y to python long
	PyObject* xr = PyLong_FromString(cx, NULL, 16);
	PyObject* yr = PyLong_FromString(cy, NULL, 16);

	return Py_BuildValue("[NN]", xr, yr);

}

static PyObject* compress(PyObject* self, PyObject* args)
{

	PyObject *list;
	if (!PyArg_ParseTuple(args, "O", &list)) {
		return NULL;
	}

	if (!PyList_Check(list)) {
		PyErr_SetString(PyExc_TypeError, "expected list");
		return NULL;
	}

	PyObject *Px, *Py;
	Px = PyList_GetItem(list, 0);
	Py = PyList_GetItem(list, 1);

	// check if point at infinity received
	if (Px==Py_None) {
		char pub[33] = "";
		return Py_BuildValue("s#", pub, 33);
	}

	// convert to hex string
	PyObject* fmt = PyString_FromString("%064x");
	PyObject* pyhexPx = PyString_Format(fmt, Px);
	char* hexPx = PyString_AsString(pyhexPx);
	PyObject* pyhexPy = PyString_Format(fmt, Py);
	char* hexPy = PyString_AsString(pyhexPy);

	// load into bignum type
	secp256k1_fe x, y;
	secp256k1_fe_set_hex(&x, hexPx);
	secp256k1_fe_set_hex(&y, hexPy);

	Py_DECREF(fmt);
	Py_DECREF(pyhexPx);
	Py_DECREF(pyhexPy);

	// load point in affine struct
	secp256k1_ge P;
	secp256k1_ge_set_xy(&P, &x, &y);

	size_t size; char pub[33];
	secp256k1_eckey_pubkey_serialize(&P, pub, &size, 1);

	// convert char array to python string object
	PyObject* s = PyString_FromStringAndSize(pub,size);

	return Py_BuildValue("N", s);

}


static PyObject* decompress(PyObject* self, PyObject* args)
{

	PyObject *pubs;
	if (!PyArg_ParseTuple(args, "O", &pubs)) {
		return NULL;
	}

	if (!PyString_Check(pubs)) {
		PyErr_SetString(PyExc_TypeError, "string expected");
		return NULL;
	}

	char* pub = PyString_AsString(pubs);

	// check if point at infinity received
	if (pub[0]==0) {
		return Py_BuildValue("[O,O]", Py_None,Py_None);
	}

	// perform decompression
	secp256k1_ge t;
        secp256k1_eckey_pubkey_parse(&t, pub, 33);

	// convert x and y to hex
	char cx[65];
	char cy[65];
	secp256k1_fe_get_hex(cx, &t.x);
	secp256k1_fe_get_hex(cy, &t.y);
        cx[64] = 0;
        cy[64] = 0;

	// convert hex string of x, y to python long
	PyObject* xr = PyLong_FromString(cx, NULL, 16);
	PyObject* yr = PyLong_FromString(cy, NULL, 16);

	return Py_BuildValue("[NN]", xr, yr);

}


static PyObject* valid(PyObject* self, PyObject* args)
{

	PyObject *list;
	if (!PyArg_ParseTuple(args, "O", &list)) {
		return NULL;
	}

	if (!PyList_Check(list)) {
		PyErr_SetString(PyExc_TypeError, "expected list");
		return NULL;
	}

	PyObject *Px, *Py;
	Px = PyList_GetItem(list, 0);
	Py = PyList_GetItem(list, 1);

	// check if point at infinity received
	if (Px==Py_None) {
		return Py_BuildValue("O", Py_None);
	}

	// convert to hex string
	PyObject* fmt = PyString_FromString("%064x");
	PyObject* pyhexPx = PyString_Format(fmt, Px);
	char* hexPx = PyString_AsString(pyhexPx);
	PyObject* pyhexPy = PyString_Format(fmt, Py);
	char* hexPy = PyString_AsString(pyhexPy);

	// load into bignum type
	secp256k1_fe x, y;
	secp256k1_fe_set_hex(&x, hexPx);
	secp256k1_fe_set_hex(&y, hexPy);

	Py_DECREF(fmt);
	Py_DECREF(pyhexPx);
	Py_DECREF(pyhexPy);

	// load point in affine struct
	secp256k1_ge P;
	secp256k1_ge_set_xy(&P, &x, &y);

	if (secp256k1_ge_is_valid_var(&P)) {
		return Py_BuildValue("i", 1);
	}

	return Py_BuildValue("i", 0);

}



static PyMethodDef Methods[] =
{
	{"mul", mul, METH_VARARGS, "Perform point multiplication"},
	{"add", add, METH_VARARGS, "Perform point addition"},
	{"inv", inv, METH_VARARGS, "Perform point inversion"},
	{"compress", compress, METH_VARARGS, "Compress point"},
	{"decompress", decompress, METH_VARARGS, "Decompress point"},
	{"valid", valid, METH_VARARGS, "Check whether the point is on curve"},
	{NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initsecp256k1_libsecp256k1(void)
{

	ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
	PyObject *module;
	module = Py_InitModule("secp256k1_libsecp256k1", Methods);


	// export base point g
	// convert x and y to hex
	char cx[65];
	char cy[65];
	secp256k1_fe_get_hex(cx, &secp256k1_ge_const_g.x);
	secp256k1_fe_get_hex(cy, &secp256k1_ge_const_g.y);
        cx[64] = 0;
        cy[64] = 0;

	// convert hex string of x, y to python long
	PyObject* xr = PyLong_FromString(cx, NULL, 16);
	PyObject* yr = PyLong_FromString(cy, NULL, 16);

	// export base point G as module variable "g"
	PyModule_AddObject(module, "g", Py_BuildValue("[NN]", xr, yr));

	// export group order n
	secp256k1_num order;
	secp256k1_scalar_order_get_num(&order);

	// convert n to hex
        char hbuf[32];
	char cn[65];
        secp256k1_num_get_bin(hbuf, 32, &order);
	secp256k1_fe fe;
	secp256k1_fe_set_b32(&fe, hbuf);
	secp256k1_fe_get_hex(cn, &fe);
        cn[64] = 0;

	// convert hex string of n to python long
	PyObject* n = PyLong_FromString(cn, NULL, 16);

	// export curve order as module variable "n"
	PyModule_AddObject(module, "n", Py_BuildValue("N", n));

}

static void secp256k1_fe_get_hex(char *r64, const secp256k1_fe *a) {
	secp256k1_fe b;
	int i;
	unsigned char tmp[32];
	b = *a;
	secp256k1_fe_normalize(&b);
	secp256k1_fe_get_b32(tmp, &b);
	for (i=0; i<32; i++) {
		/* Hex character table. */
		static const char *c = "0123456789ABCDEF";
		r64[2*i]   = c[(tmp[i] >> 4) & 0xF];
		r64[2*i+1] = c[(tmp[i]) & 0xF];
	}
}

static int secp256k1_fe_set_hex(secp256k1_fe *r, const char *a64) {
	int i;
	unsigned char tmp[32];
	/* Byte to hex value table. */
	static const int cvt[256] = {0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
				0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
				0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
				0, 1, 2, 3, 4, 5, 6,7,8,9,0,0,0,0,0,0,
				0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,
				0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
				0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,
				0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
				0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
				0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
				0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
				0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
				0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
				0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
				0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
				0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0};
	for (i=0; i<32; i++) {
		tmp[i] = (cvt[(unsigned char)a64[2*i]] << 4) + cvt[(unsigned char)a64[2*i+1]];
	}
	return secp256k1_fe_set_b32(r, tmp);
}
