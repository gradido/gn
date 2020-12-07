#include "JsonRequestHandlerFactory.h"

#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/DateTime.h"

#include "JsonRPCHandler.h"

#include <sstream>

using namespace gradido;

JsonRequestHandlerFactory::JsonRequestHandlerFactory(gradido::IGradidoFacade* _gf)
	: mRemoveGETParameters("^/([a-zA-Z0-9_-]*)"), gf(_gf)
{
}

Poco::Net::HTTPRequestHandler* JsonRequestHandlerFactory::createRequestHandler(const Poco::Net::HTTPServerRequest& request)
{
	//std::string uri = request.getURI();
	//std::string url_first_part;
	//std::stringstream logStream;

	//mRemoveGETParameters.extract(uri, url_first_part);

	//std::string dateTimeString = Poco::DateTimeFormatter::format(Poco::DateTime(), "%d.%m.%y %H:%M:%S");
	//logStream << dateTimeString << " call " << uri;

	//mLogging.information(logStream.str());

	return new JsonRPCHandler(gf);



	//return new JsonUnknown;
}
