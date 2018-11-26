#include "Application.h"

int main() {
	auto& app = Application::Instance();
	app.Initialize();
	app.Run();
	app.Terminate();
	return 0;
}