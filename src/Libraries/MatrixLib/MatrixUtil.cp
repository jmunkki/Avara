/*/
    Copyright ©1992, Juri Munkki
    All rights reserved.

    File: MatrixUtil.c
    Created: Saturday, October 17, 1992, 0:11
    Modified: Thursday, November 5, 1992, 11:45
/*/

#include "FastMat.h"
#include <math.h>

void	FixPrint(n)
Fixed	n;
{
	printf("%8.4lf ",n/65536.0);
}

void	VectorPrint(v)
Vector	*v;
{
	int	i;
	putchar('\n');
	for(i=0;i<4;i++)
		FixPrint((*v)[i]);

}
void	MatrixPrint(m)
Matrix	*m;
{
	int	i,j;
	
	for(i=0;i<4;i++)
	{	VectorPrint(&((*m)[i]));
 	}
	
	putchar('\n');
}
