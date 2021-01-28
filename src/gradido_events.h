#ifndef GRADIDO_EVENTS_H
#define GRADIDO_EVENTS_H

#include "gradido_interfaces.h"
#include "blockchain_gradido_translate.h"
#include "utils.h"

namespace gradido {

    // all events are here, as they depend on each in many cases
    // first ones in system's lifetime comes later in this file

    /*
    class  : public ITask {
    private:
        IGradidoFacade* gf;
    public:
        (IGradidoFacade* gf) : gf(gf) {}
        virtual void run() {

        }
    };
    */

    ///////////////////////////// miscellaneous

    class ReloadConfigTask : public ITask {
    private:
        IGradidoFacade* gf;
    public:
    ReloadConfigTask(IGradidoFacade* gf) : 
        gf(gf) {
        }
        virtual void run() {
            gf->reload_config();
        }
    };

    class AddPairedTask : public ITask {
    private:
        IGradidoFacade* gf;
        grpr::OutboundTransaction br;
        grpr::OutboundTransactionDescriptor req;
    public:
    AddPairedTask(IGradidoFacade* gf, grpr::OutboundTransaction br,
                  grpr::OutboundTransactionDescriptor req) : 
        gf(gf), br(br), req(req) {}

        virtual void run() {
            gf->on_paired_transaction(br, req);
        }
    };

    class TryPairedAgainTask : public ITask {
    private:
        IGradidoFacade* gf;
        grpr::OutboundTransaction br;
        grpr::OutboundTransactionDescriptor req;
    public:
    TryPairedAgainTask(IGradidoFacade* gf, grpr::OutboundTransaction br,
                       grpr::OutboundTransactionDescriptor req) : 
        gf(gf), br(br), req(req) {}

        virtual void run() {
            LOG("waiting for paired transaction to finish");
            HederaTimestamp hti = TransactionUtils::translate_Timestamp_from_pb(req.paired_transaction_id());
            gf->exec_once_paired_transaction_done(hti);
        }
    };

    class StartBlockchainTask : public ITask {
    private:
        IGradidoFacade* gf;
        GroupInfo gi;
    public:
        StartBlockchainTask(IGradidoFacade* gf, GroupInfo gi) : gf(gf), 
            gi(gi) {}
        virtual void run() {
            gf->create_group_blockchain(gi);
        }
    };

    ///////////////////////////// json rpc handlers
    class GetTransactionsTask : public ITask {
    public:
        using Record = typename BlockchainTypes<GradidoRecord>::Record;
    private:
        IBlockchain* b;
        uint64_t first;

        // TODO: should be done differently to allow large sizes
        std::vector<Record> result;
        int batch_size;

        bool finished;

        //        std::exception* last;

    public:
        GetTransactionsTask(IBlockchain* b,
                            uint64_t first,
                            int batch_size) : 
        b(b), first(first),
        batch_size(batch_size), finished(false) {}

        virtual void run() {
            try {
                if (!finished && b &&
                    (uint64_t)result.size() + first < 
                    b->get_transaction_count()) {
                    for (int i = 0; i < batch_size; i++) {
                        uint64_t seq_num = 
                            first + (uint64_t)result.size();
                        Record rec;
                        if (!b->get_transaction2(seq_num, rec)) {
                            finished = true;
                            break;
                        }
                        result.push_back(rec);
                    }
                } else
                    finished = true;
            } catch (std::exception& e) {
                std::string msg = "GetTransactionsTask: " + 
                    std::string(e.what());
                LOG(msg);
                finished = true;
            }
        }

        std::vector<Record>* get_result() {
            return &result;
        }

        bool is_finished() {
            return finished;
        }
    };



    ///////////////////////////// rpc handlers

    class BlockProvider : public ITask {
    private:
        IGradidoFacade* gf;
        grpr::BlockDescriptor* req;
        grpr::BlockRecord* reply;
        ICommunicationLayer::HandlerCb* cb;

        class BlockProviderContext : 
            public ICommunicationLayer::HandlerContext {
        public:
            std::string alias;
            uint32_t records_provided;

            BlockProviderContext(std::string alias) : 
            alias(alias), records_provided(0) {}
        };

    public:
        BlockProvider(IGradidoFacade* gf,
                      grpr::BlockDescriptor* req,
                      grpr::BlockRecord* reply, 
                      ICommunicationLayer::HandlerCb* cb) : 
        gf(gf), req(req), reply(reply), cb(cb) {
            if (!cb->get_ctx()) {
                std::string alias = req->blockchain_id().pub_key();
                cb->set_ctx(new BlockProviderContext(alias));
            }
        }
        virtual void run() {
            BlockProviderContext* ctx = (BlockProviderContext*)cb->get_ctx();
            // TODO: optimize; should not call on each run()
            // TODO: should use proper field for checksums
            IAbstractBlockchain* ab = gf->get_any_blockchain(ctx->alias);
            uint64_t offset = (uint64_t)GRADIDO_BLOCK_SIZE * 
                (uint64_t)req->block_id();
            uint64_t seq_num = (uint64_t)ctx->records_provided + offset;
            ctx->records_provided++;

            if (!ab || cb->get_writes_done() == 
                (uint64_t)GRADIDO_BLOCK_SIZE || 
                !ab->get_block_record(seq_num, *reply)) {
                reply->set_success(false);
            }
            cb->write_ready();
        }
    };

    class ChecksumProvider : public ITask {
    private:
        IGradidoFacade* gf;
        grpr::BlockchainId* req;
        grpr::BlockChecksum* reply;
        ICommunicationLayer::HandlerCb* cb;

        class ChecksumProviderContext : 
            public ICommunicationLayer::HandlerContext {
        public:
            std::string alias;
            uint32_t records_provided;
            std::vector<BlockInfo> bis;

            ChecksumProviderContext(std::string alias) : 
            alias(alias), records_provided(0) {}
        };

    public:
        ChecksumProvider(IGradidoFacade* gf,
                         grpr::BlockchainId* req,
                         grpr::BlockChecksum* reply, 
                         ICommunicationLayer::HandlerCb* cb) : 
        gf(gf), req(req), reply(reply), cb(cb) {
            if (!cb->get_ctx()) {
                std::string alias = req->pub_key();
                IAbstractBlockchain* ab = gf->get_any_blockchain(alias);
                ChecksumProviderContext* ctx = 
                    new ChecksumProviderContext(alias);
                if (ab) 
                    ab->get_checksums(ctx->bis);
                cb->set_ctx(ctx);
            }
        }
        virtual void run() {

            ChecksumProviderContext* ctx = (ChecksumProviderContext*)cb->get_ctx();

            // TODO: optimize; should not call on each run()
            IAbstractBlockchain* ab = gf->get_any_blockchain(ctx->alias);
            
            if (!ab || ctx->records_provided == ctx->bis.size()) {
                reply->set_success(false);
            } else {

                BlockInfo bi = ctx->bis.at(ctx->records_provided++);
                reply->set_success(true);
                reply->set_block_size(bi.size);
                reply->set_checksum(std::string((char*)bi.checksum, 
                                                BLOCKCHAIN_CHECKSUM_SIZE));
            }
            cb->write_ready();
        }
    };

    class OutboundProvider : public ITask {
    private:
        IGradidoFacade* gf;
        grpr::OutboundTransactionDescriptor* req;
        grpr::OutboundTransaction* reply;
        ICommunicationLayer::HandlerCb* cb;

    public:
        OutboundProvider(IGradidoFacade* gf,
                         grpr::OutboundTransactionDescriptor* req,
                         grpr::OutboundTransaction* reply, 
                         ICommunicationLayer::HandlerCb* cb) : 
        gf(gf), req(req), reply(reply), cb(cb) {}

        virtual void run() {

            if (cb->get_writes_done() > 0)
                reply->set_success(false);
            else {
                IBlockchain* ab = gf->get_group_blockchain(req->group());
                HederaTimestamp hti = TransactionUtils::translate_Timestamp_from_pb(req->paired_transaction_id());
                uint64_t seq_num;
                if (ab && ab->get_paired_transaction(hti, seq_num) &&
                    ab->get_block_record(seq_num, *reply))
                    reply->set_success(true);
                else
                    reply->set_success(false);
            }
            cb->write_ready();
        }
    };

    class ManageGroupProvider : public ITask {
    private:
        IGradidoFacade* gf;
        grpr::ManageGroupRequest* req;
        grpr::Ack* reply;
        ICommunicationLayer::HandlerCb* cb;

    public:
        ManageGroupProvider(IGradidoFacade* gf,
                            grpr::ManageGroupRequest* req,
                            grpr::Ack* reply, 
                            ICommunicationLayer::HandlerCb* cb) : 
        gf(gf), req(req), reply(reply), cb(cb) {}

        virtual void run() {
            if (cb->get_writes_done() > 0)
                reply->set_success(false);
            else {
                reply->set_success(true);

                // TODO: remove group
                // TODO: consider race conditions

                IGroupRegisterBlockchain* gr = gf->get_group_register();

                HederaTopicID topic_id;
                if (!gr->get_topic_id(req->group(), topic_id))
                    reply->set_success(false);
                else {
                    reply->set_success(true);

                    GroupInfo gi = GroupInfo::create(req->group(), 
                                                     topic_id);

                    gf->create_group_blockchain(gi);
                }
            }
            cb->write_ready();
        }
    };

    class UsersProvider : public ITask {
    private:
        IGradidoFacade* gf;
        grpr::BlockchainId* req;
        grpr::UserData* reply;
        ICommunicationLayer::HandlerCb* cb;

        class UsersProviderContext : 
            public ICommunicationLayer::HandlerContext {
        public:
            std::vector<std::string> users;
        };

    public:
        UsersProvider(IGradidoFacade* gf,
                            grpr::BlockchainId* req,
                            grpr::UserData* reply, 
                            ICommunicationLayer::HandlerCb* cb) : 
        gf(gf), req(req), reply(reply), cb(cb) {
            if (!cb->get_ctx()) {
                IBlockchain* ab = gf->get_group_blockchain(req->pub_key());
                if (ab) {
                    UsersProviderContext* ctx = new UsersProviderContext();
                    ctx->users = ab->get_users();
                    cb->set_ctx(ctx);
                }
            }
        }

        virtual void run() {
            UsersProviderContext* ctx = (UsersProviderContext*)cb->get_ctx();

            if (!ctx || cb->get_writes_done() >= ctx->users.size())
                reply->set_success(false);
            else {
                reply->set_pubkey(ctx->users.at(cb->get_writes_done()));
                reply->set_success(true);
            }
            cb->write_ready();
        }
    };

    class UsersProviderDeprecated : public ITask {
    private:
        IGradidoFacade* gf;
        grpr::BlockchainId* req;
        grpr::UserData* reply;
        ICommunicationLayer::HandlerCb* cb;

        class UsersProviderContext : 
            public ICommunicationLayer::HandlerContext {
        public:
            std::vector<std::string> users;
        };

    public:
        UsersProviderDeprecated(IGradidoFacade* gf,
                            grpr::BlockchainId* req,
                            grpr::UserData* reply, 
                            ICommunicationLayer::HandlerCb* cb) : 
        gf(gf), req(req), reply(reply), cb(cb) {
            if (!cb->get_ctx()) {
                IBlockchain* ab = gf->get_group_blockchain(req->group());
                if (ab) {
                    UsersProviderContext* ctx = new UsersProviderContext();
                    ctx->users = ab->get_users();
                    cb->set_ctx(ctx);
                }
            }
        }

        virtual void run() {
            UsersProviderContext* ctx = (UsersProviderContext*)cb->get_ctx();

            if (!ctx || cb->get_writes_done() >= ctx->users.size())
                reply->set_success(false);
            else {
                reply->set_pubkey(ctx->users.at(cb->get_writes_done()));
                reply->set_success(true);
            }
            cb->write_ready();
        }
    };

    class TransactionReceiver : public ITask {
    protected:
        IGradidoFacade* gf;
        grpr::Transaction* req;
        grpr::Transaction* reply;
        ICommunicationLayer::HandlerCb* cb;

        // shouldn't throw exceptions, both of those
        virtual bool validate_sig(IVersioned* ve, 
                                  IHandshakeHandler* hh) = 0;
        virtual void process_transaction(IVersioned* ve,
                                         IHandshakeHandler* hh) = 0;

        virtual void do_run(bool log_verbose, bool global_log_verbose,
                            bool handshake_expected) {
            const std::string log_msg = "ready to write " + 
                std::to_string((uint64_t)cb);

            if (log_verbose)
                LOG(log_msg);
            if (global_log_verbose)
                gf->global_log(log_msg);

            *reply = grpr::Transaction();
            reply->set_success(false);
            IVersioned* ve = 0;

            std::string err_msg;

            do {
                if (!req->success() && cb->get_writes_done() == 0) {
                    err_msg = "success == false in a request: ";
                    break;
                }

                IHandshakeHandler* hh = gf->get_handshake_handler(false);
                if (handshake_expected != (hh != 0)) {
                    err_msg = handshake_expected ? 
                        "handshake not ongoing " :
                        "handshake ongoing ";
                    break;
                }

                ve = gf->get_versioned(req->version_number());
                if (!ve) {
                    err_msg = "unsupported version_number: ";
                    break;
                }

                if (!validate_sig(ve, hh) && 
                    cb->get_writes_done() == 0) {
                    // checking signature only for first ready to write
                    err_msg = "cannot validate signature for ";
                    break;
                }

                process_transaction(ve, hh);
            } while (0);

            if (err_msg.length()) {
                err_msg += " " + debug_str(*req);
                if (log_verbose)
                    LOG(err_msg);
                if (global_log_verbose)
                    gf->global_log(err_msg);
            }

            if (ve)
                ve->sign(reply);
            cb->write_ready();
        }

    public:
        TransactionReceiver(IGradidoFacade* gf,
                            grpr::Transaction* req,
                            grpr::Transaction* reply, 
                            ICommunicationLayer::HandlerCb* cb) : 
        gf(gf), req(req), reply(reply), cb(cb) {}

    };

    class Handshake0Receiver : public TransactionReceiver {
    protected:
        virtual bool validate_sig(IVersioned* ve,
                                  IHandshakeHandler* hh) {
            if (cb->get_writes_done() == 0)
                return ve->get_request_sig_validator()->
                    create_node_handshake0(*req);
            return true;
        }
        virtual void process_transaction(IVersioned* ve,
                                         IHandshakeHandler* hh) {
            if (hh && cb->get_writes_done() == 0) {
                LOG("sending h1");
                *reply = hh->get_response_h0(*req, ve);
            }
        }

    public:
        Handshake0Receiver(IGradidoFacade* gf,
                           grpr::Transaction* req,
                           grpr::Transaction* reply, 
                           ICommunicationLayer::HandlerCb* cb) : 
        TransactionReceiver(gf, req, reply, cb) {}

        virtual void run() {
            do_run(true, false, true);
        }
    };

    class FinalizeLauncherAfterHandshake : public ITask {
    private:
        IGradidoFacade* gf;
    public:
        FinalizeLauncherAfterHandshake(IGradidoFacade* gf) : gf(gf) {}
        virtual void run() {
            gf->exit("launcher finished successfully");
        }
    };

    class Handshake2Receiver : public TransactionReceiver {
    protected:
        virtual bool validate_sig(IVersioned* ve,
                                  IHandshakeHandler* hh) {
            if (cb->get_writes_done() == 0)
                return ve->get_request_sig_validator()->
                    create_node_handshake2(*req);
            return true;
        }
        virtual void process_transaction(IVersioned* ve,
                                         IHandshakeHandler* hh) {
            if (hh && cb->get_writes_done() == 0) {
                *reply = hh->get_response_h2(*req, ve);
                // exiting after 1 second, as things are done
                gf->push_task(new FinalizeLauncherAfterHandshake(gf), 1);
            }
        }

    public:
        Handshake2Receiver(IGradidoFacade* gf,
                           grpr::Transaction* req,
                           grpr::Transaction* reply, 
                           ICommunicationLayer::HandlerCb* cb) : 
        TransactionReceiver(gf, req, reply, cb) {}

        virtual void run() {
            do_run(true, false, true);
        }
    };

    class LogMessageReceiver : public TransactionReceiver {
    protected:
        virtual bool validate_sig(IVersioned* ve,
                                  IHandshakeHandler* hh) {
            if (cb->get_writes_done() == 0)
                return ve->get_request_sig_validator()->
                    submit_log_message(*req);
            else 
                return true;
        }
        virtual void process_transaction(IVersioned* ve,
                                         IHandshakeHandler* hh) {
            if (cb->get_writes_done() == 0) {
                std::string msg;
                ve->translate_log_message(*req, msg);
                LOG("---" << msg);
                reply->set_success(true);
            }
        }

    public:
        LogMessageReceiver(IGradidoFacade* gf,
                           grpr::Transaction* req,
                           grpr::Transaction* reply, 
                           ICommunicationLayer::HandlerCb* cb) : 
        TransactionReceiver(gf, req, reply, cb) {}

        virtual void run() {
            do_run(false, false, false);
        }
    };

    class SubscribeToBlockchainReceiver : public TransactionReceiver {
    protected:
        virtual bool validate_sig(IVersioned* ve,
                                  IHandshakeHandler* hh) {
            // TODO: move up
            if (cb->get_writes_done() == 0)
                return ve->get_request_sig_validator()->
                    submit_message(*req);
            else 
                return true;
        }
        virtual void process_transaction(IVersioned* ve,
                                         IHandshakeHandler* hh) {
            if (!hh) {
                /*
                grpr::OrderingRequest ore;
                if (ve->translate(*req, ore)) {
                    grpr::Transaction t;
                    t.set_version_number(req->version_number());
                    t.set_success(gf->ordering_broadcast(ore, ve));
                    ve->sign(&t);
                } else {
                    LOG("cannot translate into OrderingRequest");
                }
                */
            }
        }

    public:
        SubscribeToBlockchainReceiver(
                             IGradidoFacade* gf,
                             grpr::Transaction* req,
                             grpr::Transaction* reply, 
                             ICommunicationLayer::HandlerCb* cb) : 
        TransactionReceiver(gf, req, reply, cb) {}

        virtual void run() {
            do_run(false, false, false);
        }
    };

    class OrderingMessageReceiver : public TransactionReceiver {
    protected:
        virtual bool validate_sig(IVersioned* ve,
                                  IHandshakeHandler* hh) {
            if (cb->get_writes_done() == 0)
                return ve->get_request_sig_validator()->
                    submit_message(*req);
            else 
                return true;
        }
        virtual void process_transaction(IVersioned* ve,
                                         IHandshakeHandler* hh) {
            if (!hh) {
                grpr::OrderingRequest ore;
                if (ve->translate(*req, ore)) {
                    grpr::Transaction t;
                    t.set_version_number(req->version_number());
                    // TODO
                    //t.set_success(gf->ordering_broadcast(ve, ore));
                    ve->sign(&t);
                } else {
                    LOG("cannot translate into OrderingRequest");
                }
            }
        }

    public:
        OrderingMessageReceiver(IGradidoFacade* gf,
                                grpr::Transaction* req,
                                grpr::Transaction* reply, 
                                ICommunicationLayer::HandlerCb* cb) : 
        TransactionReceiver(gf, req, reply, cb) {}

        virtual void run() {
            do_run(false, false, false);
        }
    };

    class PingReceiver : public ITask {
    private:
        IGradidoFacade* gf;
        grpr::Transaction* req;
        grpr::Transaction* reply;
        ICommunicationLayer::HandlerCb* cb;

    public:
        PingReceiver(IGradidoFacade* gf,
                     grpr::Transaction* req,
                     grpr::Transaction* reply, 
                     ICommunicationLayer::HandlerCb* cb) : 
        gf(gf), req(req), reply(reply), cb(cb) {}

        virtual void run() {
            // TODO: send to stderr
            reply->set_success(cb->get_writes_done() == 0);
            cb->write_ready();
        }
    };

    class SendH2 : public ITask {
    private:
        IGradidoFacade* gf;
        std::string starter_endpoint;
        grpr::Transaction h2;
        ICommunicationLayer::GrprTransactionListener* tl;
    public:
        SendH2(IGradidoFacade* gf, std::string starter_endpoint,
               grpr::Transaction h2,
               ICommunicationLayer::GrprTransactionListener* tl) : gf(gf),
            starter_endpoint(starter_endpoint), h2(h2), tl(tl) {}
        virtual void run() {
            LOG("sending handshake2");
            gf->get_communications()->send_handshake2(starter_endpoint,
                                                      h2,
                                                      tl);
        }        
    };

    class SendH0 : public ITask {
    private:
        IGradidoFacade* gf;
        std::string endpoint;
        ICommunicationLayer::GrprTransactionListener* tl;
    public:
        SendH0(IGradidoFacade* gf, std::string endpoint,
               ICommunicationLayer::GrprTransactionListener* tl) : gf(gf),
            endpoint(endpoint), tl(tl) {}
        virtual void run() {
            LOG("sending handshake0");
            IVersioned* ve = gf->get_versioned(DEFAULT_VERSION_NUMBER);
            grpr::Transaction h0;
            // TODO: store in hh
            proto::Timestamp ts = get_current_time();
            IGradidoConfig* conf = gf->get_conf();
            std::string ep = conf->get_grpc_endpoint();
            ve->prepare_h0(ts, h0, ep);
            ve->sign(&h0);
            gf->get_communications()->send_handshake0(endpoint,
                                                      h0,
                                                      tl);
        }        
    };


    ///////////////////////////// abstract blockchain

    template<typename T>
    class PushTransactionsTask : public ITask {
    private:
        ITypedBlockchain<T>* b;
        ConsensusTopicResponse transaction;
    public:
        PushTransactionsTask(ITypedBlockchain<T>* b, 
                             const ConsensusTopicResponse& transaction) : 
        b(b), transaction(transaction) {}
        virtual void run() {
            Batch<T> batch;
            TransactionUtils::translate_from_ctr(transaction, batch);
            b->add_transaction(batch);

            // TODO: stop transmission if bad
        }
    };

    template<typename T>
    class PushGrprTransactionsTask : public ITask {
    private:
        IGradidoFacade* gf;
        ITypedBlockchain<T>* b;
        grpr::Transaction transaction;
    public:
        PushGrprTransactionsTask(IGradidoFacade* gf,
                                 ITypedBlockchain<T>* b, 
                                 const grpr::Transaction& transaction) : 
        gf(gf), b(b), transaction(transaction) {}
        virtual void run() {
            Batch<T> batch;

            IVersioned* ve = gf->get_versioned(
                             transaction.version_number());
            ve->translate(transaction, batch);
            b->add_transaction(batch);

            // TODO: stop transmission if bad
        }
    };

    template<typename T>
    class ContinueTransactionsTask : public ITask {
    private:
        ITypedBlockchain<T>* b;
    public:
        ContinueTransactionsTask(ITypedBlockchain<T>* b) : 
        b(b) {}
        virtual void run() {
            b->continue_with_transactions();
        }
    };

    template<typename T>
    class ContinueValidationTask : public ITask {
    private:
        ITypedBlockchain<T>* b;
    public:
        ContinueValidationTask(ITypedBlockchain<T>* b) : 
        b(b) {}
        virtual void run() {
            b->continue_validation();
        }
    };

    ///////////////////////////// group blockchains
    class InitGroupBlockchain : public ITask {
    private:
        IGradidoFacade* gf;
        IBlockchain* b;
    public:
        InitGroupBlockchain(IGradidoFacade* gf, 
                            IBlockchain* b) : gf(gf), b(b) {}
        virtual void run() {
            b->init();
            gf->push_task(new ContinueValidationTask<GradidoRecord>(b));
        }
    };


    ///////////////////////////// group register

    class ContinueFacadeInit : public ITask {
    private:
        IGradidoFacade* gf;
    public:
        ContinueFacadeInit(IGradidoFacade* gf) : gf(gf) {}
        virtual void run() {
            gf->continue_init_after_group_register_done();
        }
    };

    class ContinueFacadeInitAfterSbDone : public ITask {
    private:
        IGradidoFacade* gf;
    public:
        ContinueFacadeInitAfterSbDone(IGradidoFacade* gf) : gf(gf) {}
        virtual void run() {
            gf->continue_init_after_sb_done();
        }
    };

    class InitGroupRegister : public ITask {
    private:
        IGradidoFacade* gf;
    public:
        InitGroupRegister(IGradidoFacade* gf) : gf(gf) {}
        virtual void run() {
            IGroupRegisterBlockchain* gr = gf->get_group_register();
            gr->init();
            gf->push_task(new ContinueValidationTask<GroupRegisterRecord>(gr));
        }
    };


}

#endif
