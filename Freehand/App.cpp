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
}
