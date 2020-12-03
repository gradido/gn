#include "gradido_facade.h"
#include <iostream>
#include <string>
#include <vector>
#include "gradido_signals.h"

#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/HTTPServer.h"
#include "json_rpc/JsonRequestHandlerFactory.h"


using namespace gradido;

int main(int argc, char** argv) {
    GradidoFacade gf;
    GradidoSignals::init(&gf);
    std::vector<std::string> params;

    for (int i = 0; i < argc; i++)
        params.push_back(std::string(argv[i]));

    try {
        LOG("starting gradido node");
        gf.init(params);
        Poco::Net::ServerSocket jsonrpc_svs(13702);
		Poco::Net::HTTPServer jsonrpc_srv(new JsonRequestHandlerFactory(&gf), jsonrpc_svs, new Poco::Net::HTTPServerParams);

		// start the json server
		jsonrpc_srv.start();

        gf.join();
        jsonrpc_srv.stop();
    } catch (Poco::Exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 2;
    }
}
