/*********************************************************************
 * Copyright 2003-2014 Intel Corporation.
 * All Rights Reserved.
 * 
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel
 * Corporation or its suppliers or licensors. Title to the Material
 * remains with Intel Corporation or its suppliers and licensors. The
 * Material contains trade secrets and proprietary and confidential
 * information of Intel or its suppliers and licensors. The Material
 * is protected by worldwide copyright and trade secret laws and
 * treaty provisions. No part of the Material may be used, copied,
 * reproduced, modified, published, uploaded, posted, transmitted,
 * distributed, or disclosed in any way without Intel's prior express
 * written permission.
 * 
 * No license under any patent, copyright, trade secret or other
 * intellectual property right is granted to or conferred upon you by
 * disclosure or delivery of the Materials, either expressly, by
 * implication, inducement, estoppel or otherwise. Any license under
 * such intellectual property rights must be express and approved by
 * Intel in writing.
 * 
 *********************************************************************/

#ifndef _H_CUSTOMPLUGINFRAMEWORK_H_
#define _H_CUSTOMPLUGINFRAMEWORK_H_

#include <h_config.h>
#include <h_vector.h>
#include <h_hashmap.h>
#include <h_idealsimdefs.h> //holds vector lookup values for function and collOp id's

/******************************************************************************************
    Below are the structs for the Function events, P2P messages, and collective operations
    that are passed to the dynamic libraries.  These are modified versions of the actual 
    messages received by the queue analyzer.
*******************************************************************************************/ 

struct FuncEventInfo
{
    FuncEventInfo( ): _threadID(0), _func(0), _sizeSent(0), _sizeRecv(0), _startTime(0), _endTime(0) {};

    FuncEventInfo( U4 _id_, U4 _fun_, U4 _szSent_, U4 _szRecv_, U8 _start_, U8 _end_ ): _threadID(_id_), _func(_fun_),
        _sizeSent(_szSent_), _sizeRecv(_szRecv_), _startTime(_start_), _endTime(_end_) {};

    const U4 _threadID;
    const U4 _func;
    const U4 _sizeSent;
    const U4 _sizeRecv;
    const U8 _startTime;
    U8 _endTime;
};

struct P2PInfo
{
    P2PInfo( ): _sendStartTime(0), _sendEndTime(0), _recvStartTime(0), _recvEndTime(0), _iRecvStartTime(0), _backwards(false),
       _sender(0), _receiver(0), _communicator(0), _tag(0), _messageSize(0), _sendFID(0), _recvFID(0) {};

    P2PInfo( U8 _sendStart_, U8 _sendEnd_, U8 _recvStart_, U8 _recvEnd_, U8 _irecvStart_, bool _back_, U4 _send_, U4 _recv_,
       U4 _comm_, U4 _tag_, U4 _szMes_, U4 _sFID_, U4 _rFID_ ): _sendStartTime(_sendStart_), _sendEndTime(_sendEnd_), 
       _recvStartTime(_recvStart_), _recvEndTime(_recvEnd_), _iRecvStartTime(_irecvStart_), _backwards(_back_), _sender(_send_), 
       _receiver(_recv_), _communicator(_comm_), _tag(_tag_), _messageSize(_szMes_), _sendFID(_sFID_), _recvFID(_rFID_) { };

    const U8 _sendStartTime;          /// time when sent of send
    U8 _sendEndTime;                  /// time when end of send
    const U8 _recvStartTime;          /// time when sent of recv
    U8 _recvEndTime;                  /// time when end of recv
    const U8 _iRecvStartTime;         /// will be same as _recvStartTime unless the recv func of this message is one that requires irecv
    bool _backwards;                  /// true if backwards message, false otherwise
    const U4 _sender;                 /// sender's thread ID
    const U4 _receiver;               /// receiver's thread ID
    const U4 _communicator;           /// communicator on which message was sent
    const U4 _tag;                    /// MPI tag of message
    const U4 _messageSize;            /// Size of message sent
    const U4 _sendFID;                /// sender's function ID
    const U4 _recvFID;                /// receiver's function ID
};

struct CollOpInfo
{
    CollOpInfo( ): _firstTime(0), _communicator(0), _collOpId(0), _root(0), _rootIndex(0), _threads(NULL) {};

    CollOpInfo( U8 _first_, U4 _comm_, U4 _id_, U4 _rootID_, U4 _rootdex_, Vector< FuncEventInfo*, ObjectPointerAllocator<FuncEventInfo*> > * _procs_ ):
        _firstTime(_first_), _communicator(_comm_), _collOpId(_id_), _root(_rootID_), _rootIndex(_rootdex_) {
            _threads = _procs_;
	};
    ~CollOpInfo() {
        delete _threads;
    };

    const U8 _firstTime;                    /// first time ever occured in this collop
    const U4 _communicator;           /// communicator on which collop was executed	
    const U4 _collOpId;               /// id (type) of this collop 
    const U4 _root;                   /// thread-id of root or -1 for NO root

    const U4 _rootIndex;              /// index of root in the _threads vector, dont care if no root
    Vector< FuncEventInfo*, ObjectPointerAllocator<FuncEventInfo*> > * _threads;           /// Vector of all the threads involved in the collective operation.
};
/**********************************************************************/


/// This is an interface compiled into the exe to act as structure to implementations of advance simulators
class CustomPluginFramework
{
public:
    CustomPluginFramework() { }

    ~CustomPluginFramework( ) { }

    void loadParams( const Vector< char* > params ) {
        _commandLineParams = params;
    }

    void setClockResolution( const F8 resolution ) {
        _clockResolution = resolution;
    }

    /// Called from default constructor of QueueAnalyzer class
    void setup( const Vector< U4 > _singleRanks_, const Vector< U4 > _collOps_, const Vector< U4 > _P2Ps_, const HashMap< U4, 
		const char* > comms, const char* _path_, const U8 _start_, const U8 _end_, const U4 _tg_ ) {

        _stfStart = _start_;
	_stfEnd = _end_;
        _numThreads = _tg_;

        if( _path_ == NULL ) {
            _stfPath = NULL;
        } else {
            _stfPath = new char[ ( int ) strlen( _path_ ) ];
            strcpy( _stfPath, _path_ ); // copy const pointer data to newly allocated space
        }

	_singleRankFuncs.resize( NUM_SINGLE_RANKS );
	for( int k = 0; k < NUM_SINGLE_RANKS; k++)
            _singleRankFuncs[k] = _singleRanks_[k];

        _collOpFuncs.resize( NUM_COLLECTIVES );
	for( int k = 0; k < NUM_COLLECTIVES; k++)
            _collOpFuncs[k] = _collOps_[k];

	_P2PFuncs.resize( NUM_P2P );
	for( int k = 0; k < NUM_P2P; k++)
            _P2PFuncs[k] = _P2Ps_[k];

	_communicatorsMapped.resize( (U4)comms.count() );
        for( HashMap< U4, const char* >::ConstIterator iter = comms.begin(); iter != comms.end(); ++iter ) {
            _communicatorsMapped.insert( (*iter).getKey(), &(*((*iter).getData())) );
        }
    }

    /// Function is called before the start of processing a tracefile, not to be used as a constructor
    /** This function is to allow simulator extensions to have a way of doing pre-tracefile operations before the
        executable starts processing the tracefile.
        @returns void
    */
    virtual void Initialize( ) = 0;

    /// Function is called at the very end of processing a tracefile, not to be used as a destructor
    /** This function is to allow simulator extensions to have a way of doing post-tracefile operations before the
        executable exits.
        @returns void
    */
    virtual void Finalize( ) = 0;

    /// Processes application events that occur in the tracefile.
    /** This event object will be a regular application event in the trace that is to processed from the queues.
        @param _event_          Application event in tracefile
        @returns void, is able to return object back to queues through non-const pointer
    */
    virtual void ProcessFunctionEvent( FuncEventInfo * _event_ ) = 0;

    /// Processes Non-Tracing events that occur in the tracefile.
    /** This event object will be a non tracing event in the trace that is to processed from the queues.  A non-tracing
        event is defined as empty white space at beginning of trace before each thread, or parts of a tracefile that have
	tracing turned off.
        @param _event_          Non-tracing pseudo event in tracefile
        @returns void, is able to return object back to queues through non-const pointer
    */
    virtual void ProcessNonTracingEvent( FuncEventInfo * _event_ ) = 0;

    /// Processes a single rank MPI event in tracefile.
    /** This event object will be a single rank MPI event in the trace that is to processed from the queues.  A single rank MPI event
        is an MPI event that only involves one thread.
        @param _event_          MPI single rank event in tracefile
        @returns void, is able to return object back to queues through non-const pointer
    */
    virtual void ProcessMPIFunctionEvent(  FuncEventInfo * _event_ ) = 0;

    /// Processes a collective operation MPI event in tracefile.
    /** This event object will be a collective operation MPI event in the trace that is ready to be processed from the queues.  This one 
        object represents all threads involved in collective operation.  The simulator WILL NOT get seperate events for each thread
	involved in the collective operation.
        @param _collop_          MPI collective operation event in tracefile
        @returns void, is able to return object back to queues through non-const pointer
    */
    virtual void ProcessCollOp( CollOpInfo * _collop_ ) = 0;

    /// Processes a point-to-point MPI event in tracefile.
    /** This event object will be a P2P MPI event in the trace that is to processed from the queues. This will be called for sendrecv, synchronous,
        or normal send to recv MPI operations.
        @param _message_          MPI P2P event that represents the send and recv thread in tracefile
        @returns void, is able to return object back to queues through non-const pointer
    */
    virtual void ProcessP2P( P2PInfo * _message_ ) = 0;

    /// Processes a non-blocking send MPI event in tracefile.
    /** This function will get an event object that represents the send operation and the corresponding P2P message that goes with that send.  This
        will only be called for non-synchronous sends, or sends that do not rely on the recv thread.  The start time of the recv may not be known.
        @param _event_            MPI application event in tracefile
        @param _message_          MPI P2P event in tracefile
        @returns void, is able to return object back to queues through non-const pointer
    */
    virtual void ProcessNonBlockingSend( FuncEventInfo * _event_, P2PInfo * _message_ ) = 0;

    /// Processes a waitall and waitsome MPI event in tracefile.
    /** This function will get a vector of all P2P messages involved with the waitall/waitsome.  There will be no individual function events corresponding
        to the send or recv thread involved in this P2P operation.
        @param _messages_          MPI P2P events involved in waitall/waitsome in tracefile
        @returns void, is able to return object back to queues through non-const pointer
    */
    virtual void ProcessP2PWaitAllSome( Vector<P2PInfo*, ObjectPointerAllocator<P2PInfo*> > * _messages_ ) = 0;

protected:
    /// Simulator command line parameters
    /** Vector of char* of parameters that did not match any keywords for ICPF, may contain invalid values, and is
        up to simulator to check.
    */
    Vector< char* > _commandLineParams;

    /// Pathway to the directory of the original STF file loaded by ICPF at runtime
    char * _stfPath;

    /// Resolution length of clock tick in seconds
    F8 _clockResolution;

    /// Start time of original STF file
    U8 _stfStart;

    /// End time of original STF file
    U8 _stfEnd;

    /// Number of threads in trace
    U4 _numThreads;

    /// Vector of single rank function id's
    /** This vector is initialized in init() call with the id values of every single rank function in the tracefile.
        Single ranks that are not in the trace file are initialized with the value -1. 
    */  
    Vector< U4 > _singleRankFuncs;

    /// Vector of collective operation id's
    /** This vector is initialized in init() call with the id values of every collective function in the tracefile.
        Given a function id number the corresponding collective operation id is returned. 
    */  
    Vector< U4 > _collOpFuncs;

    /// Vector of P2P id's
    /** This vector is initialized in init() call with the id values of every P2P function in the tracefile.
        Given a function id number the corresponding collective operation id is returned. 
    */  
    Vector< U4 > _P2PFuncs;

    /// HashMap of all communicator names.  Lookup is by id of communicator.  Data value is strings.
    HashMap< U4, const char* > _communicatorsMapped;

    /// Checks to see whether the function ID is a single rank ID
    /** @param _id_          Function ID
        @returns TRUE if _id_ is a single rank function, FALSE otherwise
    */
    bool isSingleRankFunc( U4 _id_ ) {
        return _singleRankFuncs.contains( &(_id_) );
    }

    /// Checks to see whether the function ID matches a collective operation ID
    /** @param _id_          Function ID
        @returns TRUE if _id_ is a collOp, FALSE otherwise
    */
    bool isCollectiveOperation( U4 _id_ ) {
        return _collOpFuncs.contains( &(_id_) );
    }

    /// Checks to see whether the function ID is a P2P ID
    /** @param _id_          Function ID
        @returns TRUE if _id_ is a P2P function, FALSE otherwise
    */
    bool isP2P( U4 _id_ ) {
        return _P2PFuncs.contains( &(_id_) );
    }

    /// Finds whether the given function id is an ALL-TO-ALL collective operation
    /** @param _id_          A function id number
        @returns TRUE if it is an ALL-TO-ALL collecitve operation id number, FALSE otherwise
    */
    bool isAllToAll( U4 _id_ ) {
        return ( _id_ == _collOpFuncs[MPI_Alltoall] ) || ( _id_ == _collOpFuncs[MPI_Alltoallv] ) ||
            ( _id_ == _collOpFuncs[MPI_Allgather] ) || ( _id_ == _collOpFuncs[MPI_Allgatherv] ) ||
            ( _id_ == _collOpFuncs[MPI_Allreduce] ) || ( _id_ == _collOpFuncs[MPI_Reduce_Scatter] );
    }

    /// Finds whether the given function id is an ONE-TO-ALL collective operation
    /** @param _id_          A function id number
        @returns TRUE if it is an ONE-TO-ALL collecitve operation id number, FALSE otherwise
    */
    bool isOneToAll( U4 _id_ ) {
        return ( _id_ == _collOpFuncs[MPI_Bcast] ) || ( _id_ == _collOpFuncs[MPI_Scatter] ) ||
            ( _id_ == _collOpFuncs[MPI_Scatterv] );
    }

    /// Finds whether the given function id is an ALL-TO-ONE collective operation
    /** @param _id_          A function id number
        @returns TRUE if it is an ALL-TO-ONE collecitve operation id number, FALSE otherwise
    */
    bool isAllToOne( U4 _id_ ) {
        return ( _id_ == _collOpFuncs[MPI_Gather] ) || ( _id_ == _collOpFuncs[MPI_Gatherv] ) ||
            ( _id_ == _collOpFuncs[MPI_Reduce] );
    }

    /// Finds whether the given function id is an ALL collective operation
    /** @param _id_          A function id number
        @returns TRUE if it is an ALL collecitve operation id number, FALSE otherwise
    */
    bool isAll( U4 _id_ ) {
        return ( _id_ == _collOpFuncs[MPI_Barrier] ) || ( _id_ == _collOpFuncs[MPI_Scan] );
    }

    /// Finds whether the func _id_ is a send P2P function
    /** @param _id_        Function id number of FuncEventData that occured
        @returns TRUE when the FuncEventData is a P2P send operation, FALSE otherwise
    */
    bool isSendP2P( U4 _id_ ) {
    /*    return ( _id_ == _P2PFuncs[MPI_Bsend] ) || ( _id_ == _P2PFuncs[MPI_Ibsend] ) || 
            ( _id_ == _P2PFuncs[MPI_Isend] ) || ( _id_ == _P2PFuncs[MPI_Send] ) || 
            ( _id_ == _P2PFuncs[MPI_Start] ) || ( _id_ == _P2PFuncs[MPI_Startall] )
	    || ( _id_ == _P2PFuncs[MPI_Irsend] ); */
            return ( _id_ == _P2PFuncs[MPI_Bsend] ) || ( _id_ == _P2PFuncs[MPI_Ibsend] ) || 
        ( _id_ == _P2PFuncs[MPI_Isend] ) || ( _id_ == _P2PFuncs[MPI_Send] ) || 
        ( _id_ == _P2PFuncs[MPI_Start] ) || ( _id_ == _P2PFuncs[MPI_Startall] ) ||
        ( _id_ == _P2PFuncs[MPI_Irsend] ) || ( _id_ == _P2PFuncs[MPI_Rsend] );
    }

    /// Finds whether the func _id_ is a synchronous send P2P function
    /** @param _id_        Function id number of FuncEventData that occured
        @returns TRUE when the FuncEventData is a P2P sync send operation, FALSE otherwise
    */
    bool isSyncSendP2P( U4 _id_ ) {
    /*    return ( _id_ == _P2PFuncs[MPI_Issend] ) || ( _id_ == _P2PFuncs[MPI_Rsend] ) ||
            ( _id_ == _P2PFuncs[MPI_Ssend] ) /*|| ( _id_ == _P2PFuncs[MPI_Irsend] *//*); */
        return ( _id_ == _P2PFuncs[MPI_Ssend] );
    }

    /// Finds whether the func _id_ is a send-recv P2P function
    /** @param _id_        Function id number of FuncEventData that occured
        @returns TRUE when the FuncEventData is a P2P send-recv operation, FALSE otherwise
    */
    bool isSendRecv( U4 _id_ ) {
        return ( _id_ == _P2PFuncs[MPI_Sendrecv] ) || ( _id_ == _P2PFuncs[MPI_Sendrecv_replace] );
    }

    /// Finds whether the func _id_ is a recv P2P function
    /** @param _id_        Function id number of FuncEventData that occured
        @returns TRUE when the FuncEventData is a P2P recv operation, FALSE otherwise
    */
    bool isRecv( U4 _id_ ) {
        return ( _id_ == _P2PFuncs[MPI_Recv] ) || ( _id_ == _P2PFuncs[MPI_Test] ) || 
	    ( _id_ == _P2PFuncs[MPI_Testall] ) || ( _id_ == _P2PFuncs[MPI_Testany] ) ||
	    ( _id_ == _P2PFuncs[MPI_Testsome] ) || ( _id_ == _P2PFuncs[MPI_Wait] ) || 
	    ( _id_ == _P2PFuncs[MPI_Waitall] ) || ( _id_ == _P2PFuncs[MPI_Waitany] ) ||
            ( _id_ == _P2PFuncs[MPI_Waitsome] );
    }
   
};

// the types of the class factories
typedef CustomPluginFramework* create_t();
typedef void destroy_t(CustomPluginFramework*);

#endif
