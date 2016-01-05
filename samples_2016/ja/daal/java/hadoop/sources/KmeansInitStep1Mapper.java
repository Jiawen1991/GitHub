/* file: KmeansInitStep1Mapper.java */
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

import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.BufferedWriter;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.InputStreamReader;
import java.util.Arrays;

import org.apache.hadoop.fs.*;
import org.apache.hadoop.io.*;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Mapper.Context;
import org.apache.hadoop.conf.Configuration;

import com.intel.daal.data_management.data.HomogenNumericTable;
import com.intel.daal.algorithms.kmeans.init.*;
import com.intel.daal.algorithms.kmeans.*;
import com.intel.daal.data_management.data.*;
import com.intel.daal.services.*;

public class KmeansInitStep1Mapper extends Mapper<Object, Text, IntWritable, WriteableData> {

    private static final int nBlocks = 4;
    private static final int nFeatures = 20;
    private static final int nVectorsInBlock = 10000;
    private static final long nClusters = 20;

    /* Index is supposed to be a sequence number for the split */
    private int index = 0;
    private int i = 0;
    private int totalTasks = 0;

    @Override
    public void setup(Context context) {
        index = context.getTaskAttemptID().getTaskID().getId();
        Configuration conf = context.getConfiguration();
        totalTasks = conf.getInt("mapred.map.tasks", 0);
    }

    @Override
    public void map(Object key, Text value,
                    Context context) throws IOException, InterruptedException {

        DaalContext daalContext = new DaalContext();

        /* Read a data set */
        String filePath = "/Hadoop/Kmeans/data/" + value;
        double[] data = new double[nFeatures * nVectorsInBlock];
        readData(filePath, nFeatures, nVectorsInBlock, data);

        HomogenNumericTable ntData = new HomogenNumericTable(daalContext, data, nFeatures, nVectorsInBlock);

        /* Create an algorithm to initialize the K-Means algorithm on local nodes */
        InitDistributedStep1Local kmeansLocalInit = new InitDistributedStep1Local(daalContext, Double.class, InitMethod.randomDense,
                                                                                  nClusters, nVectorsInBlock * nBlocks, nVectorsInBlock * index);
        kmeansLocalInit.input.set( InitInputId.data, ntData );

        /* Initialize the K-Means algorithm on local nodes */
        InitPartialResult pres = kmeansLocalInit.compute();

        /* Write the data prepended with a data set sequence number. Needed to know the position of the data set in the input data */
        context.write(new IntWritable(0), new WriteableData(index, pres));

        daalContext.dispose();
        index += totalTasks;
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
