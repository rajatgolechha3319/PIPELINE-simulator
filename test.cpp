#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;
#include "BranchPredictor.hpp"

int main() {
    std::ifstream infile("BranchPredictor.txt");
    std::string line;
    int val;
    cin>>val;
    SaturatingBranchPredictor predictor(val);
    int predictions = 0;
    int mispredictions = 0;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        uint32_t pc;
        bool taken;
        if (!(iss >> std::hex >> pc >> taken)) { break; }

        bool prediction = predictor.predict(pc);
        //std::cout << std::hex << pc << " " << taken << " " << prediction << std::endl;
        if(prediction != taken) {
            mispredictions++;
        }
        else{
            predictions++;
        }
        predictor.update(pc, taken);
    }
    cout<<"Percentage Accuracy: "<<(1-(double)mispredictions/(mispredictions+predictions))*100<<"\\%"<<endl;
    std::ifstream infile2("BranchPredictor.txt");
    std::string line2;
    BHRBranchPredictor predictor2(val);
    int predictions2 = 0;
    int mispredictions2 = 0;
    while (std::getline(infile2, line2)) {
        std::istringstream iss(line2);
        uint32_t pc;
        bool taken;
        if (!(iss >> std::hex >> pc >> taken)) { break; }

        bool prediction = predictor2.predict(pc);
        //std::cout << std::hex << pc << " " << taken << " " << prediction << std::endl;
        if(prediction != taken) {
            mispredictions2++;
        }
        else{
            predictions2++;
        }
        predictor2.update(pc, taken);
    }
    cout<<"Percentage Accuracy: "<<(1-(double)mispredictions2/(mispredictions2+predictions2))*100<<"\\%"<<endl;
    std::ifstream infile3("BranchPredictor.txt");
    std::string line3;
    SaturatingBHRBranchPredictor predictor3(val,1<<16);
    int predictions3 = 0;
    int mispredictions3 = 0;
    while (std::getline(infile3, line3)) {
        std::istringstream iss(line3);
        uint32_t pc;
        bool taken;
        if (!(iss >> std::hex >> pc >> taken)) { break; }

        bool prediction = predictor3.predict(pc);
        //std::cout << std::hex << pc << " " << taken << " " << prediction << std::endl;
        if(prediction != taken) {
            mispredictions3++;
        }
        else{
            predictions3++;
        }
        predictor3.update(pc, taken);
    }
    cout<<"Percentage Accuracy: "<<(1-(double)mispredictions3/(mispredictions3+predictions3))*100<<"\\%"<<endl;
    return 0;
}