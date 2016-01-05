/* file: multinomial_naive_bayes_distributed_mpi.cpp */
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
!    C++ sample of Na__ve Bayes classification in the distributed processing
!    mode.
!
!    The program trains the Na__ve Bayes model on a supplied training data set
!    and then performs classification of previously unseen data.
!******************************************************************************/

/**
 * <a name="DAAL-SAMPLE-CPP-MULTINOMIAL_NAIVE_BAYES_DISTRIBUTED"></a>
 * \example multinomial_naive_bayes_distributed_mpi.cpp
 */

#include <mpi.h>
#include "daal.h"
#include "service.h"

using namespace std;
using namespace daal;
using namespace daal::algorithms;
using namespace daal::algorithms::multinomial_naive_bayes;

/* Input data set parameters */
const string trainDatasetFileNames[4] =
{
    "./data/distributed/naivebayes_train.csv", "./data/distributed/naivebayes_train.csv",
    "./data/distributed/naivebayes_train.csv", "./data/distributed/naivebayes_train.csv"
};

string testDatasetFileName = "./data/distributed/naivebayes_test.csv";

const size_t nFeatures            = 20;
const size_t nClasses             = 20;
const size_t nBlocks              = 4;

int rankId, comm_size;
#define mpi_root 0

void trainModel();
void testModel();
void printResults();

services::SharedPtr<training::Result> trainingResult;
services::SharedPtr<classifier::prediction::Result> predictionResult;
services::SharedPtr<NumericTable> testGroundTruth;

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rankId);

    trainModel();

    if(rankId == mpi_root)
    {
        testModel();
        printResults();
    }

    MPI_Finalize();

    return 0;
}

void trainModel()
{
    /* Initialize FileDataSource<CSVFeatureManager> to retrieve the input data from a .csv file */
    FileDataSource<CSVFeatureManager> trainDataSource(trainDatasetFileNames[rankId],
                                                      DataSource::notAllocateNumericTable,
                                                      DataSource::doDictionaryFromContext);

    /* Create Numeric Tables for training data and labels */
    services::SharedPtr<NumericTable> trainData(new HomogenNumericTable<double>(nFeatures, 0, NumericTable::notAllocate));
    services::SharedPtr<NumericTable> trainGroundTruth(new HomogenNumericTable<double>(1, 0, NumericTable::notAllocate));
    services::SharedPtr<NumericTable> mergedData(new MergedNumericTable(trainData, trainGroundTruth));

    /* Retrieve the data from the input file */
    trainDataSource.loadDataBlock(mergedData.get());

    /* Create an algorithm object to train the Na__ve Bayes model based on the local-node data */
    training::Distributed<step1Local> localAlgorithm(nClasses);

    /* Pass a training data set and dependent values to the algorithm */
    localAlgorithm.input.set(classifier::training::data,   trainData);
    localAlgorithm.input.set(classifier::training::labels, trainGroundTruth);

    /* Train the Na__ve Bayes model on local nodes */
    localAlgorithm.compute();

    /* Serialize partial results required by step 2 */
    services::SharedPtr<byte> serializedData;
    InputDataArchive dataArch;
    localAlgorithm.getPartialResult()->serialize(dataArch);
    size_t perNodeArchLength = dataArch.getSizeOfArchive();

    /* Serialized data is of equal size on each node if each node called compute() equal number of times */
    if (rankId == mpi_root)
    {
        serializedData = services::SharedPtr<byte>(new byte[perNodeArchLength * nBlocks]);
    }

    byte *nodeResults = new byte[perNodeArchLength];
    dataArch.copyArchiveToArray( nodeResults, perNodeArchLength );

    /* Transfer partial results to step 2 on the root node */
    MPI_Gather( nodeResults, perNodeArchLength, MPI_CHAR, serializedData.get(), perNodeArchLength, MPI_CHAR, mpi_root,
                MPI_COMM_WORLD);

    delete[] nodeResults;

    if(rankId == mpi_root)
    {
        /* Create an algorithm object to build the final Na__ve Bayes model on the master node */
        training::Distributed<step2Master> masterAlgorithm(nClasses);

        for(size_t i = 0; i < nBlocks ; i++)
        {
            /* Deserialize partial results from step 1 */
            OutputDataArchive dataArch(serializedData.get() + perNodeArchLength * i, perNodeArchLength);

            services::SharedPtr<classifier::training::PartialResult> dataForStep2FromStep1 = services::SharedPtr<classifier::training::PartialResult>
                                                                       (new classifier::training::PartialResult());
            dataForStep2FromStep1->deserialize(dataArch);

            /* Set the local Na__ve Bayes model as input for the master-node algorithm */
            masterAlgorithm.input.add(classifier::training::partialModels, dataForStep2FromStep1);
        }

        /* Merge and finalizeCompute the Na__ve Bayes model on the master node */
        masterAlgorithm.compute();
        masterAlgorithm.finalizeCompute();

        /* Retrieve the algorithm results */
        trainingResult = masterAlgorithm.getResult();
    }
}

void testModel()
{
    /* Initialize FileDataSource<CSVFeatureManager> to retrieve the input data from a .csv file */
    FileDataSource<CSVFeatureManager> testDataSource(testDatasetFileName,
                                                     DataSource::notAllocateNumericTable,
                                                     DataSource::doDictionaryFromContext);

    /* Create Numeric Tables for testing data and labels */
    services::SharedPtr<NumericTable> testData(new HomogenNumericTable<double>(nFeatures, 0, NumericTable::notAllocate));
    testGroundTruth = services::SharedPtr<NumericTable>(new HomogenNumericTable<double>(1, 0, NumericTable::notAllocate));
    services::SharedPtr<NumericTable> mergedData(new MergedNumericTable(testData, testGroundTruth));

    /* Retrieve the data from input file */
    testDataSource.loadDataBlock(mergedData.get());

    /* Create an algorithm object to predict values of the Na__ve Bayes model */
    prediction::Batch<> algorithm(nClasses);

    /* Pass a testing data set and the trained model to the algorithm */
    algorithm.input.set(classifier::prediction::data,  testData);
    algorithm.input.set(classifier::prediction::model, trainingResult->get(classifier::training::model));

    /* Predict values of the Na__ve Bayes model */
    algorithm.compute();

    /* Retrieve the algorithm results */
    predictionResult = algorithm.getResult();
}

void printResults()
{
    printNumericTables<int, int>(testGroundTruth,
                                 predictionResult->get(classifier::prediction::prediction),
                                 "Ground truth", "Classification results",
                                 "NaiveBayes classification results (first 20 observations):", 20);
}
