#ifndef __BRANCH_PREDICTOR_HPP__
#define __BRANCH_PREDICTOR_HPP__

#include <vector>
#include <bitset>
#include <cassert>
#include <iostream>
struct BranchPredictor {
    virtual bool predict(uint32_t pc) = 0;
    virtual void update(uint32_t pc, bool taken) = 0;
};

struct SaturatingBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2> > table;
    SaturatingBranchPredictor(int value) : table(1 << 14, value) {}

    bool predict(uint32_t pc) {
        uint32_t index = pc & 0x3FFF; // mask off the 14 LSB
        if (table.at(index)==2 || table.at(index)==3) return true;
        else return false;
    }

    void update(uint32_t pc, bool taken) {
        uint32_t index = pc & 0x3FFF; // mask off the 14 LSB
        bool prediction = predict(pc);
        if ((prediction)&&(taken)){
             if (table.at(index)==2) { table.at(index) = 3; }
        }
        else if ((prediction)&& not(taken)){
             if (table.at(index)==2) { table.at(index) = 1; }
             if (table.at(index)==3) { table.at(index) = 2; }
        }
        else if (not(prediction)&&(taken)){
             if (table.at(index)==1) { table.at(index) = 2; }
             if (table.at(index)==0) { table.at(index) = 1; }
        }
        else{
             if (table.at(index)==1) { table.at(index) = 0; }
        }
    }
};

struct BHRBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2> > bhrTable;
    std::bitset<2> bhr;
    BHRBranchPredictor(int value) : bhrTable(1 << 2, value), bhr(value) {}

    bool predict(uint32_t pc) {
        // your code here
        int index = bhr.to_ulong();
        if (bhrTable.at(index) == 2 || bhrTable.at(index) == 3) return true;
        else return false;
    }

    void update(uint32_t pc, bool taken) {
        // your code here
        bool prediction = predict(pc);
        int index = bhr.to_ulong();
        if (taken) {
            if (bhr==0) { bhr = 1;}
            else if (bhr==1) { bhr = 3;}
            else if (bhr==2) { bhr = 1;}
            else if (bhr==3) { bhr = 3;}
        }
        else{
            if (bhr==0) {bhr = 0;}
            else if (bhr==1) { bhr = 2;}
            else if (bhr==2) { bhr = 0;}
            else if (bhr==3) { bhr = 2;}
        }

        if ((prediction)&&(taken)){
             if (bhrTable.at(index)==2) { bhrTable.at(index) = 3; }
        }
        else if ((prediction)&& not(taken)){
             if (bhrTable.at(index)==2) { bhrTable.at(index) = 1; }
             if (bhrTable.at(index)==3) { bhrTable.at(index) = 2; }
        }
        else if (not(prediction)&&(taken)){
             if (bhrTable.at(index)==1) { bhrTable.at(index) = 2; }
             if (bhrTable.at(index)==0) { bhrTable.at(index) = 1; }
        }
        else{
             if (bhrTable.at(index)==1) { bhrTable.at(index) = 0; }
        }

        
    }
};

struct SaturatingBHRBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2> > bhrTable;
    std::bitset<2> bhr;
    std::vector<std::bitset<2> > table;
    std::vector<std::bitset<2> > combination;
    SaturatingBHRBranchPredictor(int value, int size) : bhrTable(1 << 2, value), bhr(value), table(1 << 14, value), combination(size, value) {
        assert(size <= (1 << 16));
    }


    bool predict(uint32_t pc) {
        // your code here
            uint32_t index = pc & 0x3FFF; // mask off the 14 LSB
            int val;
            if (bhr==0) {val =0;} else if(bhr==1) { val=1;} else if(bhr==2) { val=2;} else { val=3;}
            int idx = 4*index + val;
            if (combination.at(idx)==2 || combination.at(idx)==3) return true;
            else return false;
    }

    void update(uint32_t pc, bool taken) {
        // your code here
            uint32_t index = pc & 0x3FFF; // mask off the 14 LSB
            int val;
            if (bhr==0) {val =0;} else if(bhr==1) { val=1;} else if(bhr==2) { val=2;} else { val=3;}
            int idx = 4*index + val;
            bool prediction = predict(pc);

        if (taken) {
            if (bhr==0) { bhr = 1;}
            else if (bhr==1) { bhr = 3;}
            else if (bhr==2) { bhr = 1;}
            else if (bhr==3) { bhr = 3;}
        }
        else{
            if (bhr==0) {bhr = 0;}
            else if (bhr==1) { bhr = 2;}
            else if (bhr==2) { bhr = 0;}
            else if (bhr==3) { bhr = 2;}
        }
        if ((prediction)&&(taken)){
             if (combination.at(idx)==2) { combination.at(idx) = 3; }
        }
        else if ((prediction)&& not(taken)){
             if (combination.at(idx)==2) { combination.at(idx) = 1; }
             if (combination.at(idx)==3) { combination.at(idx) = 2; }
        }
        else if (not(prediction)&&(taken)){
             if (combination.at(idx)==1) { combination.at(idx) = 2; }
             if (combination.at(idx)==0) { combination.at(idx) = 1; }
        }
        else{
             if (combination.at(idx)==1) { combination.at(idx) = 0; }
        }
    }

    
};

#endif