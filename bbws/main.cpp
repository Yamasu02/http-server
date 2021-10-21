//#include "WebServer.h"
//#include "TcepListenerIOCP.h"
//#include "ClientThreadSplit.h"
#include "TcpListener.h"

void main()
{
	//TcpListenerIOCP xt("127.0.0.1", 8080);
	
	//TcpListenerIOCP xd("127.0.0.1", 8080);
	//xd.ListeningLoop();
	//TcpListenerClientMT xd("127.0.0.1", 8080);
	//xd.ListeningLoop();
	//WebServer webServer("0.0.0.0", 8080);
	//webServer.Run();

	TcpListener tcp("127.0.0.1", 8080);
	tcp.Run();
}