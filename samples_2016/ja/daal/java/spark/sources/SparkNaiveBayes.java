/* file: SparkNaiveBayes.java */
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
//      Java sample of Na__ve Bayes classification in the distributed processing
//      mode.
//
//      The program trains the Na__ve Bayes model on a supplied training data set
//      and then performs classification of previously unseen data.
////////////////////////////////////////////////////////////////////////////////
*/

package DAAL;

import java.util.ArrayList;
import java.util.List;
import java.util.Arrays;
import java.util.Map;
import java.util.HashMap;

import org.apache.spark.api.java.*;
import org.apache.spark.api.java.function.*;
import org.apache.spark.SparkConf;

import com.intel.daal.algorithms.classifier.prediction.PredictionResult;
import com.intel.daal.algorithms.classifier.prediction.PredictionResultId;
import com.intel.daal.algorithms.classifier.prediction.NumericTableInputId;
import com.intel.daal.algorithms.classifier.prediction.ModelInputId;
import com.intel.daal.algorithms.classifier.training.InputId;
import com.intel.daal.algorithms.classifier.training.TrainingDistributedInputId;
import com.intel.daal.algorithms.classifier.training.TrainingResultId;
import com.intel.daal.algorithms.classifier.training.PartialResultId;

import com.intel.daal.algorithms.multinomial_naive_bayes.*;
import com.intel.daal.algorithms.multinomial_naive_bayes.training.*;
import com.intel.daal.algorithms.multinomial_naive_bayes.prediction.*;

import scala.Tuple2;
import com.intel.daal.data_management.data.*;
import com.intel.daal.services.*;

public class SparkNaiveBayes {
    /* Class containing the algorithm results */
    static class NaiveBayesResult {
        public HomogenNumericTable prediction;
    }

    static Model model;
    static JavaPairRDD<Integer, TrainingPartialResult> partsRDD;
    static NaiveBayesResult result = new NaiveBayesResult();
    private static final long nClasses = 20;
    private static final int  nTestObservations    = 2000;

    public static NaiveBayesResult runNaiveBayes( DaalContext context,
                                                  JavaPairRDD<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> trainDataRDD,
                                                  JavaPairRDD<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> testDataRDD) {
        trainLocal(trainDataRDD);
        model = trainMaster(context);

        testModel(context, testDataRDD, model);

        return result;
    }

    private static void trainLocal(JavaPairRDD<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> trainDataRDD) {
        partsRDD = trainDataRDD.mapToPair(
        new PairFunction<Tuple2<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>>, Integer, TrainingPartialResult>() {
            public Tuple2<Integer, TrainingPartialResult> call(Tuple2<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> tup) {
                DaalContext context = new DaalContext();

                /* Create an algorithm to train the Na__ve Bayes model on local nodes */
                TrainingDistributedStep1Local algorithm = new TrainingDistributedStep1Local(context, Double.class, TrainingMethod.defaultDense,
                                                                                            nClasses);
                /* Set the input data on local nodes */
                tup._2._1.unpack(context);
                tup._2._2.unpack(context);
                algorithm.input.set(InputId.data, tup._2._1);
                algorithm.input.set(InputId.labels, tup._2._2);

                /* Train the Na__ve Bayes model on local nodes */
                TrainingPartialResult pres = algorithm.compute();
                pres.pack();

                context.dispose();

                return new Tuple2<Integer, TrainingPartialResult>(tup._1, pres);
            }
        });
    }

    private static Model trainMaster(DaalContext context) {

        /* Create an algorithm to train the Na__ve Bayes model on the master node */
        TrainingDistributedStep2Master algorithm = new TrainingDistributedStep2Master(context, Double.class, TrainingMethod.defaultDense,
                                                                                      nClasses);

        List<Tuple2<Integer, TrainingPartialResult>> parts_List = partsRDD.collect();

        /* Add partial results computed on local nodes to the algorithm on the master node */
        for (Tuple2<Integer, TrainingPartialResult> value : parts_List) {
            value._2.unpack(context);
            algorithm.input.add(TrainingDistributedInputId.partialModels, value._2 );
        }

        /* Train the Na__ve Bayes model on the master node */
        algorithm.compute();

        /* Finalize computations and retrieve the training results */
        TrainingResult trainingResult = algorithm.finalizeCompute();

        Model model = trainingResult.get(TrainingResultId.model);

        return model;
    }

    private static void testModel(DaalContext context, JavaPairRDD<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> testData,
                                  final Model model) {

        /* Create algorithm objects to predict values of the Na__ve Bayes model with the defaultDense method */
        PredictionBatch algorithm = new PredictionBatch(context, Double.class, PredictionMethod.defaultDense, nClasses);

        /* Pass the test data to the algorithm */
        List<Tuple2<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>>> parts_List = testData.collect();
        for (Tuple2<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> value : parts_List) {
            value._2._1.unpack(context);
            algorithm.input.set(NumericTableInputId.data, value._2._1);
        }
        algorithm.input.set(ModelInputId.model, model);

        /* Compute the prediction results */
        PredictionResult predictionResult = algorithm.compute();

        /* Retrieve the results */
        result.prediction = (HomogenNumericTable)predictionResult.get(PredictionResultId.prediction);
    }
}
