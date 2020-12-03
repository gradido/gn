#include "JsonRPCHandler.h"

#include "../blockchain_gradido.h"

JsonRPCHandler::JsonRPCHandler(gradido::GradidoFacade* _gf)
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

void JsonRPCHandler::getTransactions(const std::string& groupAlias, Poco::UInt64 lastKnownSequenceNumber, Json& response)
{
    auto blockchain = gf->get_group_blockchain(groupAlias);
    if(0 == blockchain) {
        response = {{ "state", "error"}, {"msg", "group blockchain not found"} };
    } else {
        auto transaction_count = blockchain->get_transaction_count();
        auto block_count = blockchain->get_block_count();
        std::vector<Json> block_jsons;
        for(int i = 0; i < block_count; i++) {
            gradido::grpr::BlockRecord proto_record;
            gradido::GradidoRecord gradido_record;
            gradido_record.record_type = gradido::GRADIDO_TRANSACTION;
            gradido::GradidoBlockchainType::Record record;

            auto gradido_blockchain = (gradido::BlockchainGradido*)blockchain;

            if(!blockchain->get_block_record(i, proto_record)) continue;

            //int ex;
            //auto record2 = gradido_blockchain->get_record(i, ex);
            if(!blockchain->get_transaction(i, gradido_record.transaction)) continue;
            //memcpy((char*)&record, proto_record.record().data(), sizeof(gradido::GradidoBlockchainType::Record));

            //printf("record size: %d\n", proto_record.record().size());
            std::stringstream ss;
            gradido::dump_transaction_in_json(gradido_record, ss);
            std::string block_json_string = ss.str();
            Json json_block_record;
            try {
                ss >> json_block_record;
            } catch(const std::exception & ex) {
                //response = {{ "state", "success"}, {"msg", "json exception"}, {"details", ex.what()}, {"json", ss.str()} };
                //return;
                block_jsons.push_back(block_json_string);
                continue;
            }
            block_jsons.push_back(json_block_record);
        }
        response["blocks"] = block_jsons;
        response["transaction_count"] = transaction_count;
        response["block_count"] = block_count;
        response["state"] = "success";

        //response = { { "state", "success"} };
    }
}

