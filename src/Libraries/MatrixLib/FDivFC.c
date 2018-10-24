static  short   doubleFudgePlus[4] = { 0x0, 0, 0, 0 };
static  short   doubleFudgeMinus[4] = { 0x8000, 0, 0, 0 };
static  long    doubleFudgeB[2] = { 0, 0 };

Fixed   FDivFC(
	Fixed	a,
	Fixed	b)
{
	if(b < 0)
	{
		doubleFudgeB[1] = -b;

		if(a < 0)
		{
			*(long *)(1+doubleFudgePlus) = -a;

			return 	* (double *) doubleFudgePlus /
					* (double *) doubleFudgeB;
		}
		else
		{
			*(long *)(1+doubleFudgeMinus) = a;

			return	* (double *) doubleFudgeMinus /
					* (double *) doubleFudgeB;
		}
	}
	else
	{
		doubleFudgeB[1] = b;

		if(a > 0)
		{
			*(long *)(1+doubleFudgePlus) = a;

			return	* (double *) doubleFudgePlus /
					* (double *) doubleFudgeB;
		}
		else
		{	*(long *)(1+doubleFudgeMinus) = -a;

			return	* (double *) doubleFudgeMinus /
					* (double *) doubleFudgeB;
		}
	}
}
