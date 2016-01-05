/* file: SampleNaiveBayes.java */
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
 //     Java sample of Na__ve Bayes classification.
 //
 //     The program trains the Na__ve Bayes model on a supplied training data set
 //     and then performs classification of previously unseen data.
 ////////////////////////////////////////////////////////////////////////////////
 */

package DAAL;

import java.util.ArrayList;
import java.util.List;
import java.util.Arrays;
import java.util.Iterator;
import java.util.Map;
import java.nio.IntBuffer;

import org.apache.spark.api.java.*;
import org.apache.spark.api.java.function.*;
import org.apache.spark.SparkConf;

import scala.Tuple2;

import com.intel.daal.data_management.data.*;
import com.intel.daal.data_management.data_source.*;
import com.intel.daal.services.*;

public class SampleNaiveBayes {
    public static void main(String[] args) {
        DaalContext context = new DaalContext();

        /* Create JavaSparkContext that loads defaults from the system properties and the classpath and sets the name */
        JavaSparkContext sc = new JavaSparkContext(new SparkConf().setAppName("Spark Naive Bayes"));
        StringDataSource templateDataSource = new StringDataSource( context, "" );

        String trainDataFilesPath       = "/Spark/NaiveBayes/data/NaiveBayes_train_?.csv";
        String trainDataLabelsFilesPath = "/Spark/NaiveBayes/data/NaiveBayes_train_labels_?.csv";
        String testDataFilesPath        = "/Spark/NaiveBayes/data/NaiveBayes_test_1.csv";
        String testDataLabelsFilesPath  = "/Spark/NaiveBayes/data/NaiveBayes_test_labels_1.csv";

        /* Read the training data and labels from a specified path */
        JavaPairRDD<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> trainDataAndLabelsRDD =
            DistributedHDFSDataSet.getMergedDataAndLabelsRDD(trainDataFilesPath, trainDataLabelsFilesPath, sc, templateDataSource);

        /* Read the test data and labels from a specified path */
        JavaPairRDD<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> testDataAndLabelsRDD =
            DistributedHDFSDataSet.getMergedDataAndLabelsRDD(testDataFilesPath, testDataLabelsFilesPath, sc, templateDataSource);

        /* Compute the results of the Na__ve Bayes algorithm for dataRDD */
        SparkNaiveBayes.NaiveBayesResult result = SparkNaiveBayes.runNaiveBayes(context, trainDataAndLabelsRDD, testDataAndLabelsRDD);

        /* Print the results */
        HomogenNumericTable expected = null;
        List<Tuple2<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>>> parts_List = testDataAndLabelsRDD.collect();
        for (Tuple2<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> value : parts_List) {
            expected = value._2._2;
            expected.unpack( context );
        }
        HomogenNumericTable predicted = result.prediction;

        printClassificationResult(expected, predicted, "Ground truth", "Classification results",
                                  "NaiveBayes classification results (first 20 observations):", 20);
        context.dispose();
    }

    public static void printClassificationResult(NumericTable groundTruth, NumericTable classificationResults,
                                                 String header1, String header2, String message, int nMaxRows) {
        int nCols = (int) groundTruth.getNumberOfColumns();
        int nRows = Math.min((int) groundTruth.getNumberOfRows(), nMaxRows);

        IntBuffer dataGroundTruth = IntBuffer.allocate(nCols * nRows);
        dataGroundTruth = groundTruth.getBlockOfRows(0, nRows, dataGroundTruth);

        IntBuffer dataClassificationResults = IntBuffer.allocate(nCols * nRows);
        dataClassificationResults = classificationResults.getBlockOfRows(0, nRows, dataClassificationResults);

        System.out.println(message);
        System.out.println(header1 + "\t" + header2);
        for(int i = 0; i < nRows; i++) {
            for(int j = 0; j < 1; j++) {
                System.out.format("%+d\t\t%+d\n", dataGroundTruth.get(i * nCols + j), dataClassificationResults.get(i * nCols + j));
            }
        }
    }
}
