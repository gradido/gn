#include "handshake_handler.h"
#include "gradido_events.h"

#define H1_H2_SETTLE_TIME_SECONDS 1

namespace gradido {

    void HandshakeHandler::SbRecordAdded::on_transaction(
                                          grpr::Transaction& t) {
        if (t.success()) {
            IVersioned* ve = 
                gf->get_versioned(t.version_number());
            if (ve) {
                bool sig_ok = ve->get_response_sig_validator()->
                    create_node_handshake0(t);
                if (sig_ok) {
                    
                    // TODO: check transaction id
                    gf->continue_init_after_handshake_done();
                } else {
                    std::string msg = 
                        "cannot validate signature for " + 
                        t.DebugString();
                    gf->exit(msg);
                }
            } else {
                std::string msg = 
                    "unsupported version_number: " + 
                    t.DebugString();
                gf->exit(msg);
            }
        } else {
            std::string msg = 
                "success == false in a request: " + 
                t.DebugString();
            gf->exit(msg);
        }
    }

    void HandshakeHandler::on_transaction(grpr::Transaction& t) {
        if (t.success()) {
            IVersioned* ve = 
                gf->get_versioned(t.version_number());
            if (ve) {
                bool sig_ok = ve->get_response_sig_validator()->
                    create_node_handshake0(t);
                if (sig_ok) {
                    grpr::Handshake3 h3;
                    if (ve->translate(t, h3)) {
                        if (h3.transaction_id().seconds() == 
                            h3_ts.seconds() &&
                            h3.transaction_id().nanos() == 
                            h3_ts.nanos()) {
                            grpr::Transaction t;
                            if (ve->translate_sb_transaction(
                                           h3.sb_transaction(), t)) {
                                ve->sign(&t);
                                ICommunicationLayer* c = 
                                    gf->get_communications();
                                std::string ep = 
                                    gf->get_sb_ordering_node_endpoint();
                                c->submit_to_blockchain(ep, t, &sbra);
                            } else {
                                std::string msg = 
                                    "h3 bytes is not transaction " + 
                                    t.DebugString();
                                gf->exit(msg);
                            }
                        } else {
                            std::string msg = 
                                "h3 ts doesn't match " + 
                                t.DebugString();
                            gf->exit(msg);
                        }
                    } else {
                        std::string msg = 
                            "cannot acquire Handshake3 from " + 
                            t.DebugString();
                        gf->exit(msg);
                    }
                } else {
                    std::string msg = 
                        "cannot validate signature for " + 
                        t.DebugString();
                    gf->exit(msg);
                }
            } else {
                std::string msg = 
                    "unsupported version_number: " + 
                    t.DebugString();
                gf->exit(msg);
            }
        } else {
            std::string msg = 
                "success == false in a request: " + 
                t.DebugString();
            gf->exit(msg);
        }
    }

    grpr::Transaction HandshakeHandler::get_response_h0(
                      grpr::Transaction req,
                      IVersioned* ve) {
        grpr::Transaction res;
        grpr::Handshake0 h0;
        if (!ve->translate(req, h0))
            gf->exit("cannot translate h0");
        if (!ve->prepare_h1(h0.transaction_id(), res))
            gf->exit("cannot prepare h1");

        ve->sign(&res);

        grpr::Transaction t2;
        std::string starter_endpoint = h0.starter_endpoint();
        h3_ts = get_current_time();
        if (!ve->prepare_h2(h3_ts, t2))
            gf->exit("cannot prepare h2");
        ve->sign(&t2);

        gf->push_task(new SendH2(gf, starter_endpoint, t2, this),
                      H1_H2_SETTLE_TIME_SECONDS);

        return res;
    }

    void StarterHandshakeHandler::on_transaction(grpr::Transaction& t) {
        
    }

    grpr::Transaction StarterHandshakeHandler::get_response_h2(
                      grpr::Transaction req,
                      IVersioned* ve) {
        grpr::Transaction res;
        grpr::Handshake0 h0;
        if (!ve->translate(req, h0))
            gf->exit("cannot translate h0");
        if (!ve->prepare_h1(h0.transaction_id(), res))
            gf->exit("cannot prepare h1");

        ve->sign(&res);

        grpr::Transaction t2;
        proto::Timestamp ts = get_current_time();
        std::string starter_endpoint = h0.starter_endpoint();
        if (!ve->prepare_h2(ts, t2))
            gf->exit("cannot prepare h2");
        ve->sign(&t2);

        gf->push_task(new SendH2(gf, starter_endpoint, t2, this),
                      H1_H2_SETTLE_TIME_SECONDS);

        return res;
    }

}
