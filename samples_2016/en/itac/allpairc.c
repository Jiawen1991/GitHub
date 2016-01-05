#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "testutil.h"

#define TEST_SIZE	2000

/*
 * Check that requests have been set to null
 */
static void rq_check( MPI_Request *requests, int n, char *msg)
{
  int i;

  for (i=0; i<n; i++) 
    if (requests[i] != MPI_REQUEST_NULL) {
      printf ("Nonnull request in %s\n",msg);
      exit (10);
    }
}

/*
 * Initialize test data buffer with integral sequence, taking the rank into account.
 */
static void init_test_data(double *buf, int n, int rank)
{
  int i;

  for (i=0; i<n; i++)
    buf[i] = (double)(i + rank * 10000);
}

/*
 * Clear test data buffer.
 */
static void clear_test_data(double *buf, int n)
{
  int i;

  for (i=0; i<n; i++)
    buf[i] = 0.0;
}

/*
 * Verify test data buffer.
 */
static void verify_test_data(double *buf, int count, int n, int rank, char *name)
{
  int i;

  for (i=0; i<count; i++)
    if (buf[i] != ((double)(i + rank * 10000)) ) {
      printf ("Invalid data %f at %d of %d in %s\n",buf[i], i, count, name);
      MPI_Abort(MPI_COMM_WORLD, 108);
    }

  for (i=count; i<n; i++)
    if (buf[i] != 0.0) {
      printf ("Invalid data %f at %d of %d in %s\n",buf[i], i, n, name);
      MPI_Abort(MPI_COMM_WORLD, 109);
    }
}

/*
 * Check for correct source, tag, count, and data in test message.
 */
static void msg_check (double *recv_buf, int source, int tag, int count,
                       MPI_Status *status, int n, char *name)
{
  int ierr, rank, recv_src, recv_tag, recv_count;

  recv_src = status->MPI_SOURCE;
  recv_tag = status->MPI_TAG;

  ierr = MPI_Comm_rank( MPI_COMM_WORLD, &rank);
  ierr = MPI_Get_count(status, MPI_DOUBLE, &recv_count);
  if (recv_src != source) {
    printf ("[%d] Unexpected source: %d in %s\n", rank, recv_src, name);
    MPI_Abort(MPI_COMM_WORLD, 101);
  }

  if (recv_tag != tag) {
    printf ("[%d] Unexpected tag: %d in %s\n", rank, recv_tag, name);
    MPI_Abort(MPI_COMM_WORLD, 102);
  }

  if (recv_count != count) {
    printf ("[%d] Unexpected count: %d in %s\n", rank, recv_count, name);
    MPI_Abort(MPI_COMM_WORLD, 103);
  }
  verify_test_data(recv_buf, count, n, source, name );
}


static void test_pair (void)
{
  int prev, next, count, tag, index, i, outcount, indices[2];
  int rank, size, flag, ierr, reqcount;
  double send_buf[TEST_SIZE], recv_buf[TEST_SIZE];
  double buffered_send_buf[TEST_SIZE * 2 + MPI_BSEND_OVERHEAD]; /* factor of two is based on guessing - only dynamic allocation would be safe */
  void *buffer;
  MPI_Status statuses[2];
  MPI_Status status;
  MPI_Request requests[2];
  MPI_Comm dupcom, intercom;
#ifdef V_T

  struct _VT_FuncFrameHandle {
      char *name;
      int func;
      int frame;
  };
  typedef struct _VT_FuncFrameHandle VT_FuncFrameHandle_t;

  VT_FuncFrameHandle_t normal_sends,
      buffered_sends,
      buffered_persistent_sends,
      ready_sends,
      sync_sends,
      nblock_sends,
      nblock_rsends,
      nblock_ssends,
      pers_sends,
      pers_rsends,
      pers_ssends,
      sendrecv,
      sendrecv_repl,
      intercomm;

  int classid;
  VT_classdef( "Application:test_pair", &classid );


#define VT_REGION_DEF( _name, _nameframe, _class ) \
        (_nameframe).name=_name; \
        VT_funcdef( (_nameframe).name, _class, &((_nameframe).func) );
#define VT_BEGIN_REGION( _nameframe ) \
        LOCDEF(); \
        VT_begin( (_nameframe).func )
#define VT_END_REGION( _nameframe ) \
        LOCDEF(); VT_end( (_nameframe).func )
#else
#define VT_REGION_DEF( _name, _nameframe, _class )
#define VT_BEGIN_REGION( _nameframe )
#define VT_END_REGION( _nameframe )

#endif




  ierr = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  ierr = MPI_Comm_size(MPI_COMM_WORLD, &size);
  if ( size < 2 ) {
      if ( rank == 0 ) {
	  printf("Program needs to be run on at least 2 processes.\n");
      }
      ierr = MPI_Abort( MPI_COMM_WORLD, 66 );
  }
  ierr = MPI_Comm_dup(MPI_COMM_WORLD, &dupcom);

  if ( rank >= 2 ) {
      /*      printf( "%d Calling finalize.\n", rank ); */
      ierr = MPI_Finalize( );
      exit(0);
  }

  next = rank + 1;
  if (next >= 2)
    next = 0;

  prev = rank - 1;
  if (prev < 0)
    prev = 1;

  VT_REGION_DEF( "Normal_Sends", normal_sends, classid );
  VT_REGION_DEF( "Buffered_Sends", buffered_sends, classid );
  VT_REGION_DEF( "Buffered_Persistent_Sends", buffered_persistent_sends, classid );
  VT_REGION_DEF( "Ready_Sends", ready_sends, classid );
  VT_REGION_DEF( "Sync_Sends", sync_sends, classid );
  VT_REGION_DEF( "nblock_Sends", nblock_sends, classid );
  VT_REGION_DEF( "nblock_RSends", nblock_rsends, classid );
  VT_REGION_DEF( "nblock_SSends", nblock_ssends, classid );
  VT_REGION_DEF( "Pers_Sends", pers_sends, classid );
  VT_REGION_DEF( "Pers_RSends", pers_rsends, classid );
  VT_REGION_DEF( "Pers_SSends", pers_ssends, classid );
  VT_REGION_DEF( "SendRecv", sendrecv, classid );
  VT_REGION_DEF( "SendRevc_Repl", sendrecv_repl, classid );
  VT_REGION_DEF( "InterComm", intercomm, classid );



/*
 * Normal sends
 */

  VT_BEGIN_REGION( normal_sends );

  if (rank == 0)
    printf ("Send\n");

  tag = 0x100;
  count = TEST_SIZE / 5;

  clear_test_data(recv_buf,TEST_SIZE);

  if (rank == 0) {
      init_test_data(send_buf,TEST_SIZE,0);

    LOCDEF();

    MPI_Send(send_buf, count, MPI_DOUBLE, next, tag, MPI_COMM_WORLD);
    MPI_Recv(recv_buf, TEST_SIZE, MPI_DOUBLE, MPI_ANY_SOURCE,
              MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    msg_check(recv_buf, prev, tag, count, &status, TEST_SIZE, "send and recv");
  }
  else {

    LOCDEF();

    MPI_Recv(recv_buf, TEST_SIZE, MPI_DOUBLE,MPI_ANY_SOURCE, MPI_ANY_TAG,
             MPI_COMM_WORLD, &status);
    msg_check( recv_buf, prev, tag, count, &status, TEST_SIZE,"send and recv");
    init_test_data(recv_buf,TEST_SIZE,1);
    MPI_Send(recv_buf, count, MPI_DOUBLE, next, tag, MPI_COMM_WORLD);

  }

  VT_END_REGION( normal_sends );


/*
 * Buffered sends
 */

  VT_BEGIN_REGION( buffered_sends );

  if (rank == 0)
    printf ("Buffered Send\n");

  tag = 138;
  count = TEST_SIZE / 5;

  clear_test_data(recv_buf,TEST_SIZE);

  if (rank == 0) {
      init_test_data(send_buf,TEST_SIZE,0);

    LOCDEF();

    MPI_Buffer_attach(buffered_send_buf, sizeof(buffered_send_buf));
    MPI_Bsend(send_buf, count, MPI_DOUBLE, next, tag, MPI_COMM_WORLD);
    MPI_Buffer_detach(&buffer, &size);
    if(buffer != buffered_send_buf || size != sizeof(buffered_send_buf)) {
        printf ("[%d] Unexpected buffer returned by MPI_Buffer_detach(): %p/%d != %p/%d\n", rank, buffer, size, buffered_send_buf, (int)sizeof(buffered_send_buf));
        MPI_Abort(MPI_COMM_WORLD, 201);
    }
    MPI_Recv(recv_buf, TEST_SIZE, MPI_DOUBLE, MPI_ANY_SOURCE,
              MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    msg_check(recv_buf, prev, tag, count, &status, TEST_SIZE, "send and recv");
  }
  else {

    LOCDEF();

    MPI_Recv(recv_buf, TEST_SIZE, MPI_DOUBLE,MPI_ANY_SOURCE, MPI_ANY_TAG,
             MPI_COMM_WORLD, &status);
    msg_check( recv_buf, prev, tag, count, &status, TEST_SIZE,"send and recv");
    init_test_data(recv_buf,TEST_SIZE,1);
    MPI_Send(recv_buf, count, MPI_DOUBLE, next, tag, MPI_COMM_WORLD);

  }

  VT_END_REGION( buffered_sends );


/*
 * Buffered sends
 */

  VT_BEGIN_REGION( buffered_persistent_sends );

  if (rank == 0)
    printf ("Buffered Persistent Send\n");

  tag = 238;
  count = TEST_SIZE / 5;

  clear_test_data(recv_buf,TEST_SIZE);

  if (rank == 0) {
      init_test_data(send_buf,TEST_SIZE,0);

    LOCDEF();

    MPI_Buffer_attach(buffered_send_buf, sizeof(buffered_send_buf));
    MPI_Bsend_init(send_buf, count, MPI_DOUBLE, next, tag, MPI_COMM_WORLD, requests);
    MPI_Start(requests);
    MPI_Wait(requests, statuses);
    MPI_Request_free(requests);
    MPI_Buffer_detach(&buffer, &size);
    if(buffer != buffered_send_buf || size != sizeof(buffered_send_buf)) {
        printf ("[%d] Unexpected buffer returned by MPI_Buffer_detach(): %p/%d != %p/%d\n", rank, buffer, size, buffered_send_buf, (int)sizeof(buffered_send_buf));
        MPI_Abort(MPI_COMM_WORLD, 201);
    }
    MPI_Recv(recv_buf, TEST_SIZE, MPI_DOUBLE, MPI_ANY_SOURCE,
              MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    msg_check(recv_buf, prev, tag, count, &status, TEST_SIZE, "send and recv");
  }
  else {

    LOCDEF();

    MPI_Recv(recv_buf, TEST_SIZE, MPI_DOUBLE,MPI_ANY_SOURCE, MPI_ANY_TAG,
             MPI_COMM_WORLD, &status);
    msg_check( recv_buf, prev, tag, count, &status, TEST_SIZE,"send and recv");
    init_test_data(recv_buf,TEST_SIZE,1);
    MPI_Send(recv_buf, count, MPI_DOUBLE, next, tag, MPI_COMM_WORLD);

  }

  VT_END_REGION( buffered_persistent_sends );


/*
 * Ready sends.  Note that we must insure that the receive is posted
 * before the rsend; this requires using Irecv.
 */


  VT_BEGIN_REGION( ready_sends );

  if (rank == 0)
    printf ("Rsend\n");

  tag = 1456;
  count = TEST_SIZE / 3;

  clear_test_data(recv_buf,TEST_SIZE);

  if (rank == 0) {
      init_test_data(send_buf,TEST_SIZE,0);
    MPI_Recv(MPI_BOTTOM, 0, MPI_INT, next, tag, MPI_COMM_WORLD, &status);
    MPI_Rsend(send_buf, count, MPI_DOUBLE, next, tag, MPI_COMM_WORLD);
    MPI_Probe(MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
    if (status.MPI_SOURCE != prev)
      printf ("Incorrect src, expected %d, got %d\n",prev, status.MPI_SOURCE);

    if (status.MPI_TAG != tag)
      printf ("Incorrect tag, expected %d, got %d\n",tag, status.MPI_TAG);

    MPI_Get_count(&status, MPI_DOUBLE, &i);
    if (i != count)
      printf ("Incorrect count, expected %d, got %d\n",count,i);

    MPI_Recv(recv_buf, TEST_SIZE, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG,
             MPI_COMM_WORLD, &status);

    msg_check( recv_buf, prev, tag, count, &status, TEST_SIZE,
               "rsend and recv");
  }
  else {
    MPI_Irecv(recv_buf, TEST_SIZE, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG,
              MPI_COMM_WORLD, requests);
    MPI_Send( MPI_BOTTOM, 0, MPI_INT, next, tag, MPI_COMM_WORLD);
    MPI_Wait(requests, &status);

    msg_check( recv_buf, prev, tag, count, &status, TEST_SIZE,
               "rsend and recv");
    init_test_data(recv_buf,TEST_SIZE,1);
    MPI_Send(recv_buf, count, MPI_DOUBLE, next, tag, MPI_COMM_WORLD);
  }

  VT_END_REGION( ready_sends );

/*
 * Synchronous sends
 */

  VT_BEGIN_REGION( sync_sends );

  if (rank == 0)
    printf ("Ssend\n");

  tag = 1789;
  count = TEST_SIZE / 3;

  clear_test_data(recv_buf,TEST_SIZE);

  if (rank == 0) {
      init_test_data(send_buf,TEST_SIZE,0);
    MPI_Iprobe(MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &flag, &status);
    if (flag)
      printf ("Iprobe succeeded! source %d, tag %d\n",status.MPI_SOURCE,
                                                      status.MPI_TAG);

    MPI_Ssend(send_buf, count, MPI_DOUBLE, next, tag, MPI_COMM_WORLD);

    while (!flag)
      MPI_Iprobe(MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &flag, &status);

    if (status.MPI_SOURCE != prev)
      printf ("Incorrect src, expected %d, got %d\n",prev, status.MPI_SOURCE);

    if (status.MPI_TAG != tag)
      printf ("Incorrect tag, expected %d, got %d\n",tag, status.MPI_TAG);

    MPI_Get_count(&status, MPI_DOUBLE, &i);

    if (i != count)
      printf ("Incorrect count, expected %d, got %d\n",count,i);

    MPI_Recv(recv_buf, TEST_SIZE, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG,
             MPI_COMM_WORLD, &status);
    msg_check( recv_buf, prev, tag, count, &status, TEST_SIZE, "ssend and recv");
  }
  else {
    MPI_Recv(recv_buf, TEST_SIZE, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG,
             MPI_COMM_WORLD, &status);
    msg_check( recv_buf, prev, tag, count, &status, TEST_SIZE, "ssend and recv"); init_test_data(recv_buf,TEST_SIZE,1);
    MPI_Ssend(recv_buf, count, MPI_DOUBLE, next, tag, MPI_COMM_WORLD);
  }

  VT_END_REGION( sync_sends );

/*
 * Nonblocking normal sends
 */

  VT_BEGIN_REGION( nblock_sends );

  if (rank == 0)
    printf ("Isend\n");

  tag = 2123;
  count = TEST_SIZE / 5;

  clear_test_data(recv_buf,TEST_SIZE);

  if (rank == 0) {
    MPI_Irecv(recv_buf, TEST_SIZE, MPI_DOUBLE,MPI_ANY_SOURCE, MPI_ANY_TAG,
              MPI_COMM_WORLD, requests);
    init_test_data(send_buf,TEST_SIZE,0);
    MPI_Isend(send_buf, count, MPI_DOUBLE, next, tag, MPI_COMM_WORLD,
              (requests+1));
    MPI_Waitall(2, requests, statuses);
    rq_check( requests, 2, "isend and irecv" );

    msg_check(recv_buf,prev,tag,count,statuses, TEST_SIZE,"isend and irecv");
  }
  else {
    MPI_Recv(recv_buf, TEST_SIZE, MPI_DOUBLE,MPI_ANY_SOURCE, MPI_ANY_TAG,
             MPI_COMM_WORLD, &status);
    msg_check(recv_buf,prev,tag,count,&status, TEST_SIZE,"isend and irecv"); init_test_data(recv_buf,TEST_SIZE,1);
    MPI_Isend(recv_buf, count, MPI_DOUBLE, next, tag,MPI_COMM_WORLD,
              (requests));
    MPI_Wait((requests), &status);
    rq_check(requests, 1, "isend (and recv)");
  }



  VT_END_REGION( nblock_sends );

/*
 * Nonblocking ready sends
 */


  VT_BEGIN_REGION( nblock_rsends );

  if (rank == 0)
    printf ("Irsend\n");

  tag = 2456;
  count = TEST_SIZE / 3;

  clear_test_data(recv_buf,TEST_SIZE);

  if (rank == 0) {
    MPI_Irecv(recv_buf, TEST_SIZE, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG,
              MPI_COMM_WORLD, requests);
    init_test_data(send_buf,TEST_SIZE,0);
    MPI_Sendrecv( MPI_BOTTOM, 0, MPI_INT, next, 0,
                  MPI_BOTTOM, 0, MPI_INT, next, 0,
                  dupcom, &status);
    MPI_Irsend(send_buf, count, MPI_DOUBLE, next, tag, MPI_COMM_WORLD,
               (requests+1));
    reqcount = 0;
    while (reqcount != 2) {
      MPI_Waitany( 2, requests, &index, statuses);
      if( index == 0 ) {
	  memcpy( &status, statuses, sizeof(status) );
      }
      reqcount++;
    }

    rq_check( requests, 1, "irsend and irecv");
    msg_check(recv_buf,prev,tag,count,&status, TEST_SIZE,"irsend and irecv");
  }
  else {
    MPI_Irecv(recv_buf, TEST_SIZE, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG,
              MPI_COMM_WORLD, requests);
    MPI_Sendrecv( MPI_BOTTOM, 0, MPI_INT, next, 0,
                  MPI_BOTTOM, 0, MPI_INT, next, 0,
                  dupcom, &status);
    flag = 0;
    while (!flag)
      MPI_Test(requests, &flag, &status);

    rq_check( requests, 1, "irsend and irecv (test)");
    msg_check( recv_buf, prev, tag, count, &status, TEST_SIZE,
               "irsend and irecv"); init_test_data(recv_buf,TEST_SIZE,1);
    MPI_Irsend(recv_buf, count, MPI_DOUBLE, next, tag,
               MPI_COMM_WORLD, requests);
    MPI_Waitall(1, requests, statuses);
    rq_check( requests, 1, "irsend and irecv");
  }

  VT_END_REGION( nblock_rsends );

/*
 * Nonblocking synchronous sends
 */

  VT_BEGIN_REGION( nblock_ssends );

  if (rank == 0)
    printf ("Issend\n");

  tag = 2789;
  count = TEST_SIZE / 3;
  clear_test_data(recv_buf,TEST_SIZE);

  if (rank == 0) {
    MPI_Irecv(recv_buf, TEST_SIZE, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG,
              MPI_COMM_WORLD, requests );
    init_test_data(send_buf,TEST_SIZE,0);
    MPI_Issend(send_buf, count, MPI_DOUBLE, next, tag, MPI_COMM_WORLD,
               (requests+1));
    flag = 0;
    while (!flag)
      MPI_Testall(2, requests, &flag, statuses);

    rq_check( requests, 2, "issend and irecv (testall)");
    msg_check( recv_buf, prev, tag, count, statuses, TEST_SIZE, 
               "issend and recv");
  }
  else {
    MPI_Recv(recv_buf, TEST_SIZE, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG,
             MPI_COMM_WORLD, &status);
    msg_check( recv_buf, prev, tag, count, &status, TEST_SIZE,
               "issend and recv"); init_test_data(recv_buf,TEST_SIZE,1);
    MPI_Issend(recv_buf, count, MPI_DOUBLE, next, tag, MPI_COMM_WORLD,requests);

    flag = 0;
    while (!flag)
      MPI_Testany(1, requests, &index, &flag, statuses);

    rq_check( requests, 1, "issend and recv (testany)");
  }


  VT_END_REGION( nblock_ssends );


/*
 * Persistent normal sends
 */

  VT_BEGIN_REGION( pers_sends );

  if (rank == 0)
    printf ("Send_init\n");

  tag = 3123;
  count = TEST_SIZE / 5;

  clear_test_data(recv_buf,TEST_SIZE);

  MPI_Send_init(send_buf, count, MPI_DOUBLE, next, tag,
                MPI_COMM_WORLD, requests);
  MPI_Recv_init(recv_buf, TEST_SIZE, MPI_DOUBLE,MPI_ANY_SOURCE, MPI_ANY_TAG,
                MPI_COMM_WORLD, (requests+1));

  if (rank == 0) {
      init_test_data(send_buf,TEST_SIZE,0);
    MPI_Startall(2, requests);
    MPI_Waitall(2, requests, statuses);
    msg_check( recv_buf, prev, tag, count, (statuses+1),
               TEST_SIZE, "persistent send/recv");
  }
  else {
    MPI_Start((requests+1));
    MPI_Wait((requests+1), &status);
    msg_check( recv_buf, prev, tag, count, &status, TEST_SIZE,
               "persistent send/recv");
    init_test_data(send_buf,TEST_SIZE,1);


    MPI_Start(requests);
    MPI_Wait(requests, &status);
  }
  MPI_Request_free(requests);
  MPI_Request_free((requests+1));


  VT_END_REGION( pers_sends );

/*
 * Persistent ready sends
 */

  VT_BEGIN_REGION( pers_rsends );

  if (rank == 0)
    printf ("Rsend_init\n");

  tag = 3456;
  count = TEST_SIZE / 3;

  clear_test_data(recv_buf,TEST_SIZE);

  MPI_Rsend_init(send_buf, count, MPI_DOUBLE, next, tag,
                  MPI_COMM_WORLD, requests);
  MPI_Recv_init(recv_buf, TEST_SIZE, MPI_DOUBLE, MPI_ANY_SOURCE,
                 MPI_ANY_TAG, MPI_COMM_WORLD, (requests+1));

  if (rank == 0) {
      init_test_data(send_buf,TEST_SIZE,0); MPI_Barrier( MPI_COMM_WORLD );
    MPI_Startall(2, requests);
    reqcount = 0;
    while (reqcount != 2) {
      MPI_Waitsome(2, requests, &outcount, indices, statuses);
      for (i=0; i<outcount; i++) {
        if (indices[i] == 1) {
          msg_check( recv_buf, prev, tag, count, (statuses+i),
                     TEST_SIZE, "waitsome");
        }
	reqcount++;
      }
    }
  }
  else {
    MPI_Start((requests+1)); MPI_Barrier( MPI_COMM_WORLD );
    flag = 0;
    while (!flag)
      MPI_Test((requests+1), &flag, &status);

    msg_check( recv_buf, prev, tag, count, &status, TEST_SIZE, "test");

    init_test_data(send_buf,TEST_SIZE,1);

 
    MPI_Start(requests);
    MPI_Wait(requests, &status);
  }
  MPI_Request_free(requests);
  MPI_Request_free((requests+1));


  VT_END_REGION( pers_rsends );


/*
 * Persistent synchronous sends
 */


  VT_BEGIN_REGION( pers_ssends );

  if (rank == 0)
    printf ("Ssend_init\n");

  tag = 3789;
  count = TEST_SIZE / 3;

  clear_test_data(recv_buf,TEST_SIZE);

  MPI_Ssend_init(send_buf, count, MPI_DOUBLE, next, tag,
                 MPI_COMM_WORLD, (requests+1));
  MPI_Recv_init(recv_buf, TEST_SIZE, MPI_DOUBLE, MPI_ANY_SOURCE,
                 MPI_ANY_TAG, MPI_COMM_WORLD, requests);

  if (rank == 0) {
      init_test_data(send_buf,TEST_SIZE,0);
    MPI_Startall(2, requests);

    reqcount = 0;
    while (reqcount != 2) {
      MPI_Testsome(2, requests, &outcount, indices, statuses);
      for (i=0; i<outcount; i++) {
        if (indices[i] == 0) {
          msg_check( recv_buf, prev, tag, count, (statuses+i),
                     TEST_SIZE, "testsome");
        }
	reqcount++;
      }
    }
  }
  else {
    MPI_Start(requests);
    flag = 0;
    while (!flag)
      MPI_Testany(1, requests, &index, &flag, statuses);

    msg_check( recv_buf, prev, tag, count, statuses, TEST_SIZE, "testany" );

    init_test_data(send_buf,TEST_SIZE,1);


     MPI_Start((requests+1));
     MPI_Wait((requests+1), &status);
  }
  MPI_Request_free(requests);
  MPI_Request_free((requests+1));


  VT_END_REGION( pers_ssends );


/*
 * Send/receive.
 */


  VT_BEGIN_REGION( sendrecv );

  if (rank == 0)
    printf ("Sendrecv\n");

  tag = 4123;
  count = TEST_SIZE / 5;

  clear_test_data(recv_buf,TEST_SIZE);

  if (rank == 0) {
      init_test_data(send_buf,TEST_SIZE,0);
    MPI_Sendrecv(send_buf, count, MPI_DOUBLE, next, tag,
                 recv_buf, count, MPI_DOUBLE, prev, tag,
                 MPI_COMM_WORLD, &status );

    msg_check( recv_buf, prev, tag, count, &status, TEST_SIZE,
               "sendrecv");
  }
  else {
    MPI_Recv(recv_buf, TEST_SIZE, MPI_DOUBLE, MPI_ANY_SOURCE,
             MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    msg_check( recv_buf, prev, tag, count, &status, TEST_SIZE,
               "recv/send"); init_test_data(recv_buf,TEST_SIZE,1);
    MPI_Send(recv_buf, count, MPI_DOUBLE, next, tag, MPI_COMM_WORLD);
  }


  VT_END_REGION( sendrecv );

#ifdef V_T
  VT_flush();
#endif


/*
 * Send/receive replace.
 */

  VT_BEGIN_REGION( sendrecv_repl );

  if (rank == 0)
    printf ("Sendrecv_replace\n");

  tag = 4456;
  count = TEST_SIZE / 3;

  if (rank == 0) {
      init_test_data(recv_buf, TEST_SIZE,0);
    for (i=count; i< TEST_SIZE; i++)
      recv_buf[i] = 0.0;

    MPI_Sendrecv_replace(recv_buf, count, MPI_DOUBLE,
                         next, tag, prev, tag, MPI_COMM_WORLD, &status);
    msg_check( recv_buf, prev, tag, count, &status, TEST_SIZE,
               "sendrecvreplace");
  }
  else {
    clear_test_data(recv_buf,TEST_SIZE);
    MPI_Recv(recv_buf, TEST_SIZE, MPI_DOUBLE, MPI_ANY_SOURCE,
             MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    msg_check( recv_buf, prev, tag, count, &status, TEST_SIZE,
               "recv/send for replace"); init_test_data(recv_buf,TEST_SIZE,1);
    MPI_Send(recv_buf, count, MPI_DOUBLE, next, tag, MPI_COMM_WORLD);
  }

  VT_END_REGION( sendrecv_repl );


/*
 * Send/Receive via inter-communicator
 */

  VT_BEGIN_REGION( intercomm );

  MPI_Intercomm_create(MPI_COMM_SELF, 0, MPI_COMM_WORLD, next, 1, &intercom);

  if (rank == 0)
    printf ("Send via inter-communicator\n");

  tag = 4018;
  count = TEST_SIZE / 5;

  clear_test_data(recv_buf,TEST_SIZE);

  if (rank == 0) {
      init_test_data(send_buf,TEST_SIZE,0);

    LOCDEF();

    MPI_Send(send_buf, count, MPI_DOUBLE, 0, tag, intercom);
    MPI_Recv(recv_buf, TEST_SIZE, MPI_DOUBLE, MPI_ANY_SOURCE,
              MPI_ANY_TAG, intercom, &status);
    msg_check(recv_buf, 0, tag, count, &status, TEST_SIZE, "send and recv via inter-communicator");
  }
  else if (rank == 1) {

    LOCDEF();

    MPI_Recv(recv_buf, TEST_SIZE, MPI_DOUBLE,MPI_ANY_SOURCE, MPI_ANY_TAG,
             intercom, &status);
    msg_check( recv_buf, 0, tag, count, &status, TEST_SIZE,"send and recv via inter-communicator");
    init_test_data(recv_buf,TEST_SIZE,0);
    MPI_Send(recv_buf, count, MPI_DOUBLE, 0, tag, intercom);

  }

  VT_END_REGION( normal_sends );



  MPI_Comm_free(&intercom);
  MPI_Comm_free(&dupcom);
} 

int main (int argc, char *argv[])
{
  int ierr;
  int rank;
  static char buffer[80];
  MPI_Request req = MPI_REQUEST_NULL;
  MPI_Status status, status2;
#ifdef V_T
  double ts;
  int messageframe;
#endif

  ierr = MPI_Init(&argc,&argv);
#ifdef V_T
  ts = VT_timestamp();
#endif



  /* this used to be buggy... */
  MPI_Wait( &req, &status );

  ierr = MPI_Barrier(MPI_COMM_WORLD);
  test_pair();





  MPI_Comm_rank ( MPI_COMM_WORLD, &rank );
  if ( getenv ("VT_ABORT_BEFORE_FINALIZE") ) {
      if ( atoi ( getenv ("VT_ABORT_BEFORE_FINALIZE") ) < 2 )
          MPI_Abort( MPI_COMM_WORLD, 10 );

      if ( !rank ) {
	  *((char *)NULL) = 0;
      } else {
	  MPI_Barrier ( MPI_COMM_WORLD );
      }
  }

  /* test some other aspects of message transfer: persistent send with MPI_PROC_NULL */
  MPI_Send_init( &ierr, 1, MPI_INT, MPI_PROC_NULL, 100, MPI_COMM_WORLD, &req );
  MPI_Start( &req );
  MPI_Wait( &req, &status );
  MPI_Start( &req );
  MPI_Wait( &req, &status );
  MPI_Request_free( &req );

  /* persistent receive with MPI_PROC_NULL */
  MPI_Recv_init( &ierr, 1, MPI_INT, MPI_PROC_NULL, 100, MPI_COMM_WORLD, &req );
  MPI_Start( &req );
  MPI_Wait( &req, &status );
  MPI_Start( &req );
  MPI_Wait( &req, &status );
  MPI_Request_free( &req );

  /* real reuse of persistent communication */
  if( rank & 1 ) {
      MPI_Recv_init( buffer, sizeof(buffer), MPI_CHAR, rank^1, 101, MPI_COMM_WORLD, &req );
  } else {
      MPI_Send_init( buffer, sizeof(buffer), MPI_CHAR, rank^1, 101, MPI_COMM_WORLD, &req );
  }
  MPI_Start( &req );
  MPI_Wait( &req, &status );
  MPI_Start( &req );
  MPI_Wait( &req, &status );
  MPI_Request_free( &req );

  /* send to MPI_PROC_NULL */
  MPI_Send( buffer, sizeof(buffer), MPI_CHAR, MPI_PROC_NULL, 103, MPI_COMM_WORLD );

  /* cancelled receive */
  MPI_Irecv( buffer, sizeof(buffer), MPI_CHAR, rank^1, 105, MPI_COMM_WORLD, &req );
  MPI_Cancel( &req );
  MPI_Wait( &req, &status2 );

#ifdef V_T
  printf( "Time: %f\n", VT_timestamp()-ts );
#endif
  ierr = MPI_Finalize();

  return ierr;
}






