#include "Window.h"
#include <sstream>

Window::WindowClass Window::WindowClass::wndClass;

Window::WindowClass::WindowClass() noexcept
	:
	hInst(GetModuleHandle(nullptr))
{
	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = HandleMsgSetup;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetInstance();
	wc.hIcon = nullptr;
	wc.hCursor = nullptr;
	wc.lpszClassName = GetName();
	wc.hIconSm = nullptr;
	RegisterClassEx(&wc);
}

Window::WindowClass::~WindowClass() noexcept
{
	UnregisterClass(wndClassName, GetInstance());
}

const char* Window::WindowClass::GetName() noexcept
{
	return wndClassName;
}

HINSTANCE Window::WindowClass::GetInstance() noexcept
{
	return wndClass.hInst;
}

Window::Window(int width, int height, const char* name)
	:
	width(width),
	height(height)
{
	// calculate window size based on desired client region size
	RECT wr = { 0 };
	wr.left = 100;
	wr.right = width + wr.left;
	wr.top = 100;
	wr.bottom = height + wr.top;

	if (AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, FALSE) == 0) {
		throw FHWND_LAST_EXCEPT();
	}

	// create window and get a handle
	hWnd = CreateWindow(
		WindowClass::GetName(),
		name,
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wr.right - wr.left,
		wr.bottom - wr.top,
		nullptr,
		nullptr,
		WindowClass::GetInstance(),
		this
	);

	// Check for error
	if (hWnd == nullptr)
	{
		throw FHWND_LAST_EXCEPT();
	}

	// sho that windo
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	pGfx = std::make_unique<Graphics>(hWnd);
}

Window::~Window()
{
	DestroyWindow(hWnd);
}

void Window::SetTitle(const std::string& title)
{
	if (SetWindowText(hWnd, title.c_str()) == 0)
	{
		throw FHWND_LAST_EXCEPT();
	}
}

// Non-blocking event pump! Hooray.
std::optional<int> Window::ProcessMessages()
{
	MSG msg;

	// If there's messages in the queue, remove and dispatch them
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		// Unlike GetMessage, PeekMessage doesn't signal quit through its return value,
		// so we need to explicitly check for quit messages
		if (msg.message == WM_QUIT)
		{
			// Magic conversion to optional..
			return (int)msg.wParam;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// If we're not quitting, just return an empty optional
	return {};
}

Graphics& Window::Gfx()
{
	return *pGfx;
}

LRESULT CALLBACK Window::HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	// If we got a create message, do some *shennanigans*
	if (msg == WM_NCCREATE)
	{
		// extract pointer to our Window instance
		const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		Window* const pWnd = static_cast<Window*>(pCreate->lpCreateParams);

		// set WinAPI-managed user data to store a pointer back to our Window instance
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));

		// and now update our WNDPROC to point to our "thunk" shim
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Window::HandleMsgThunk));

		// finally, forward this message to our message instance
		return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
	}

	// If for some reason we received a message before WM_NCCREATE, just use the default handler
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

// Shim between Win32 and our Window instance
LRESULT CALLBACK Window::HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	// Get a pointer to our Window instance
	Window* const pWnd = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	// And forward the message to it
	return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
}

bool Window::IsInClientRegion(const POINTS& pt) const
{
	return pt.x >= 0 && pt.x <= width && pt.y >= 0 && pt.y < height;
}

LRESULT Window::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	switch (msg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	case WM_KILLFOCUS:
		keyboard.ClearState();
		break;
	
	/*** KEYBOARD MESSAGES ***/
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (!(lParam & 0x40000000 || keyboard.AutorepeatIsEnabled()))
		{
			keyboard.OnKeyPressed(static_cast<unsigned char>(wParam));
		}
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		keyboard.OnKeyReleased(static_cast<unsigned char>(wParam));
		break;
	case WM_CHAR:
		keyboard.OnChar(static_cast<unsigned char>(wParam));
		break;
	/*** END KEYBOARD MESSAGES ***/
	/*** MOUSE MESSAGES ***/
	case WM_MOUSEMOVE:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		// If we're in the "client region", raise a move event
		if (IsInClientRegion(pt))
		{ 
			mouse.OnMouseMove(pt.x, pt.y);
			
			// If the mouse wasn't previously in the window, mark it as such and capture it
			if (!mouse.IsInWindow())
			{ 
				SetCapture(hWnd);
				mouse.OnMouseEnter();
			}
		}
		// Otherwise, only raise a move event if a button is down
		else
		{
			if (wParam & (MK_LBUTTON | MK_RBUTTON))
			{ 
				mouse.OnMouseMove(pt.x, pt.y);
			}
			// No buttons held, release capture and log that the mouse has left
			else
			{
				ReleaseCapture();
				mouse.OnMouseLeave();
			}
		}

		break;
	}
	case WM_LBUTTONDOWN:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		mouse.OnLeftPressed(pt.x, pt.y);
		break;
	}
	case WM_RBUTTONDOWN:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		mouse.OnRightPressed(pt.x, pt.y);
		break;
	}
	case WM_LBUTTONUP:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		mouse.OnLeftReleased(pt.x, pt.y);

		// Release mouse if this event was outside the client area
		if (!IsInClientRegion(pt))
		{
			ReleaseCapture();
			mouse.OnMouseLeave();
		}
		break;
	}
	case WM_RBUTTONUP:
	{
	const POINTS pt = MAKEPOINTS(lParam);
		mouse.OnRightReleased(pt.x, pt.y);

		// Release mouse if this event was outside the client area
		if (!IsInClientRegion(pt))
		{
			ReleaseCapture();
			mouse.OnMouseLeave();
		}
		break;
	}
	case WM_MOUSEWHEEL:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
		mouse.OnWheelDelta(pt.x, pt.y, delta);
		break;
	}
	/*** END MOUSE MESSAGES ***/
	}

	// Fallback to DefWindowProc if unhandled
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

Window::Exception::Exception(int line, const char* file, HRESULT hr)
	:
	FreehandException(line, file),
	hr(hr)
{}

const char* Window::Exception::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< "[Error Code] " << GetErrorCode() << std::endl
		<< "[Description] " << GetErrorString() << std::endl
		<< GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}

const char* Window::Exception::GetType() const noexcept
{
	return "Freehand Window Exception";
}

std::string Window::Exception::TranslateErrorCode(HRESULT hr) noexcept
{
	char* pMsgBuf = nullptr;
	// With the FORMAT_MESSAGE_ALLOCATE_BUFFER flag we're asking Windows to allocate a buffer for us
	DWORD nMsgLen = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&pMsgBuf), 0, nullptr
	);

	// Check to make sure the message was formatted correctly
	if (nMsgLen == 0)
	{
		return "Unidentified error code";
	}

	// Copy error string from the Windows allocated buffer into a std::string
	std::string errorString = pMsgBuf;

	// Free the Windows buffer
	LocalFree(pMsgBuf);

	return errorString;
}

HRESULT Window::Exception::GetErrorCode() const noexcept
{
	return hr;
}

std::string Window::Exception::GetErrorString() const noexcept
{
	return TranslateErrorCode(hr);
}
