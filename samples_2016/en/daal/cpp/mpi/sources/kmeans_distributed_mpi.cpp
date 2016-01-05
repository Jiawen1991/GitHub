/* file: kmeans_distributed_mpi.cpp */
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
!    C++ sample of K-Means clustering in the distributed processing mode
!******************************************************************************/

/**
 * <a name="DAAL-SAMPLE-CPP-KMEANS_DISTRIBUTED"></a>
 * \example kmeans_distributed_mpi.cpp
 */

#include <mpi.h>
#include "daal.h"
#include "service.h"
#include "stdio.h"

using namespace std;
using namespace daal;
using namespace daal::algorithms;

/* K-Means algorithm parameters */
const size_t nClusters   = 20;
const size_t nIterations = 5;
const size_t nBlocks     = 4;
const size_t nVectorsInBlock = 10000;

/* Parameters for computing */
byte   *nodeCentroids;
size_t CentroidsArchLength;
services::SharedPtr<NumericTable> centroids;
InputDataArchive centroidsDataArch;

/* Input data set parameters */
const string dataFileNames[4] =
{
    "./data/distributed/kmeans.csv", "./data/distributed/kmeans.csv",
    "./data/distributed/kmeans.csv", "./data/distributed/kmeans.csv"
};

int rankId, comm_size;
#define mpi_root 0

void init();
void compute();

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rankId);

    init();

    for(size_t it = 0; it < nIterations; it++)
    {
        compute();
    }

    delete[] nodeCentroids;

    /* Print the clusterization results */
    if(rankId == mpi_root)
    {
        printNumericTable(centroids, "First 10 dimensions of centroids:", 20, 10);
    }

    MPI_Finalize();

    return 0;
}

void init()
{
    /* Initialize FileDataSource<CSVFeatureManager> to retrieve the input data from a .csv file */
    FileDataSource<CSVFeatureManager> dataSource(dataFileNames[rankId], DataSource::doAllocateNumericTable,
                                                 DataSource::doDictionaryFromContext);
    /* Retrieve the input data */
    dataSource.loadDataBlock();

    /* Create an algorithm to compute k-means on local nodes */
    kmeans::init::Distributed<step1Local,double,kmeans::init::randomDense> localInit(nClusters, nBlocks * nVectorsInBlock, rankId * nVectorsInBlock);

    /* Set the input data set to the algorithm */
    localInit.input.set(kmeans::init::data, dataSource.getNumericTable());

    /* Compute k-means */
    localInit.compute();

    /* Serialize partial results required by step 2 */
    services::SharedPtr<byte> serializedData;
    InputDataArchive dataArch;
    localInit.getPartialResult()->serialize( dataArch );
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
        /* Create an algorithm to compute k-means on the master node */
        kmeans::init::Distributed<step2Master, double, kmeans::init::randomDense> masterInit(nClusters);

        for( size_t i = 0; i < nBlocks ; i++ )
        {
            /* Deserialize partial results from step 1 */
            OutputDataArchive dataArch( serializedData.get() + perNodeArchLength * i, perNodeArchLength );

            services::SharedPtr<kmeans::init::PartialResult> dataForStep2FromStep1 = services::SharedPtr<kmeans::init::PartialResult>(
                                                                               new kmeans::init::PartialResult() );
            dataForStep2FromStep1->deserialize(dataArch);

            /* Set local partial results as input for the master-node algorithm */
            masterInit.input.add(kmeans::init::partialResults, dataForStep2FromStep1 );
        }

        /* Merge and finalizeCompute k-means on the master node */
        masterInit.compute();
        masterInit.finalizeCompute();

        centroids = masterInit.getResult()->get(kmeans::init::centroids);
    }
}

void compute()
{
    if(rankId == mpi_root)
    {
        /*Retrieve the algorithm results and serialize them */
        centroids->serialize( centroidsDataArch );
        CentroidsArchLength = centroidsDataArch.getSizeOfArchive();
    }

    /* Get partial results from the root node */
    MPI_Bcast( &CentroidsArchLength, sizeof(size_t), MPI_CHAR, mpi_root, MPI_COMM_WORLD );

    nodeCentroids = new byte[ CentroidsArchLength ];

    if(rankId == mpi_root)
    {
        centroidsDataArch.copyArchiveToArray( nodeCentroids, CentroidsArchLength );
    }

    MPI_Bcast( nodeCentroids, CentroidsArchLength, MPI_CHAR, mpi_root, MPI_COMM_WORLD );

    /* Deserialize centroids data */
    OutputDataArchive centroidsDataArch( nodeCentroids, CentroidsArchLength );

    centroids = services::SharedPtr<NumericTable>( new HomogenNumericTable<double>() );

    centroids->deserialize(centroidsDataArch);

    /* Initialize FileDataSource<CSVFeatureManager> to retrieve the input data from a .csv file */
    FileDataSource<CSVFeatureManager> dataSource(dataFileNames[rankId], DataSource::doAllocateNumericTable,
                                                 DataSource::doDictionaryFromContext);

    /* Retrieve the input data */
    dataSource.loadDataBlock();

    /* Create an algorithm to compute k-means on local nodes */
    kmeans::Distributed<step1Local> localAlgorithm(nClusters);

    /* Set the input data set to the algorithm */
    localAlgorithm.input.set(kmeans::data,           dataSource.getNumericTable());
    localAlgorithm.input.set(kmeans::inputCentroids, centroids);

    /* Compute k-means */
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
        /* Create an algorithm to compute k-means on the master node */
        kmeans::Distributed<step2Master> masterAlgorithm(nClusters);

        for( size_t i = 0; i < nBlocks ; i++ )
        {
            /* Deserialize partial results from step 1 */
            OutputDataArchive dataArch( serializedData.get() + perNodeArchLength * i, perNodeArchLength );

            services::SharedPtr<kmeans::PartialResult> dataForStep2FromStep1 = services::SharedPtr<kmeans::PartialResult>(
                                                                               new kmeans::PartialResult() );
            dataForStep2FromStep1->deserialize(dataArch);

            /* Set local partial results as input for the master-node algorithm */
            masterAlgorithm.input.add(kmeans::partialResults, dataForStep2FromStep1 );
        }

        /* Merge and finalizeCompute k-means on the master node */
        masterAlgorithm.compute();
        masterAlgorithm.finalizeCompute();

        /* Retrieve the algorithm results */
        centroids = masterAlgorithm.getResult()->get(kmeans::centroids);
    }
}
