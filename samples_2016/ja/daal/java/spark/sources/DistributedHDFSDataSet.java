/* file: DistributedHDFSDataSet.java */
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

import com.intel.daal.services.Disposable;
import com.intel.daal.data_management.data.*;
import com.intel.daal.data_management.data_source.*;
import com.intel.daal.services.*;

import java.io.*;
import java.util.*;

import org.apache.spark.api.java.*;
import org.apache.spark.api.java.function.*;
import org.apache.spark.SparkConf;

import scala.Tuple2;

import com.intel.daal.data_management.data.CSRNumericTable;
import com.intel.daal.data_management.data.NumericTable;

/**
 * @brief Model is the base class for classes that represent models, such as
 * a linear regression or support vector machine (SVM) classifier.
 */
public class DistributedHDFSDataSet extends DistributedDataSet implements Serializable {

    /**
     * @brief Default constructor
     */
    public DistributedHDFSDataSet( String filename, StringDataSource dds ) {
        _filename = filename;
    }

    public DistributedHDFSDataSet( String filename, String labelsfilename, StringDataSource dds ) {
        _filename = filename;
        _labelsfilename = labelsfilename;
    }
    protected String _filename;
    protected String _labelsfilename;

    public JavaPairRDD<Integer, HomogenNumericTable> getAsPairRDDPartitioned( JavaSparkContext sc, int minPartitions, final long maxRowsPerTable ) {
        JavaRDD<String> rawData = sc.textFile(_filename, minPartitions);
        JavaPairRDD<String, Long> dataWithId = rawData.zipWithIndex();

        JavaPairRDD<Integer, HomogenNumericTable> data = dataWithId.mapPartitionsToPair(
        new PairFlatMapFunction<Iterator<Tuple2<String, Long>>, Integer, HomogenNumericTable>() {
            public List<Tuple2<Integer, HomogenNumericTable>> call(Iterator<Tuple2<String, Long>> it) {

                DaalContext context = new DaalContext();
                long maxRows = maxRowsPerTable;
                long curRow  = 0;
                ArrayList<Tuple2<Integer, HomogenNumericTable>> tables = new ArrayList<Tuple2<Integer, HomogenNumericTable>>();

                StringDataSource dataSource = new StringDataSource( context, "" );

                while (it.hasNext()) {

                    dataSource.setData( it.next()._1 );
                    dataSource.loadDataBlock( 1, curRow, maxRows );

                    curRow++;

                    if( curRow == maxRows || !(it.hasNext()) ) {
                        HomogenNumericTable table = (HomogenNumericTable)dataSource.getNumericTable();
                        table.setNumberOfRows(curRow);
                        table.pack();

                        Tuple2<Integer, HomogenNumericTable> tuple = new Tuple2<Integer, HomogenNumericTable>(0, table);
                        tables.add(tuple);

                        dataSource = new StringDataSource( context, "" );

                        curRow = 0;
                    }
                }

                context.dispose();

                return tables;
            }
        } );

        return data;
    }

    public JavaPairRDD<Integer, HomogenNumericTable> getAsPairRDD( JavaSparkContext sc ) {

        JavaPairRDD<Tuple2<String, String>, Long> dataWithId = sc.wholeTextFiles(_filename).zipWithIndex();

        JavaPairRDD<Integer, HomogenNumericTable> data = dataWithId.mapToPair(
        new PairFunction<Tuple2<Tuple2<String, String>, Long>, Integer, HomogenNumericTable>() {
            public Tuple2<Integer, HomogenNumericTable> call(Tuple2<Tuple2<String, String>, Long> tup) {

                DaalContext context = new DaalContext();

                String data = tup._1._2;

                long nVectors = 0;
                for(int i = 0; i < data.length(); i++) {
                    if(data.charAt(i) == '\n') {
                        nVectors++;
                    }
                }

                StringDataSource sdds = new StringDataSource( context, "" );
                sdds.setData( data );

                sdds.createDictionaryFromContext();
                sdds.allocateNumericTable();
                sdds.loadDataBlock( nVectors );

                HomogenNumericTable dataTable = (HomogenNumericTable)sdds.getNumericTable();

                dataTable.pack();
                context.dispose();

                return new Tuple2<Integer, HomogenNumericTable>(tup._2.intValue(), dataTable);
            }
        } );

        return data;
    }

    public JavaPairRDD<Integer, HomogenNumericTable> getAsPairRDDWithIndex( JavaSparkContext sc ) {
        JavaPairRDD<Tuple2<String, String>, Long> dataWithId = sc.wholeTextFiles(_filename).zipWithIndex();

        JavaPairRDD<Integer, HomogenNumericTable> data = dataWithId.mapToPair(
        new PairFunction<Tuple2<Tuple2<String, String>, Long>, Integer, HomogenNumericTable>() {
            public Tuple2<Integer, HomogenNumericTable> call(Tuple2<Tuple2<String, String>, Long> tup) {

                DaalContext context = new DaalContext();

                String data = tup._1._2;

                long nVectors = 0;
                for(int i = 0; i < data.length(); i++) {
                    if(data.charAt(i) == '\n') {
                        nVectors++;
                    }
                }

                StringDataSource sdds = new StringDataSource( context, "" );
                sdds.setData( data );

                sdds.createDictionaryFromContext();
                sdds.allocateNumericTable();
                sdds.loadDataBlock( nVectors );

                HomogenNumericTable dataTable = (HomogenNumericTable)sdds.getNumericTable();

                dataTable.pack();
                context.dispose();

                String fileName = tup._1._1;
                String[] tokens = fileName.split("[_\\.]");
                return new Tuple2<Integer, HomogenNumericTable>(Integer.parseInt(tokens[tokens.length - 2]) - 1, dataTable);
            }
        } );

        return data;
    }

    public JavaPairRDD<Integer, CSRNumericTable> getCSRAsPairRDD( JavaSparkContext sc ) throws IOException {

        JavaPairRDD<Tuple2<String, String>, Long> dataWithId = sc.wholeTextFiles(_filename).zipWithIndex();

        JavaPairRDD<Integer, CSRNumericTable> data = dataWithId.mapToPair(
        new PairFunction<Tuple2<Tuple2<String, String>, Long>, Integer, CSRNumericTable>() {
            public Tuple2<Integer, CSRNumericTable> call(Tuple2<Tuple2<String, String>, Long> tup) throws IOException {

                DaalContext context = new DaalContext();

                String data = tup._1._2;

                CSRNumericTable dataTable = createSparseTable(context, data);
                dataTable.pack();

                context.dispose();

                return new Tuple2<Integer, CSRNumericTable>(tup._2.intValue(), dataTable);
            }
        } );

        return data;
    }

    public JavaPairRDD<Integer, CSRNumericTable> getCSRAsPairRDDWithIndex( JavaSparkContext sc ) throws IOException {

        JavaPairRDD<Tuple2<String, String>, Long> dataWithId = sc.wholeTextFiles(_filename).zipWithIndex();

        JavaPairRDD<Integer, CSRNumericTable> data = dataWithId.mapToPair(
        new PairFunction<Tuple2<Tuple2<String, String>, Long>, Integer, CSRNumericTable>() {
            public Tuple2<Integer, CSRNumericTable> call(Tuple2<Tuple2<String, String>, Long> tup) throws IOException {

                DaalContext context = new DaalContext();

                String data = tup._1._2;

                CSRNumericTable dataTable = createSparseTable(context, data);
                dataTable.pack();

                context.dispose();

                String fileName = tup._1._1;
                String[] tokens = fileName.split("[_\\.]");
                return new Tuple2<Integer, CSRNumericTable>(Integer.parseInt(tokens[tokens.length - 2]) - 1, dataTable);
            }
        } );

        return data;
    }

    public static JavaPairRDD<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> getMergedDataAndLabelsRDD(String trainDatafilesPath,
                                                                                                                   String trainDataLabelsfilesPath,
                                                                                                                   JavaSparkContext sc,
                                                                                                                   StringDataSource tempDataSource) {
        DistributedHDFSDataSet ddTrain  = new DistributedHDFSDataSet(trainDatafilesPath, tempDataSource );
        DistributedHDFSDataSet ddLabels = new DistributedHDFSDataSet(trainDataLabelsfilesPath, tempDataSource );

        JavaPairRDD<Integer, HomogenNumericTable> dataRDD   = ddTrain.getAsPairRDDWithIndex(sc);
        JavaPairRDD<Integer, HomogenNumericTable> labelsRDD = ddLabels.getAsPairRDDWithIndex(sc);

        JavaPairRDD<Integer, Tuple2<Iterable<HomogenNumericTable>, Iterable<HomogenNumericTable>>> dataAndLablesRDD = dataRDD.cogroup(labelsRDD);

        JavaPairRDD<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>> mergedDataAndLabelsRDD = dataAndLablesRDD.mapToPair(
        new PairFunction<Tuple2<Integer, Tuple2<Iterable<HomogenNumericTable>, Iterable<HomogenNumericTable>>>, Integer, Tuple2<HomogenNumericTable,
                                                                                                                           HomogenNumericTable>>() {

            public Tuple2<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>>
            call(Tuple2<Integer, Tuple2<Iterable<HomogenNumericTable>, Iterable<HomogenNumericTable>>> tup) {

                HomogenNumericTable dataNT = tup._2._1.iterator().next();
                HomogenNumericTable labelsNT = tup._2._2.iterator().next();

                return new Tuple2<Integer, Tuple2<HomogenNumericTable, HomogenNumericTable>>(tup._1, new Tuple2<HomogenNumericTable,
                                                                                             HomogenNumericTable>(dataNT, labelsNT));
            }
        }).cache();

        mergedDataAndLabelsRDD.count();
        return mergedDataAndLabelsRDD;
    }

    public static CSRNumericTable createSparseTable(DaalContext context, String inputData) throws IOException {

        String[] elements = inputData.split("\n");

        String rowIndexLine = elements[0];
        String columnsLine = elements[1];
        String valuesLine = elements[2];

        int nVectors = getRowLength(rowIndexLine);
        long[] rowOffsets = new long[nVectors];

        readRow(rowIndexLine, 0, nVectors, rowOffsets);
        nVectors = nVectors - 1;

        int nCols = getRowLength(columnsLine);

        long[] colIndices = new long[nCols];
        readRow(columnsLine, 0, nCols, colIndices);

        int nNonZeros = getRowLength(valuesLine);

        double[] data = new double[nNonZeros];
        readRow(valuesLine, 0, nNonZeros, data);

        long maxCol = 0;
        for(int i = 0; i < nCols; i++) {
            if(colIndices[i] > maxCol) {
                maxCol = colIndices[i];
            }
        }
        int nFeatures = (int)maxCol;

        if (nCols != nNonZeros || nNonZeros != (rowOffsets[nVectors] - 1) || nFeatures == 0 || nVectors == 0) {
            throw new IOException("Unable to read input data");
        }

        return new CSRNumericTable(context, data, colIndices, rowOffsets, nFeatures, nVectors);
    }

    public static void readRow(String line, int offset, int nCols, double[] data) throws IOException {
        if (line == null) {
            throw new IOException("Unable to read input dataset");
        }

        String[] elements = line.split(",");
        for (int j = 0; j < nCols; j++) {
            data[offset + j] = Double.parseDouble(elements[j]);
        }
    }

    public static void readRow(String line, int offset, int nCols, long[] data) throws IOException {
        if (line == null) {
            throw new IOException("Unable to read input dataset");
        }

        String[] elements = line.split(",");
        for (int j = 0; j < nCols; j++) {
            data[offset + j] = Long.parseLong(elements[j]);
        }
    }

    public static void readSparseData(String dataset, int nVectors, int nNonZeroValues,
                                      long[] rowOffsets, long[] colIndices, double[] data) {
        try {
            BufferedReader bufferedReader = new BufferedReader(new FileReader(dataset));
            readRow(bufferedReader.readLine(), 0, nVectors + 1, rowOffsets);
            readRow(bufferedReader.readLine(), 0, nNonZeroValues, colIndices);
            readRow(bufferedReader.readLine(), 0, nNonZeroValues, data);
            bufferedReader.close();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        catch (NumberFormatException e) {
            e.printStackTrace();
        }
    }

    private static int getRowLength(String line) {
        String[] elements = line.split(",");
        return elements.length;
    }
}
