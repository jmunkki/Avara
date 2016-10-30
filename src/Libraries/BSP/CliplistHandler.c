/*
    Copyright ©1995, Juri Munkki
    All rights reserved.

    File: CliplistHandler.c
    Created: Monday, May 8, 1995, 11:01
    Modified: Monday, May 8, 1995, 11:01
*/

#include "CBSPPart.h"

void	ClipListHandler(
	short			*countP,
	Fixed2DPoint	*pointList,
	Fixed			x,
	Fixed			y)
{
	short	i;
	short	count = *countP;

	if(count == 0)				//	Empty list, add new point to the list.
	{	pointList[0].x.f = x;
		pointList[0].y.f  = y;
		count++;
	}
	else
	if(count == 1)				//	One point in existing list.
	{	if(pointList[0].x.f == x &&
		   pointList[0].y.f  == y)
		{	count--;	//	Remove duplicate point.
		}
		else				//	Add second point.
		{	pointList[1].x.f = x;
			pointList[1].y.f  = y;
			count++;
		}
	}
	else
	{	//	Now this gets a bit more complicated, as we have to keep the points sorted.
		//	Fortunately this bit of code is called _very_ rarely. It's only used for
		//	concave polygons that split into more than two parts (normally 2 = in+out)
		//	when clipped against a plane:
		//
		//		  a|b
		//		aaa|		(Example = 3 parts = a, b and c)
		//		 aa|c
		//		  a|cc
		//

		Boolean	same = false;
		short	position;

		for(i = 0; i < count; i++)
		{	if(pointList[i].x.f == x && pointList[i].y.f == y)
			{	position = i;
				same = true;
				i = count;	//	End loop.
			}
		}
		
		if(same)
		{	//	Delete duplicate point from list.
			
			count--;
			for(i=position; i < count; i++)
			{	pointList[i] = pointList[i+1];
			}
		}
		else
		{	Fixed	xspan, yspan;
			Fixed	xabs, yabs;
		
			xspan = pointList[count-1].x.f - pointList[0].x.f;
			xabs = (xspan < 0) ? -xspan : xspan;

			yspan = pointList[count-1].y.f - pointList[0].y.f;
			yabs = (xspan < 0) ? -xspan : xspan;
			position = count;
			
			if(xabs > yabs)
			{	if(xspan > 0)
				{	for(i = 0; i < count; i++)
					{	if(pointList[i].x.f > x)
						{	position = i;
							break;	//	End loop.
						}
					}
				}
				else
				{	for(i = 0; i < count; i++)
					{	if(pointList[i].x.f < x)
						{	position = i;
							break;	//	End loop.
						}
					}
				}
			}
			else
			{	if(yspan > 0)
				{	for(i = 0; i < count; i++)
					{	if(pointList[i].y.f > y)
						{	position = i;
							break;	//	End loop.
						}
					}
				}
				else
				{	for(i = 0; i < count; i++)
					{	if(pointList[i].y.f < y)
						{	position = i;
							break;	//	End loop.
						}
					}
				}
			}

			//	Insert new point into list.
			if(count < MAXCROSSPOINTS)
			{	for(i=count-1;i >= position; i--)
				{	pointList[i+1] = pointList[i];
				}
				
				pointList[position].x.f = x;
				pointList[position].y.f = y;
				count++;
			}	
		}
		
	}

	*countP = count;
}