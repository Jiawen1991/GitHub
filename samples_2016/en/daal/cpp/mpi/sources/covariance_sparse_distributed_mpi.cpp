/* file: covariance_sparse_distributed_mpi.cpp */
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
!    C++ sample of sparse variance-covariance matrix computation in the
!    distributed processing mode
!
!******************************************************************************/

/**
 * <a name="DAAL-SAMPLE-CPP-COVARIANCE_SPARSE_DISTRIBUTED"></a>
 * \example covariance_sparse_distributed_mpi.cpp
 */

#include <mpi.h>
#include "daal.h"
#include "service.h"

using namespace std;
using namespace daal;
using namespace daal::algorithms;

typedef float  dataFPType;          /* Data floating-point type */
typedef double algorithmFPType;     /* Algorithm floating-point type */

/* Input data set parameters */
const size_t nBlocks         = 4;

int rankId, comm_size;
#define mpi_root 0

const string datasetFileNames[] =
{
    "./data/distributed/covcormoments_sparse_1.csv",
    "./data/distributed/covcormoments_sparse_2.csv",
    "./data/distributed/covcormoments_sparse_3.csv",
    "./data/distributed/covcormoments_sparse_4.csv"
};

int main(int argc, char *argv[])
{
    checkArguments(argc, argv, 4, &datasetFileNames[0], &datasetFileNames[1], &datasetFileNames[2], &datasetFileNames[3]);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rankId);

    /* Initialize FileDataSource<CSVFeatureManager> to retrieve the input data from a .csv file */
    CSRNumericTable *dataTable = createSparseTable<dataFPType>(datasetFileNames[rankId]);

    /* Create an algorithm to compute a sparse variance-covariance matrix on local nodes */
    covariance::Distributed<step1Local, algorithmFPType, covariance::fastCSR> localAlgorithm;

    /* Set the input data set to the algorithm */
    localAlgorithm.input.set(covariance::data, services::SharedPtr<CSRNumericTable>(dataTable));

    /* Compute a sparse variance-covariance matrix */
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
        /* Create an algorithm to compute a sparse variance-covariance matrix on the master node */
        covariance::Distributed<step2Master, algorithmFPType, covariance::fastCSR> masterAlgorithm;

        for( size_t i = 0; i < nBlocks ; i++ )
        {
            /* Deserialize partial results from step 1 */
            OutputDataArchive dataArch( serializedData.get() + perNodeArchLength * i, perNodeArchLength );

            services::SharedPtr<covariance::PartialResult> dataForStep2FromStep1 =
                    services::SharedPtr<covariance::PartialResult>( new covariance::PartialResult() );

            dataForStep2FromStep1->deserialize(dataArch);

            /* Set local partial results as input for the master-node algorithm */
            masterAlgorithm.input.add(covariance::partialResults, dataForStep2FromStep1 );
        }

        /* Merge and finalizeCompute a sparse variance-covariance matrix on the master node */
        masterAlgorithm.compute();
        masterAlgorithm.finalizeCompute();

        /* Retrieve the algorithm results */
        services::SharedPtr<covariance::Result> result = masterAlgorithm.getResult();

        /* Print the results */
        printNumericTable(result->get(covariance::covariance), "Covariance matrix (upper left square 10*10) :", 10, 10);
        printNumericTable(result->get(covariance::mean),       "Mean vector:", 1, 10);
    }

    MPI_Finalize();

    return 0;
}
