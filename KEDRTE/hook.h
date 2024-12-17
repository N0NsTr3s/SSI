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

//Not used
typedef COLORREF(*GreSetTextColor_t)(HDC hdc,COLORREF color);

typedef int(*ReleaseDC_t)(HDC hdC);

typedef BOOL(*DeleteObjectApp_t)(HANDLE hObj);

bool call_kernel_function(void* kernle_function_address);

NTSTATUS hook_handler(PVOID called_param);

//Not used
typedef int(*SetBkMode_t)(
	HDC hdc,
	int mode
);


typedef BOOL(*NtGdiExtTextOutW_t) (
	HDC          hdc,
	INT          x,
	INT          y,
	UINT         flags,
	const RECT* lprect,
	const LPWSTR str,
	UINT         count,
	const INT* lpDx,
	DWORD        cp
	);

BOOL extTextOutA(HDC hdc, INT x, INT y, UINT fuOptions, RECT* lprc, LPCSTR lpString, UINT cch, INT* lpDx);
BOOL extTextOutW(HDC hdc, INT x, INT y, UINT fuOptions, RECT* lprc, LPWSTR lpString, UINT cwc, INT* lpDx);
namespace nullhook
{

	bool call_kernel_function(void* kernel_function_address);
	NTSTATUS hook_handler(PVOID called_param);
	INT FrameRect(HDC hDC, CONST RECT* lprc, HBRUSH hbr, int thickness);

}