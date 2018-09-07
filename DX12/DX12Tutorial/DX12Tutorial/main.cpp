#include <iostream>
#include <Windows.h>
#include <memory>
#include "Application.h"
using namespace std;

int main()
{
	auto& app = Application::Instance();
	app.Initialize();
	app.Run();
	app.Terminate();

	return 0;
}
