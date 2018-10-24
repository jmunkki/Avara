#include "MixedMode.h"
#include "FastMat.h"

long	__procinfo = 
		 kThinkCStackBased
		| RESULT_SIZE(SIZE_CODE(sizeof(UniversalProcPtr)))
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(short)));

UniversalProcPtr	main(short a);

UniversalProcPtr	main(short a)
{
	UniversalProcPtr	theProc;

	switch(a)
	{	case kPowerFlaggedVMProduct:
			{	static	RoutineDescriptor	powerFlaggedVMProduct = BUILD_ROUTINE_DESCRIPTOR(
							kThinkCStackBased
							| RESULT_SIZE(SIZE_CODE(0))
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(long))) 
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(Vector *))) 
							| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(Vector *))) 
							| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(Matrix *))),
							(ProcPtr)FlaggedVectorMatrix34ProductC);
				theProc = &powerFlaggedVMProduct;
			}
			break;

		default:
			theProc = NULL;
			break;
	}
	
	return theProc;
}
