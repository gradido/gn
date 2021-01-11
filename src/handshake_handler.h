#ifndef HANDHSAKE_HANDLER_H
#define HANDHSAKE_HANDLER_H

#include "gradido_interfaces.h"

namespace gradido {

    class HandshakeHandler : public IHandshakeHandler,
        public ICommunicationLayer::GrprTransactionListener {
    private:
        IGradidoFacade* gf;
        proto::Timestamp h3_ts;

        class SbRecordAdded : 
            public ICommunicationLayer::GrprTransactionListener {
        private:
            IGradidoFacade* gf;
        public:
            SbRecordAdded(IGradidoFacade* gf) : gf(gf) {}
            virtual void on_transaction(grpr::Transaction& t);
        };
        SbRecordAdded sbra;
    public:
        virtual void on_transaction(grpr::Transaction& t);
    public:
        HandshakeHandler(IGradidoFacade* gf) : gf(gf), sbra(gf) {}
        virtual grpr::Transaction get_response_h0(grpr::Transaction req,
                                                  IVersioned* ve);
        virtual grpr::Transaction get_response_h2(grpr::Transaction req,
                                                  IVersioned* ve) {NOT_SUPPORTED;}
    };

    class StarterHandshakeHandler : public IHandshakeHandler,
        public ICommunicationLayer::GrprTransactionListener {
    private:
        IGradidoFacade* gf;
        bool h0_complete;
    public:
        virtual void on_transaction(grpr::Transaction& t);
    public:
        StarterHandshakeHandler(IGradidoFacade* gf) : gf(gf), 
            h0_complete(false) {}
        virtual grpr::Transaction get_response_h0(grpr::Transaction req,
                                                  IVersioned* ve) {NOT_SUPPORTED;}
        virtual grpr::Transaction get_response_h2(grpr::Transaction req,
                                                  IVersioned* ve);
    };

}

#endif
