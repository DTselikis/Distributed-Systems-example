/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "RPC.h"

bool_t
xdr_intMatrix (XDR *xdrs, intMatrix *objp)
{
	register int32_t *buf;

	 if (!xdr_array (xdrs, (char **)&objp->numArray.numArray_val, (u_int *) &objp->numArray.numArray_len, 100,
		sizeof (int), (xdrproc_t) xdr_int))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_floatMatrix (XDR *xdrs, floatMatrix *objp)
{
	register int32_t *buf;

	 if (!xdr_array (xdrs, (char **)&objp->muledArray.muledArray_val, (u_int *) &objp->muledArray.muledArray_len, 100,
		sizeof (float), (xdrproc_t) xdr_float))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_mulFloat (XDR *xdrs, mulFloat *objp)
{
	register int32_t *buf;

	 if (!xdr_array (xdrs, (char **)&objp->numArray.numArray_val, (u_int *) &objp->numArray.numArray_len, 100,
		sizeof (int), (xdrproc_t) xdr_int))
		 return FALSE;
	 if (!xdr_float (xdrs, &objp->multiplier))
		 return FALSE;
	return TRUE;
}
