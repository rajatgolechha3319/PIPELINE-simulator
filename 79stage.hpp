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
    std::vector<std::string> operands;
    std::vector<std::string> branched;
    std::vector<std::string> loader;
    static const int MAX = (1 << 20);
    int data[MAX >> 2] = {0};
    std::vector<std::vector<std::string> > commands;
    std::string latch_MEM2[5],latch_MEM1[5],latch_ALU2[5],latch_ALU1[5],latch_RR[5],latch_ID2[5], latch_ID1[5], latch_IF2[5], latch_IF1[5] = {"NULL", "NULL", "NULL", "NULL", "NULL"};
    std::string dummy[5] = {"NULL"};
    bool branching=false,write=false;
    int stage[5];
    int stall = 0;
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
        operands = {"add", "sub", "addi", "mul", "sll", "slt", "or", "and", "srl"};
        branched = {"bne", "beq", "j"};
        loader = {"lw", "sw"};
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
                // if (!checkRegister(reg))
                // 	return -3;
                int address = registers[registerMap[reg]] + offset;
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
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
        }else{
            // std::cout << "fetching instruction for (using IF1) " << label << '\n';
            latch_IF1[0]=label,latch_IF1[1]=r1,latch_IF1[2]=r2,latch_IF1[3]=r3,latch_IF1[4]=label;
        }
        if(label=="bne"||label=="beq"||label=="j"){
            branching=true;
        }
    }
    void IF2(std::string label,std::string r1,std::string r2, std::string r3,int empty){
        if (label=="NOP"){
            // std::cout << "NOP is there" << '\n';
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
        }else{
            // std::cout << "fetching instruction for (using IF2) " << label << '\n';
            latch_IF2[0]=latch_IF1[0],latch_IF2[1]=latch_IF1[1],latch_IF2[2]=latch_IF1[2];
            latch_IF2[3]=latch_IF1[3],latch_IF2[4]=latch_IF1[4];
        }
    }
    void ID1(std::string label,std::string r1,std::string r2, std::string r3,int empty){
        if (label=="NOP"){
            // std::cout << "NOP is there" << '\n';
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
        }else{
            // std::cout << "fetching reisters for (using ID1) " << label << '\n';
            latch_ID1[0]=latch_IF2[0],latch_ID1[1]=latch_IF2[1],latch_ID1[2]=latch_IF2[2];
            latch_ID1[3]=latch_IF2[3],latch_ID1[4]=latch_IF2[4];
        }
    }
    void ID2(std::string label,std::string r1,std::string r2, std::string r3,int empty){
        if (label=="NOP"){
            // std::cout << "NOP is there" << '\n';
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
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
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
        }
        else{
            // std::cout << "reading reisters for (using RR) " << label << '\n';
            if (label=="bne"||label=="beq"){
                latch_RR[1]=std::to_string(registers[registerMap[r1]]),latch_RR[2]=std::to_string(registers[registerMap[r2]]);
                latch_RR[3]=latch_ID2[3];
            }else if (label=="sw"){
                latch_RR[1]=std::to_string(registers[registerMap[r1]]),latch_RR[2]=latch_ID2[2];
                latch_RR[3]=latch_ID2[3];
            }else if (label=="lw"){
                latch_RR[1]=latch_ID2[1],latch_RR[2]=latch_ID2[2];
                latch_RR[3]=latch_ID2[3];
            }else if (label=="j"){
                latch_ID2[1]=latch_ID1[1],latch_ID2[2]=latch_ID1[2];
                latch_ID2[3]=latch_ID1[3];
            }else if (label=="addi"){
                latch_RR[1]=latch_ID2[1],latch_RR[2]=std::to_string(registers[registerMap[r2]]);
                latch_RR[3]=latch_ID2[3];
            }else{
                latch_RR[1]=latch_ID2[1],latch_RR[2]=std::to_string(registers[registerMap[r2]]);
                latch_RR[3]=std::to_string(registers[registerMap[r3]]);
            }
            if (!(label=="sw"||label=="bne"||label=="beq"||label=="j")){
                registerLock[latch_RR[1]]+=1;
            }
        }
    }
    void ALU1(std::string label,std::string r1,std::string r2, std::string r3,int empty){
        if (label=="NOP"){
            // std::cout << "NOP is there" << '\n';
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
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
        }
    }
    void ALU2(std::string label,std::string r1,std::string r2, std::string r3,int empty){
        if (label=="NOP"){
            // std::cout << "NOP is there" << '\n';
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
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
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
        }
        else{
            // std::cout << "fetching reisters for (using MEM1) " << label << '\n';
            // std::cout<<latch_ALU2[1]<<"\n";
            latch_MEM1[0]=latch_ALU2[0],latch_MEM1[1]=latch_ALU2[1],latch_MEM1[2]=latch_ALU2[2];
            latch_MEM1[3]=latch_ALU2[3],latch_MEM1[4]=latch_ALU2[4];
        }
    }    
    void MEM2(std::string label,std::string r1,std::string r2, std::string r3,int empty){
        if (label=="NOP"){
            // std::cout << "NOP is there" << '\n';
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
        }
        else{
            // std::cout << "fetching reisters for (using MEM2) " << label << '\n';
            if(label=="lw"){
                latch_MEM2[4]= std::to_string(data[stoi(latch_MEM1[4])]);
            }else if (label=="sw"){
                // std::cout<<stoi(latch_MEM1[4])<<'\n';
                data[stoi(latch_MEM1[4])]=stoi(latch_MEM1[1]);
                latch_MEM2[4]= std::to_string(data[stoi(latch_MEM1[4])]);
            }
            latch_MEM2[0]=latch_MEM1[0],latch_MEM2[1]=latch_MEM1[1],latch_MEM2[2]=latch_MEM1[2];
            latch_MEM2[3]=latch_MEM1[3];
        }
    }
    void RW(std::string label,std::string r1,std::string r2, std::string r3,int empty){
        if (label=="NOP"){
            // std::cout << "NOP is there" << '\n';
        }
        else if (empty){
            // std::cout << "stage is empty" << '\n';
        }else{
            // std::cout<<label<<'\n';
            // std::cout << "fetching reisters for (using RW) " << label << '\n';
            if(!(label=="bne"||label=="beq"||label=="j"||label=="sw"||label=="lw")){
                registers[registerMap[latch_ALU1[1]]]=stoi(latch_ALU1[4]);
                if (registerLock[latch_ALU1[1]]>0){
                    registerLock[latch_ALU1[1]]-=1;
                }
            }else if(label=="lw") {
                // std::cout<<latch_MEM2[4]<<'\n';
                registers[registerMap[latch_MEM2[1]]]=stoi(latch_MEM2[4]);
                if (registerLock[latch_MEM2[1]]>0){
                    registerLock[latch_MEM2[1]]-=1;
                }
            }else{
                // std::cout<<"doing nothing "<<label <<'\n';
                return;
            }
        }
    }
    void executeCommandsPipelined(){
        int clockCycle = 0;
        pipes.push_back({commands[0][0],commands[0][1],commands[0][2],commands[0][3],"IF1"});
        printRegisters(0);
        std::cout<<0<<'\n';
        while(!pipes.empty()){
        // check dependency
            ++clockCycle;
            std::vector<std::string> RR_command;
            std::string labeling1, labeling2;
            for(auto x: pipes){
                if (x[4]=="RR"){
                    RR_command=x;
                    std::string depend;
                    if (x[0]=="sw"){
                        int lapren = x[2].find("(");
                        depend = x[2].substr(lapren+1,x[2].size()-lapren-2); 
                        if (registerLock[depend]>0||registerLock[x[1]]>0){
                            stall=1;
                        }else{
                            stall=0;
                        }
                    }else if(x[0]=="lw"){
                        int lapren = x[2].find("(");
                        depend = x[2].substr(lapren+1,x[2].size()-lapren-2); 
                        if (registerLock[depend]>0){
                            stall=1;
                        }else{
                            stall=0;
                        }
                    }else if (x[0]=="bne"||x[0]=="beq"){
                        if (registerLock[x[2]]>0||registerLock[x[1]]>0){
                            stall=1;
                        }else{
                            stall=0;
                        }
                    }else if((x[0]=="addi")){
                        if (registerLock[x[2]]>0){
                            stall=1;
                        }else{
                            stall=0;
                        }
                    
                    }else if (!(x[0]=="j")){
                        if (registerLock[x[2]]>0||registerLock[x[3]]>0){
                            stall=1;
                        }else{
                            stall=0;
                        }
                    }
                }
            }
            if(RR_command.empty()){
                stall=0;
            }
            // RW stage
            std::vector<std::string> x;
            bool RR_found=false;
            // for(int i=0;i<pipes.size()-1;i++){
            //     if (pipes[i][0]=="lw"){
            //         if( !(pipes[i+1][0]=="lw"||pipes[i+1][0]=="sw"||pipes[i+1][0]=="bne"||pipes[i+1][0]=="beq"||pipes[i+1][0]=="j")){
            //             if(pipes[i][4]=="MEM2"&&pipes[i+1][4]=="RW"){
            //                 RR_found=true;
            //             }
            //         }
            //     }
            // }
            if (RR_found==false){
                int i=0;
                while(i<pipes.size()){
                    if (pipes[i][4]=="RW"){
                        x=pipes[i];
                        if (!(x[0]=="bne"||x[0]=="beq"||x[0]=="j"||x[0]=="sw")){
                            if (write==false){
                                write=true;
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
            for(int i=0;i<pipes.size();i++){
                x=pipes[i];
                if (pipes[i][4]=="ALU1"){
                    ALU1(pipes[i][0],pipes[i][1],pipes[i][2],pipes[i][3],0);
                    pipes[i][4]="RW";
                }
            }
            // if(x.empty()){
            //     ALU1("","","","",1);
            // }
            x.clear();
            // std::cout<<stall<<'\n';
            if (stall==0){
                // RR stage
                for(int i=0;i<pipes.size();i++){
                    x=pipes[i];
                    if (pipes[i][4]=="RR"){
                        RR(pipes[i][0],pipes[i][1],pipes[i][2],pipes[i][3],0);
                        if(pipes[i][0]=="lw"||pipes[i][0]=="sw"){
                            pipes[i][4]="ALU2";
                        }else{
                            pipes[i][4]="ALU1";
                        }
                    }
                }
                // if(x.empty()){
                //     RR("","","","",1);
                // }
                x.clear();            
                // ID2 stage
                for(int i=0;i<pipes.size();i++){
                    x=pipes[i];
                    if (pipes[i][4]=="ID2"){
                        ID2(pipes[i][0],pipes[i][1],pipes[i][2],pipes[i][3],0);
                        pipes[i][4]="RR";
                    }
                }
                // if(x.empty()){
                //     ID2("","","","",1);
                // }
                x.clear();            
                // ID1 stage
                for(int i=0;i<pipes.size();i++){
                    x=pipes[i];
                    if (pipes[i][4]=="ID1"){
                        ID1(pipes[i][0],pipes[i][1],pipes[i][2],pipes[i][3],0);
                        pipes[i][4]="ID2";
                    }
                }
                // if(x.empty()){
                //     ID1("","","","",1);
                // }
                x.clear();
                // IF2 stage
                for(int i=0;i<pipes.size();i++){
                    x=pipes[i];
                    if (pipes[i][4]=="IF2"){
                        IF2(pipes[i][0],pipes[i][1],pipes[i][2],pipes[i][3],0);
                        pipes[i][4]="ID1";
                    }
                }
                // if(x.empty()){
                //     IF2("","","","",1);
                // }
                x.clear();
                // IF1 stage
                for(int i=0;i<pipes.size();i++){
                    x=pipes[i];
                    if (pipes[i][4]=="IF1"){
                        IF1(pipes[i][0],pipes[i][1],pipes[i][2],pipes[i][3],0);
                        pipes[i][4]="IF2";
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
                        }else if (j=="branch"){
                            if (stoi(latch_ALU1[4])==1){
                                // std::cout<<"hello"<<'\n';
                                PCcurr=address[latch_ALU1[3]];
                                // std::cout<<commands.size()<<'\n';
                                // std::cout<<PCcurr<<'\n';
                            }else{
                                PCcurr+=1;
                            }
                        }
                        if(PCcurr<commands.size()){
                            pipes.push_back({commands[PCcurr][0],commands[PCcurr][1],commands[PCcurr][2],commands[PCcurr][3],"IF1"});
                        }
                        
                    }else{
                        if(PCcurr+1<commands.size()){
                            PCcurr+=1;
                            pipes.push_back({commands[PCcurr][0],commands[PCcurr][1],commands[PCcurr][2],commands[PCcurr][3],"IF1"});
                        }else if (PCcurr+1==commands.size()){
                            PCcurr+=1;
                        }
                    }
                }
                found=false;
            }
            printRegisters(clockCycle);
            // std::cout<<labeling1<<'\n';
            if (labeling1=="sw"){
                std::cout<<1<<" "<<labeling2<<" "<<data[stoi(labeling2)]<<'\n';
            }else{
                std::cout<<0<<'\n';
            }

        // check RW according to write bool

        // check MEM2,MEM1,ALU2,ALU1,RR,ID2,ID1,IF2,IF1

        // upadte pc according to branching bool append the instruction according to it
        }

    }
    void printRegisters(int clockCycle)
    {
        // std::cout << "Cycle number: " << clockCycle << '\n';
        for (int i = 0; i < 32; ++i)
            std::cout << registers[i] << ' ';
        std::cout << std::dec << '\n';
    }
};
#endif