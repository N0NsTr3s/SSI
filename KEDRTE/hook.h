#pragma once
#include "Memory.h"




typedef HBRUSH(*NtGdiSelectBrush_t)(_In_ HDC hdc, _In_ HBRUSH hbr);

typedef BOOL(*NtPatBlt_t)(_In_ HDC,
	_In_ int x,
	_In_ int y,
	_In_ int w,
	_In_ int z,
	_In_ DWORD);

typedef HDC(*NtUserGetDC_t)(HWND hWnd);

typedef HBRUSH(*NtGdiCreateSolidBrush_t)(_In_ COLORREF crColor, _In_opt_ HBRUSH hbr);


typedef int(*ReleaseDC_t)(HDC hdC);

typedef BOOL(*DeleteObjectApp_t)(HANDLE hObj);

bool call_kernel_function(void* kernle_function_address);

NTSTATUS hook_handler(PVOID called_param);

namespace nullhook
{
	bool call_kernel_function(void* kernel_function_address);
	NTSTATUS hook_handler(PVOID called_param);
	INT FrameRect(HDC hDC, CONST RECT* lprc, HBRUSH hbr, int thickness);

}