#include "blockchain_gradido.h"

// TODO: remove
#include <sstream>

namespace gradido {

    void GradidoGroupBlockchain::calc_dependent_fields(Transaction& tr) {
        // previous transaction for a member
        // new balance
        // result
        // TODO: finish here
        tr.result = SUCCESS;
    }


    void GradidoGroupBlockchain::update_indexes(Transaction& tr) {
        
    }

    GradidoGroupBlockchain::Validator::Validator() {
    }
    void GradidoGroupBlockchain::Validator::validate(uint64_t rec_num, const Transaction& rec) {
    }

    Transaction GradidoGroupBlockchain::Validator::prepare() {
    }
    
    GradidoGroupBlockchain::GradidoGroupBlockchain(GroupInfo gi,
                                                   std::string root_folder,
                                                   IGradidoFacade* gf) :
        gf(gf),
        topic_id(gi.topic_id),
        blockchain(gi.alias,
                   root_folder,
                   GRADIDO_BLOCK_SIZE,
                   &validator) {
        SAFE_PT(pthread_mutex_init(&queue_lock, 0));
        SAFE_PT(pthread_mutex_init(&blockchain_lock, 0));
    }

    GradidoGroupBlockchain::~GradidoGroupBlockchain() {
        pthread_mutex_destroy(&queue_lock);
        pthread_mutex_destroy(&blockchain_lock);
    }

    HederaTopicID GradidoGroupBlockchain::get_topic_id() {
        return topic_id;
    }

    void GradidoGroupBlockchain::init() {
        blockchain.init();
    }

    IBlockchain::AddTransactionResult GradidoGroupBlockchain::add_transaction(const Transaction& tr) {
        {
            MLock lock(queue_lock); 
            inbound.push(tr);
        }

        return continue_with_transactions();
    }

    IBlockchain::AddTransactionResult GradidoGroupBlockchain::continue_with_transactions() {

        int batch_size = gf->get_conf()->blockchain_append_batch_size();
        BusyGuard bd(*this);
        while (1) {
            Transaction tr;
            {
                MLock lock(queue_lock); 
                pthread_t tt = pthread_self();

                if (inbound.size() == 0) {
                    return IBlockchain::DONE;
                } else {
                    if (is_busy) {
                        if (!pthread_equal(tt, write_owner))
                            return IBlockchain::DONE;
                    } else {
                        bd.start();
                        is_busy = true;
                        write_owner = tt;
                    }
                }
                if (batch_size <= 0)
                    return IBlockchain::TRY_AGAIN;
                tr = inbound.top();
                inbound.pop();
            }

            if (!is_sequence_number_ok(tr)) {
                MLock lock(queue_lock); 
                inbound.push(tr);
                return IBlockchain::TRY_AGAIN_LATER;
            }

            calc_dependent_fields(tr);
            update_indexes(tr);
            append(tr);
            batch_size--;
        }        
    }

    bool GradidoGroupBlockchain::TransactionCompare::operator()(
                    const Transaction& lhs, 
                    const Transaction& rhs) const {

        return lhs.hedera_transaction.sequenceNumber < 
            rhs.hedera_transaction.sequenceNumber;
    }

    bool GradidoGroupBlockchain::is_sequence_number_ok(
                                 const Transaction& tr) {
        MLock lock(blockchain_lock); 
        return true;
        // TODO: finish
    }

    void GradidoGroupBlockchain::append(const Transaction& tr) {
        MLock lock(blockchain_lock); 
        std::stringstream ss;
        dump_transaction_in_json(tr, ss);
        LOG(ss.str());
        blockchain.append(tr);
    }



}
