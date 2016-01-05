/*********************************************************************
 * Copyright (C) 2003-2009 Intel Corporation.  All Rights Reserved.
 *
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel Corporation
 * or its suppliers or licensors.  Title to the Material remains with
 * Intel Corporation or its suppliers and licensors.  The Material is
 * protected by worldwide copyright and trade secret laws and treaty
 * provisions.  No part of the Material may be used, copied, reproduced,
 * modified, published, uploaded, posted, transmitted, distributed, or
 * disclosed in any way without Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other
 * intellectual property right is granted to or conferred upon you by
 * disclosure or delivery of the Materials, either expressly, by
 * implication, inducement, estoppel or otherwise.  Any license under
 * such intellectual property rights must be express and approved by
 * Intel in writing.
 * 
 *********************************************************************/

#include <h_custompluginframework.h>

class DoubleSim : public CustomPluginFramework
{
public:
    virtual void Initialize( ) {
        return;
    }

    virtual void Finalize( ) {
        return;
    }

    virtual void ProcessFunctionEvent( FuncEventInfo * _event_ ) {
        _event_->_endTime += ( _event_->_endTime - _event_->_startTime );
    }

    virtual void ProcessNonTracingEvent( FuncEventInfo * _event_ ) {
        _event_->_endTime += ( _event_->_endTime - _event_->_startTime );
    }

    virtual void ProcessMPIFunctionEvent( FuncEventInfo * _event_ ) {
        _event_->_endTime += ( _event_->_endTime - _event_->_startTime );
    }

    virtual void ProcessCollOp( CollOpInfo * _collop_ ) {
        //Traverse vector of CollOpInfo
        for( U4 k = 0; k < _collop_->_threads->count(); k++) { 
            (*(_collop_->_threads))[k]->_endTime += ( (*(_collop_->_threads))[k]->_endTime - (*(_collop_->_threads))[k]->_startTime );
        }
    }

    virtual void ProcessP2P( P2PInfo * _message_ ) {
        if( isSyncSendP2P( _message_->_sendFID ) )
            processSyncSend( _message_ );
        else
            processSendRecv( _message_ );
    }

    virtual void ProcessNonBlockingSend( FuncEventInfo * _event_, P2PInfo * _message_ ) {
        _event_->_endTime += ( _event_->_endTime - _event_->_startTime );
    }

    virtual void ProcessP2PWaitAllSome( Vector<P2PInfo*, ObjectPointerAllocator<P2PInfo*> > * _messages_ ) {
        for( Vector<P2PInfo*, ObjectPointerAllocator<P2PInfo*> >::Iterator iter = _messages_->begin(); iter != _messages_->end(); ++iter ) {
            (*iter)->_recvEndTime += ( (*iter)->_recvEndTime - (*iter)->_recvStartTime );

            //double length of sync send if need be
            if( isSyncSendP2P( (*iter)->_sendFID ) )   (*iter)->_sendEndTime += ( (*iter)->_sendEndTime - (*iter)->_sendStartTime );
        }
    }

private:
    void processSyncSend( P2PInfo * _message_ ) {
        _message_->_recvEndTime += ( _message_->_recvEndTime - _message_->_recvStartTime );
        _message_->_sendEndTime += ( _message_->_sendEndTime - _message_->_sendStartTime );
    }

    void processSendRecv( P2PInfo * _message_ ) {
        _message_->_recvEndTime += ( _message_->_recvEndTime - _message_->_recvStartTime );
    }
};

// the class factories
#ifndef _WINDLL
        extern "C" CustomPluginFramework* create() {
#else
        extern "C" __declspec( dllexport ) CustomPluginFramework* create() {
#endif
    return new DoubleSim;
}


#ifndef _WINDLL
        extern "C" void destroy(CustomPluginFramework* plug) {
#else
        extern "C" __declspec( dllexport ) void destroy(CustomPluginFramework* plug) {
#endif
    delete plug;
}

