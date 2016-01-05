/* file: NaiveBayesStep2TrainingReducerAndPrediction.java */
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
 */

package DAAL;

import java.io.*;

import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Reducer;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.*;
import org.apache.hadoop.fs.FileSystem;

import com.intel.daal.algorithms.classifier.training.InputId;
import com.intel.daal.algorithms.classifier.training.TrainingDistributedInputId;
import com.intel.daal.algorithms.classifier.training.TrainingResultId;
import com.intel.daal.algorithms.classifier.training.PartialResultId;

import com.intel.daal.data_management.data.HomogenNumericTable;
import com.intel.daal.algorithms.multinomial_naive_bayes.*;
import com.intel.daal.algorithms.multinomial_naive_bayes.training.*;
import com.intel.daal.data_management.data.*;

import com.intel.daal.algorithms.classifier.prediction.PredictionResult;
import com.intel.daal.algorithms.classifier.prediction.PredictionResultId;
import com.intel.daal.algorithms.classifier.prediction.NumericTableInputId;
import com.intel.daal.algorithms.classifier.prediction.ModelInputId;
import com.intel.daal.algorithms.multinomial_naive_bayes.prediction.*;
import com.intel.daal.services.*;

public class NaiveBayesStep2TrainingReducerAndPrediction extends
    Reducer<IntWritable, WriteableData, IntWritable, WriteableData> {

    private static final long nClasses = 20;

    private static final int nDataFeatures   = 20;
    private static final int nLabelsFeatures = 1;
    private static final int nVectors        = 2000;

    @Override
    public void reduce(IntWritable key, Iterable<WriteableData> values, Context context)
    throws IOException, InterruptedException {

        DaalContext daalContext = new DaalContext();

        /* Create an algorithm to train the Na__ve Bayes algorithm on the master node */
        TrainingDistributedStep2Master naiveBayesMaster = new TrainingDistributedStep2Master(daalContext, Double.class,
                                                                                             TrainingMethod.defaultDense,
                                                                                             nClasses);
        for (WriteableData value : values) {
            TrainingPartialResult pr = (TrainingPartialResult)value.getObject(daalContext);
            naiveBayesMaster.input.add(TrainingDistributedInputId.partialModels, pr);
        }
        /* Train the Na__ve Bayes algorithm on the master node */
        naiveBayesMaster.compute();

        /* Finalize computations and retrieve the results */
        TrainingResult res = naiveBayesMaster.finalizeCompute();

        Model model = res.get(TrainingResultId.model);

        /* Test the Na__ve Bayes model */
        prediction(daalContext, model, context);

        daalContext.dispose();
    }

    public void prediction(DaalContext daalContext, Model model, Context context) throws IOException, InterruptedException {
        /* Read a data set */
        String dataFilePath = "/Hadoop/NaiveBayes/data/naivebayes_test.csv";
        String labelsFilePath = "/Hadoop/NaiveBayes/data/naivebayes_test_labels.csv";

        double[] data = new double[nDataFeatures * nVectors];
        double[] labels = new double[nVectors];

        readData(dataFilePath, nDataFeatures, nVectors, data);
        readData(labelsFilePath, nLabelsFeatures, nVectors, labels);

        HomogenNumericTable ntData = new HomogenNumericTable(daalContext, data, nDataFeatures, nVectors);
        HomogenNumericTable expectedLabels = new HomogenNumericTable(daalContext, labels, nLabelsFeatures, nVectors);

        /* Create an algorithm to train the Na__ve Bayes algorithm on local nodes */
        PredictionBatch algorithm = new PredictionBatch(daalContext, Double.class, PredictionMethod.defaultDense, nClasses);

        algorithm.input.set(NumericTableInputId.data, ntData);
        algorithm.input.set(ModelInputId.model, model);

        /* Test the Na__ve Bayes algorithm on local nodes */
        PredictionResult res = algorithm.compute();

        /* Retrieve the results */
        HomogenNumericTable predictedlabels = (HomogenNumericTable)res.get(PredictionResultId.prediction);

        context.write(new IntWritable(0), new WriteableData(predictedlabels));
        context.write(new IntWritable(1), new WriteableData(expectedLabels));
    }

    private static void readData(String dataset, int nFeatures, int nVectors, double[] data) {
        System.out.println("readData " + dataset);
        try {
            Path pt = new Path(dataset);
            FileSystem fs = FileSystem.get(new Configuration());
            BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(fs.open(pt)));

            int nLine = 0;
            for (String line; ((line = bufferedReader.readLine()) != null) && (nLine < nVectors); nLine++) {
                String[] elements = line.split(",");
                for (int j = 0; j < nFeatures; j++) {
                    data[nLine * nFeatures + j] = Double.parseDouble(elements[j]);
                }
            }
            bufferedReader.close();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        catch (NumberFormatException e) {
            e.printStackTrace();
        }
    }
}
