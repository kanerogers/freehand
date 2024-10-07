#pragma once
#include "FreehandWin.h"
#include "FreehandException.h"
#include "Keyboard.h"
#include "Mouse.h"

#include <optional>

class Window
{
public:
	class Exception : public FreehandException
	{
	public:
		Exception(int line, const char* file, HRESULT hr);
		const char* what() const noexcept override;
		virtual const char* GetType()const noexcept override;
		static std::string TranslateErrorCode(HRESULT hr) noexcept;
		HRESULT GetErrorCode() const noexcept;
		std::string GetErrorString() const noexcept;
	private:
		HRESULT hr;
	};
private:
	// Singleton to manage registration & cleanup of Window class
	class WindowClass
	{
	public:
		static const char* GetName() noexcept;
		static HINSTANCE GetInstance() noexcept;
	private:
		WindowClass() noexcept;
		~WindowClass() noexcept;
		WindowClass(const WindowClass&) = delete;
		WindowClass& operator=(const WindowClass&) = delete;
		static constexpr const char* wndClassName = "Freehand Window";
		static WindowClass wndClass;
		HINSTANCE hInst;
	};
public:
	Window(int width, int height, const char* name);
	~Window();
	Window& operator=(const Window&) = delete;
	void SetTitle(const std::string& title);
	static std::optional<int> ProcessMessages();
private:
	static LRESULT CALLBACK HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
	static LRESULT CALLBACK HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
	bool IsInClientRegion(const POINTS& pt) const;
	LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
public:
	Keyboard keyboard;
	Mouse mouse;
private:
	int width;
	int height;
	HWND hWnd;
};

// Error exception helper macro
#define FHWND_EXCEPT(hr) Window::Exception(__LINE__, __FILE__, hr)
#define FHWND_LAST_EXCEPT(hr) Window::Exception(__LINE__, __FILE__, GetLastError())
