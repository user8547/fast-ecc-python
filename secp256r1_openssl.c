#include <Python.h>

#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>

#ifndef CURVE
#define CURVE NID_X9_62_prime256v1
#endif

EC_GROUP *ecgrp;
BN_CTX *ctx;

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

	PyObject *valuex, *valuey;
	valuex = PyList_GetItem(list, 0);
	valuey = PyList_GetItem(list, 1);

	// check if point at infinity received
	if (valuex==Py_None) {
		return Py_BuildValue("[O,O]", Py_None,Py_None);
	}


	// convert to hex string
	PyObject* fmt = PyString_FromString("%064x");
	PyObject* pyhexPx = PyString_Format(fmt, valuex);
	char* hexPx = PyString_AsString(pyhexPx);
	PyObject* pyhexPy = PyString_Format(fmt, valuey);
	char* hexPy = PyString_AsString(pyhexPy);
	PyObject* pyhexP = PyString_FromFormat("04%s%s", hexPx, hexPy);
	char* hexP = PyString_AsString(pyhexP);
        PyObject* pyhex = PyString_Format(fmt, value);
        char* hex = PyString_AsString(pyhex);


	// load point
	EC_POINT *P = EC_POINT_new(ecgrp);
	EC_POINT_hex2point(ecgrp, hexP, P, ctx);

	Py_DECREF(fmt);
	Py_DECREF(pyhexPx);
	Py_DECREF(pyhexPy);
	Py_DECREF(pyhexP);
	Py_DECREF(pyhex);

	// load scalar
	BIGNUM *scalar_bn = BN_new();
	BN_hex2bn(&scalar_bn, hex);

	// point to store result
	EC_POINT *r = EC_POINT_new(ecgrp);

	// use optimization if multiplication is performed on the base point
	if (EC_POINT_cmp(ecgrp, EC_GROUP_get0_generator(ecgrp), P, ctx)==0) {

		// perform scalar multiplication of G
		EC_POINT_mul(ecgrp, r, scalar_bn, NULL, NULL, ctx);

	} else {

		// perform scalar multiplication
		EC_POINT_mul(ecgrp, r, NULL, P, scalar_bn, ctx);

	}

	// check if the result is point at infinity
	if (EC_POINT_is_at_infinity(ecgrp, r)) {
		return Py_BuildValue("[O,O]", Py_None,Py_None);
	}

	// convert hex string to python long x and y
	char *hexr = EC_POINT_point2hex(ecgrp, r, 4, ctx);
	PyObject* yr = PyLong_FromString(&hexr[66], NULL, 16);
	hexr[66] = 0;
	PyObject* xr = PyLong_FromString(&hexr[2], NULL, 16);

	BN_free(scalar_bn);
	EC_POINT_free(P);
	EC_POINT_free(r);
	free(hexr);

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

	// convert to hex string and load point A
	PyObject* fmt = PyString_FromString("%064x");
	PyObject* pyhexAx = PyString_Format(fmt, oAx);
	char* hexAx = PyString_AsString(pyhexAx);
	PyObject* pyhexAy = PyString_Format(fmt, oAy);
	char* hexAy = PyString_AsString(pyhexAy);
	PyObject* pyhexA = PyString_FromFormat("04%s%s", hexAx, hexAy);
	char* hexA = PyString_AsString(pyhexA);

	// load point A
	EC_POINT *A = EC_POINT_new(ecgrp);
	EC_POINT_hex2point(ecgrp, hexA, A, ctx);

	// convert to hex string and load point B
	PyObject* pyhexBx = PyString_Format(fmt, oBx);
	char* hexBx = PyString_AsString(pyhexBx);
	PyObject* pyhexBy = PyString_Format(fmt, oBy);
	char* hexBy = PyString_AsString(pyhexBy);
	PyObject* pyhexB = PyString_FromFormat("04%s%s", hexBx, hexBy);
	char* hexB = PyString_AsString(pyhexB);

	// load point B
	EC_POINT *B = EC_POINT_new(ecgrp);
	EC_POINT_hex2point(ecgrp, hexB, B, ctx);

	Py_DECREF(fmt);
	Py_DECREF(pyhexAx);
	Py_DECREF(pyhexAy);
	Py_DECREF(pyhexA);
	Py_DECREF(pyhexBx);
	Py_DECREF(pyhexBy);
	Py_DECREF(pyhexB);

	EC_POINT *r = EC_POINT_new(ecgrp);

	// perform addition
	EC_POINT_add(ecgrp, r, A, B, ctx);

	// check if the result is point at infinity
	if (EC_POINT_is_at_infinity(ecgrp, r)) {
		return Py_BuildValue("[O,O]", Py_None,Py_None);
	}

	// convert hex string to python long x and y
	char *hexr = EC_POINT_point2hex(ecgrp, r, 4, ctx);
	PyObject* yr = PyLong_FromString(&hexr[66], NULL, 16);
	hexr[66] = 0;
	PyObject* xr = PyLong_FromString(&hexr[2], NULL, 16);

	EC_POINT_free(A);
	EC_POINT_free(B);
	EC_POINT_free(r);
	free(hexr);

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
	PyObject* pyhexP = PyString_FromFormat("04%s%s", hexPx, hexPy);
	char* hexP = PyString_AsString(pyhexP);


	// load point
	EC_POINT *P = EC_POINT_new(ecgrp);
	EC_POINT_hex2point(ecgrp, hexP, P, ctx);

	Py_DECREF(fmt);
	Py_DECREF(pyhexPx);
	Py_DECREF(pyhexPy);
	Py_DECREF(pyhexP);

	// perform inverting
	EC_POINT_invert(ecgrp, P, ctx);

	// convert hex string to python long x and y
	char *hexr = EC_POINT_point2hex(ecgrp, P, 4, ctx);
	PyObject* yr = PyLong_FromString(&hexr[66], NULL, 16);
	hexr[66] = 0;
	PyObject* xr = PyLong_FromString(&hexr[2], NULL, 16);

	EC_POINT_free(P);
	free(hexr);

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
	PyObject* pyhexP = PyString_FromFormat("04%s%s", hexPx, hexPy);
	char* hexP = PyString_AsString(pyhexP);


	// load point
	EC_POINT *P = EC_POINT_new(ecgrp);
	EC_POINT_hex2point(ecgrp, hexP, P, ctx);

	Py_DECREF(fmt);
	Py_DECREF(pyhexPx);
	Py_DECREF(pyhexPy);
	Py_DECREF(pyhexP);

	// convert point to char array
	char pub[33];
	EC_POINT_point2oct(ecgrp, P, 2, pub, 33, ctx);

	// convert char array to python string object
	PyObject* s = PyString_FromStringAndSize(pub,33);

	EC_POINT_free(P);

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

	// load point
	EC_POINT *P = EC_POINT_new(ecgrp);
	EC_POINT_oct2point(ecgrp, P, pub, 33, ctx);

	// convert hex string to python long x and y
	char *hexr = EC_POINT_point2hex(ecgrp, P, 4, ctx);
	PyObject* yr = PyLong_FromString(&hexr[66], NULL, 16);
	hexr[66] = 0;
	PyObject* xr = PyLong_FromString(&hexr[2], NULL, 16);

	EC_POINT_free(P);
	free(hexr);

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
	PyObject* pyhexP = PyString_FromFormat("04%s%s", hexPx, hexPy);
	char* hexP = PyString_AsString(pyhexP);

	// load point
	EC_POINT *P = EC_POINT_new(ecgrp);
	EC_POINT_hex2point(ecgrp, hexP, P, ctx);

	Py_DECREF(fmt);
	Py_DECREF(pyhexPx);
	Py_DECREF(pyhexPy);
	Py_DECREF(pyhexP);

	if (EC_POINT_is_on_curve(ecgrp, P, ctx)) {
		EC_POINT_free(P);
		return Py_BuildValue("i", 1);
	}

	EC_POINT_free(P);

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

#if CURVE == NID_X9_62_prime256v1
PyMODINIT_FUNC initsecp256r1_openssl(void)
#else
PyMODINIT_FUNC initsecp256k1_openssl(void)
#endif
{
	PyObject *module;

#if CURVE == NID_X9_62_prime256v1
	module = Py_InitModule("secp256r1_openssl", Methods);
#else
	module = Py_InitModule("secp256k1_openssl", Methods);
#endif

	ecgrp = EC_GROUP_new_by_curve_name(CURVE);
	ctx = BN_CTX_new();

	// precompute mult
	EC_GROUP_precompute_mult(ecgrp, ctx);

	// export base point g
	// convert point to hex
	const EC_POINT *gen = EC_GROUP_get0_generator(ecgrp);
	char *hex = EC_POINT_point2hex(ecgrp, gen, 4, ctx);

	// convert hex string to python long x and y
	PyObject* yr = PyLong_FromString(&hex[66], NULL, 16);
	hex[66] = 0;
	PyObject* xr = PyLong_FromString(&hex[2], NULL, 16);

	// export base point G as module variable "g"
	PyModule_AddObject(module, "g", Py_BuildValue("[NN]", xr, yr));


	// export group order n
	// convert n to hex
	BIGNUM *order_bn = BN_new();
	EC_GROUP_get_order(ecgrp, order_bn, ctx);
	char *cn = BN_bn2hex(order_bn);

	// convert hex string of n to python long
	PyObject* n = PyLong_FromString(cn, NULL, 16);

	// export curve order as module variable "n"
	PyModule_AddObject(module, "n", Py_BuildValue("N", n));

}
