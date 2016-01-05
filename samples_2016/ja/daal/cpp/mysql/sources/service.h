/* file: service.h */
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
!    Auxiliary functions used in C++ samples
!******************************************************************************/

#ifndef _SERVICE_H
#define _SERVICE_H

#include "daal.h"

using namespace daal::data_management;

#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdarg>
#include <vector>
#include <queue>

#include "error_handling.h"

size_t readTextFile(const std::string &datasetFileName, daal::byte **data)
{
    std::ifstream file(datasetFileName .c_str(), std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        fileOpenError(datasetFileName .c_str());
    }

    std::streampos end = file.tellg();
    file.seekg(0, std::ios::beg);

    size_t fileSize = static_cast<size_t>(end);

    (*data) = new daal::byte[fileSize];
    checkAllocation(data);

    if (!file.read((char *)(*data), fileSize))
    {
        delete[] data;
        fileReadError();
    }

    return fileSize;
}

template <typename item_type>
void readLine(std::string &line, size_t nCols, item_type *data, size_t firstPos = 0)
{
    std::stringstream iss(line);

    for (size_t col = 0; col < nCols; ++col)
    {
        std::string val;
        std::getline(iss, val, ',');

        std::stringstream convertor(val);
        convertor >> data[firstPos + col];
    }
}

template <typename item_type>
void readRowUnknownLength(std::string &line, item_type **data, size_t &nCols)
{
    std::stringstream iss(line);

    nCols = 0;
    std::string val;

    while (std::getline(iss, val, ','))
    {
        nCols++;
    }

    *data = new item_type[nCols];
    readLine<item_type>(line, nCols, *data);
}

template <typename item_type>
daal::data_management::CSRNumericTable *createSparseTable(std::string datasetFileName)
{
    size_t *rowOffsets, *colIndices;
    item_type *data;
    size_t nFeatures, nVectors, nNonZeros;

    std::ifstream file(datasetFileName .c_str());

    if (!file.is_open())
    {
        fileOpenError(datasetFileName .c_str());
    }

    size_t nCols;
    std::string valuesLine, columnsLine, rowIndexLine;

    std::getline(file, rowIndexLine);
    readRowUnknownLength<size_t>(rowIndexLine, &rowOffsets, nVectors);
    nVectors = nVectors - 1;

    std::getline(file, columnsLine);
    readRowUnknownLength<size_t>(columnsLine, &colIndices, nCols);

    std::getline(file, valuesLine);
    readRowUnknownLength<item_type>(valuesLine, &data, nNonZeros);

    size_t maxCol = 0;
    for(size_t  i = 0; i < nCols; i++)
    {
        if(colIndices[i] > maxCol)
        {
            maxCol = colIndices[i];
        }
    }
    nFeatures = maxCol;

    if (nCols != nNonZeros ||
        nNonZeros != (rowOffsets[nVectors] - 1) ||
        nFeatures == 0 ||
        nVectors == 0)
    {
        delete [] data;
        delete [] colIndices;
        delete [] rowOffsets;
        sparceFileReadError();
    }

    return new daal::data_management::CSRNumericTable(data, colIndices, rowOffsets, nFeatures, nVectors);
}

void deleteSparseTable(daal::data_management::CSRNumericTable *nt)
{
    size_t *values, *colIndices, *rowOffsets;
    nt->getArrays((void **)&values, &colIndices, &rowOffsets);
    delete [] values;
    delete [] colIndices;
    delete [] rowOffsets;
    delete nt;
}

void printAprioriItemsets(daal::services::SharedPtr<daal::data_management::NumericTable> largeItemsetsTable,
                          daal::services::SharedPtr<daal::data_management::NumericTable> largeItemsetsSupportTable)
{
    size_t largeItemsetCount = largeItemsetsSupportTable->getNumberOfRows();
    size_t nItemsInLargeItemsets = largeItemsetsTable->getNumberOfRows();
    const size_t *largeItemsets = ((daal::data_management::HomogenNumericTable<size_t> *)
                                   largeItemsetsTable.get())->getArray();
    const size_t *largeItemsetsSupportData = ((daal::data_management::HomogenNumericTable<size_t> *)
                                              largeItemsetsSupportTable.get())->getArray();
    const size_t nItemsetToPrint = 20;
    std::vector<std::vector<size_t> > largeItemsetsVector;
    largeItemsetsVector.resize(largeItemsetCount);

    for (size_t i = 0; i < nItemsInLargeItemsets; i++)
    {
        largeItemsetsVector[largeItemsets[2 * i]].push_back(largeItemsets[2 * i + 1]);
    }

    std::vector<size_t> supportVector;
    supportVector.resize(largeItemsetCount);

    for (size_t i = 0; i < largeItemsetCount; i++)
    {
        supportVector[largeItemsetsSupportData[2 * i]] = largeItemsetsSupportData[2 * i + 1];
    }

    std::cout << std::endl << "Apriori example program results" << std::endl;

    std::cout << std::endl << "Last " << nItemsetToPrint << " large itemsets: " << std::endl;
    std::cout << std::endl << "Itemset" << "\t\t\tSupport" << std::endl;

    size_t iMin = ((largeItemsetCount > nItemsetToPrint) ? largeItemsetCount - nItemsetToPrint : 0);
    for (size_t i = iMin; i < largeItemsetCount; i++)
    {
        std::cout << "{";
        for(size_t l = 0; l < largeItemsetsVector[i].size() - 1; l++)
        {
            std::cout << largeItemsetsVector[i][l] << ", ";
        }
        std::cout << largeItemsetsVector[i][largeItemsetsVector[i].size() - 1] << "}\t\t";

        std::cout << supportVector[i] << std::endl;
    }
}

void printAprioriRules(daal::services::SharedPtr<daal::data_management::NumericTable> leftItemsTable,
                       daal::services::SharedPtr<daal::data_management::NumericTable> rightItemsTable,
                       daal::services::SharedPtr<daal::data_management::NumericTable> confidenceTable)
{
    size_t nRules = confidenceTable->getNumberOfRows();
    size_t nLeftItems  = leftItemsTable->getNumberOfRows();
    size_t nRightItems = rightItemsTable->getNumberOfRows();
    size_t *leftItems = ((daal::data_management::HomogenNumericTable<size_t> *)leftItemsTable.get())->getArray();
    size_t *rightItems = ((daal::data_management::HomogenNumericTable<size_t> *)rightItemsTable.get())->getArray();
    double *confidence = ((daal::data_management::HomogenNumericTable<double> *)confidenceTable.get())->getArray();

    const size_t nRulesToPrint = 20;
    std::vector<std::vector<size_t> > leftItemsVector;
    leftItemsVector.resize(nRules);

    if (nRules == 0)
    {
        std::cout << std::endl << "No association rules were found " << std::endl;
        return;
    }

    for (size_t i = 0; i < (size_t)nLeftItems; i++)
    {
        leftItemsVector[leftItems[2 * i]].push_back(leftItems[2 * i + 1]);
    }

    std::vector<std::vector<size_t> > rightItemsVector;
    rightItemsVector.resize(nRules);

    for (size_t i = 0; i < nRightItems; i++)
    {
        rightItemsVector[rightItems[2 * i]].push_back(rightItems[2 * i + 1]);
    }

    std::vector<double> confidenceVector;
    confidenceVector.resize(nRules);

    for (size_t i = 0; i < (size_t)nRules; i++)
    {
        confidenceVector[i] = confidence[i];
    }

    std::cout << std::endl << "Last " << nRulesToPrint << " association rules: " << std::endl;
    std::cout << std::endl << "Rule" << "\t\t\t\tConfidence" << std::endl;
    size_t iMin = ((nRules > nRulesToPrint) ? (nRules - nRulesToPrint) : 0);

    for (size_t i = iMin; i < nRules; i++)
    {
        std::cout << "{";
        for(size_t l = 0; l < leftItemsVector[i].size() - 1; l++)
        {
            std::cout << leftItemsVector[i][l] << ", ";
        }
        std::cout << leftItemsVector[i][leftItemsVector[i].size() - 1] << "} => {";

        for(size_t l = 0; l < rightItemsVector[i].size() - 1; l++)
        {
            std::cout << rightItemsVector[i][l] << ", ";
        }
        std::cout << rightItemsVector[i][rightItemsVector[i].size() - 1] << "}\t\t";

        std::cout << confidenceVector[i] << std::endl;
    }
}

bool isFull(daal::data_management::NumericTableIface::StorageLayout layout)
{
    int layoutInt = (int) layout;
    if (daal::data_management::packed_mask & layoutInt)
    {
        return false;
    }
    return true;
}

bool isUpper(daal::data_management::NumericTableIface::StorageLayout layout)
{
    if (layout == daal::data_management::NumericTableIface::upperPackedSymmetricMatrix  ||
        layout == daal::data_management::NumericTableIface::upperPackedTriangularMatrix)
    {
        return true;
    }
    return false;
}

bool isLower(daal::data_management::NumericTableIface::StorageLayout layout)
{
    if (layout == daal::data_management::NumericTableIface::lowerPackedSymmetricMatrix  ||
        layout == daal::data_management::NumericTableIface::lowerPackedTriangularMatrix)
    {
        return true;
    }
    return false;
}

template <typename T>
void printArray(T *array, const size_t nPrintedCols, const size_t nPrintedRows, const size_t nCols, std::string message)
{
    std::cout << std::setiosflags(std::ios::left);
    std::cout << message << std::endl;
    for (size_t i = 0; i < nPrintedRows; i++)
    {
        for(size_t j = 0; j < nPrintedCols; j++)
        {
            std::cout << std::setw(10) << std::setiosflags(std::ios::fixed) << std::setprecision(3);
            std::cout << array[i * nCols + j];
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

template <typename T>
void printArray(T *array, const size_t nCols, const size_t nRows, std::string message)
{
    printArray(array, nCols, nRows, nCols, message);
}

template <typename T>
void printLowerArray(T *array, const size_t nPrintedRows, std::string message)
{
    std::cout << std::setiosflags(std::ios::left);
    std::cout << message << std::endl;
    int ind = 0;
    for (size_t i = 0; i < nPrintedRows; i++)
    {
        for(size_t j = 0; j <= i ; j++)
        {
            std::cout << std::setw(10) << std::setiosflags(std::ios::fixed) << std::setprecision(3);
            std::cout << array[ind++];
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

template <typename T>
void printUpperArray(T *array, const size_t nPrintedCols, const size_t nPrintedRows, const size_t nCols,
                     std::string message)
{
    std::cout << std::setiosflags(std::ios::left);
    std::cout << message << std::endl;
    int ind = 0;
    for (size_t i = 0; i < nPrintedRows; i++)
    {
        for(size_t j = 0; j < i; j++)
        {
            std::cout << "          ";
        }
        for(size_t j = i; j < nPrintedCols; j++)
        {
            std::cout << std::setw(10) << std::setiosflags(std::ios::fixed) << std::setprecision(3);
            std::cout << array[ind++];
        }
        for(size_t j = nPrintedCols; j < nCols; j++)
        {
            ind++;
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void printNumericTable(daal::data_management::NumericTable *dataTable, char *message = "",
                       size_t nPrintedRows = 0, size_t nPrintedCols = 0)
{
    size_t nRows = dataTable->getNumberOfRows();
    size_t nCols = dataTable->getNumberOfColumns();
    daal::data_management::NumericTableIface::StorageLayout layout = dataTable->getDataLayout();

    if(nPrintedRows != 0)
    {
        nPrintedRows = std::min(nRows, nPrintedRows);
    }
    else
    {
        nPrintedRows = nRows;
    }

    if(nPrintedCols != 0)
    {
        nPrintedCols = std::min(nCols, nPrintedCols);
    }
    else
    {
        nPrintedCols = nCols;
    }

    BlockDescriptor<double> block;
    if(isFull(layout))
    {
        dataTable->getBlockOfRows(0, nRows, readOnly, block);
        printArray<double>(block.getBlockPtr(), nPrintedCols, nPrintedRows, nCols, message);
        dataTable->releaseBlockOfRows(block);
    }
    else
    {
        daal::data_management::PackedArrayNumericTableIface *packedTable =
            dynamic_cast<daal::data_management::PackedArrayNumericTableIface *>(dataTable);
        packedTable->getPackedArray(readOnly, block);
        if(isLower(layout))
        {
            printLowerArray<double>(block.getBlockPtr(), nPrintedRows, message);
        }
        else if(isUpper(layout))
        {
            printUpperArray<double>(block.getBlockPtr(), nPrintedCols, nPrintedRows, nCols, message);
        }
        packedTable->releasePackedArray(block);
    }
}


void printNumericTable(daal::data_management::NumericTable &dataTable, char *message = "",
                       size_t nPrintedRows = 0, size_t nPrintedCols = 0)
{
    printNumericTable(&dataTable, message, nPrintedRows, nPrintedCols);
}

void printNumericTable(const daal::services::SharedPtr<daal::data_management::NumericTable> &dataTable, char *message = "",
                       size_t nPrintedRows = 0, size_t nPrintedCols = 0)
{
    printNumericTable(dataTable.get(), message, nPrintedRows, nPrintedCols);
}

void printPackedNumericTable(daal::data_management::NumericTable *dataTable, size_t nFeatures, char *message = "")
{
    BlockDescriptor<double> block;

    dataTable->getBlockOfRows(0, 1, readOnly, block);

    double *dataDouble = block.getBlockPtr();

    std::cout << std::setiosflags(std::ios::left);
    std::cout << message << std::endl;
    size_t index = 0;
    for (size_t i = 0; i < nFeatures; i++)
    {
        for(size_t j = 0; j <= i; j++, index++)
        {
            std::cout << std::setw(10) << std::setiosflags(std::ios::fixed) << std::setprecision(3);
            std::cout << dataDouble[index];
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;

    dataTable->releaseBlockOfRows(block);
}

void printPackedNumericTable(daal::data_management::NumericTable &dataTable, size_t nFeatures, char *message = "")
{
    printPackedNumericTable(&dataTable, nFeatures, message);
}

template<typename type1, typename type2>
void printNumericTables(daal::data_management::NumericTable *dataTable1,
                        daal::data_management::NumericTable *dataTable2,
                        char *title1 = "", char *title2 = "", char *message = "", size_t nPrintedRows = 0)
{
    size_t nRows1 = dataTable1->getNumberOfRows();
    size_t nRows2 = dataTable2->getNumberOfRows();
    size_t nCols1 = dataTable1->getNumberOfColumns();
    size_t nCols2 = dataTable2->getNumberOfColumns();

    BlockDescriptor<type1> block1;
    BlockDescriptor<type2> block2;

    size_t nRows = std::min(nRows1, nRows2);
    if (nPrintedRows != 0)
    {
        nRows = std::min(std::min(nRows1, nRows2), nPrintedRows);
    }

    dataTable1->getBlockOfRows(0, nRows, readOnly, block1);
    dataTable2->getBlockOfRows(0, nRows, readOnly, block2);

    type1 *dataDouble1 = block1.getBlockPtr();
    type2 *dataDouble2 = block2.getBlockPtr();

    size_t columnWidth = 15;
    std::cout << std::setiosflags(std::ios::left);
    std::cout << message << std::endl;
    std::cout << std::setw(columnWidth * nCols1) << title1;
    std::cout << std::setw(columnWidth * nCols2) << title2 << std::endl;
    for (size_t i = 0; i < nRows; i++)
    {
        for (size_t j = 0; j < nCols1; j++)
        {
            std::cout << std::setw(columnWidth) << std::setiosflags(std::ios::fixed) << std::setprecision(3);
            std::cout << dataDouble1[i * nCols1 + j];
        }
        for (size_t j = 0; j < nCols2; j++)
        {
            std::cout << std::setprecision(0) << std::setw(columnWidth) << dataDouble2[i * nCols2 + j];
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;

    dataTable1->releaseBlockOfRows(block1);
    dataTable2->releaseBlockOfRows(block2);
}

template<typename type1, typename type2>
void printNumericTables(daal::data_management::NumericTable *dataTable1,
                        daal::data_management::NumericTable &dataTable2,
                        char *title1 = "", char *title2 = "", char *message = "", size_t nPrintedRows = 0)
{
    printNumericTables<type1, type2>(dataTable1, &dataTable2, title1, title2, message, nPrintedRows);
}

void printNumericTables(daal::data_management::NumericTable *dataTable1,
                        daal::data_management::NumericTable *dataTable2,
                        char *title1 = "", char *title2 = "", char *message = "", size_t nPrintedRows = 0)
{
    size_t nRows1 = dataTable1->getNumberOfRows();
    size_t nRows2 = dataTable2->getNumberOfRows();
    size_t nCols1 = dataTable1->getNumberOfColumns();
    size_t nCols2 = dataTable2->getNumberOfColumns();

    BlockDescriptor<double> block1;
    BlockDescriptor<double> block2;

    size_t nRows = std::min(nRows1, nRows2);
    if (nPrintedRows != 0)
    {
        nRows = std::min(std::min(nRows1, nRows2), nPrintedRows);
    }

    dataTable1->getBlockOfRows(0, nRows, readOnly, block1);
    dataTable2->getBlockOfRows(0, nRows, readOnly, block2);

    double *dataDouble1 = block1.getBlockPtr();
    double *dataDouble2 = block2.getBlockPtr();

    std::cout << std::setiosflags(std::ios::left);
    std::cout << message << std::endl;
    std::cout << std::setw(10 * nCols1) << title1;
    std::cout << std::setw(10 * nCols2) << title2 << std::endl;
    for (size_t i = 0; i < nRows; i++)
    {
        for (size_t j = 0; j < nCols1; j++)
        {
            std::cout << std::setw(10) << std::setiosflags(std::ios::fixed) << std::setprecision(3);
            std::cout << dataDouble1[i * nCols1 + j];
        }
        for (size_t j = 0; j < nCols2; j++)
        {
            std::cout << std::setprecision(0) << std::setw(10) << dataDouble2[i * nCols2 + j];
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;

    dataTable1->releaseBlockOfRows(block1);
    dataTable2->releaseBlockOfRows(block2);
}

void printNumericTables(daal::data_management::NumericTable *dataTable1,
                        daal::data_management::NumericTable &dataTable2,
                        char *title1 = "", char *title2 = "", char *message = "", size_t nPrintedRows = 0)
{
    printNumericTables(dataTable1, &dataTable2, title1, title2, message, nPrintedRows);
}

template<typename type1, typename type2>
void printNumericTables(daal::services::SharedPtr<daal::data_management::NumericTable> dataTable1,
                        daal::services::SharedPtr<daal::data_management::NumericTable> dataTable2,
                        char *title1 = "", char *title2 = "", char *message = "", size_t nPrintedRows = 0)
{
    printNumericTables<type1, type2>(dataTable1.get(), dataTable2.get(), title1, title2, message, nPrintedRows);
}

bool checkFileIsAvailable(std::string filename, bool needExit = false)
{
    std::ifstream file(filename.c_str());
    if(file.good())
    {
        return true;
    }
    else
    {
        std::cout << "Can't open file " << filename << std::endl;
        if(needExit)
        {
            exit(fileError);
        }
        return false;
    }
}

void checkArguments(int argc, char *argv[], int count, ...)
{
    std::string **filelist = new std::string*[count];
    va_list ap;
    va_start(ap, count);
    for (int i = 0; i < count; i++)
    {
        filelist[i] = va_arg(ap, std::string *);
    }
    va_end(ap);
    if(argc == 1)
    {
        for (int i = 0; i < count; i++)
        {
            checkFileIsAvailable(*(filelist[i]), true);
        }
    }
    else if(argc == (count + 1))
    {
        bool isAllCorrect = true;
        for (int i = 0; i < count; i++)
        {
            if(!checkFileIsAvailable(argv[i + 1]))
            {
                isAllCorrect = false;
                break;
            }
        }
        if(isAllCorrect == true)
        {
            for(int i = 0; i < count; i++)
            {
                (*filelist[i]) = argv[i + 1];
            }
        }
        else
        {
            std::cout << "Warning: Try to open default datasetFileNames" << std::endl;
            for (int i = 0; i < count; i++)
            {
                checkFileIsAvailable(*(filelist[i]), true);
            }
        }
    }
    else
    {
        std::cout << "Usage: " << argv[0] << " [ ";
        for(int i = 0; i < count; i++)
        {
            std::cout << "<filename_" << i << "> ";
        }
        std::cout << "]" << std::endl;
        std::cout << "Warning: Try to open default datasetFileNames" << std::endl;
        for (int i = 0; i < count; i++)
        {
            checkFileIsAvailable(*(filelist[i]), true);
        }
    }
    delete [] filelist;
}

void copyBytes(daal::byte *dst, daal::byte *src, size_t size)
{
    for(size_t i = 0; i < size; i++)
    {
        dst[i] = src[i];
    }
}

size_t checkBytes(daal::byte *dst, daal::byte *src, size_t size)
{
    for(size_t i = 0; i < size; i++)
    {
        if(dst[i] != src[i]) { return i + 1; }
    }
    return 0;
}

static const unsigned int crcRem[] =
{
    0x00000000, 0x741B8CD6, 0xE83719AC, 0x9C2C957A, 0xA475BF8E, 0xD06E3358, 0x4C42A622, 0x38592AF4,
    0x3CF0F3CA, 0x48EB7F1C, 0xD4C7EA66, 0xA0DC66B0, 0x98854C44, 0xEC9EC092, 0x70B255E8, 0x04A9D93E,
    0x79E1E794, 0x0DFA6B42, 0x91D6FE38, 0xE5CD72EE, 0xDD94581A, 0xA98FD4CC, 0x35A341B6, 0x41B8CD60,
    0x4511145E, 0x310A9888, 0xAD260DF2, 0xD93D8124, 0xE164ABD0, 0x957F2706, 0x0953B27C, 0x7D483EAA,
    0xF3C3CF28, 0x87D843FE, 0x1BF4D684, 0x6FEF5A52, 0x57B670A6, 0x23ADFC70, 0xBF81690A, 0xCB9AE5DC,
    0xCF333CE2, 0xBB28B034, 0x2704254E, 0x531FA998, 0x6B46836C, 0x1F5D0FBA, 0x83719AC0, 0xF76A1616,
    0x8A2228BC, 0xFE39A46A, 0x62153110, 0x160EBDC6, 0x2E579732, 0x5A4C1BE4, 0xC6608E9E, 0xB27B0248,
    0xB6D2DB76, 0xC2C957A0, 0x5EE5C2DA, 0x2AFE4E0C, 0x12A764F8, 0x66BCE82E, 0xFA907D54, 0x8E8BF182,
    0x939C1286, 0xE7879E50, 0x7BAB0B2A, 0x0FB087FC, 0x37E9AD08, 0x43F221DE, 0xDFDEB4A4, 0xABC53872,
    0xAF6CE14C, 0xDB776D9A, 0x475BF8E0, 0x33407436, 0x0B195EC2, 0x7F02D214, 0xE32E476E, 0x9735CBB8,
    0xEA7DF512, 0x9E6679C4, 0x024AECBE, 0x76516068, 0x4E084A9C, 0x3A13C64A, 0xA63F5330, 0xD224DFE6,
    0xD68D06D8, 0xA2968A0E, 0x3EBA1F74, 0x4AA193A2, 0x72F8B956, 0x06E33580, 0x9ACFA0FA, 0xEED42C2C,
    0x605FDDAE, 0x14445178, 0x8868C402, 0xFC7348D4, 0xC42A6220, 0xB031EEF6, 0x2C1D7B8C, 0x5806F75A,
    0x5CAF2E64, 0x28B4A2B2, 0xB49837C8, 0xC083BB1E, 0xF8DA91EA, 0x8CC11D3C, 0x10ED8846, 0x64F60490,
    0x19BE3A3A, 0x6DA5B6EC, 0xF1892396, 0x8592AF40, 0xBDCB85B4, 0xC9D00962, 0x55FC9C18, 0x21E710CE,
    0x254EC9F0, 0x51554526, 0xCD79D05C, 0xB9625C8A, 0x813B767E, 0xF520FAA8, 0x690C6FD2, 0x1D17E304,
    0x5323A9DA, 0x2738250C, 0xBB14B076, 0xCF0F3CA0, 0xF7561654, 0x834D9A82, 0x1F610FF8, 0x6B7A832E,
    0x6FD35A10, 0x1BC8D6C6, 0x87E443BC, 0xF3FFCF6A, 0xCBA6E59E, 0xBFBD6948, 0x2391FC32, 0x578A70E4,
    0x2AC24E4E, 0x5ED9C298, 0xC2F557E2, 0xB6EEDB34, 0x8EB7F1C0, 0xFAAC7D16, 0x6680E86C, 0x129B64BA,
    0x1632BD84, 0x62293152, 0xFE05A428, 0x8A1E28FE, 0xB247020A, 0xC65C8EDC, 0x5A701BA6, 0x2E6B9770,
    0xA0E066F2, 0xD4FBEA24, 0x48D77F5E, 0x3CCCF388, 0x0495D97C, 0x708E55AA, 0xECA2C0D0, 0x98B94C06,
    0x9C109538, 0xE80B19EE, 0x74278C94, 0x003C0042, 0x38652AB6, 0x4C7EA660, 0xD052331A, 0xA449BFCC,
    0xD9018166, 0xAD1A0DB0, 0x313698CA, 0x452D141C, 0x7D743EE8, 0x096FB23E, 0x95432744, 0xE158AB92,
    0xE5F172AC, 0x91EAFE7A, 0x0DC66B00, 0x79DDE7D6, 0x4184CD22, 0x359F41F4, 0xA9B3D48E, 0xDDA85858,
    0xC0BFBB5C, 0xB4A4378A, 0x2888A2F0, 0x5C932E26, 0x64CA04D2, 0x10D18804, 0x8CFD1D7E, 0xF8E691A8,
    0xFC4F4896, 0x8854C440, 0x1478513A, 0x6063DDEC, 0x583AF718, 0x2C217BCE, 0xB00DEEB4, 0xC4166262,
    0xB95E5CC8, 0xCD45D01E, 0x51694564, 0x2572C9B2, 0x1D2BE346, 0x69306F90, 0xF51CFAEA, 0x8107763C,
    0x85AEAF02, 0xF1B523D4, 0x6D99B6AE, 0x19823A78, 0x21DB108C, 0x55C09C5A, 0xC9EC0920, 0xBDF785F6,
    0x337C7474, 0x4767F8A2, 0xDB4B6DD8, 0xAF50E10E, 0x9709CBFA, 0xE312472C, 0x7F3ED256, 0x0B255E80,
    0x0F8C87BE, 0x7B970B68, 0xE7BB9E12, 0x93A012C4, 0xABF93830, 0xDFE2B4E6, 0x43CE219C, 0x37D5AD4A,
    0x4A9D93E0, 0x3E861F36, 0xA2AA8A4C, 0xD6B1069A, 0xEEE82C6E, 0x9AF3A0B8, 0x06DF35C2, 0x72C4B914,
    0x766D602A, 0x0276ECFC, 0x9E5A7986, 0xEA41F550, 0xD218DFA4, 0xA6035372, 0x3A2FC608, 0x4E344ADE
};

unsigned int getCRC32( daal::byte *input, unsigned int prevRes, size_t len)
{
    size_t i;
    daal::byte *p;

    unsigned int res, highDigit, nextDigit;
    const unsigned int crcPoly = 0xBA0DC66B;

    p = input;

    res = prevRes;

    for ( i = 0; i < len; i++)
    {
        highDigit = res >> 24;
        nextDigit = (unsigned int)(p[len - 1 - i]);
        res       = (res << 8) ^ nextDigit;
        res       = res ^ crcRem[highDigit];
    }

    if (res >= crcPoly)
    {
        res = res ^ crcPoly;
    }

    return res;
}

#endif
