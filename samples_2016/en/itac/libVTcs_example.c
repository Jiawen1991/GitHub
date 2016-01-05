/*******************************************************************
 * 
 * Copyright 2004-2008 Intel Corporation.
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
 *******************************************************************/

// This example demonstrates the approach which can be used to trace 
// multi-process non-MPI applications. 
// Processes in non-MPI applications are created and communicate using 
// non-standard and varying methods. Therefore a special version of 
// the ITC library was developed that neither relies on MPI nor on 
// the applications communication, but rather implements its own 
// communication layer using TCP/IP. That ITC library is called libVTcs.
// This example is linked against that library.

// This example uses MPI as means of communication and process handling. 
// Since it's not linked against the normal ITC library, tracing of MPI 
// communications has to be done with VT API calls.
// This example uses MPI just for simplification. Otherwise, the application 
// has to handle the startup and termination of all processes itself. 
// Also it has to implement process communications itself.

// This example application consists of 1 executable module. 
// This is done for simplification. But real multi-process non-MPI 
// application can consist of multiple executable modules. For example, 
// one module is for a server part, another one for a client.

// Need to run at least 3 processes for this example application:
// one of them (rank 0) starts VTserver and does nothing else, 
// two others exchange messages.
// It doesn't mean that we have client-server model here. 
// It only means that the application has to start VTserver itself for 
// the tracing.

// IMPORTANT:
// Some of the VT API calls may block, especially VT_initialize(). 
// Execute them in a separate thread if the process wants to continue. 
// These pending calls can be aborted with VT_abort(), for example 
// if another process failed to initialize trace data collection. 
// This failure has to be communicated by the application itself and 
// it also has to terminate the VTserver by sending it a kill signal, 
// because it cannot be guaranteed that all processes and the VTserver will 
// detect all failures that might prevent establishing the communication.

// See ITC-ReferenceGuide for more information about tracing of non-MPI 
// applications. The section you need to review is called 
// 'Tracing of Distributed Applications'.
 
#include <mpi.h>
#include <VT.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to simulate a work
void perform_dummy_work(int nIterationCount)
{
    for (int i = 0; i < nIterationCount; i++)
    {
        for (int q = 0; q < 10000000; q++)
        {
            q = q + 1; 
            q--; 
            int f = q-2; f++; 
        } 
    }
} // perform_dummy_work

int main(int argc, char **argv)
{
    int nRank = -1, nProcCount = -1, nClientCount = -1;
    MPI_Status status;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &nRank);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcCount);
    nClientCount = nProcCount - 1;

    if (nRank == 0)
    {
        //////////////////////////////////////////////////////////////////////////
        // Gather contact infos from other proc and add them to VTserver cmd line

        char *pVTsrvCmdLineBuf = NULL;
        int nVTsrvCmdLineBufSize = -1;

        nVTsrvCmdLineBufSize = 80 + 80 * nClientCount;
        pVTsrvCmdLineBuf = (char*)malloc(nVTsrvCmdLineBufSize);
        strcpy(pVTsrvCmdLineBuf, "VTserver");

        for (int i = 0; i < nClientCount; ++i)
        {
            int nBufLen = strlen(pVTsrvCmdLineBuf);

            pVTsrvCmdLineBuf[nBufLen++] = ' ';
            MPI_Recv(&pVTsrvCmdLineBuf[nBufLen],
              nVTsrvCmdLineBufSize - nBufLen,
              MPI_CHAR, i + 1, 0,
              MPI_COMM_WORLD, 
              &status);

        } // for cycle through all clients

        // Gather contact infos from other proc and add them to VTserver cmd line
        //////////////////////////////////////////////////////////////////////////

        // Specify trace file name
        sprintf(&pVTsrvCmdLineBuf[strlen(pVTsrvCmdLineBuf)], 
                " --logfile-name %s", "libVTcs_example.stf");

        //////////////////////////////////////////////////////////////////////////
        // Start VTserver
        
        fprintf(stderr, "Executing %s\n", pVTsrvCmdLineBuf);
        fflush(stderr);

        // This call is blocking. The execition will continue after 
        // other proc call VT_finalize().
        // Note that it's possible to run VTserver without blocking 
        // of this process. It can be useful, if, for example, you want that 
        // rank 0 does some useful work. 
        // If you need this, you need to use fork&exec or CreateProcess() 
        // to run VTserver. Process creation/management is system-specific. 
        // So, it's not used here for simplification purpose.
        int nResult = system(pVTsrvCmdLineBuf);

        fprintf(stderr, "VTserver returned %d\n", nResult);
        fflush(stderr);

        // Start VTserver
        //////////////////////////////////////////////////////////////////////////

        if (pVTsrvCmdLineBuf)
        {
            free(pVTsrvCmdLineBuf);
        }

    } // rank 0
    else
    { // rank != 0

        const char *szContactInfo = NULL;

        char szClientName[MPI_MAX_PROCESSOR_NAME + 1] = "";
        sprintf(szClientName, "client_%d", nRank);

        // The following call allocates a port for TCP/IP 
        // communication with the VTserver or other clients 
        // and generates a string which identifies the machine 
        // and this port
        VT_clientinit(nRank, szClientName, &szContactInfo);

        // Send the contact info to rank 0 */
        MPI_Send((void *)szContactInfo, strlen(szContactInfo) + 1, 
                 MPI_CHAR, 0, 0, MPI_COMM_WORLD);

        // The following call actually establishes 
        // the communication with VTserver.
        // This call is blocking.
        VT_initialize(&argc, &argv);

        // Exchange messages with process 
        // whose rank differs by one.
        // Rank 0 must be excluded.
        int nPeerRank = (nRank & 1) ? nRank + 1 : nRank - 1;

        if ( nPeerRank > 0 && nPeerRank < nProcCount )
        {
            // Send a message from a process with lower rank first, 
            // then let the one with the higher rank reply.
            // Log the send/receive operations. 
            // Note that you have to log communications via VT API 
            // calls, sinse the application is not linked against 
            // the normal ITC library.
             
            if (nRank < nPeerRank)
            {
                VT_log_sendmsg(nPeerRank, 0, 0, VT_COMM_WORLD, VT_NOSCL);
                MPI_Send(NULL, 0, MPI_CHAR, nPeerRank, 0, MPI_COMM_WORLD);

                perform_dummy_work(3);

                MPI_Recv(NULL, 0, MPI_CHAR, nPeerRank, 0, MPI_COMM_WORLD, &status);
                VT_log_recvmsg(status.MPI_SOURCE, 0, status.MPI_TAG,
                        VT_COMM_WORLD, VT_NOSCL);

                perform_dummy_work(1);
            }
            else
            {
                perform_dummy_work(1);

                MPI_Recv(NULL, 0, MPI_CHAR, nPeerRank, 0, MPI_COMM_WORLD, &status);
                VT_log_recvmsg(status.MPI_SOURCE, 0, status.MPI_TAG, 
                               VT_COMM_WORLD, VT_NOSCL);

                VT_log_sendmsg(nPeerRank, 0, 0, VT_COMM_WORLD, VT_NOSCL);
                MPI_Send(NULL, 0, MPI_CHAR, nPeerRank, 0, MPI_COMM_WORLD);

                perform_dummy_work(1);
            }

        } // if appropriate neighbour rank was found

        // Finalize the trace data collection
        VT_finalize();

    } // rank != 0

    MPI_Finalize();

    return 0;

} // main

