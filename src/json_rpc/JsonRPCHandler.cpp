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
		response = { { "state", "error" },{ "msg", "method not known" }, {"details", request.method()} };
	}
}

void JsonRPCHandler::getTransactions(
                     const std::string& groupAlias,
                     Poco::UInt64 lastKnownSequenceNumber,
                     Json& response) {

    using namespace gradido;

    printf("[JsonRPCHandler::getTransactions] group alias: %s, last know sequence number: %d\n", groupAlias.data(), lastKnownSequenceNumber);

    IBlockchain* blockchain = gf->get_group_blockchain(groupAlias);

    std::string transactions_json_temp;
    if (0 == blockchain) {
        response = {{"state", "error"},
                    {"msg", "group blockchain not found"} };
    } else {
        auto transaction_count = blockchain->get_transaction_count();
        auto block_count = blockchain->get_block_count ();

        printf("[JsonRPCHandler::getTransactions] transaction count: %d, block count: %d\n", transaction_count, block_count);

        try {
            int batch_size = gf->get_conf()->get_general_batch_size();
            GetTransactionsTask task(blockchain,
                                     lastKnownSequenceNumber,
                                     batch_size);
            printf("batch_size: %d\n", batch_size);
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
            printf("[JsonRPCHandler::getTransactions] records count: %d\n", records->size());
            for (auto i : *records) {
                if (is_first)
                    is_first = false;
                else
                    ss << ",\n";
                dump_transaction_in_json<GradidoRecord>(i, ss);
            }
            ss << "]";
            transactions_json_temp = ss.str();
            Json contents = Json::parse(transactions_json_temp);

            response["blocks"] = contents;
            response["transaction_count"] = records->size();
            response["state"] = "success";
        } catch (std::exception& e) {
            std::string msg = "get_transactions(): " +
                std::string(e.what());
            LOG(msg);
            LOG(transactions_json_temp);
            response = {
                {"state", "error"},
                {"msg", msg},
                {"details", transactions_json_temp}

            };
        } catch (...) {
            LOG("unknown error");
            gf->exit(1);
        }
    }
}

