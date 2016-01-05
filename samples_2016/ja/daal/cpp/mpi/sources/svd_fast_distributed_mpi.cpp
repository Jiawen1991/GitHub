/* file: svd_fast_distributed_mpi.cpp */
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
!    C++ sample of computing singular value decomposition (SVD) in the
!    distributed processing mode
!******************************************************************************/

/**
 * <a name="DAAL-EXAMPLE-CPP-SVD_FAST_DISTRIBUTED_MPI"></a>
 * \example svd_fast_distributed_mpi.cpp
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
const size_t nBlocks      = 4;

const string datasetFileNames[] =
{
    "./data/distributed/svd_1.csv",
    "./data/distributed/svd_2.csv",
    "./data/distributed/svd_3.csv",
    "./data/distributed/svd_4.csv"
};

void computestep1Local();
void computeOnMasterNode();
void finalizeComputestep1Local();

int rankId;
int commSize;
#define mpiRoot 0

services::SharedPtr<data_management::DataCollection> dataFromStep1ForStep3;
services::SharedPtr<NumericTable> Sigma;
services::SharedPtr<NumericTable> V    ;
services::SharedPtr<NumericTable> Ui   ;

services::SharedPtr<byte> serializedData;
size_t perNodeArchLength;

int main(int argc, char *argv[])
{
    checkArguments(argc, argv, 4, &datasetFileNames[0], &datasetFileNames[1], &datasetFileNames[2], &datasetFileNames[3]);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &commSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &rankId);

    if(nBlocks != commSize)
    {
        if(rankId == mpiRoot)
        {
            printf("%d MPI ranks != %zu datasets available, so please start exactly %zu ranks.\n", commSize, nBlocks, nBlocks);
        }
        MPI_Finalize();
        return 0;
    }

    computestep1Local();

    if (rankId == mpiRoot)
    {
        computeOnMasterNode();
    }

    finalizeComputestep1Local();

    /* Print the results */
    if (rankId == mpiRoot)
    {
        printNumericTable(Sigma, "Singular values:");
        printNumericTable(V,     "Right orthogonal matrix V:");
        printNumericTable(Ui   , "Part of left orthogonal matrix U from root node:", 10);
    }

    MPI_Finalize();

    return 0;
}

void computestep1Local()
{
    /* Initialize FileDataSource<CSVFeatureManager> to retrieve the input data from a .csv file */
    FileDataSource<CSVFeatureManager> dataSource(datasetFileNames[rankId], DataSource::doAllocateNumericTable,
                                                 DataSource::doDictionaryFromContext);

    /* Retrieve the input data */
    dataSource.loadDataBlock();

    /* Create an algorithm to compute SVD on local nodes */
    svd::Distributed<step1Local> alg;

    alg.input.set( svd::data, dataSource.getNumericTable() );

    /* Compute SVD */
    alg.compute();

    services::SharedPtr<data_management::DataCollection> dataFromStep1ForStep2;
    dataFromStep1ForStep2 = alg.getPartialResult()->get( svd::outputOfStep1ForStep2 );
    dataFromStep1ForStep3 = alg.getPartialResult()->get( svd::outputOfStep1ForStep3 );

    /* Serialize partial results required by step 2 */
    InputDataArchive dataArch;
    dataFromStep1ForStep2->serialize( dataArch );
    perNodeArchLength = dataArch.getSizeOfArchive();

    /* Serialized data is of equal size on each node if each node called compute() equal number of times */
    if (rankId == mpiRoot)
    {
        serializedData = services::SharedPtr<byte>( new byte[ perNodeArchLength * nBlocks ] );
    }

    byte *nodeResults = new byte[ perNodeArchLength ];
    dataArch.copyArchiveToArray( nodeResults, perNodeArchLength );

    /* Transfer partial results to step 2 on the root node */
    MPI_Gather( nodeResults, perNodeArchLength, MPI_CHAR, serializedData.get(), perNodeArchLength, MPI_CHAR, mpiRoot,
                MPI_COMM_WORLD);

    delete[] nodeResults;
}

void computeOnMasterNode()
{
    /* Create an algorithm to compute SVD on the master node */
    svd::Distributed<step2Master> alg;

    for (size_t i = 0; i < nBlocks; i++)
    {
        /* Deserialize partial results from step 1 */
        OutputDataArchive dataArch( serializedData.get() + perNodeArchLength * i, perNodeArchLength );

        services::SharedPtr<data_management::DataCollection> dataForStep2FromStep1 =
            services::SharedPtr<data_management::DataCollection>( new data_management::DataCollection() );
        dataForStep2FromStep1->deserialize(dataArch);

        alg.input.add( svd::inputOfStep2FromStep1, i, dataForStep2FromStep1 );
    }

    /* Compute SVD */
    alg.compute();

    services::SharedPtr<svd::DistributedPartialResult> pres = alg.getPartialResult();
    services::SharedPtr<KeyValueDataCollection> inputForStep3FromStep2 = pres->get( svd::outputOfStep2ForStep3 );

    for (size_t i = 0; i < nBlocks; i++)
    {
        /* Serialize partial results to transfer to local nodes for step 3 */
        InputDataArchive dataArch;
        (*inputForStep3FromStep2)[i]->serialize( dataArch );

        if( i == 0 )
        {
            perNodeArchLength = dataArch.getSizeOfArchive();
            /* Serialized data is of equal size for each node if it was equal in step 1 */
            serializedData = services::SharedPtr<byte>( new byte[ perNodeArchLength * nBlocks ] );
        }

        dataArch.copyArchiveToArray( serializedData.get() + perNodeArchLength * i, perNodeArchLength );
    }

    services::SharedPtr<svd::Result> res = alg.getResult();

    Sigma = res->get(svd::singularValues     );
    V     = res->get(svd::rightSingularMatrix);
}

void finalizeComputestep1Local()
{
    /* Get the size of the serialized input */
    MPI_Bcast( &perNodeArchLength, sizeof(size_t), MPI_CHAR, mpiRoot, MPI_COMM_WORLD );

    byte *nodeResults = new byte[ perNodeArchLength ];

    /* Transfer partial results from the root node */

    MPI_Scatter(serializedData.get(), perNodeArchLength, MPI_CHAR, nodeResults, perNodeArchLength, MPI_CHAR, mpiRoot,
                MPI_COMM_WORLD);

    /* Deserialize partial results from step 2 */
    OutputDataArchive dataArch( nodeResults, perNodeArchLength );

    services::SharedPtr<data_management::DataCollection> dataFromStep2ForStep3 =
        services::SharedPtr<data_management::DataCollection>( new data_management::DataCollection() );
    dataFromStep2ForStep3->deserialize(dataArch);

    delete[] nodeResults;

    /* Create an algorithm to compute SVD on the master node */
    svd::Distributed<step3Local> alg;

    alg.input.set( svd::inputOfStep3FromStep1, dataFromStep1ForStep3 );
    alg.input.set( svd::inputOfStep3FromStep2, dataFromStep2ForStep3 );

    /* Compute SVD */
    alg.compute();
    alg.finalizeCompute();

    services::SharedPtr<svd::Result> res = alg.getResult();

    Ui = res->get(svd::leftSingularMatrix);
}
