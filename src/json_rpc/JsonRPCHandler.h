#ifndef __JSON_RPC_INTERFACE_JSON_JSON_RPC_HANDLER_
#define __JSON_RPC_INTERFACE_JSON_JSON_RPC_HANDLER_

#include "JsonRequestHandler.h"
#include "gradido_facade.h"

class JsonRPCHandler : public JsonRequestHandler
{
public:
    JsonRPCHandler(gradido::GradidoFacade* _gf);
	void handle(const jsonrpcpp::Request& request, Json& response);

protected:
	//void putTransaction(const std::string& transactionBinary, const std::string& groupPublicBinary, Json& response);
	void getTransactions(const std::string& groupAlias, Poco::UInt64 lastKnownSequenceNumber, Json& response);

	gradido::GradidoFacade* gf;

};

#endif // __JSON_RPC_INTERFACE_JSON_JSON_RPC_HANDLER_
