/* file: SparkImplicitALSSparse.java */
/*
 //  Copyright(C) 2014-2015 Intel Corporation. All Rights Reserved.
 //
 //  The source code, information  and  material ("Material") contained herein is
 //  owned  by Intel Corporation or its suppliers or licensors, and title to such
 //  Material remains  with Intel Corporation  or its suppliers or licensors. The
 //  Material  contains proprietary information  of  Intel or  its  suppliers and
 //  licensors. The  Material is protected by worldwide copyright laws and treaty
 //  provisions. No  part  of  the  Material  may  be  used,  copied, reproduced,
 //  modified, published, uploaded, posted, transmitted, distributed or disclosed
 //  in any way  without Intel's  prior  express written  permission. No  license
 //  under  any patent, copyright  or  other intellectual property rights  in the
 //  Material  is  granted  to  or  conferred  upon  you,  either  expressly,  by
 //  implication, inducement,  estoppel or  otherwise.  Any  license  under  such
 //  intellectual  property  rights must  be express  and  approved  by  Intel in
 //  writing.
 //
 //  *Third Party trademarks are the property of their respective owners.
 //
 //  Unless otherwise  agreed  by Intel  in writing, you may not remove  or alter
 //  this  notice or  any other notice embedded  in Materials by Intel or Intel's
 //  suppliers or licensors in any way.
 //
 ////////////////////////////////////////////////////////////////////////////////
 //  Content:
 //     Java sample of the implicit alternating least squares (ALS) algorithm in
 //     the distributed processing mode.
 //
 //     The program trains the implicit ALS model on a supplied training data
 //     set.
 ////////////////////////////////////////////////////////////////////////////////
 */

package DAAL;

import java.util.LinkedList;
import java.util.List;
import java.io.IOException;
import java.lang.ClassNotFoundException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.nio.DoubleBuffer;

import org.apache.spark.api.java.*;
import org.apache.spark.api.java.function.*;
import org.apache.spark.SparkConf;
import org.apache.spark.broadcast.Broadcast;
import java.util.Collections;
import java.util.Comparator;
import java.nio.IntBuffer;

import scala.Tuple2;
import scala.Tuple3;

import com.intel.daal.algorithms.implicit_als.*;
import com.intel.daal.algorithms.implicit_als.training.*;
import com.intel.daal.algorithms.implicit_als.training.init.*;
import com.intel.daal.algorithms.implicit_als.prediction.ratings.*;
import com.intel.daal.data_management.data.*;
import com.intel.daal.services.*;

public class SparkImplicitALSSparse {

    public static class TrainingResult {
        JavaPairRDD<Integer, DistributedPartialResultStep4> itemsFactors;
        JavaPairRDD<Integer, DistributedPartialResultStep4> usersFactors;
    }

    static final int nUsers = 46;           /* Full number of users */
    static final int nItems = 21;           /* Full number of items */
    static final int nFactors = 2;          /* Number of factors */
    static final int maxIterations = 5;     /* Number of iterations of the implicit ALS training algorithm */

    public static TrainingResult trainModel(
        JavaSparkContext sc,
        JavaPairRDD<Integer, CSRNumericTable> dataRDD,
        JavaPairRDD<Integer, CSRNumericTable> transposedDataRDD
    )
    throws IOException, ClassNotFoundException {
        Long[] usersPartition = computePartition(dataRDD);
        Long[] itemsPartition = computePartition(transposedDataRDD);

        long nBlocks = dataRDD.count();
        JavaPairRDD<Integer, KeyValueDataCollection> usersOutBlocks = computeOutBlocks(dataRDD, itemsPartition, nBlocks);
        JavaPairRDD<Integer, KeyValueDataCollection> itemsOutBlocks = computeOutBlocks(transposedDataRDD, usersPartition, nBlocks);

        JavaPairRDD<Integer, DistributedPartialResultStep4> itemsPartialResultLocal = initializeModel(transposedDataRDD);
        JavaPairRDD<Integer, DistributedPartialResultStep4> usersPartialResultLocal = null;

        JavaPairRDD<Integer, DistributedPartialResultStep2> step2MasterResultCopies = null;

        JavaPairRDD<Integer, DistributedPartialResultStep1> step1LocalResult = null;
        JavaPairRDD<Integer, DistributedPartialResultStep3> step3LocalResult = null;

        Broadcast<Long[]> usersPartitionBroadcast = sc.broadcast(usersPartition);
        Broadcast<Long[]> itemsPartitionBroadcast = sc.broadcast(itemsPartition);

        for (int iteration = 0; iteration < maxIterations; iteration++) {
            step1LocalResult = computeStep1Local(itemsPartialResultLocal);
            step2MasterResultCopies = computeStep2Master(sc, step1LocalResult);
            step3LocalResult = computeStep3Local(itemsPartitionBroadcast, itemsPartialResultLocal, itemsOutBlocks);
            usersPartialResultLocal = computeStep4Local(step2MasterResultCopies, step3LocalResult, dataRDD);

            step1LocalResult = computeStep1Local(usersPartialResultLocal);
            step2MasterResultCopies = computeStep2Master(sc, step1LocalResult);
            step3LocalResult = computeStep3Local(usersPartitionBroadcast, usersPartialResultLocal, usersOutBlocks);
            itemsPartialResultLocal = computeStep4Local(step2MasterResultCopies, step3LocalResult, transposedDataRDD);
        }
        TrainingResult result = new TrainingResult();
        result.itemsFactors = itemsPartialResultLocal;
        result.usersFactors = usersPartialResultLocal;
        return result;
    }

    public static JavaRDD<Tuple3<Integer, Integer, RatingsResult>> testModel(
        JavaPairRDD<Integer, DistributedPartialResultStep4> usersFactors,
        JavaPairRDD<Integer, DistributedPartialResultStep4> itemsFactors
    ) {

        JavaPairRDD<Tuple2<Integer, DistributedPartialResultStep4>, Tuple2<Integer, DistributedPartialResultStep4>> allPairs =
            usersFactors.cartesian(itemsFactors);

        JavaRDD<Tuple3<Integer, Integer, RatingsResult>> predictedRatings = allPairs.map(
                    new Function<Tuple2<Tuple2<Integer, DistributedPartialResultStep4>, Tuple2<Integer, DistributedPartialResultStep4>>,
        Tuple3<Integer, Integer, RatingsResult>>() {
            public Tuple3<Integer, Integer, RatingsResult> call(
                Tuple2<Tuple2<Integer, DistributedPartialResultStep4>, Tuple2<Integer, DistributedPartialResultStep4>> tup) {
                DaalContext context = new DaalContext();

                DistributedPartialResultStep4 usersPartialResultLocal = tup._1._2;
                usersPartialResultLocal.unpack(context);
                DistributedPartialResultStep4 itemsPartialResultLocal = tup._2._2;
                itemsPartialResultLocal.unpack(context);

                RatingsDistributed algorithm = new RatingsDistributed(context, Double.class, RatingsMethod.defaultDense);
                algorithm.parameter.setNFactors(nFactors);

                algorithm.input.set(RatingsPartialModelInputId.usersPartialModel,
                                    usersPartialResultLocal.get(DistributedPartialResultStep4Id.outputOfStep4ForStep1));
                algorithm.input.set(RatingsPartialModelInputId.itemsPartialModel,
                                    itemsPartialResultLocal.get(DistributedPartialResultStep4Id.outputOfStep4ForStep1));
                algorithm.compute();
                RatingsResult result = algorithm.finalizeCompute();


                usersPartialResultLocal.pack();
                itemsPartialResultLocal.pack();
                result.pack();

                context.dispose();
                return new Tuple3<Integer, Integer, RatingsResult>(tup._1._1, tup._2._1, result);
            }
        });

        return predictedRatings;
    }


    public static JavaPairRDD<Integer, DistributedPartialResultStep4> initializeModel(JavaPairRDD<Integer, CSRNumericTable> transposedDataRDD) {
        return transposedDataRDD.mapToPair(
        new PairFunction<Tuple2<Integer, CSRNumericTable>, Integer, DistributedPartialResultStep4>() {
            public Tuple2<Integer, DistributedPartialResultStep4> call(Tuple2<Integer, CSRNumericTable> tup) {
                DaalContext context = new DaalContext();
                tup._2.unpack(context);

                /* Create an algorithm object to initialize the implicit ALS model with the fastCSR method */
                InitDistributed initAlgorithm = new InitDistributed(context, Double.class, InitMethod.fastCSR);
                initAlgorithm.parameter.setFullNUsers(nUsers);
                initAlgorithm.parameter.setNFactors(nFactors);
                initAlgorithm.parameter.setSeed(initAlgorithm.parameter.getSeed() + tup._1);

                /* Pass a training data set and dependent values to the algorithm */
                initAlgorithm.input.set(InitInputId.data, tup._2);

                /* Initialize the implicit ALS model */
                InitPartialResult initPartialResult = initAlgorithm.compute();

                PartialModel partialModel = initPartialResult.get(InitPartialResultId.partialModel);

                DistributedPartialResultStep4 returnValue = new DistributedPartialResultStep4(context);
                returnValue.set(DistributedPartialResultStep4Id.outputOfStep4ForStep1, partialModel);
                returnValue.pack();

                context.dispose();

                return new Tuple2<Integer, DistributedPartialResultStep4>(tup._1, returnValue);
            }
        });
    }

    public static JavaPairRDD<Integer, DistributedPartialResultStep1> computeStep1Local(
        JavaPairRDD<Integer, DistributedPartialResultStep4> partialResultLocal) {
        return partialResultLocal.mapToPair(
        new PairFunction<Tuple2<Integer, DistributedPartialResultStep4>, Integer, DistributedPartialResultStep1>() {
            public Tuple2<Integer, DistributedPartialResultStep1> call(Tuple2<Integer, DistributedPartialResultStep4> tup) {
                DaalContext context = new DaalContext();
                tup._2.unpack(context);

                /* Create algorithm objects to compute a implisit ALS algorithm in the distributed processing mode using the fastCSR method */
                DistributedStep1Local algorithm = new DistributedStep1Local(context, Double.class, TrainingMethod.fastCSR);
                algorithm.parameter.setNFactors(nFactors);

                /* Set input objects for the algorithm */
                algorithm.input.set(PartialModelInputId.partialModel,
                                    tup._2.get(DistributedPartialResultStep4Id.outputOfStep4ForStep1));

                /* Compute partial estimates on local nodes */
                DistributedPartialResultStep1 step1LocalResult = algorithm.compute();
                step1LocalResult.pack();
                tup._2.pack();

                context.dispose();
                return new Tuple2<Integer, DistributedPartialResultStep1>(tup._1, step1LocalResult);
            }
        }
               );
    }

    public static JavaPairRDD<Integer, DistributedPartialResultStep2> computeStep2Master(
        JavaSparkContext sc,
        JavaPairRDD<Integer, DistributedPartialResultStep1> step1LocalResult)
    throws IOException, ClassNotFoundException {
        DaalContext context = new DaalContext();

        List<Tuple2<Integer, DistributedPartialResultStep1>> step1LocalResultList = step1LocalResult.collect();

        /* Create algorithm objects to compute a implisit ALS algorithm in the distributed processing mode using the fastCSR method */
        DistributedStep2Master algorithm = new DistributedStep2Master(context, Double.class, TrainingMethod.fastCSR);
        algorithm.parameter.setNFactors(nFactors);
        int nBlocks = (int)step1LocalResultList.size();

        /* Set input objects for the algorithm */
        for (int i = 0; i < nBlocks; i++) {
            step1LocalResultList.get(i)._2.unpack(context);
            algorithm.input.add(MasterInputId.inputOfStep2FromStep1, step1LocalResultList.get(i)._2);
        }

        /* Compute a partial estimate on the master node from the partial estimates on local nodes */
        DistributedPartialResultStep2 step2MasterResult = algorithm.compute();

        /* Create deep copies of master result */
        step2MasterResult.pack();
        byte[] buffer = serializeObject(step2MasterResult);
        List<DistributedPartialResultStep2> list = new LinkedList<DistributedPartialResultStep2>();
        for(int i = 0; i < nBlocks; i++) {
            list.add((DistributedPartialResultStep2)deserializeObject(buffer));
        }

        JavaPairRDD<Integer, DistributedPartialResultStep2> rdd = sc.parallelize(list, nBlocks).zipWithIndex().mapToPair(
        new PairFunction<Tuple2<DistributedPartialResultStep2, Long>, Integer, DistributedPartialResultStep2>() {
            public Tuple2<Integer, DistributedPartialResultStep2> call(Tuple2<DistributedPartialResultStep2, Long> tup) {
                return new Tuple2<Integer, DistributedPartialResultStep2>((int) (long)tup._2, tup._1);
            }
        }
                );
        context.dispose();
        return rdd;
    }

    public static JavaPairRDD<Integer, DistributedPartialResultStep3> computeStep3Local(
        final Broadcast<Long[]> partition,
        JavaPairRDD<Integer, DistributedPartialResultStep4> partialResultLocal,
        JavaPairRDD<Integer, KeyValueDataCollection> outBlocks) {

        JavaPairRDD<Integer, Tuple2<DistributedPartialResultStep4, KeyValueDataCollection>> joined = partialResultLocal.join(outBlocks);

        return joined.mapToPair(
        new PairFunction<Tuple2<Integer, Tuple2<DistributedPartialResultStep4, KeyValueDataCollection>>, Integer, DistributedPartialResultStep3>() {
            public Tuple2<Integer, DistributedPartialResultStep3> call(
                Tuple2<Integer, Tuple2<DistributedPartialResultStep4, KeyValueDataCollection>> tup) {

                DaalContext context = new DaalContext();
                tup._2._1.unpack(context);
                tup._2._2.unpack(context);

                DistributedStep3Local algorithm = new DistributedStep3Local(context, Double.class, TrainingMethod.fastCSR);
                algorithm.parameter.setNFactors(nFactors);

                long[] offsetArray = new long[1];
                Long[] array = partition.value();
                offsetArray[0] = array[tup._1];
                HomogenNumericTable offsetTable = new HomogenNumericTable(context, offsetArray, 1, 1);
                algorithm.input.set(PartialModelInputId.partialModel, tup._2._1.get(DistributedPartialResultStep4Id.outputOfStep4ForStep3));
                algorithm.input.set(Step3LocalCollectionInputId.partialModelBlocksToNode, tup._2._2);
                algorithm.input.set(Step3LocalNumericTableInputId.offset, offsetTable);

                DistributedPartialResultStep3 partialResult = algorithm.compute();
                partialResult.pack();
                tup._2._1.pack();
                context.dispose();
                return new Tuple2<Integer, DistributedPartialResultStep3>(tup._1, partialResult);
            }
        }
               );
    }

    public static JavaPairRDD<Integer, Tuple3<CSRNumericTable, Iterable<Tuple2<Integer, PartialModel>>, DistributedPartialResultStep2>>
    redistributeBlocks(
        JavaPairRDD<Integer, DistributedPartialResultStep2> step2MasterResult,
        JavaPairRDD<Integer, DistributedPartialResultStep3> step3LocalResult,
        JavaPairRDD<Integer, CSRNumericTable> dataRDD) {

        JavaPairRDD<Integer, Tuple2<Integer, PartialModel>> step3LocalResultKeyValue = step3LocalResult.flatMapToPair(
        new PairFlatMapFunction<Tuple2<Integer, DistributedPartialResultStep3>, Integer, Tuple2<Integer, PartialModel>>() {
            public Iterable<Tuple2<Integer, Tuple2<Integer, PartialModel>>> call(Tuple2<Integer, DistributedPartialResultStep3> tup) {
                DaalContext context = new DaalContext();
                DistributedPartialResultStep3 partialResultStep3 = tup._2;
                partialResultStep3.unpack(context);

                KeyValueDataCollection collection = partialResultStep3.get(DistributedPartialResultStep3Id.outputOfStep3ForStep4);

                List<Tuple2<Integer, Tuple2<Integer, PartialModel>>> list = new LinkedList<Tuple2<Integer, Tuple2<Integer, PartialModel>>>();

                for(int i = 0; i < collection.size(); i++) {
                    PartialModel m1 = (PartialModel)collection.getValueByIndex(i);
                    m1.pack();
                    Tuple2<Integer, PartialModel> blockFromIdWithModel = new Tuple2<Integer, PartialModel>(tup._1, m1);
                    Tuple2<Integer, Tuple2<Integer, PartialModel>> blockToIdWithTuple =
                        new Tuple2<Integer, Tuple2<Integer, PartialModel>>((int)collection.getKeyByIndex(i), blockFromIdWithModel);
                    list.add(blockToIdWithTuple);
                }
                context.dispose();

                return list;
            }
        });

        JavaPairRDD<Integer, Iterable<Tuple2<Integer, PartialModel>>> blocksFromOtherNodes = step3LocalResultKeyValue.groupByKey();

        JavaPairRDD<Integer, Tuple2<CSRNumericTable, Iterable<Tuple2<Integer, PartialModel>>>> rddWithData = dataRDD.join(blocksFromOtherNodes);

        JavaPairRDD<Integer, Tuple3<CSRNumericTable, Iterable<Tuple2<Integer, PartialModel>>, DistributedPartialResultStep2>> rddToCompute =
            rddWithData.join(step2MasterResult).mapToPair(
                new PairFunction <
                Tuple2<Integer, Tuple2<Tuple2<CSRNumericTable, Iterable<Tuple2<Integer, PartialModel>>>, DistributedPartialResultStep2>>,
                Integer, Tuple3<CSRNumericTable, Iterable<Tuple2<Integer, PartialModel>>, DistributedPartialResultStep2>
                >
        () {
            public Tuple2<Integer, Tuple3<CSRNumericTable, Iterable<Tuple2<Integer, PartialModel>>, DistributedPartialResultStep2>> call(
                Tuple2<Integer, Tuple2<Tuple2<CSRNumericTable, Iterable<Tuple2<Integer, PartialModel>> >, DistributedPartialResultStep2>> tup) {
                return new Tuple2<Integer, Tuple3<CSRNumericTable, Iterable<Tuple2<Integer, PartialModel>>, DistributedPartialResultStep2>>(tup._1,
                        new Tuple3<CSRNumericTable, Iterable<Tuple2<Integer, PartialModel>>, DistributedPartialResultStep2>(
                            tup._2._1._1, tup._2._1._2, tup._2._2));
            }
        }
            );
        return rddToCompute;
    }

    public static JavaPairRDD<Integer, DistributedPartialResultStep4> computeStep4Local(
        JavaPairRDD<Integer, DistributedPartialResultStep2> step2MasterResult,
        JavaPairRDD<Integer, DistributedPartialResultStep3> step3LocalResult,
        JavaPairRDD<Integer, CSRNumericTable> dataRDD) {

        JavaPairRDD<Integer, Tuple3<CSRNumericTable, Iterable<Tuple2<Integer, PartialModel>>, DistributedPartialResultStep2>> rddToCompute =
            redistributeBlocks(step2MasterResult, step3LocalResult, dataRDD);

        return rddToCompute.mapToPair(
                   new PairFunction<Tuple2<Integer, Tuple3<CSRNumericTable, Iterable<Tuple2<Integer, PartialModel>>, DistributedPartialResultStep2>>,
        Integer, DistributedPartialResultStep4>() {
            public Tuple2<Integer, DistributedPartialResultStep4> call(
                Tuple2<Integer, Tuple3<CSRNumericTable, Iterable<Tuple2<Integer, PartialModel>>, DistributedPartialResultStep2>> tup) {
                DaalContext context = new DaalContext();
                Tuple3<CSRNumericTable, Iterable<Tuple2<Integer, PartialModel>>, DistributedPartialResultStep2> tuple = tup._2;
                CSRNumericTable dataTable = tuple._1();
                dataTable.unpack(context);
                KeyValueDataCollection step4LocalInput = new KeyValueDataCollection(context);
                for (Tuple2<Integer, PartialModel> item : tuple._2()) {
                    item._2.unpack(context);
                    step4LocalInput.set(item._1, item._2);
                }

                tuple._3().unpack(context);
                long addr = tuple._3().getCObject();

                DistributedStep4Local algorithm = new DistributedStep4Local(context, Double.class, TrainingMethod.fastCSR);
                algorithm.parameter.setNFactors(nFactors);

                algorithm.input.set(Step4LocalPartialModelsInputId.partialModels, step4LocalInput);
                algorithm.input.set(Step4LocalNumericTableInputId.partialData, dataTable);
                algorithm.input.set(Step4LocalNumericTableInputId.inputOfStep4FromStep2,
                                    tuple._3().get(DistributedPartialResultStep2Id.outputOfStep2ForStep4));

                DistributedPartialResultStep4 partialResultLocal = algorithm.compute();
                partialResultLocal.pack();

                context.dispose();
                return new Tuple2<Integer, DistributedPartialResultStep4>(tup._1, partialResultLocal);
            }
        }
               );
    }

    public static Long[] computePartition(JavaPairRDD<Integer, CSRNumericTable> dataRDD) {
        JavaPairRDD<Integer, Long> numbersOfRowsRDD = dataRDD.mapToPair(
        new PairFunction<Tuple2<Integer, CSRNumericTable>, Integer, Long>() {
            public Tuple2<Integer, Long> call(Tuple2<Integer, CSRNumericTable> tup) {
                return new Tuple2<Integer, Long>(tup._1, tup._2.getNumberOfRows()); // maybe unpack
            }
        });

        List<Tuple2<Integer, Long>> numbersOfRows = numbersOfRowsRDD.collect();
        Comparator<Tuple2<Integer, Long>> comparator = new Comparator<Tuple2<Integer, Long>>() {
            public int compare(Tuple2<Integer, Long> tupleA,
                               Tuple2<Integer, Long> tupleB) {
                return tupleA._1.compareTo(tupleB._1);
            }
        };

        Collections.sort(numbersOfRows, comparator);

        Long[] partition = new Long[numbersOfRows.size() + 1];
        partition[0] = new Long(0);
        for(int i = 0; i < partition.length - 1; i++) {
            partition[i + 1] = partition[i] + numbersOfRows.get(i)._2;
        }
        return partition;
    }

    public static JavaPairRDD<Integer, KeyValueDataCollection> computeOutBlocks(JavaPairRDD<Integer, CSRNumericTable> dataRDD,
            final Long[] dataBlockPartition, final long nBlocksLong) {
        return dataRDD.mapToPair(
        new PairFunction<Tuple2<Integer, CSRNumericTable>, Integer, KeyValueDataCollection>() {
            public Tuple2<Integer, KeyValueDataCollection> call(Tuple2<Integer, CSRNumericTable> tup) {
                DaalContext context = new DaalContext();
                tup._2.unpack(context);
                CSRNumericTable data = tup._2;
                int nRows = (int)data.getNumberOfRows();
                int nBlocks = (int)nBlocksLong;
                boolean[] blockIdFlags = new boolean[nBlocks * nRows];
                for (int i = 0; i < nRows * nBlocks; i++) {
                    blockIdFlags[i] = false;
                }

                long[] rowOffsets = data.getRowOffsetsArray();
                long[] colIndices = data.getColIndicesArray();

                for (int i = 0; i < nRows; i++) {
                    for (long j = rowOffsets[i] - 1; j < rowOffsets[i + 1] - 1; j++) {
                        for (int k = 1; k < nBlocks + 1; k++) {
                            if (dataBlockPartition[k - 1] <= colIndices[(int)j] - 1 && colIndices[(int)j] - 1 < dataBlockPartition[k]) {
                                blockIdFlags[(k - 1) * nRows + i] = true;
                            }
                        }
                    }
                }

                long[] nNotNull = new long[nBlocks];
                for (int i = 0; i < nBlocks; i++) {
                    nNotNull[i] = 0;
                    for (int j = 0; j < nRows; j++) {
                        if (blockIdFlags[i * nRows + j]) {
                            nNotNull[i] += 1;
                        }
                    }
                }
                KeyValueDataCollection result = new KeyValueDataCollection(context);

                for (int i = 0; i < nBlocks; i++) {
                    HomogenNumericTable indicesTable = new HomogenNumericTable(context, Integer.class, 1, nNotNull[i],
                            NumericTable.AllocationFlag.DoAllocate);
                    IntBuffer indicesBuffer = IntBuffer.allocate((int)nNotNull[i]);
                    indicesBuffer = indicesTable.getBlockOfRows(0, nNotNull[i], indicesBuffer);
                    int indexId = 0;
                    for (int j = 0; j < nRows; j++) {
                        if (blockIdFlags[i * nRows + j]) {
                            indicesBuffer.put(indexId, j);
                            indexId++;
                        }
                    }
                    indicesTable.releaseBlockOfRows(0, nNotNull[i], indicesBuffer);
                    result.set(i, indicesTable);
                }
                result.pack();
                context.dispose();

                return new Tuple2<Integer, KeyValueDataCollection>(tup._1, result);
            }
        }
               );
    }


    private static byte[] serializeObject(SerializableBase serializableObject) throws IOException {
        /* Create an output stream to serialize the object */
        ByteArrayOutputStream outputByteStream = new ByteArrayOutputStream();
        ObjectOutputStream outputStream = new ObjectOutputStream(outputByteStream);

        /* Serialize the numeric table into the output stream */
        outputStream.writeObject(serializableObject);

        /* Store the serialized data in an array */
        byte[] buffer = outputByteStream.toByteArray();
        return buffer;
    }

    private static SerializableBase deserializeObject(byte[] buffer) throws IOException, ClassNotFoundException {
        /* Create an input stream to deserialize the object from the array */
        ByteArrayInputStream inputByteStream = new ByteArrayInputStream(buffer);
        ObjectInputStream inputStream = new ObjectInputStream(inputByteStream);

        /* Create a numeric table object */
        SerializableBase restoredDataTable = (SerializableBase)inputStream.readObject();

        return restoredDataTable;
    }

}
