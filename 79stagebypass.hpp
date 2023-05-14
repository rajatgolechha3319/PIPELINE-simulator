#ifndef MIPS_PROCESSOR_HPP
#define MIPS_PROCESSOR_HPP
#include <unordered_map>
#include <string.h>
#include <string>
#include <functional>
#include <vector>
#include <cstring>
#include <cmath>
#include <fstream>
#include <exception>
#include <iostream>
#include <algorithm>
#include <boost/tokenizer.hpp>
struct MIPS_Architecture{
    int registers[32] = {0}, PCcurr = 0, PCnext;
    std::unordered_map<std::string, int> registerMap, address, registerLock;
    std::vector<std::vector<std::string>> pipes;
    static const int MAX = (1 << 20);
    int data[MAX >> 2] = {0};
    std::vector<std::vector<std::string> > commands;
    std::string latch_MEM2[5],latch_MEM1[5],latch_ALU2[5],latch_ALU1[5],latch_RR[5],latch_ID2[5], latch_ID1[5], latch_IF2[5], latch_IF1[5] = {"NULL", "NULL", "NULL", "NULL", "NULL"};
    bool branching=false,write=false;
    int stall = 0;
    std::string bypass_RR_ALU1,bypass_RR_MEM2,bypass_RR_RW;
    std::string bypass_final1,bypass_final2;
    int bypass1=0;
    int bypass2=0;
    std::vector<int> commandCount;
    enum exit_code
    {
        SUCCESS = 0,
        INVALID_REGISTER,
        INVALID_LABEL,
        INVALID_ADDRESS,
        SYNTAX_ERROR,
        MEMORY_ERROR
    };

    MIPS_Architecture(std::ifstream &file){
        for (int i = 0; i < 32; i++){
            registerMap["$" + std::to_string(i)] = i;
            registerLock["$" + std::to_string(i)] = 0;
        }
        registerMap["$zero"] = 0, registerLock["$zero"] = 0;
        registerMap["$at"] = 1, registerLock["$at"] = 0;
        registerMap["$v0"] = 2, registerLock["$v0"] = 0;
        registerMap["$v1"] = 3, registerLock["$v1"] = 0;
        for (int i = 0; i < 4; ++i)
            registerMap["$a" + std::to_string(i)] = i + 4, registerLock["$a" + std::to_string(i)] = 0;
        for (int i = 0; i < 8; ++i)
            registerMap["$t" + std::to_string(i)] = i + 8, registerMap["$s" + std::to_string(i)] = i + 16, registerLock["$t" + std::to_string(i)] = 0, registerLock["$s" + std::to_string(i)] = 0;
        registerMap["$t8"] = 24, registerLock["$t8"] = 0;
        registerMap["$t9"] = 25, registerLock["$t9"] = 0;
        registerMap["$k0"] = 26, registerLock["$k0"] = 0;
        registerMap["$k1"] = 27, registerLock["$k1"] = 0;
        registerMap["$gp"] = 28, registerLock["$gp"] = 0;
        registerMap["$sp"] = 29, registerLock["$sp"] = 0;
        registerMap["$s8"] = 30, registerLock["$s8"] = 0;
        registerMap["$ra"] = 31, registerLock["$ra"] = 1;
        constructCommands(file);
        commandCount.assign(commands.size(), 0);
    }
    bool is_number(const std::string& s) {
        if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) {
            return false;
        }
        char* endPtr;
        strtol(s.c_str(), &endPtr, 10);
        if (*endPtr != '\0') {
            return false;
        }
            return true;
        }
    bool checkDependency(std::vector<std::string>command,std::string depend){
        if(command[0]=="NOP"|| command[0]=="empty"){
            return false;
        }
        if (!(command[0]=="sw"||command[0]=="beq"||command[0]=="bne"||command[0]=="j")){
            if(is_number(depend)){
                return false;
            }
            if (registerMap[command[1]]==registerMap[depend]){
                return true;
            }else{
                return false;
            }
        }else{
            return false;
        }
    }
    void parseCommand(std::string line){
        line = line.substr(0, line.find('#'));
        std::vector<std::string> command;
        boost::tokenizer<boost::char_separator<char>> tokens(line, boost::char_separator<char>(", \t"));
        for (auto &s : tokens){
            command.push_back(s);
        }
        if (command.empty()){
            return;
        }
        else if (command.size() == 1){
            std::string label = command[0].back() == ':' ? command[0].substr(0, command[0].size() - 1) : "?";
            if (address.find(label) == address.end())
                address[label] = commands.size();
            else
                address[label] = -1;
            command.clear();
        }
        else if (command[0].back() == ':'){
            std::string label = command[0].substr(0, command[0].size() - 1);
            if (address.find(label) == address.end())
                address[label] = commands.size();
            else
                address[label] = -1;
            command = std::vector<std::string>(command.begin() + 1, command.end());
        }
        else if (command[0].find(':') != std::string::npos){
            int idx = command[0].find(':');
            std::string label = command[0].substr(0, idx);
            if (address.find(label) == address.end())
                address[label] = commands.size();
            else
                address[label] = -1;
            command[0] = command[0].substr(idx + 1);
        }
        else if (command[1][0] == ':'){
            if (address.find(command[0]) == address.end())
                address[command[0]] = commands.size();
            else
                address[command[0]] = -1;
            command[1] = command[1].substr(1);
            if (command[1] == "")
                command.erase(command.begin(), command.begin() + 2);
            else
                command.erase(command.begin(), command.begin() + 1);
        }
        if (command.empty())
            return;
        if (command.size() > 4)
            for (int i = 4; i < (int)command.size(); ++i)
                command[3] += " " + command[i];
        command.resize(4);
        commands.push_back(command);
    }

    void constructCommands(std::ifstream &file){
        std::string line;
        while (getline(file, line))
            parseCommand(line);
        file.close();
    }
    int op(std::string opcode, std::string r2, std::string r3){
        int a,b;
        a = stoi(r2);
        b = stoi(r3);
        if (opcode == "add"){
            return a+b;
        }else if (opcode == "sub"){
            return a-b;
        }else if (opcode == "mul"){
            return a*b;
        }else if (opcode == "slt"){
            return a<b;
        }else if (opcode == "sll"){
            return a<<b;
        }else if (opcode == "slr"){
            return a>>b;
        }else if (opcode == "and"){
            return a&b;
        }else if (opcode == "or"){
            return a|b;
        }else if (opcode == "addi"){
            return a+b;
        }
    }
    int branch(std::string opcode, std::string r2, std::string r3){
        int a,b;
        if (opcode == "bne"){
            a = stoi(r2);
            b = stoi(r3);
            return a!=b;
        }else if (opcode == "beq"){
            a = stoi(r2);
            b = stoi(r3);
            return a==b;
        }else if(opcode == "j"){
            return 1;
        }
    }
    int locateAddress(std::string location){
        if (location.back() == ')'){
            try{
                int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
                std::string reg = location.substr(lparen + 1);
                reg.pop_back();
                int address;
                if(is_number(reg)){
                    address = stoi(reg) + offset;
                }else{
                    address = registers[registerMap[reg]] + offset;
                }
                if (address % 4 || address < int(4 * commands.size()) || address >= MAX)                
                    return -3;
                return address / 4;
            }
            catch (std::exception &e){
                return -4;
            }
        }
        try{
            int address = stoi(location);
            if (address % 4 || address < int(4 * commands.size()) || address >= MAX)
                return -3;
            return address / 4;
        }
        catch (std::exception &e){
            return -4;
        }
    }
    int load(std::string opcode, std::string r2, std::string r3){
        int address = locateAddress(r3);
        return address;
    }
    void IF1(std::string label,std::string r1,std::string r2, std::string r3,int empty){
        if (label=="NOP"){
            // std::cout << "NOP is there" << '\n';
            return;
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
            return;
        }else{
            // std::cout << "fetching instruction for (using IF1) " << label << '\n'
            latch_IF1[0]=label,latch_IF1[1]=r1,latch_IF1[2]=r2,latch_IF1[3]=r3,latch_IF1[4]=label;
        }
        if(label=="bne"||label=="beq"||label=="j"){
            branching=true;
        }
    }
    void IF2(std::string label,std::string r1,std::string r2, std::string r3,int empty){
        if (label=="NOP"){
            // std::cout << "NOP is there" << '\n';
            return;
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
            return;
        }else{
            // std::cout << "fetching instruction for (using IF2) " << label << '\n';
            latch_IF2[0]=latch_IF1[0],latch_IF2[1]=latch_IF1[1],latch_IF2[2]=latch_IF1[2];
            latch_IF2[3]=latch_IF1[3],latch_IF2[4]=latch_IF1[4];
        }
    }
    void ID1(std::string label,std::string r1,std::string r2, std::string r3,int empty){
        if (label=="NOP"){
            // std::cout << "NOP is there" << '\n';
            return;
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
            return;
        }else{
            // std::cout << "fetching reisters for (using ID1) " << label << '\n';
            latch_ID1[0]=latch_IF2[0],latch_ID1[1]=latch_IF2[1],latch_ID1[2]=latch_IF2[2];
            latch_ID1[3]=latch_IF2[3],latch_ID1[4]=latch_IF2[4];
        }
    }
    void ID2(std::string label,std::string r1,std::string r2, std::string r3,int empty){
        if (label=="NOP"){
            // std::cout << "NOP is there" << '\n';
            return;
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
            return;
        }else{
            // std::cout << "fetching reisters for (using ID2) " << label << '\n';
            latch_ID2[0]=latch_ID1[0],latch_ID2[1]=latch_ID1[1],latch_ID2[2]=latch_ID1[2];
            latch_ID2[3]=latch_ID1[3],latch_ID2[4]=latch_ID1[4];
        }
        if(label=="j"){
            branching=false;
        }
    }
    void RR(std::string label,std::string r1,std::string r2, std::string r3,int empty){
        if (label=="NOP"){
            // std::cout << "NOP is there" << '\n';
            return;
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
            return;
        }
        else{
            // std::cout << "reading reisters for (using RR) " << label << '\n';
            if (label=="bne"||label=="beq"){
                latch_RR[1]=std::to_string(registers[registerMap[r1]]),latch_RR[2]=std::to_string(registers[registerMap[r2]]);
                latch_RR[3]=latch_ID2[3];
                if (bypass1==1){
                    latch_RR[1]=bypass_final1;
                }
                if(bypass2==1){
                    latch_RR[2]=bypass_final2;
                }
            }else if (label=="sw"){
                latch_RR[1]=std::to_string(registers[registerMap[r1]]),latch_RR[2]=latch_ID2[2];
                latch_RR[3]=latch_ID2[3];
                if (bypass1==1){
                    latch_RR[1]=bypass_final1;
                }
                if(bypass2==1){
                    int lapren = latch_ID2[2].find("(");
                    std::string depend = latch_ID2[2].substr(0,lapren);
                    std::string offset = depend+"("+bypass_final2+")";
                    latch_RR[2]=offset;
                }
            }else if (label=="lw"){
                latch_RR[1]=latch_ID2[1],latch_RR[2]=latch_ID2[2];
                latch_RR[3]=latch_ID2[3];
                if(bypass2==1){
                    int lapren = latch_ID2[2].find("(");
                    std::string depend = latch_ID2[2].substr(0,lapren);
                    std::string offset = depend+"("+bypass_final2+")";
                    latch_RR[2]=offset;
                }
            }else if (label=="j"){
                latch_RR[0]=latch_ID2[0],latch_RR[1]=latch_ID2[1],latch_RR[2]=latch_ID2[2];
                latch_RR[3]=latch_ID2[3];
            }else if (label=="addi"){
                latch_RR[0]=latch_ID2[0],latch_RR[1]=latch_ID2[1],latch_RR[2]=std::to_string(registers[registerMap[r2]]);
                latch_RR[3]=latch_ID2[3];
                if (bypass1==1){
                    latch_RR[2]=bypass_final1;
                }
            }else{
                latch_RR[0]=latch_ID2[0],latch_RR[1]=latch_ID2[1],latch_RR[2]=std::to_string(registers[registerMap[r2]]);
                latch_RR[3]=std::to_string(registers[registerMap[r3]]);
                if (bypass1==1){
                    latch_RR[2]=bypass_final1;
                }
                if(bypass2==1){
                    latch_RR[3]=bypass_final2;
                }
            }
            if (!(label=="sw"||label=="bne"||label=="beq"||label=="j")){
                registerLock[latch_RR[1]]+=1;
            }
        }
    }
    void ALU1(std::string label,std::string r1,std::string r2, std::string r3,int empty){
        if (label=="NOP"){
            // std::cout << "NOP is there" << '\n';
            return;
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
            return;
        }else{
            // std::cout << "reading reisters for (using ALU1) " << label << '\n';
            std::string result1;
            if (label=="bne"||label=="beq"){
                branching=false;
                result1 = std::to_string(branch(label,latch_RR[1],latch_RR[2]));
            }else if (label=="j"){
                result1=latch_RR[4];
            }else{
                result1 = std::to_string(op(label,latch_RR[2],latch_RR[3]));
            }
            latch_ALU1[0]=latch_RR[0],latch_ALU1[1]=latch_RR[1],latch_ALU1[2]=latch_RR[2];
            latch_ALU1[3]=latch_RR[3],latch_ALU1[4]=result1;
            bypass_RR_ALU1=latch_ALU1[4];
        }
    }
    void ALU2(std::string label,std::string r1,std::string r2, std::string r3,int empty){
        if (label=="NOP"){
            // std::cout << "NOP is there" << '\n';
            return;
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
            return;
        }else{
            // std::cout << "reading reisters for (using ALU2) " << label << '\n';
            std::string result1= std::to_string(load(label,latch_RR[1],latch_RR[2]));
            latch_ALU2[0]=latch_RR[0],latch_ALU2[1]=latch_RR[1],latch_ALU2[2]=latch_RR[2];
            latch_ALU2[3]=latch_RR[3],latch_ALU2[4]=result1;
        }
    }
    void MEM1(std::string label,std::string r1,std::string r2, std::string r3,int empty){
        if (label=="NOP"){
            // std::cout << "NOP is there" << '\n';
            return;
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
            return;
        }
        else{
            // std::cout << "fetching reisters for (using MEM1) " << label << '\n';
            latch_MEM1[0]=latch_ALU2[0],latch_MEM1[1]=latch_ALU2[1],latch_MEM1[2]=latch_ALU2[2];
            latch_MEM1[3]=latch_ALU2[3],latch_MEM1[4]=latch_ALU2[4];
        }
    }    
    void MEM2(std::string label,std::string r1,std::string r2, std::string r3,int empty){
        if (label=="NOP"){
            // std::cout << "NOP is there" << '\n';
            return;
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
            return;
        }
        else{
            // std::cout << "fetching reisters for (using MEM2) " << label << '\n';
            if(label=="lw"){
                latch_MEM2[4]= std::to_string(data[stoi(latch_MEM1[4])]);
            }else if (label=="sw"){
                data[stoi(latch_MEM1[4])]=stoi(latch_MEM1[1]);
                latch_MEM2[4]= std::to_string(data[stoi(latch_MEM1[4])]);
            }
            latch_MEM2[0]=latch_MEM1[0],latch_MEM2[1]=latch_MEM1[1],latch_MEM2[2]=latch_MEM1[2];
            latch_MEM2[3]=latch_MEM1[3];
            bypass_RR_MEM2= latch_MEM2[4];
        }
    }
    void RW(std::string label,std::string r1,std::string r2, std::string r3,int empty){
        if (label=="NOP"){
            // std::cout << "NOP is there" << '\n';
            return;
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
            return;
        }else{
            // std::cout << "fetching reisters for (using RW) " << label << '\n';
            if(!(label=="bne"||label=="beq"||label=="j"||label=="sw"||label=="lw")){
                registers[registerMap[latch_ALU1[1]]]=stoi(latch_ALU1[4]);
                if (registerLock[latch_ALU1[1]]>0){
                    registerLock[latch_ALU1[1]]-=1;
                }
                bypass_RR_RW=latch_ALU1[4];
            }else if(label=="lw") {
                registers[registerMap[latch_MEM2[1]]]=stoi(latch_MEM2[4]);
                if (registerLock[latch_MEM2[1]]>0){
                    registerLock[latch_MEM2[1]]-=1;
                }
                bypass_RR_RW=latch_MEM2[4];
            }else{
                return;
            }
        }
    }
    void executeCommandsPipelined(){
        int clockCycle = 0;
        pipes.push_back({commands[0][0],commands[0][1],commands[0][2],commands[0][3],"IF1"});
        printRegisters();
        std::cout<<0<<'\n';
        while(!pipes.empty()){
        // check dependency
            std::string labeling1, labeling2;
            ++clockCycle;
                int found1=false,found2=false;
                std::string depend1,depend2;
            bypass1=0,bypass2=0,stall=0;
            std::vector<std::string> RR_commmand;
            int index1,index2;
            for(int i=0 ;i<pipes.size();i++){
                if (pipes[i][4]=="RR"){
                    RR_commmand=pipes[i];
                    index1=i,index2=i;
                }
            }
            std::vector<std::vector<std::string> >v = pipes;
            if(RR_commmand.empty()){
                bypass2=0,bypass1=0;stall=0;
            }else{
                if(RR_commmand[0]=="sw"){
                    if (registerLock[RR_commmand[1]]>0){
                        found1=true,depend1=RR_commmand[1];
                    }
                    int lapren = RR_commmand[2].find("(");
                    std::string y = RR_commmand[2].substr(lapren+1,RR_commmand[2].size()-lapren-2);
                    if (registerLock[y]>0){
                        found2=true,depend2=y;
                    }
                }else if (RR_commmand[0]=="lw"){
                    int lapren = RR_commmand[2].find("(");
                    std::string y = RR_commmand[2].substr(lapren+1,RR_commmand[2].size()-lapren-2);
                    if (registerLock[y]>0){
                        found2=true,depend2=y;
                    }                     
                }else if (RR_commmand[0]=="bne"||RR_commmand[0]=="beq"){
                    if (registerLock[RR_commmand[1]]>0){
                        found1=true,depend1=RR_commmand[1];
                    }
                    if (registerLock[RR_commmand[2]]>0){
                        found2=true,depend2=RR_commmand[2];
                    } 
                }else if (!(RR_commmand[0]=="j")){
                    if (registerLock[RR_commmand[2]]>0){
                        found1=true,depend1=RR_commmand[2];
                    }
                    if (registerLock[RR_commmand[3]]>0){
                        found2=true,depend2=RR_commmand[3];
                    } 
                }
            }
            // RW stage
            std::vector<std::string> x;
            bool RW_found=false;
            // for(int i=0;i<pipes.size()-1;i++){
            //     if (pipes[i][0]=="lw"|| pipes[i][0]=="sw"){
            //         if( !(pipes[i+1][0]=="lw"||pipes[i+1][0]=="sw"||pipes[i+1][0]=="bne"||pipes[i+1][0]=="beq"||pipes[i+1][0]=="j")){
            //             if(pipes[i][4]=="MEM2"&&pipes[i+1][4]=="RW"){
            //                 RW_found=true;

            //             }
            //         }
            //     }
            // }
            if (RW_found==false){
                int i=0;
                while(i<pipes.size()){
                    if (pipes[i][4]=="RW"){
                        x=pipes[i];
                        if (!(x[0]=="bne"||x[0]=="beq"||x[0]=="j"||x[0]=="sw")){
                            if (write==false){
                                write=true;
                                // std::cout<<x[4]<<'\n';
                                RW(x[0],x[1],x[2],x[3],0);
                                pipes.erase(pipes.begin()+i);
                            }else{
                                i+=1;
                            }
                        }else{
                            RW(x[0],x[1],x[2],x[3],0);
                            pipes.erase(pipes.begin()+i);
                        }
                    }else{
                        i=i+1;
                    }
                }
                // if(x.empty()){
                //     RW("","","","",1);
                // } 
                x.clear();
                write=false;
            }
            // MEM2 stage
            for(int i=0;i<pipes.size();i++){
                x=pipes[i];
                if (pipes[i][4]=="MEM2"){
                    // for(auto x: latch_MEM1){
                    //     std::cout<<x<<' ';
                    // }
                    // std::cout<<'\n';
                    labeling1 = pipes[i][0],labeling2=latch_MEM1[4];
                    MEM2(pipes[i][0],pipes[i][1],pipes[i][2],pipes[i][3],0);
                    pipes[i][4]="RW";
                }
            }
            // if(x.empty()){
            //     MEM2("","","","",1);
            // }
            x.clear();
            // MEM1 stage
            for(int i=0;i<pipes.size();i++){
                x=pipes[i];
                if (pipes[i][4]=="MEM1"){
                    MEM1(pipes[i][0],pipes[i][1],pipes[i][2],pipes[i][3],0);
                    pipes[i][4]="MEM2";
                }
            }
            // if(x.empty()){
            //     MEM1("","","","",1);
            // }   
            x.clear();
            // ALU2 stage
            for(int i=0;i<pipes.size();i++){
                x=pipes[i];
                if (pipes[i][4]=="ALU2"){
                    ALU2(pipes[i][0],pipes[i][1],pipes[i][2],pipes[i][3],0);
                    pipes[i][4]="MEM1";
                }
            }
            // if(x.empty()){
            //     ALU2("","","","",1);
            // }   
            x.clear();
            // ALU1 stage
            bool ALU1_found=false;
            for(int i=0;i<pipes.size();i++){
                x=pipes[i];
                if (pipes[i][4]=="ALU1"){
                    if (RW_found==false){
                        ALU1(pipes[i][0],pipes[i][1],pipes[i][2],pipes[i][3],0);
                        pipes[i][4]="RW";
                    }
                    else{
                        ALU1_found = true;
                    }
                }
            }
            // if(x.empty()){
            //     ALU1("","","","",1);
            // }
            x.clear();
            
            if((found1==true)){
                index1-=1;
                while(index1>=0){
                    if (checkDependency(v[index1],depend1)){
                        if (v[index1][4]=="ALU1"){
                            bypass1=1,bypass_final1=bypass_RR_ALU1;
                            break;
                        }else if (v[index1][4]=="MEM2"){
                            bypass1=1,bypass_final1=bypass_RR_MEM2;
                            break;
                        }else if (v[index1][4]=="RW"){
                            bypass1=1,bypass_final1=bypass_RR_RW;
                            break;
                        }else{
                            bypass1=0,stall=1;
                            break;
                        }
                    }
                    index1-=1;
                }
            }else{
                bypass1=0;
            }
            if((found2==true&&stall==0)){
                index2-=1;
                while(index2>=0){
                    if (checkDependency(v[index2],depend2)){
                        if (v[index2][4]=="ALU1"){
                            bypass2=1,bypass_final2=bypass_RR_ALU1;
                            break;
                        }else if (v[index2][4]=="MEM2"){
                            bypass2=1,bypass_final2=bypass_RR_MEM2;
                            break;
                        }else if (v[index2][4]=="RW"){
                            bypass2=1,bypass_final2=bypass_RR_RW;
                            break;
                        }else{
                            bypass2=0,stall=1;
                            break;
                        }
                    }
                    index2-=1;
                }
            }else{
                bypass2=0;
            }
            if (stall==0){
                // RR stage
                bool RR_found = false;
                for(int i=0;i<pipes.size();i++){
                    x=pipes[i];
                    if (pipes[i][4]=="RR"){
                        if(pipes[i][0]=="lw"||pipes[i][0]=="sw"){
                            RR(pipes[i][0],pipes[i][1],pipes[i][2],pipes[i][3],0);
                            pipes[i][4]="ALU2";
                        }else{
                            if(ALU1_found==false){
                                RR(pipes[i][0],pipes[i][1],pipes[i][2],pipes[i][3],0);
                                pipes[i][4]="ALU1";
                            }else{
                                RR_found=true;

                            }
                        }
                    }
                }
                // if(x.empty()){
                //     RR("","","","",1);
                // }
                x.clear();            
                // ID2 stage
                bool ID2_found=false;
                for(int i=0;i<pipes.size();i++){
                    x=pipes[i];
                    if (pipes[i][4]=="ID2"){
                        if (RR_found==false){
                            ID2(pipes[i][0],pipes[i][1],pipes[i][2],pipes[i][3],0);
                            pipes[i][4]="RR";
                        }else{
                            ID2_found=true;
                        }    
                    }
                }
                // if(x.empty()){
                //     ID2("","","","",1);
                // }
                x.clear();            
                // ID1 stage
                bool ID1_found=false;
                for(int i=0;i<pipes.size();i++){
                    x=pipes[i];
                    if (pipes[i][4]=="ID1"){
                        if (ID2_found==false){
                            ID1(pipes[i][0],pipes[i][1],pipes[i][2],pipes[i][3],0);
                            pipes[i][4]="ID2";
                        }else{
                            ID1_found=true;
                        }
                    }
                }
                // if(x.empty()){
                //     ID1("","","","",1);
                // }
                x.clear();
                // IF2 stage
                bool IF2_found=false;
                for(int i=0;i<pipes.size();i++){
                    x=pipes[i];
                    if (pipes[i][4]=="IF2"){
                        if(ID1_found==false){
                            IF2(pipes[i][0],pipes[i][1],pipes[i][2],pipes[i][3],0);
                            pipes[i][4]="ID1";
                        }else{
                            IF2_found=true;
                        }
                    }
                }
                
                // if(x.empty()){
                //     IF2("","","","",1);
                // }
                x.clear();
                // IF1 stage
                bool IF1_found=false;
                for(int i=0;i<pipes.size();i++){
                    x=pipes[i];
                    if (pipes[i][4]=="IF1"){
                        if ((pipes[i][0]=="lw"||pipes[i][0]=="sw"||pipes[i][0]=="beq"||pipes[i][0]=="bne"||pipes[i][0]=="j")){
                            IF1(pipes[i][0],pipes[i][1],pipes[i][2],pipes[i][3],0);
                            pipes[i][4]="IF2";
                        }else{
                            if(IF2_found==false){
                                IF1(pipes[i][0],pipes[i][1],pipes[i][2],pipes[i][3],0);
                                pipes[i][4]="IF2";
                            }else{
                                IF1_found=true;
                            }
                        }
                    }
                }
                // if(x.empty()){
                //     IF1("","","","",1);
                // }
                x.clear();
                bool found =false;
                std:: string j;
                for(int i=0;i<pipes.size();i++){
                    if ((pipes[i][4]=="RW")&&(pipes[i][0]=="bne"||pipes[i][0]=="beq")){
                        found=true;
                        j="branch";
                    }else if ((pipes[i][4]=="RR")&&(pipes[i][0]=="j")){
                        found=true;
                        j="j";
                    }
                }
                if(branching==false){
                    if(found ==true){
                        if (j=="j"){
                            PCcurr=address[latch_ID2[1]];
                        }else{
                            if (stoi(latch_ALU1[4])){
                                PCcurr=address[latch_ALU1[3]];
                            }else{
                                PCcurr+=1;
                            }
                        }
                        if(PCcurr<commands.size()){
                            pipes.push_back({commands[PCcurr][0],commands[PCcurr][1],commands[PCcurr][2],commands[PCcurr][3],"IF1"});
                        }
                    }else{
                        if(PCcurr+1<commands.size()){
                            if (IF1_found==false){
                                PCcurr+=1;
                                pipes.push_back({commands[PCcurr][0],commands[PCcurr][1],commands[PCcurr][2],commands[PCcurr][3],"IF1"});
                            }
                        }else if (PCcurr+1==commands.size()){
                            PCcurr+=1;
                        }
                    }
                }
                found=false;
            }
            printRegisters();
            // std::cout<<labeling1<<'\n';
            if (labeling1=="sw"){
                std::cout<<1<<" "<<labeling2<<" "<<data[stoi(labeling2)]<<'\n';
            }else{
                std::cout<<0<<'\n';
            }
        }
    }
    void printRegisters()
    {
        // std::cout << "Cycle number: " << clockCycle << '\n';
        for (int i = 0; i < 32; ++i)
            std::cout << registers[i] << ' ';
        std::cout << std::dec << '\n';
    }
};
#endif