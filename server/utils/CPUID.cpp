#include "..\stdafx.h"
#include "CPUID.hpp"
#include <intrin.h>

namespace caspar {
namespace utils {

CPUID::CPUID() : 
	FPU(0),
	VME(0),		
	DE(0),		
	PSE(0),		
	TSC(0),		
	MSR(0),		
	PAE(0),		
	MCE(0),		
	CX8(0),		
	APIC(0),	
	SEP(0),		
	MTRR(0),	
	PGE(0),		
	MCA(0),		
	CMOV(0),	
	PAT(0),		
	PSE_36(0),	
	PSN(0),		
	CLFSH(0),		
	DS(0),	
	ACPI(0),		
	MMX(0),	
	FXSR(0),
	SSE(0),	
	SSE2(0),	
	SSE3(0),	
	SSSE3(0),	 
	SSE4_1(0),	
	SSE4_2(0),	
	SSE5(0),	
	SS(0),	
	HTT(0),	
	TM(0),	
	IA_64(0),
	Family(0),
	Model(0),
	ModelEx(0),
	Stepping(0),
	FamilyEx(0),
	Brand(0),
	Type(0)
{		
	int CPUInfo[4] = {-1};
	__cpuid(CPUInfo, 0);	

	int nIds = CPUInfo[0];
	ID.append(reinterpret_cast<char*>(&CPUInfo[1]), 4);
	ID.append(reinterpret_cast<char*>(&CPUInfo[3]), 4);
	ID.append(reinterpret_cast<char*>(&CPUInfo[2]), 4);

	for (int i = 0; i <= nIds; ++i)
	{
		__cpuid(CPUInfo, i);

		if  (i == 1)
		{
			Stepping =	(CPUInfo[0]      ) & 0x0F;
			Model =		(CPUInfo[0] >> 4 ) & 0x0F;
			Family =	(CPUInfo[0] >> 8 ) & 0x0F;
			Type =		(CPUInfo[0] >> 12) & 0x03;
			ModelEx =	(CPUInfo[0] >> 16) & 0x0F;
			FamilyEx =	(CPUInfo[0] >> 20) & 0xFF;
			Brand =		(CPUInfo[1]		 ) & 0xFF;			

			if(ID == "GenuineIntel") 
			{
				FPU		= (CPUInfo[3] & (1 << 0))  != 0;
				VME		= (CPUInfo[3] & (1 << 1))  != 0;
				DE		= (CPUInfo[3] & (1 << 2))  != 0;
				PSE		= (CPUInfo[3] & (1 << 3))  != 0;
				TSC		= (CPUInfo[3] & (1 << 4))  != 0;
				MSR		= (CPUInfo[3] & (1 << 5))  != 0;
				PAE		= (CPUInfo[3] & (1 << 6))  != 0;
				MCE		= (CPUInfo[3] & (1 << 7))  != 0;
				CX8		= (CPUInfo[3] & (1 << 8))  != 0;
				APIC	= (CPUInfo[3] & (1 << 9))  != 0;
				//		= (CPUInfo[3] & (1 << 10)) != 0;
				SEP		= (CPUInfo[3] & (1 << 11)) != 0;
				MTRR	= (CPUInfo[3] & (1 << 12)) != 0;
				PGE		= (CPUInfo[3] & (1 << 13)) != 0;
				MCA		= (CPUInfo[3] & (1 << 14)) != 0;
				CMOV	= (CPUInfo[3] & (1 << 15)) != 0;
				PAT		= (CPUInfo[3] & (1 << 16)) != 0;
				PSE_36	= (CPUInfo[3] & (1 << 17)) != 0;
				PSN		= (CPUInfo[3] & (1 << 18)) != 0;
				CLFSH	= (CPUInfo[3] & (1 << 19)) != 0;
				//		= (CPUInfo[3] & (1 << 20)) != 0;
				DS		= (CPUInfo[3] & (1 << 21)) != 0;
				ACPI	= (CPUInfo[3] & (1 << 22)) != 0;
				MMX		= (CPUInfo[3] & (1 << 23)) != 0;
				FXSR	= (CPUInfo[3] & (1 << 24)) != 0;
				SSE		= (CPUInfo[3] & (1 << 25)) != 0;
				SSE2	= (CPUInfo[3] & (1 << 26)) != 0;
				SS		= (CPUInfo[3] & (1 << 27)) != 0;
				HTT		= (CPUInfo[3] & (1 << 28)) != 0;
				TM		= (CPUInfo[3] & (1 << 29)) != 0;
				//		= (CPUInfo[3] & (1 << 30)) != 0;
				IA_64	= (CPUInfo[3] & (1 << 31)) != 0;

				SSE3	= (CPUInfo[2] & (1 << 0))  != 0;
				SSSE3	= (CPUInfo[2] & (1 << 9))  != 0;
				SSE4_1	= (CPUInfo[2] & (1 << 19)) != 0;
				SSE4_2	= (CPUInfo[2] & (1 << 20)) != 0;
			}
			else if(ID == "AuthenticAMD") 
			{
			}
		}
	}	

	if(SSE5)
		SIMD = utils::SSE5;
	else if(SSE4_2)
		SIMD = utils::SSE4_2;
	else if(SSE4_1)
		SIMD = utils::SSE4_1;
	else if(SSSE3)
		SIMD = utils::SSSE3;
	else if(SSE3)
		SIMD = utils::SSE3;
	else if(SSE2)
		SIMD = utils::SSE2;
	else if(SSE)
		SIMD = utils::SSE;
	else 
		SIMD = utils::REF;
}

}
}
