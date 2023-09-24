// 键盘过滤，过滤键盘中断消息
#ifndef _CTRL2_IDT_HOOK_H
#define _CTRL2_IDT_HOOK_H

#include <ntddk.h>

typedef unsigned char P2C_U8;
typedef unsigned short P2C_U16;
typedef unsigned long P2C_U32;

#define P2C_MAKELONG(low, high) ((P2C_U32)(((P2C_U16)((P2C_U32)(low) & 0xffff)) | \
								((P2C_U32)((P2C_U16)((P2C_U32)(high) & 0xffff))) <<16))
#define P2C_LOW16_OF_32(data) ((P2C_U16)(((P2C_U32)data) & 0xffff))
#define P2C_HIGH16_OF_32(data) ((P2C_U16)(((P2C_U32)data) >> 16))

#pragma pack(push, 1)
typedef struct P2C_IDTR_
{
	P2C_U16 limit;	// 范围
	P2C_U32 base;	// 基地址
}P2C_IDTR, *PP2C_IDTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct P2C_IDT_ENTRY_
{
	P2C_U16 offset_low;
	P2C_U16 selector;
	P2C_U8 reserved;
	P2C_U8 type : 4;
	P2C_U8 always0 : 1;
	P2C_U8 dp1 : 2;
	P2C_U8 present : 1;
	P2C_U16 offset_high;
}P2C_IDT_ENTRY, * PP2C_IDT_ENTRY;
#pragma pack(pop)


void p2cHookInt93(BOOLEAN hook_or_unhook); // HOOK adn UNHOOK
#endif // !_CTRL2_IDT_HOOK_H