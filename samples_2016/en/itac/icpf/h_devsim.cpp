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

class DevSim : public CustomPluginFramework
{
public:
    virtual void Initialize( ) {
        return;
    }

    virtual void Finalize( ) {
        return;
    }

    virtual void ProcessFunctionEvent( FuncEventInfo * _event_ ) {
        return;
    }

    virtual void ProcessNonTracingEvent( FuncEventInfo * _event_ ) {
        return;
    }

    virtual void ProcessMPIFunctionEvent( FuncEventInfo * _event_ ) {
        return;
    }

    virtual void ProcessCollOp( CollOpInfo * _collop_ ) {
        return;
    }

    virtual void ProcessP2P( P2PInfo * _message_ ) {
        return;
    }

    virtual void ProcessNonBlockingSend( FuncEventInfo * _event_, P2PInfo * _message_ ) {
        return;
    }

    virtual void ProcessP2PWaitAllSome( Vector<P2PInfo*, ObjectPointerAllocator<P2PInfo*> > * _messages_ ) {
        return;
    }
};

// the class factories
#ifndef _WINDLL
        extern "C" CustomPluginFramework* create() {
#else
        extern "C" __declspec( dllexport ) CustomPluginFramework* create() {
#endif
    return new DevSim;
}


#ifndef _WINDLL
        extern "C" void destroy(CustomPluginFramework* plug) {
#else
        extern "C" __declspec( dllexport ) void destroy(CustomPluginFramework* plug) {
#endif
    delete plug;
}
