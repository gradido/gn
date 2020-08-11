#include "gradido_facade.h"
#include "utils.h"
#include "blockchain_gradido.h"


namespace gradido {

    class StartBlockchainTask : public ITask {
    private:
        IGradidoFacade* gf;
    public:
        StartBlockchainTask(IGradidoFacade* gf) : gf(gf) {
        }
        virtual void run() {
            IBlockchain* b = 0;
            while ((b = gf->pop_to_initialize_blockchain()) != 0) {
                b->init();
            }
        }
    };

    class AttemptToContinueBlockchainTask : public ITask {
    private:
        IBlockchain* b;
        IGradidoFacade* gf;
    public:
        AttemptToContinueBlockchainTask(IBlockchain* b, IGradidoFacade* gf) : b(b), gf(gf) {
        }
        virtual void run() {
            if (b->continue_with_transactions() == IBlockchain::TRY_AGAIN) 
                gf->push_task(new AttemptToContinueBlockchainTask(b, gf));
        }
    };
    
    class ProcessTransactionTask : public ITask {
    private:
        IBlockchain* b;
        IGradidoFacade* gf;
        // TODO: optimize with move
        ConsensusTopicResponse transaction;
    public:
        ProcessTransactionTask(IBlockchain* b, IGradidoFacade* gf,
                               const ConsensusTopicResponse& transaction) :
            b(b), gf(gf), transaction(transaction) {
        }
        virtual void run() {
            Transaction t = TransactionUtils::translate_from_pb(transaction);
            if (b->add_transaction(t) == IBlockchain::TRY_AGAIN)
                gf->push_task(new AttemptToContinueBlockchainTask(b, gf));
        }
    };

    class GradidoTransactionListener : public CommunicationLayer::TransactionListener {
    private:
        IBlockchain* b;
        IGradidoFacade* gf;
    public:
        GradidoTransactionListener(IBlockchain* b, IGradidoFacade* gf) : 
            b(b), gf(gf) {
        }
        virtual void on_transaction(ConsensusTopicResponse& transaction) {
            gf->push_task(new ProcessTransactionTask(b, gf, transaction));
        }

    };

    GradidoFacade::GradidoFacade() {
        SAFE_PT(pthread_mutex_init(&main_lock, 0));
    }
    GradidoFacade::~GradidoFacade() {
        pthread_mutex_destroy(&main_lock);
        // TODO: delete blockchains
    }

    void GradidoFacade::init() {
        config.init();
        worker_pool.init(config.get_worker_count());
        communication_layer.init(config.get_io_worker_count());


        std::string data_root = config.get_data_root_folder();
        // TODO: multiple endpoints
        std::string endpoint = config.get_hedera_mirror_endpoint();
        for (auto i : config.get_group_info_list()) {
            IBlockchain* ch = new GradidoGroupBlockchain(i, data_root, this);
            blockchains.insert(std::pair<uint64_t, IBlockchain*>(i.group_id, ch));
            uninitialized_gis.push(ch);
            std::shared_ptr<GradidoTransactionListener> gtl =
                std::make_shared<GradidoTransactionListener>(ch, this);
            communication_layer.receive_gradido_transactions(
                          endpoint,
                          i.topic_id,
                          gtl);
        }

        for (int i = 0; i < config.get_number_of_concurrent_blockchain_initializations(); i++) {
            ITask* task = new StartBlockchainTask(this);
            worker_pool.push(task);
        }
    }

    void GradidoFacade::join() {
        worker_pool.join();
    }

    IBlockchain* GradidoFacade::pop_to_initialize_blockchain() {
        MLock lock(main_lock);
        if (uninitialized_gis.size() == 0)
            return 0;
        IBlockchain* res = uninitialized_gis.top();
        uninitialized_gis.pop();
        return res;
    }

    IBlockchain* GradidoFacade::get_group_blockchain(uint64_t group_id) {
        auto ii = blockchains.find(group_id);
        if (ii == blockchains.end()) 
            return 0;
        return ii->second;
    }

    void GradidoFacade::push_task(ITask* task) {
        worker_pool.push(task);
    }

    IGradidoConfig* GradidoFacade::get_conf() {
        return &config;
    }
    

}


