#include "handshake_handler.h"
#include "gradido_events.h"

#define H1_H2_SETTLE_TIME_SECONDS 1

namespace gradido {

    grpr::Transaction HandshakeHandler::get_h3_signed_contents() {
        return h3_signed;
    }


    void HandshakeHandler::SbRecordAdded::on_transaction(
                                          grpr::Transaction& t) {
        std::string err_msg;

        do {
            if (!t.success()) {
                err_msg = "success == false in a request";
                break;
            }

            IVersioned* ve = 
                gf->get_versioned(t.version_number());
            if (!ve) {
                err_msg = "unsupported version_number";
                break;
            }

            bool sig_ok = ve->get_response_sig_validator()->
                create_node_handshake0(t);
            if (!sig_ok) {
                err_msg = "cannot validate signature";
                break;
            }

            // TODO: check transaction id
            gf->continue_init_after_handshake_done();
        } while (0);

        if (err_msg.length()) {
            err_msg += " " + debug_str(t);
            gf->exit(err_msg);
        }        
    }

    void HandshakeHandler::on_transaction(grpr::Transaction& t) {
        std::string err_msg;

        do {
            if (h2_complete) {
                if (t.success())
                    err_msg = "handshake expected to have single h3 response";
                break;
            }

            if (!t.success()) {
                err_msg = "success == false in a request";
                break;
            }

            IVersioned* ve = 
                gf->get_versioned(t.version_number());
            if (!ve) {
                err_msg = "unsupported version_number";
                break;
            }

            bool sig_ok = ve->get_response_sig_validator()->
                create_node_handshake2(t);
            if (!sig_ok) {
                err_msg = "cannot validate signature";
                break;
            }

            grpr::Handshake3 h3;
            if (!ve->translate(t, h3)) {
                err_msg = "cannot acquire Handshake3";
                break;
            }

            /* TODO: fix
            if (!(h3.transaction_id().seconds() == 
                  h3_ts.seconds() &&
                  h3.transaction_id().nanos() == 
                  h3_ts.nanos())) {
                err_msg = "h3 ts doesn't match";
                break;
            }
            */

            grpr::Transaction t2;
            if (!ve->translate_sb_transaction(h3.sb_transaction(), 
                                              t2)) {
                err_msg = "h3 bytes is not transaction";
                break;
            }
            
            ve->sign(&t);
            h2_complete = true;

            if (gf->get_conf()->is_sb_host()) {
                LOG("appending own entry to subcluster blockchain");
                h3_signed = t2;                
                gf->continue_init_after_handshake_done();
            } else {
                LOG("submitting own entry to subcluster blockchain");
                ICommunicationLayer* c = 
                    gf->get_communications();
                std::string ep = 
                    gf->get_sb_ordering_node_endpoint();
                c->submit_to_blockchain(ep, t2, &sbra);
            }
        } while (0);

        if (err_msg.length()) {
            err_msg += " " + debug_str(t);
            gf->exit(err_msg);
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
        res.set_success(true);
        ve->sign(&res);

        grpr::Transaction t2;
        std::string starter_endpoint = h0.starter_endpoint();
        h3_ts = get_current_time();
        std::string type = gf->get_node_type_str();
        if (!ve->prepare_h2(h3_ts, type, t2))
            gf->exit("cannot prepare h2");
        ve->sign(&t2);

        gf->push_task(new SendH2(gf, starter_endpoint, t2, this),
                      H1_H2_SETTLE_TIME_SECONDS);

        return res;
    }

    void StarterHandshakeHandler::on_transaction(grpr::Transaction& t) {
        std::string err_msg;

        do {
            if (h0_complete) {
                if (t.success())
                    err_msg = "handshake expected to have single h1 response";
                break;
            }


            if (!t.success()) {
                err_msg = "success == false in a request";
                break;
            }

            IVersioned* ve = 
                gf->get_versioned(t.version_number());
            if (!ve) {
                err_msg = "unsupported version_number";
                break;
            }

            bool sig_ok = ve->get_response_sig_validator()->
                create_node_handshake0(t);
            if (!sig_ok) {
                err_msg = "cannot validate signature";
                break;
            }

            grpr::Handshake1 h1;
            if (!ve->translate(t, h1)) {
                err_msg = "cannot acquire Handshake1";
                break;
            }

            // TODO: check if transaction_id is equal
            // TODO: check if pub_key is contained in sig_map
            h0_complete = true;

        } while (0);

        if (err_msg.length()) {
            err_msg += " " + debug_str(t);
            gf->exit(err_msg);
        }        
    }

    grpr::Transaction StarterHandshakeHandler::get_response_h2(
                      grpr::Transaction req,
                      IVersioned* ve) {

        if (!h0_complete)
            gf->exit("h0 not yet completed");

        grpr::Handshake2 h2;
        if (!ve->translate(req, h2))
            gf->exit("cannot translate h2");

        grpr::Transaction sb_transaction;
        if (!ve->prepare_add_node(h2.transaction_id(),
                                  h2.type(),
                                  sb_transaction))
            gf->exit("cannot prepare sb_transaction");
        ve->sign(&sb_transaction);

        grpr::Transaction t2;
        if (!ve->prepare_h3(h2.transaction_id(), sb_transaction, t2))
            gf->exit("cannot prepare h3");
        ve->sign(&t2);

        return t2;
    }

}
