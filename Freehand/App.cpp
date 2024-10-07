#include "App.h"

App::App()
	:
	wnd(800, 600, "An Window")
{}

int App::Go()
{
	while (true)
	{
		// Process messages! If we got a return value, then it's time to quit
		if (const auto eCode = Window::ProcessMessages())
		{
			// Return with the error code
			return *eCode;
		}

		// All messages processed
		DoFrame();
	}
}

void App::DoFrame()
{
	const float c = sin(timer.Peek()) / 2.0f + 0.5f;
	wnd.Gfx().ClearBuffer(c, c, 1.0f);
	wnd.Gfx().EndFrame();
}
