/* file: SampleLinearRegressionQR.java */
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
 //     Java sample of multiple linear regression.
 //
 //     The program trains the multiple linear regression model on a training
 //     data set with a QR decomposition-based method and computes regression for
 //     the test data.
 ////////////////////////////////////////////////////////////////////////////////
 */

package DAAL;

import java.util.ArrayList;
import java.util.List;
import java.util.Arrays;
import java.util.Iterator;
import java.util.Map;
import java.nio.IntBuffer;
import java.nio.DoubleBuffer;

import org.apache.spark.api.java.*;
import org.apache.spark.api.java.function.*;
import org.apache.spark.SparkConf;

import scala.Tuple2;

import com.intel.daal.data_management.data.*;
import com.intel.daal.data_management.data_source.*;
import com.intel.daal.services.*;

public class SampleLinearRegressionQR {
    public static void main(String[] args) {
        DaalContext context = new DaalContext();

        /* Create JavaSparkContext that loads defaults from the system properties and the classpath and sets the name */
        StringDataSource templateDataSource = new StringDataSource( context, "" );
        JavaSparkContext sc                 = new JavaSparkContext(new SparkConf().setAppName("Spark Linear Regression"));

        String trainDataFilesPath       = "/Spark/LinearRegressionQR/data/LinearRegressionQR_train_?.csv";
        String trainDataLabelsFilesPath = "/Spark/LinearRegressionQR/data/LinearRegressionQR_train_labels_?.csv";
        String testDataFilesPath        = "/Spark/LinearRegressionQR/data/LinearRegressionQR_test_1.csv";
        String testDataLabelsFilesPath  = "/Spark/LinearRegressionQR/data/LinearRegressionQR_test_labels_1.csv";

        /* Read the training data and labels from a specified path */
        JavaPairRDD<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> trainDataAndLabelsRDD =
            DistributedHDFSDataSet.getMergedDataAndLabelsRDD(trainDataFilesPath, trainDataLabelsFilesPath, sc, templateDataSource);

        /* Read the test data and labels from a specified path */
        JavaPairRDD<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> testDataAndLabelsRDD =
            DistributedHDFSDataSet.getMergedDataAndLabelsRDD(testDataFilesPath, testDataLabelsFilesPath, sc, templateDataSource);

        /* Compute linear regression for dataRDD */
        SparkLinearRegressionQR.LinearRegressionResult result = SparkLinearRegressionQR.runLinearRegression(context, trainDataAndLabelsRDD,
                                                                                                            testDataAndLabelsRDD);
        /* Print the results */
        HomogenNumericTable expected = null;

        List<Tuple2<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>>> parts_List = testDataAndLabelsRDD.collect();
        for (Tuple2<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> value : parts_List) {
            expected = value._2._2;
            expected.unpack( context );
        }
        HomogenNumericTable predicted = result.prediction;
        HomogenNumericTable beta      = result.beta;

        printNumericTable("Coefficients:", beta);
        printNumericTable("First 10 rows of results (obtained): ", predicted, 10);
        printNumericTable("First 10 rows of results (expected): ", expected, 10);

        context.dispose();
    }

    public static void printNumericTable(String header, NumericTable nt, long nPrintedRows, long nPrintedCols) {
        long nNtCols = nt.getNumberOfColumns();
        long nNtRows = nt.getNumberOfRows();
        long nRows = nNtRows;
        long nCols = nNtCols;

        if(nPrintedRows > 0) {
            nRows = Math.min(nNtRows, nPrintedRows);
        }

        DoubleBuffer result = DoubleBuffer.allocate((int)(nNtCols * nRows));
        result = nt.getBlockOfRows(0, nRows, result);

        if(nPrintedCols > 0) {
            nCols = Math.min(nNtCols, nPrintedCols);
        }

        StringBuilder builder = new StringBuilder();
        builder.append(header);
        builder.append("\n");
        for (long i = 0; i < nRows; i++) {
            for (long j = 0; j < nCols; j++) {
                String tmp = String.format("%-6.3f   ", result.get((int)(i * nNtCols + j)));
                builder.append(tmp);
            }
            builder.append("\n");
        }
        System.out.println(builder.toString());
    }

    public static void printNumericTable(String header, NumericTable nt) {
        printNumericTable(header, nt, nt.getNumberOfRows());
    }

    public static void printNumericTable(String header, NumericTable nt, long nRows) {
        printNumericTable(header, nt, nRows, nt.getNumberOfColumns());
    }
}
