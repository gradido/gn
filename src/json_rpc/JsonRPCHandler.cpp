#include "JsonRPCHandler.h"

#include "blockchain_gradido.h"
#include "gradido_events.h"

JsonRPCHandler::JsonRPCHandler(gradido::IGradidoFacade* _gf)
: gf(_gf)
{


}


void JsonRPCHandler::handle(const jsonrpcpp::Request& request, Json& response)
{
    if(request.method() == "getTransactions") {
        if (request.params().has("groupAlias") && request.params().has("lastKnownSequenceNumber")) {
            auto group_alias = request.params().get<std::string>("groupAlias");
            auto sequence_number = request.params().get<Poco::UInt64>("lastKnownSequenceNumber");
            getTransactions(group_alias, sequence_number, response);

        } else {
			response = { { "state", "error" },{ "msg", "missing parameter" } };
		}
    }
	/*if (request.method == "puttransaction") {
		if (request.params.has("group") && request.params.has("transaction")) {
			auto group = convertBinTransportStringToBin(request.params.get<std::string>("group"));
			printf("transaction: %s\n", request.params.get<std::string>("transaction").data());
			auto transaction = convertBinTransportStringToBin(request.params.get<std::string>("transaction"));
			if ("" == group || "" == transaction) {
				response = { {"state", "error"}, {"msg", "wrong parameter format"} };
			}
			else {
				putTransaction(transaction, group, response);
			}
		}
		else {
			response = { { "state", "error" },{ "msg", "missing parameter" } };
		}
	}
	else if (request.method == "getlasttransaction") {
		response = { {"state" , "success"}, {"transaction", ""} };
	}*/
	else {
		response = { { "state", "error" },{ "msg", "method not known" } };
	}
}

void JsonRPCHandler::getTransactions(
                     const std::string& groupAlias, 
                     Poco::UInt64 lastKnownSequenceNumber, 
                     Json& response) {

    using namespace gradido;

    IBlockchain* blockchain = gf->get_group_blockchain(groupAlias);
    if (0 == blockchain) {
        response = {{"state", "error"}, 
                    {"msg", "group blockchain not found"} };
    } else {
        try {
            int batch_size = gf->get_conf()->get_general_batch_size();
            GetTransactionsTask task(blockchain,
                                     lastKnownSequenceNumber,
                                     batch_size);
            while (!task.is_finished()) {
                gf->push_task_and_join(&task);
            }

            using Record = typename GetTransactionsTask::Record;

            // not optimal for large amount of data; should be streaming
            // that instead to client
            std::stringstream ss;
            ss << "[\n";
            bool is_first = true;
            std::vector<Record>* records = task.get_result();
            for (auto i : *records) {
                if (is_first)
                    is_first = false;
                else 
                    ss << ",\n";
                dump_transaction_in_json<GradidoRecord>(i, ss);
            }
            ss << "]";

            Json contents;
            ss >> contents;
            response["blocks"] = contents;
            response["transaction_count"] = records->size();
            response["state"] = "success";
        } catch (std::exception& e) {
            std::string msg = "get_transactions(): " + 
                std::string(e.what());
            LOG(msg);
            response = {{"state", "error"}, 
                        {"msg", msg} };
        } catch (...) {
            LOG("unknown error");
            gf->exit(1);
        }
    }
}

