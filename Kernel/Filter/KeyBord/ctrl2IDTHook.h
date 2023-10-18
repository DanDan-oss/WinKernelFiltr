// 键盘过滤，过滤键盘中断消息
#ifndef _CTRL2_IDT_HOOK_H
#define _CTRL2_IDT_HOOK_H

#include <ntddk.h>

typedef unsigned char P2C_U8;
typedef unsigned short P2C_U16;
typedef unsigned long P2C_U32;
typedef unsigned long long P2C_U64;

#define OBUFFER_FULL 0x02
#define IBUFFER_FULL 0x01

#define P2C_MAKELONG32(low, high) ((P2C_U32)(((P2C_U16)((P2C_U32)(low) & 0xffff)) | \
								((P2C_U32)((P2C_U16)((P2C_U32)(high) & 0xffff))) <<16))
#define P2C_MAKELONG64(low, high) ((P2C_U64)(((P2C_U32)((P2C_U64)(low) & 0xffffffff)) | \
								((P2C_U64)((P2C_U32)((P2C_U64)(high) & 0xffffffff))) <<32))
#define P2C_LOW16_OF_32(data) ((P2C_U16)(((P2C_U32)data) & 0xffff))
#define P2C_LOW32_OF_64(data) ((P2C_U32)(((P2C_U64)data) & 0xffffffff))
#define P2C_HIGH16_OF_32(data) ((P2C_U16)(((P2C_U32)data) >> 16))
#define P2C_HIGH32_OF_64(data) ((P2C_U32)(((P2C_U64)data) >> 32))
#define P2C_IDTR_OF_ISR64(high, middle, low) ((P2C_U64)( ((P2C_U32)((P2C_U64)(low) & 0xffff)) | \
											((P2C_U64)((P2C_U32)((P2C_U64)(middle) & 0xffff))) <<16 | \
											((P2C_U64)((P2C_U32)((P2C_U64)(high) & 0xffffffff))) <<32 ))

#if _WIN64
#define P2C_MAKELONG P2C_MAKELONG64
#define P2C_IDT_ENTRY P2C_IDT_ENTRY64
#define PP2C_IDT_ENTRY PP2C_IDT_ENTRY64
#define P2C_IDTR P2C_IDTR64
#define PP2C_IDTR PP2C_IDTR64
#define P2C_IDTR_OF_ISR P2C_IDTR_OF_ISR64
#elif _WIN32
#define P2C_MAKELONG P2C_MAKELONG32
#define P2C_IDT_ENTRY P2C_IDT_ENTR32
#define PP2C_IDT_ENTRY PP2C_IDT_ENTRY32
#define P2C_IDTR P2C_IDTR32
#define PP2C_IDTR PP2C_IDTR32
#endif

#pragma pack(push, 1)
typedef struct _P2C_IDTR32
{
	P2C_U16 limit;	// 范围
	P2C_U32 base;	// 基地址
}P2C_IDTR32, *PP2C_IDTR32;

typedef struct _P2C_IDTR64
{
	P2C_U16 limit;	// 范围
	P2C_U64 base;	// 基地址
}P2C_IDTR64, * PP2C_IDTR64;

typedef struct _P2C_IDT_ENTRY32
{
	P2C_U16 offset_low;
	P2C_U16 selector;
	P2C_U8 reserved;
	P2C_U8 type : 4;
	P2C_U8 always0 : 1;
	P2C_U8 dp1 : 2;
	P2C_U8 present : 1;
	P2C_U16 offset_high;
}P2C_IDT_ENTRY32, * PP2C_IDT_ENTRY32;

typedef struct _P2C_IDT_ENTRY64
{
	P2C_U16 offset_low;
	P2C_U16 selector;
	P2C_U16 IstIndex : 3;
	P2C_U16 Reserved0 : 5;
	P2C_U16 type : 5;
	P2C_U16 Dpl : 2;
	P2C_U16 Present : 1;
	P2C_U16 OffsetMiddle;
	P2C_U32 offset_high;
	P2C_U32 Reserved1;
	//P2C_U64 Alignment;
}P2C_IDT_ENTRY64, * PP2C_IDT_ENTRY64;

#pragma pack(pop)


void* GetIdtIsr(unsigned int Index);


//void p2cHookInt93(BOOLEAN hook_or_unhook); // HOOK adn UNHOOK


void _stdcall p2cUserFilter();
ULONG _stdcall p2cWaitForkbRead();
ULONG _stdcall p2cWaitForkbWrite();
#endif // !_CTRL2_IDT_HOOK_H