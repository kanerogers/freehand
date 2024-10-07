#pragma once
#include "Window.h"

class App
{
public:
	App();
	// Frame / Message loop
	int Go();
private:
	void DoFrame();
private:
	Window wnd;
};
