/*/
    Copyright ©1992-1995, Juri Munkki
    All rights reserved.

    File: FastMat.h
    Created: Friday, October 16, 1992, 23:55
    Modified: Wednesday, November 29, 1995, 1:13
/*/

#pragma once

#include "FastMatConfig.h"

void	InitMatrix();
void	CloseMatrix();

typedef	Fixed	Vector[4];	/*	A vector consists of 4 fixed point numbers: x,y,z,w	*/
typedef	Vector	Matrix[4];	/*	A matrix consists of 4 vectors.						*/
typedef struct
{
	Fixed	w,x,y,z;
}	Quaternion;

void	QuaternionToMatrix(Quaternion *q, Matrix *m);
void	MatrixToQuaternion(Matrix *m, Quaternion *q);

/*	The following "structures" make copying vectors and matrices a lot easier.		*/
typedef	struct	{	Vector	theVec; }	VectorStruct;
typedef	struct	{	Matrix	theMatrix;}	MatrixStruct;

#define	VECTORCOPY(a,b) *(VectorStruct *)a = *(VectorStruct *)b
#define	MATRIXCOPY(a,b) *(MatrixStruct *)a = *(MatrixStruct *)b

/*	Function types:		*/
typedef void 	VMProduct(long n, Vector *vs, Vector *vd, Matrix *m);
typedef void 	MMProduct(Matrix *vs, Matrix *vd, Matrix *m);
typedef Fixed	FFtoF(Fixed a,Fixed b);
typedef	Fixed	FFFtoF(Fixed a, Fixed b, Fixed c);
typedef	void	FtoL(Fixed a, long *b);
typedef	Fixed	FFunction();

#define	MAXFIXED	0x7FFFffffL
#define	MINFIXED	0x80000000L

/*	FIX3 results in the n/1000 as a fixed point number								*/
#define	FIX3(n)	((long)((n)*8192L/125L))
/*	FIX results in the integer number as a fixed point number						*/
#define	FIX(n) ((long)(n*65536L))

/*	Prototypes for internal routines:												*/

void	InitTrigTables();
void	CloseTrigTables();

void VectorMatrixProductC(long n, Vector *vs, Vector *vd, Matrix *m);
void VectorMatrixProduct68000(long n, Vector *vs, Vector *vd, Matrix *m);
void VectorMatrixProduct68020(long n, Vector *vs, Vector *vd, Matrix *m);
void VectorMatrix33Product020(long n, Vector *vs, Vector *vd, Matrix *m);
void VectorMatrix34Product020(long n, Vector *vs, Vector *vd, Matrix *m);
void FlaggedVectorMatrix34Product020(long n, Vector *vs, Vector *vd, Matrix *m);
void FlaggedVectorMatrix34ProductC(long n, Vector *vs, Vector *vd, Matrix *m);
void FlaggedVectorMatrix34ProductF(long n, Vector *vs, Vector *vd, Matrix *m);
void CombineTransforms68020(Matrix *vs, Matrix *vd, Matrix *m);

void TransformMinMax(Matrix *m, Fixed *min, Fixed *max, Vector *dest);
void TransformBoundingBox(Matrix *m, Fixed *min, Fixed *max, Vector *dest);

void	OneMatrix(Matrix *OneMatrix);
void	Transpose(Matrix *aMatrix);

void	MRotateX(Fixed s, Fixed c, Matrix *theMatrix);
void	MRotateY(Fixed s, Fixed c, Matrix *theMatrix);
void	MRotateZ(Fixed s, Fixed c, Matrix *theMatrix);
void	MTranslate(Fixed xt, Fixed yt, Fixed zt, Matrix *theMatrix);

Fixed	FMulC(Fixed a,Fixed b);
Fixed	FMul68000(Fixed a,Fixed b);
Fixed	FMul68020(Fixed a,Fixed b);

Fixed	FDivC(Fixed a,Fixed b);
Fixed	FDiv68000(Fixed a,Fixed b);
Fixed	FDiv68020(Fixed a,Fixed b);

Fixed	FMulDivC(Fixed a,Fixed b, Fixed c);
Fixed	FMulDiv68000(Fixed a,Fixed b, Fixed c);
Fixed	FMulDiv68020(Fixed a,Fixed b, Fixed c);
Fixed	FMulDiv68020V(Fixed a,Fixed b, Fixed c);	//	Overflow version for perspective transform

#define	FHALFPI			102944
#define	FONEPI			205887
#define	FTWOPI			411775

Fixed	FDegSin(Fixed a);	//	Full circle is 360.0 (fixed point)
Fixed	FDegCos(Fixed a);
Fixed	FRadSin(Fixed a);	//	Full circle is 2 Pi (fixed point, of course)
Fixed	FRadCos(Fixed a);
Fixed	FOneSin(Fixed a);	//	Full circle is 1.0 (fixed point)
Fixed	FOneCos(Fixed a);
Fixed	FRadArcCos(Fixed n);	//	Instead of FRadArcSin and FRadArcCos use FRadArcTan2
Fixed	FRadArcSin(Fixed n);	//	when you can! ArcSin and ArcCos are for small values only.
Fixed	FRadArcTan2(Fixed x, Fixed y);
Fixed	FOneArcTan2(Fixed x, Fixed y);
void	FindSpaceAngle(Fixed *delta, Fixed *heading, Fixed *pitch);
void	FindSpaceAngleAndNormalize(Fixed *delta, Fixed *heading, Fixed *pitch);

#define	FRadToDeg(angle)	FMul(angle, 3754936)
#define	FDegToRad(angle)	FMul(angle, 1144)
#define	FOneToDeg(angle)	((angle)*360)
#define	FDegToOne(angle)	((angle)/360)
#define	FOneToRad(angle)	FMul(angle, 41175)
#define	FRadToOne(angle)	FMul(angle, 10430)

Fixed	FSqroot(long *ab);
Fixed	FSqrt(Fixed n);

void	FSquareAccumulate68000(Fixed n, long *acc);
void	FSquareAccumulate68020(Fixed n, long *acc);

void	FSquareSubtract68000(Fixed n, long *acc);
void	FSquareSubtract68020(Fixed n, long *acc);

Fixed	VectorLength(long n, Fixed *v);
Fixed	NormalizeVector(long n, Fixed *v);	//	Returns length

Fixed	FRandom68000();
Fixed	FRandomBeta();
Fixed	FRandomC();
Fixed	FDistanceEstimate(Fixed dx, Fixed dy, Fixed dz);
Fixed	FDistanceOverEstimate(Fixed dx, Fixed dy, Fixed dz);
void	InverseTransform(Matrix *trans,Matrix *inv);

#ifdef THINK_C
	#if __option(a4_globals)
		#define	_INITVALUE_(a)
	#else
		#define	_INITVALUE_(a) = a
	#endif
#else
	#define	_INITVALUE_(a) = a
#ifdef	__MWERKS__
	#ifdef powerc
	#undef FASTMAT020
	#endif	
#else
	#undef FASTMAT020
#endif
#endif

#ifdef FASTMAT020
/*	If only 68020 (and 030 and 040) code is to be used,
**	use defines instead of function pointers.
*/

#ifdef FASTMATINLINE

#pragma parameter __D0	FMul(__D0, __D1)
Fixed	FMul(Fixed a, Fixed b) = { 0x4C01, 0x0C01, 0x3001, 0x4840 };
#pragma parameter __D0	FDivNZ(__D0, __D1)
Fixed	FDivNZ(Fixed a, Fixed b) = { 0x4840, 0x2400, 0x4240, 0x48C2, 0x4C41, 0x0C02 };
#pragma parameter __D0	FDiv(__D0, __D1)
Fixed	FDiv(Fixed a, Fixed b) =
					{	0x4A81, 0x670C, 0x4840, 0x2400,
						0x4240, 0x48C2, 0x4C41, 0x0C02	};

#pragma parameter __D0	FMulDiv(__D0, __D1, __D2)
Fixed	FMulDiv(Fixed a, Fixed b, Fixed c) =
					{	0x4A82, 0x6708, 0x4C01, 0x0C01, 0x4C42, 0x0C01};

#pragma parameter __D0	FMulDivNZ(__D0, __D1, __D2)
Fixed	FMulDivNZ(Fixed a, Fixed b, Fixed c) =	{	0x4C01, 0x0C01, 0x4C42, 0x0C01};

#pragma parameter __D0	FMulDivV(__D0, __D1, __D2)
Fixed	FMulDivV(Fixed a, Fixed b, Fixed c) =
					{	0x4A82, 0x6714, 0x4C01, 0x0C01, 0x4C42, 0x0C01, 
					 	0x680A, 0x70FF, 0xE488, 0x4A81, 0x6A02, 0x4480};

#pragma parameter __D0	FMulDivVNZ(__D0, __D1, __D2)
Fixed	FMulDivVNZ(Fixed a, Fixed b, Fixed c) =
					{	0x4C01, 0x0C01, 0x4C42, 0x0C01, 
					 	0x680A, 0x70FF, 0xE488, 0x4A81, 0x6A02, 0x4480};

#pragma parameter		FSquareAccumulate(__D1, __A0)
void	FSquareAccumulate(Fixed n, Fixed *acc) =
					{	0x2418, 0x4C01, 0x1C00, 0xD390, 0xD580, 0x2102 };

#pragma parameter		FSquareSubtract(__D1, __A0)
void	FSquareSubtract(Fixed n, Fixed *acc) =
					{	0x2418, 0x4C01, 0x1C00, 0x9390, 0x9580, 0x2102 };

#else

#define FMul							FMul68020
#define FMulDiv							FMulDiv68020
#define FMulDivNZ						FMulDiv68020
#define FMulDivV						FMulDiv68020V
#define FMulDivVNZ						FMulDiv68020V
#define FDiv							FDiv68020
#define FDivNZ							FDiv68020
#define FSquareAccumulate				FSquareAccumulate68020
#define FSquareSubtract					FSquareSubtract68020

#endif

#define VectorMatrixProduct				VectorMatrixProduct68020
#define VectorMatrix33Product			VectorMatrix33Product020
#define VectorMatrix34Product			VectorMatrix34Product020
#define	CombineTransforms				CombineTransforms68020
#define FRandom							FRandom68000

#ifdef FASTMAT
		VMProduct	*FlaggedVectorMatrix34Product	_INITVALUE_(FlaggedVectorMatrix34Product020);
#else
extern	VMProduct	*FlaggedVectorMatrix34Product;
#endif

#else	// support for 68000 as well as 68020

#define	FDivNZ							FDiv

/*
**	Instead of calling routines directly, pointers to the internal routines
**	are stored in global variables so that the fastest routine is always
**	used:
*/

#ifdef FASTMAT

VMProduct	*VectorMatrixProduct			_INITVALUE_(VectorMatrixProductC);
VMProduct	*VectorMatrix33Product			_INITVALUE_(VectorMatrixProductC);
VMProduct	*VectorMatrix34Product			_INITVALUE_(VectorMatrixProductC);
VMProduct	*FlaggedVectorMatrix34Product	_INITVALUE_(VectorMatrixProductC);
FFtoF		*FMul							_INITVALUE_(FMulC);
FFtoF		*FDiv							_INITVALUE_(FDivC);
FFFtoF		*FMulDiv						_INITVALUE_(FMulDivC);
FFFtoF		*FMulDivV						_INITVALUE_(FMulDivC);
FtoL		*FSquareAccumulate				_INITVALUE_(FSquareAccumulate68000);
FtoL		*FSquareSubtract				_INITVALUE_(FSquareSubtract68000);
FFunction	*FRandom						_INITVALUE_(FRandomC);


#define		CombineTransforms(a,b,c)		VectorMatrixProduct(4,a,b,c)

#else
extern	VMProduct	*VectorMatrixProduct;
extern	VMProduct	*VectorMatrix33Product;
extern	VMProduct	*VectorMatrix34Product;
extern	VMProduct	*FlaggedVectorMatrix34Product;
extern	FFtoF		*FMul;
extern	FFtoF		*FDiv;
extern	FFFtoF		*FMulDiv;
extern	FFFtoF		*FMulDivV;
extern	FtoL		*FSquareAccumulate;
extern	FtoL		*FSquareSubtract;
extern	FFunction	*FRandom;

#endif
#endif

#define	FASTMATPOWERPLUGID	130
enum	{	kPowerFlaggedVMProduct	};


#ifdef FASTMAT
		Fixed		FRandSeed;
		Fixed		FRandSeedBeta;
#else
extern	Fixed		FRandSeed;
extern	Fixed		FRandSeedBeta;
#endif
