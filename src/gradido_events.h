#ifndef GRADIDO_EVENTS_H
#define GRADIDO_EVENTS_H

#include "gradido_interfaces.h"
#include "blockchain_gradido_translate.h"

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
                std::string alias = req->group().group();
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
        grpr::GroupDescriptor* req;
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
                         grpr::GroupDescriptor* req,
                         grpr::BlockChecksum* reply, 
                         ICommunicationLayer::HandlerCb* cb) : 
        gf(gf), req(req), reply(reply), cb(cb) {
            if (!cb->get_ctx()) {
                std::string alias = req->group();
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
        grpr::ManageGroupResponse* reply;
        ICommunicationLayer::HandlerCb* cb;

    public:
        ManageGroupProvider(IGradidoFacade* gf,
                            grpr::ManageGroupRequest* req,
                            grpr::ManageGroupResponse* reply, 
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
        grpr::GroupDescriptor* req;
        grpr::UserData* reply;
        ICommunicationLayer::HandlerCb* cb;

        class UsersProviderContext : 
            public ICommunicationLayer::HandlerContext {
        public:
            std::vector<std::string> users;
        };

    public:
        UsersProvider(IGradidoFacade* gf,
                            grpr::GroupDescriptor* req,
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
