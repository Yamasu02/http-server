#include "WebServer.h"

void main()
{
	WebServer webServer("0.0.0.0", 8080);

	webServer.Run();

	system("pause");
}