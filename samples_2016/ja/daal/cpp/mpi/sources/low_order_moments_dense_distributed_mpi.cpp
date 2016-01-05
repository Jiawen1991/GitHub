/* file: low_order_moments_dense_distributed_mpi.cpp */
/*******************************************************************************
!  Copyright(C) 2014-2015 Intel Corporation. All Rights Reserved.
!
!  The source code, information  and  material ("Material") contained herein is
!  owned  by Intel Corporation or its suppliers or licensors, and title to such
!  Material remains  with Intel Corporation  or its suppliers or licensors. The
!  Material  contains proprietary information  of  Intel or  its  suppliers and
!  licensors. The  Material is protected by worldwide copyright laws and treaty
!  provisions. No  part  of  the  Material  may  be  used,  copied, reproduced,
!  modified, published, uploaded, posted, transmitted, distributed or disclosed
!  in any way  without Intel's  prior  express written  permission. No  license
!  under  any patent, copyright  or  other intellectual property rights  in the
!  Material  is  granted  to  or  conferred  upon  you,  either  expressly,  by
!  implication, inducement,  estoppel or  otherwise.  Any  license  under  such
!  intellectual  property  rights must  be express  and  approved  by  Intel in
!  writing.
!
!  *Third Party trademarks are the property of their respective owners.
!
!  Unless otherwise  agreed  by Intel  in writing, you may not remove  or alter
!  this  notice or  any other notice embedded  in Materials by Intel or Intel's
!  suppliers or licensors in any way.
!
!*******************************************************************************
!  Content:
!    C++ sample of computing low order moments in the distributed processing
!    mode
!
!******************************************************************************/

/**
 * <a name="DAAL-SAMPLE-CPP-LOW_ORDER_MOMENTS_DENSE_DISTRIBUTED"></a>
 * \example low_order_moments_dense_distributed_mpi.cpp
 */

#include <mpi.h>
#include "daal.h"
#include "service.h"

using namespace std;
using namespace daal;
using namespace daal::algorithms;

/* Input data set parameters */
const size_t nBlocks         = 4;

int rankId, comm_size;
#define mpi_root 0

const string datasetFileNames[] =
{
    "./data/distributed/covcormoments_1.csv",
    "./data/distributed/covcormoments_2.csv",
    "./data/distributed/covcormoments_3.csv",
    "./data/distributed/covcormoments_4.csv"
};

int main(int argc, char *argv[])
{
    checkArguments(argc, argv, 4, &datasetFileNames[0], &datasetFileNames[1], &datasetFileNames[2], &datasetFileNames[3]);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rankId);

    /* Initialize FileDataSource<CSVFeatureManager> to retrieve the input data from a .csv file */
    FileDataSource<CSVFeatureManager> dataSource(datasetFileNames[rankId], DataSource::doAllocateNumericTable,
                                                 DataSource::doDictionaryFromContext);

    /* Retrieve the input data */
    dataSource.loadDataBlock();

    /* Create an algorithm to compute low order moments on local nodes */
    low_order_moments::Distributed<step1Local> localAlgorithm;

    /* Set the input data set to the algorithm */
    localAlgorithm.input.set(low_order_moments::data, dataSource.getNumericTable());

    /* Compute low order moments */
    localAlgorithm.compute();

    /* Serialize partial results required by step 2 */
    services::SharedPtr<byte> serializedData;
    InputDataArchive dataArch;
    localAlgorithm.getPartialResult()->serialize( dataArch );
    size_t perNodeArchLength = dataArch.getSizeOfArchive();

    /* Serialized data is of equal size on each node if each node called compute() equal number of times */
    if (rankId == mpi_root)
    {
        serializedData = services::SharedPtr<byte>( new byte[ perNodeArchLength * nBlocks ] );
    }

    byte *nodeResults = new byte[ perNodeArchLength ];
    dataArch.copyArchiveToArray( nodeResults, perNodeArchLength );

    /* Transfer partial results to step 2 on the root node */
    MPI_Gather( nodeResults, perNodeArchLength, MPI_CHAR, serializedData.get(), perNodeArchLength, MPI_CHAR, mpi_root,
                MPI_COMM_WORLD);

    delete[] nodeResults;

    if(rankId == mpi_root)
    {
        /* Create an algorithm to compute low order moments on the master node */
        low_order_moments::Distributed<step2Master> masterAlgorithm;

        for( size_t i = 0; i < nBlocks ; i++ )
        {
            /* Deserialize partial results from step 1 */
            OutputDataArchive dataArch( serializedData.get() + perNodeArchLength * i, perNodeArchLength );

            services::SharedPtr<low_order_moments::PartialResult> dataForStep2FromStep1 = services::SharedPtr<low_order_moments::PartialResult>(
                                                                                                        new low_order_moments::PartialResult() );

            dataForStep2FromStep1->deserialize(dataArch);

            /* Set local partial results as input for the master-node algorithm */
            masterAlgorithm.input.add(low_order_moments::partialResults, dataForStep2FromStep1 );
        }

        /* Merge and finalizeCompute low order moments on the master node */
        masterAlgorithm.compute();
        masterAlgorithm.finalizeCompute();

        /* Retrieve the algorithm results */
        services::SharedPtr<low_order_moments::Result> res = masterAlgorithm.getResult();

        /* Print the results */
        printNumericTable(res->get(low_order_moments::minimum),              "Minimum:");
        printNumericTable(res->get(low_order_moments::maximum),              "Maximum:");
        printNumericTable(res->get(low_order_moments::sum),                  "Sum:");
        printNumericTable(res->get(low_order_moments::sumSquares),           "Sum of squares:");
        printNumericTable(res->get(low_order_moments::sumSquaresCentered),   "Sum of squared difference from the means:");
        printNumericTable(res->get(low_order_moments::mean),                 "Mean:");
        printNumericTable(res->get(low_order_moments::secondOrderRawMoment), "Second order raw moment:");
        printNumericTable(res->get(low_order_moments::variance),             "Variance:");
        printNumericTable(res->get(low_order_moments::standardDeviation),    "Standard deviation:");
        printNumericTable(res->get(low_order_moments::variation),            "Variation:");
    }

    MPI_Finalize();

    return 0;
}
