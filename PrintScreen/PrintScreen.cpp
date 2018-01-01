#include <Windows.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

#define SCREENSHOT_DIR L"screenshots"

WCHAR fileNameBuffer[256];

LPCWSTR GetFileName()
{
	SYSTEMTIME lt;

	GetLocalTime(&lt);

	wsprintfW(fileNameBuffer,
		SCREENSHOT_DIR L"/%d%02d%02d-%02d%02d%02d.%d.png",
		lt.wYear,
		lt.wMonth,
		lt.wDay,
		lt.wHour,
		lt.wMinute,
		lt.wSecond,
		lt.wMilliseconds);

	return fileNameBuffer;
}

class PrintScreen {
private:
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	CLSID encoderClsid;
	HBITMAP hbmScreen = NULL;

public:
	PrintScreen()
	{
		Gdiplus::GdiplusStartup(&this->gdiplusToken, &this->gdiplusStartupInput, NULL);

		if (this->GetEncoderClsid(L"image/png", &this->encoderClsid) == -1)
		{
			MessageBoxW(NULL, L"The PNG encoder is not installed.", L"Failed", MB_OK);
		}
	}

	~PrintScreen()
	{
		DeleteObject(this->hbmScreen);
		Gdiplus::GdiplusShutdown(this->gdiplusToken);
	}

	int ScreenCapture()
	{
		HDC hdcScreen;
		HDC hdcMemDC = NULL;

		hdcScreen = GetDC(NULL);

		hdcMemDC = CreateCompatibleDC(hdcScreen);
		if (!hdcMemDC)
		{
			MessageBoxW(NULL, L"CreateCompatibleDC has failed", L"Failed", MB_OK);
			return -1;
		}

		this->hbmScreen = CreateCompatibleBitmap(hdcScreen, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
		if (!this->hbmScreen)
		{
			MessageBoxW(NULL, L"CreateCompatibleBitmap Failed", L"Failed", MB_OK);
			return -1;
		}

		SelectObject(hdcMemDC, this->hbmScreen);

		if (!BitBlt(hdcMemDC,
			0, 0,
			GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
			hdcScreen,
			0, 0,
			SRCCOPY))
		{
			MessageBoxW(NULL, L"BitBlt has failed", L"Failed", MB_OK);
			return -1;
		}

		DeleteObject(hdcMemDC);
		ReleaseDC(NULL, hdcScreen);

		return 0;
	}

	int SavePNG(LPCWSTR fileName)
	{
		Gdiplus::Status status;
		Gdiplus::Bitmap *image = Gdiplus::Bitmap::FromHBITMAP(this->hbmScreen, NULL);

		status = image->Save(fileName, &this->encoderClsid, NULL);

		delete image;

		return status == Gdiplus::Ok ? 0 : -1;
	}

private:
	int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
	{
		UINT numEncoders = 0;
		UINT size = 0;

		Gdiplus::ImageCodecInfo* encoders = NULL;

		Gdiplus::GetImageEncodersSize(&numEncoders, &size);
		if (size == 0)
		{
			return -1;
		}

		encoders = (Gdiplus::ImageCodecInfo*)(malloc(size));
		if (encoders == NULL)
		{
			return -1;
		}

		Gdiplus::GetImageEncoders(numEncoders, size, encoders);

		for (UINT j = 0; j < numEncoders; ++j)
		{
			if (wcscmp(encoders[j].MimeType, format) == 0)
			{
				*pClsid = encoders[j].Clsid;
				free(encoders);
				return j;
			}
		}

		free(encoders);
		return -1;
	}
};

LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
	if (wParam == WM_KEYUP) {
		KBDLLHOOKSTRUCT hookParam = *((KBDLLHOOKSTRUCT*)lParam);
		if (hookParam.vkCode == VK_SNAPSHOT) {
			PrintScreen* prtscr = new PrintScreen;

			prtscr->ScreenCapture();
			prtscr->SavePNG(GetFileName());

			delete prtscr;
		}
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	CreateDirectoryW(SCREENSHOT_DIR, NULL);

	HHOOK hhk = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)&KeyboardProc, GetModuleHandle(NULL), 0);

	MSG msg;

	while (GetMessage(&msg, nullptr, 0, 0)) {}

	UnhookWindowsHookEx(hhk);

	return (int)msg.wParam;
}
