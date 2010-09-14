/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#pragma once

namespace caspar { namespace common {

enum SIMD
{
	AUTO,
	REF,
	SSE,
	SSE2,
	SSE3,
	SSSE3,
	SSE4_1,
	SSE4_2,
	SSE5
};

struct cpuid
{
	cpuid();

	bool FPU;		//Floating Point Unit
	bool VME;		//Virtual Mode Extension
	bool DE;		//Debugging Extension
	bool PSE;		//Page Size Extension
	bool TSC;		//Time Stamp Counter
	bool MSR;		//Model Specific Registers
	bool PAE;		//Physical Address Extesnion
	bool MCE;		//Machine Check Extension
	bool CX8;		//CMPXCHG8 Instruction
	bool APIC;		//On-chip APIC Hardware
	bool SEP;		//SYSENTER SYSEXIT
	bool MTRR;		//Machine Type Range Registers
	bool PGE;		//Global Paging Extension
	bool MCA;		//Machine Check Architecture
	bool CMOV;		//Conditional Move Instrction
	bool PAT;		//Page Attribute Table
	bool PSE_36;	//36-bit Page Size Extension
	bool PSN;		//96-bit Processor Serial Number
	bool CLFSH;		//CLFLUSH Instruction
	bool DS;		//Debug Trace Store
	bool ACPI;		//ACPI Support
	bool MMX;		//MMX Technology
	bool FXSR;		//FXSAVE FXRSTOR (Fast save and restore)
	bool SSE;		//Streaming SIMD Extensions
	bool SSE2;		//Streaming SIMD Extensions 2
	bool SSE3;
	bool SSSE3; 
	bool SSE4_1;
	bool SSE4_2;
	bool SSE5;
	bool SS;		//Self-Snoop
	bool HTT;		//Hyper-Threading Technology
	bool TM;		//Thermal Monitor Supported
	bool IA_64;		//IA-64 capable	

	int Family;
	int Model;
	int ModelEx;
	int Stepping;
	int FamilyEx;
	int Brand; 
	int Type;

	SIMD SIMD;

	//int CacheLineSize;
	//int LogicalProcessorCount;
	//int LocalAPICID;

};

}}