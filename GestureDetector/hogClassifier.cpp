//
//  hogClassifier.cpp
//  GestureDetector
//
//  Created by Zichen Zheng on 12/1/13.
//  Copyright (c) 2013 Zichen Zheng. All rights reserved.
//

#include "hogClassifier.h"

void hogTrain(char* trainFileList, vector<string>& labels) {
    ifstream fs;
    fs.open(trainFileList);
    if (fs == NULL) {
        cerr << "Error: training file list \"" << trainFileList<< "\" does not exist\n";
        exit(-1);
    }
    
    HOGDescriptor hog(hogWinSize, hogBlockSize, hogBlockStride, hogCellSize, hogNBins);
    DEBUGMODE {
        cout << "HOG descriptor length = " << hog.getDescriptorSize() << endl;
    }
    
    string label, imgpath;
    ofstream dataFile;
    string dataFilePath;
    dataFilePath.append(ROOT_DIR).append(FEATURES_DIR).append(trainDataFileName);
    dataFile.open(dataFilePath.c_str());
    cout << "Extracting HOG features ...\n";
    while (fs >> label) {
        int catnum = hasCategory(labels, label);
        if (catnum < 0) {
            labels.push_back(label);
            catnum = (int) labels.size() - 1;
        }
        
        fs >> imgpath;
        DEBUGMODE {
            cout << "Extracting HOG features for " << imgpath << endl;
        }
        Mat img = imread(imgpath);
        assert(img.data);  // image should not be empty
        Mat resizedImg = Mat::zeros(hogWinSize.width, hogWinSize.height, CV_8UC3);
        vector<float> featVec;
        resize(img, resizedImg, hogWinSize, 0, 0, INTER_CUBIC);
        hog.compute(resizedImg, featVec, hogWinStride);  // compute HOG feature
        
        // Store features into files following the libsvm dataset format:
        // <label> 1:<feat_1> 2:<feat_2> ... d:<feat_d>
        // where d is the number of features (i.e. dimensionality).
        dataFile << catnum << " ";  // write label
        for (int i = 0; i < featVec.size(); i++) {   // write data
            dataFile << i+1 << ":" << featVec[i] << " ";
        }
        dataFile << endl;
    }
    
    fs.close();
    dataFile.close();
    
    string svmScaleFilePath, scaledDataFilePath, svmModelFilePath;
    svmScaleFilePath.append(ROOT_DIR).append(FEATURES_DIR).append(svmScaleRangeFileName);
    scaledDataFilePath.append(ROOT_DIR).append(FEATURES_DIR).append(scaledTrainDataFileName);
    svmModelFilePath.append(ROOT_DIR).append(FEATURES_DIR).append(svmModelFileName);
    
    string command;  // UNIX command
    
    // scale data to range [-1 1]
    command.append(ROOT_DIR).append(LIBSVM_DIR).append("svm-scale -l -1 -u 1 -s ").append(svmScaleFilePath).append(" ").append(dataFilePath).append(" > ").append(scaledDataFilePath);
    cout << "Scaling training data ... \n";
    DEBUGMODE {
        cout << command << endl;
    }
    system(command.c_str()); // execute UNIX command
    
    // train SVM classifier
    command.clear();
    svmTrainRoutine.append(svmParam);
    DEBUGMODE {} else svmTrainRoutine.append("-q ");
    command.append(ROOT_DIR).append(LIBSVM_DIR).append(svmTrainRoutine).append(scaledDataFilePath).append(" ").append(svmModelFilePath);
    cout << "Training SVM classifier ... \n";
    DEBUGMODE {
        cout <<command << endl;
    }
    system(command.c_str()); // execute UNIX command
}


void hogTest(char* testFileList, const vector<string>& labels) {
    ifstream fs;
    fs.open(testFileList);
    if (fs == NULL) {
        cerr << "Error: testing file list \"" << testFileList<< "\" does not exist\n";
        exit(-1);
    }
    
    HOGDescriptor hog(hogWinSize, hogBlockSize, hogBlockStride, hogCellSize, hogNBins);
    DEBUGMODE {
        cout << "HOG descriptor length = " << hog.getDescriptorSize() << endl;
    }
    
    string label, imgpath;
    ofstream dataFile;
    string dataFilePath;
    dataFilePath.append(ROOT_DIR).append(FEATURES_DIR).append(testDataFileName);
    dataFile.open(dataFilePath.c_str());
    cout << "Extracting HOG features ...\n";
    while (fs >> label) {
        int catnum = hasCategory(labels, label);
        assert(catnum >= 0);
        
        fs >> imgpath;
        DEBUGMODE {
            cout << "Extracting HOG features for " << imgpath << endl;
        }
        Mat img = imread(imgpath);
        assert(img.data);  // image should not be empty
        Mat resizedImg = Mat::zeros(hogWinSize.width, hogWinSize.height, CV_8UC3);
        vector<float> featVec;
        resize(img, resizedImg, hogWinSize, 0, 0, INTER_CUBIC);
        hog.compute(resizedImg, featVec, hogWinStride);  // compute HOG feature
        
        // Store features into files following the libsvm dataset format:
        // <label> 1:<feat_1> 2:<feat_2> ... d:<feat_d>
        // where d is the number of features (i.e. dimensionality).
        dataFile << catnum << " ";  // write label
        for (int i = 0; i < featVec.size(); i++) {   // write data
            dataFile << i+1 << ":" << featVec[i] << " ";
        }
        dataFile << endl;
    }
    fs.close();
    dataFile.close();
    
    string svmScaleFilePath, scaledDataFilePath, svmModelFilePath, resultFilePath;
    svmScaleFilePath.append(ROOT_DIR).append(FEATURES_DIR).append(svmScaleRangeFileName);
    scaledDataFilePath.append(ROOT_DIR).append(FEATURES_DIR).append(scaledTestDataFileName);
    svmModelFilePath.append(ROOT_DIR).append(FEATURES_DIR).append(svmModelFileName);
    resultFilePath.append(ROOT_DIR).append(FEATURES_DIR).append(testResultFileName);
    
    string command;  // UNIX command
    
    // scale testing data
    command.append(ROOT_DIR).append(LIBSVM_DIR).append("svm-scale -r ").append(svmScaleFilePath).append(" ").append(dataFilePath).append(" > ").append(scaledDataFilePath);
    cout << "Scaling testing data ... \n";
    DEBUGMODE {
        cout << command << endl;
    }
    system(command.c_str()); // execute UNIX command

    // predict by SVM classifier
    command.clear();
    command.append(ROOT_DIR).append(LIBSVM_DIR).append(svmPredictRoutine).append(scaledDataFilePath).append(" ").append(svmModelFilePath).append(" ").append(resultFilePath);
    cout << "Predicting ... \n";
    DEBUGMODE {
        cout << command << endl;
    }
    system(command.c_str()); // execute UNIX command
}


int hogPredict(const Mat& img, string rootDir) {
    if (rootDir.empty()) rootDir = ROOT_DIR;
    assert(img.data);  // image should not be empty
    Mat resizedImg = Mat::zeros(hogWinSize.width, hogWinSize.height, CV_8UC3);
    vector<float> featVec;
    resize(img, resizedImg, hogWinSize, 0, 0, INTER_CUBIC);
    
    HOGDescriptor hog(hogWinSize, hogBlockSize, hogBlockStride, hogCellSize, hogNBins);
    DEBUGMODE {
        cout << "HOG descriptor length = " << hog.getDescriptorSize() << endl;
    }
    hog.compute(resizedImg, featVec, hogWinStride);  // compute HOG feature
    
    ofstream dataFile;
    string dataFilePath;
    dataFilePath.append(rootDir).append(FEATURES_DIR).append(predictDataFileName);
    dataFile.open(dataFilePath.c_str());
    dataFile << "-1 ";
    for (int i = 0; i < featVec.size(); i++) {   // write data
        dataFile << i+1 << ":" << featVec[i] << " ";
    }
    dataFile.close();
    
    string svmScaleFilePath, scaledDataFilePath, svmModelFilePath, resultFilePath;
    svmScaleFilePath.append(rootDir).append(FEATURES_DIR).append(svmScaleRangeFileName);
    scaledDataFilePath.append(rootDir).append(FEATURES_DIR).append(scaledPredictDataFileName);
    svmModelFilePath.append(rootDir).append(FEATURES_DIR).append(svmModelFileName);
    resultFilePath.append(rootDir).append(FEATURES_DIR).append(predictResultFileName);
    
    string command;  // UNIX command
    
    // scale testing data
    command.append(rootDir).append(LIBSVM_DIR).append("svm-scale -r ").append(svmScaleFilePath).append(" ").append(dataFilePath).append(" > ").append(scaledDataFilePath);
    DEBUGMODE {
        cout << "Scaling frame ... \n" << command << endl;
    }
    system(command.c_str()); // execute UNIX command
    
    // predict by SVM classifier
    command.clear();
    DEBUGMODE {} else svmPredictRoutine.append("-q ");
    command.append(rootDir).append(LIBSVM_DIR).append(svmPredictRoutine).append(scaledDataFilePath).append(" ").append(svmModelFilePath).append(" ").append(resultFilePath);
    DEBUGMODE {
        cout << "Predicting ... \n" <<command << endl;
    }
    system(command.c_str()); // execute UNIX command
    
    ifstream fs;
    fs.open(resultFilePath.c_str());
    int label;
    fs >> label;
    fs.close();
    
    return label;
}


int hasCategory(const vector<string>& labels, const string& label) {
    for (int i = 0; i < labels.size(); i++) {
        string str = labels[i];
        if (str.compare(label) == 0) return i;
    }
    return -1;
}
