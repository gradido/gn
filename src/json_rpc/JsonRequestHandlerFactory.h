#ifndef __DR_JSON_REQUEST_HANDLER_FACTORY_H
#define __DR_JSON_REQUEST_HANDLER_FACTORY_H

#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/RegularExpression.h"
#include "gradido_interfaces.h"


#define HTTP_PAGES_COUNT 1

class JsonRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory
{
public:
	JsonRequestHandlerFactory(gradido::IGradidoFacade* _gf);
	Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request);

protected:
	Poco::RegularExpression mRemoveGETParameters;
	gradido::IGradidoFacade* gf;
};

#endif // __DR_JSON_REQUEST_HANDLER_FACTORY_H
