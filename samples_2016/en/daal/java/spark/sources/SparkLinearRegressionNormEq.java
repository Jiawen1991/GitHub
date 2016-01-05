/* file: SparkLinearRegressionNormEq.java */
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
//      Java sample of multiple linear regression in the distributed processing
//      mode.
//
//      The program trains the multiple linear regression model on a training
//      data set with the normal equations method and computes regression for
//      the test data.
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

import com.intel.daal.algorithms.linear_regression.*;
import com.intel.daal.algorithms.linear_regression.training.*;
import com.intel.daal.algorithms.linear_regression.prediction.*;

import scala.Tuple2;
import com.intel.daal.data_management.data.*;
import com.intel.daal.services.*;

public class SparkLinearRegressionNormEq {
    /* Class containing the algorithm results */
    static class LinearRegressionResult {
        public HomogenNumericTable prediction;
        public HomogenNumericTable beta;
    }

    static Model model;
    static JavaPairRDD<Integer, PartialResult> partsRDD;
    static LinearRegressionResult result = new LinearRegressionResult();
    private static final long nClasses = 20;

    public static LinearRegressionResult runLinearRegression( DaalContext context,
                                                              JavaPairRDD<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> trainDataRDD,
                                                              JavaPairRDD<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> testDataRDD) {
        trainLocal(trainDataRDD);
        model = trainMaster(context);

        testModel(context, testDataRDD, model);
        return result;
    }

    private static void trainLocal(JavaPairRDD<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> trainDataRDD) {
        partsRDD = trainDataRDD.mapToPair(
        new PairFunction<Tuple2<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>>, Integer, PartialResult>() {
            public Tuple2<Integer, PartialResult> call(Tuple2<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> tup) {
                DaalContext context = new DaalContext();

                /* Create an algorithm object to train the multiple linear regression model with the normal equations method */
                TrainingDistributedStep1Local linearRegressionTraining = new TrainingDistributedStep1Local(context, Double.class,
                                                                                                           TrainingMethod.normEqDense);
                /* Set the input data on local nodes */
                tup._2._1.unpack(context);
                tup._2._2.unpack(context);
                linearRegressionTraining.input.set(TrainingInputId.data,  tup._2._1);
                linearRegressionTraining.input.set(TrainingInputId.dependentVariable, tup._2._2);

                /* Build a partial multiple linear regression model */
                PartialResult pres = linearRegressionTraining.compute();
                pres.pack();

                context.dispose();

                return new Tuple2<Integer, PartialResult>(tup._1, pres);
            }
        });
    }

    private static Model trainMaster(DaalContext context) {

        /* Create an algorithm object to train the multiple linear regression model with the normal equations method */
        TrainingDistributedStep2Master linearRegressionTraining = new TrainingDistributedStep2Master(context, Double.class,
                                                                                                     TrainingMethod.normEqDense);

        List<Tuple2<Integer, PartialResult>> parts_List = partsRDD.collect();

        /* Add partial results computed on local nodes to the algorithm on the master node */
        for (Tuple2<Integer, PartialResult> value : parts_List) {
            value._2.unpack(context);
            linearRegressionTraining.input.add(MasterInputId.partialModels, value._2 );
        }

        /* Build and retrieve the final multiple linear regression model */
        linearRegressionTraining.compute();

        TrainingResult trainingResult = linearRegressionTraining.finalizeCompute();

        Model model = trainingResult.get(TrainingResultId.model);
        result.beta = (HomogenNumericTable)model.getBeta();

        return model;
    }

    private static void testModel(DaalContext context, JavaPairRDD<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> testData,
                                  final Model model) {

        /* Create algorithm objects to predict values of multiple linear regression with the default method */
        PredictionBatch linearRegressionPredict = new PredictionBatch(context, Double.class, PredictionMethod.defaultDense);

        /* Pass the test data to the algorithm */
        List<Tuple2<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>>> parts_List = testData.collect();
        for (Tuple2<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> value : parts_List) {
            value._2._1.unpack(context);
            linearRegressionPredict.input.set(PredictionInputId.data, value._2._1);
        }

        linearRegressionPredict.input.set(PredictionInputId.model, model);

        /* Compute and retrieve the prediction results */
        PredictionResult predictionResult = linearRegressionPredict.compute();

        result.prediction = (HomogenNumericTable)predictionResult.get(PredictionResultId.prediction);
    }
}
