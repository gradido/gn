#ifndef GRADIDO_EVENTS_H
#define GRADIDO_EVENTS_H

#include "gradido_interfaces.h"

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

    class ValidateGroupRegister : public ITask {
    private:
        IGradidoFacade* gf;
    public:
        ValidateGroupRegister(IGradidoFacade* gf) : gf(gf) {}
        virtual void run() {
            gf->get_group_register()->continue_validation();
            // group register adds events according to what is needed
            // (it may want to continue, or wait for data from other
            // nodes, or push_task(ContinueFacadeInit))
        }
    };

    class InitGroupRegister : public ITask {
    private:
        IGradidoFacade* gf;
    public:
        InitGroupRegister(IGradidoFacade* gf) : gf(gf) {}
        virtual void run() {
            gf->get_group_register()->init();
            gf->push_task(new ValidateGroupRegister(gf));
        }
    };


}

#endif
