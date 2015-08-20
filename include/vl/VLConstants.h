/*
    File:       VLConstants.h

    Function:	Contains various constants for VL.

    Author:     Andrew Willmott

    Copyright:  (c) 1999-2000, Andrew Willmott
*/

#ifndef __VLConstants__
#define __VLConstants__

//#include <math.h>


// --- Mathematical constants -------------------------------------------------


#ifdef M_PI
const Real			vl_pi = M_PI;
const Real			vl_halfPi = M_PI_2;
#elif defined(_PI)
const Real			vl_pi = _PI;
const Real			vl_halfPi = vl_pi / 2.0;
#else
const Real			vl_pi = 3.14159265358979323846;
const Real			vl_halfPi = vl_pi / 2.0;
#endif

//#ifdef HUGE_VAL
//const Double vl_inf = HUGE_VAL; 
//#endif

enum	ZeroOrOne	{ vl_zero = 0, vl_0 = 0, vl_one = 1, vl_I = 1, vl_1 = 1 };
enum	Block		{ vl_Z = 0, vl_B = 1, vl_block = 1 };
enum	Axis		{ vl_x, vl_y, vl_z, vl_w };
typedef	Axis		vl_axis;

const UInt32		VL_REF_FLAG = UInt32(1) << (sizeof(UInt32) * 8 - 1);
const UInt32 		VL_REF_MASK = (~VL_REF_FLAG);
enum				{ VL_SV_END = -1 };
const UInt32		VL_SV_MAX_INDEX = (1 << 30);
const Int 			VL_MAX_STEPS = 10000;

#endif
