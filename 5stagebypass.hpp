#ifndef MIPS_PROCESSOR_HPP
#define MIPS_PROCESSOR_HPP
#include <unordered_map>
#include <string>
#include <string.h>
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
    std::vector<std::string> operands, branched,loader;
    static const int MAX = (1 << 20);
    int data[MAX >> 2] = {0};
    std::vector<std::vector<std::string> > commands;
    std::string latch4[5], latch3[5], latch2[5], latch1[5] = {"NULL", "NULL", "NULL", "NULL", "NULL"};
    int stall = 0;
    std::vector<int> commandCount;
    int bypass1=0,bypass2=0,bypass3=0,bypass4=0;
    std::string bypass_ID_EX,bypass_ID_MEM;
    enum exit_code{
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
        pipes = {{"empty"}, {"empty"}, {"empty"}, {"empty"}, {"empty"}};
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

    bool search(std::vector<std::string> searcher, std::string element){
        for (int i = 0; i < searcher.size(); i++){
            if (searcher[i] == element){
                return true;
            }
        }
        return false;
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
        if (is_number(r2)){
            a = stoi(r2);
        }else{
            a=registers[registerMap[r2]];
        }
        if (is_number(r3)){
            b = stoi(r3);
        }else{
            b=registers[registerMap[r3]];
        }
        if (opcode == "add"){
            return a+b;
        }
        else if (opcode == "sub"){
            return a-b;
        }
        else if (opcode == "mul"){
            return a*b;
        }
        else if (opcode == "slt"){
            return a<b;
        }
        else if (opcode == "sll"){
            return a<<b;
        }
        else if (opcode == "slr"){
            return a>>b;
        }
        else if (opcode == "and"){
            return a&b;
        }
        else if (opcode == "or"){
            return a|b;
        }
        else if (opcode == "addi"){
            return a+b;
        }
    }
    int branch(std::string opcode, std::string r2, std::string r3){
        int a,b;
        if (is_number(r2)){
            a = stoi(r2);
        }else{
            a=registers[registerMap[r2]];
        }
        if (is_number(r3)){
            b = stoi(r3);
        }else{
            b=registers[registerMap[r3]];
        }
        if (opcode == "bne"){
            return a!=b;
        }
        else if (opcode == "beq"){
            return a==b;
        }
        else if(opcode == "j"){
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
                // std::cout<<reg<<'\n';
                
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
                // std::cout<<"hell<<'\n';
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
    void IF(std::string label, int empty){
        if (empty){
            // std::cout << "stage is empty" << '\n';
        }
        else if (label == "NOP"){
            // std::cout << "NOP is there" << '\n';
        }
        else{
            // std::cout << "fetching instruction for (using IF) " << label << '\n';
        }
    }
    void ID(std::string label, int empty){
        if (empty){
            // std::cout << "stage is empty" << '\n';
        }
        else if (label == "NOP"){
            // std::cout << "NOP is there" << '\n';
        }
        else{
            // std::cout << "fetching registers for (using ID) " << label << '\n';
        }
    }
    int EX(std::string label, std::string r1, std::string r2, int empty){
        if (empty){
            // std::cout << "stage is empty" << '\n';
            return INT32_MAX;
        }
        else if (label == "NOP"){
            // std::cout << "NOP is there" << '\n';
            return INT32_MAX;
        }
        else{
            int result;
            // std::cout << "using ALU for " << label << '\n';
            if (search(operands, label)){
                result = op(label, r1, r2);
            }
            else if (search(branched, label)){

                result = branch(label, r1, r2);
            }
            else{
                result = load(label, r2, r1);
            }
            return result;
        }
    }
    int MEM(std::string label, std::string r1, std::string r2, int empty){
        if (empty){
            // std::cout << "stage is empty" << '\n';
            return INT32_MAX;
        }
        else if (label == "NOP"){
            // std::cout << "NOP is there" << '\n';
            return INT32_MAX;
        }
        else{
            if (label=="lw" || label=="sw"){
                // std::cout << "using MEM for" << label << '\n';
                if (label == "lw"){

                    return data[stoi(r2)];
                }
                else{
                    data[stoi(r2)] = registers[registerMap[r1]];
                    return data[stoi(r2)];
                }
            }else{
                // std::cout << "doing nothing" << '\n';
                if (r2 != "NULL"){
                    return stoi(r2);
                }
                else{
                    return INT32_MAX;
                }
            }
        }
    }
    void WB(std::string label, std::string r1, std::string result1, int empty){
        if (empty){
            // std::cout << "stage is empty" << '\n';
            return;
        }
        else if (label == "NOP"){
            // std::cout << "NOP is there" << '\n';
            return;
        }
        else if (label == "NULL"){
            return;
        }
        else{
            int result = stoi(result1);
            if (search(operands, label)){
                registers[registerMap[r1]] = result;
                registerLock[r1] = 0;
                // std::cout << "writing in memory register for " << label << '\n';
            }
            else if (label == "lw"){
                registers[registerMap[r1]] = result;
                registerLock[r1] = 0;
                // std::cout << "writing in memory register for " << label << '\n';
            }
            else{
                // std::cout << "doing nothing" << '\n';
            }
        }
    }
    bool checkDependency(std::vector<std::string>command,std::string depend){
        if(command[0]=="NOP"|| command[0]=="empty"){
            return false;
        }
        std::string opcode = command[0];
        std::string check = command[1];
        if (search(operands,opcode) || opcode =="lw"){
            if(is_number(depend)){
                return false;
            }
            if (registerMap[check]==registerMap[depend]){
                return true;
            }else{
                return false;
            }
        }else{
            return false;
        }
    }
    void executeCommandsPipelined(){
        int clockCycle = 0;
        pipes.insert(pipes.begin(), commands[0]);
        pipes.pop_back();
         printRegisters(0);
        std::cout<<0<<'\n';
        while (!(( pipes[0][0] == "empty" | pipes[0][0] == "NOP" )  &&  ( pipes[1][0] == "empty" | pipes[1][0] == "NOP" )  && ( pipes[2][0] == "empty" | pipes[2][0] == "NOP" )  && ( pipes[3][0] == "empty" | pipes[3][0] == "NOP" )  && ( pipes[4][0] == "empty" | pipes[4][0] == "NOP" ) ) ){
            ++clockCycle;
            // std::cout << "clock cycle " << clockCycle << '\n';
            std::vector<std::string> command;
            std::string labeling1,labeling2;
            if (PCcurr < commands.size()){
                command = commands[PCcurr];
            }
            else{
                command.push_back("empty");
            }
            std::string label = command[0];
            if (pipes[4][0] != "NOP" && pipes[4][0] != "empty"){
                WB(latch4[0], latch4[1], latch4[4], 0);
            }else{
                WB(latch4[0], latch4[1], latch4[4], 1);
            }
            std::vector<std::string> ID_command = pipes[1];
            pipes.pop_back();
            if (ID_command[0] != "NOP" && ID_command[0] != "empty"){
                std::string opcode = ID_command[0];
                std::vector<std::string> EX_command,MEM_command;
                EX_command = pipes[2],MEM_command = pipes[3];
                std::string depend1,depend2,depend;
                if(!(opcode=="j")){
                    if(opcode == "lw" || opcode=="sw"){
                        int lapren = ID_command[2].find("(");
                        depend = ID_command[2].substr(lapren+1,ID_command[2].size()-lapren-2);
                    }else if (opcode=="bne"||opcode=="beq"){
                        depend1 = ID_command[1],depend2 = ID_command[2];
                    }else{
                        depend1 = ID_command[2],depend2 = ID_command[3];
                    }
                    if(opcode =="lw" || opcode=="sw"){
                        bypass1=0,bypass3=0;
                        if (!(checkDependency(EX_command,depend)||checkDependency(MEM_command,depend))){
                            bypass2=0,bypass4=0;
                        }else{
                            if(checkDependency(EX_command,depend)){
                                bypass2 =1 , bypass4=0;
                            }else if(checkDependency(MEM_command,depend)) {
                                bypass2 =0 , bypass4=1;
                            }else{
                                bypass2 =0 , bypass4=0;
                            }
                            if((checkDependency(EX_command,depend)&&EX_command[0]=="lw")){
                                bypass1=0,bypass2=0,bypass3=0,bypass4=0,stall=1;
                            }else{
                                stall=0;
                            }
                        }
                    }else{
                        if (!(checkDependency(EX_command,depend1)||checkDependency(EX_command,depend2)||checkDependency(MEM_command,depend1)||checkDependency(MEM_command,depend2))){

                            bypass1=0,bypass2=0,bypass3=0,bypass4=0,stall=0;
                        }else{
                            if(checkDependency(EX_command,depend1)){
                                bypass1 =1 , bypass3=0;
                                
                            }else if(checkDependency(MEM_command,depend1)) {
                                bypass1 =0 , bypass3=1;
                            }else{
                                bypass1 =0 , bypass3=0;
                            }
                            if(checkDependency(EX_command,depend2)){
                                bypass2 =1 , bypass4=0;
                                
                            }else if(checkDependency(MEM_command,depend2)){
                                bypass2 =0 , bypass4=1;
                            }else{
                                bypass2 =0 , bypass4=0;
                            }
                            if((checkDependency(EX_command,depend1)&&EX_command[0]=="lw")||(checkDependency(EX_command,depend2)&&EX_command[0]=="lw")){
                                bypass1=0,bypass2=0,bypass3=0,bypass4=0,stall=1;
                            }else{
                                stall=0;
                            }
                        }
                    }
                }else{
                    bypass1=0,bypass2=0,bypass3=0,bypass4=0,stall=0; 
                }
            }else{
                bypass1=0,bypass2=0,bypass3=0,bypass4=0,stall=0;
            }
            if (stall == 0){
                if(label == "j"){
                    std::vector<std::string> IF_c, ID_c, EX_c, MEM_c;
                    IF_c = pipes[0];
                    if (IF_c[0] == "NOP"){
                        PCcurr = address[command[1]];
                    }else{
                        PCcurr = PCcurr;
                    }
                    // for MEM stage
                    MEM_c = pipes[3];
                    int result4;
                    if (MEM_c[0] == "NOP" || MEM_c[0] == "empty" || MEM_c[0]=="j"){
                        result4 = MEM(latch3[0], latch3[1], latch3[4], 1);
                    }else{
                        result4 = MEM(latch3[0], latch3[1], latch3[4], 0);
                        latch4[0] = MEM_c[0];
                        latch4[1] = MEM_c[1];
                        latch4[2] = MEM_c[2];
                        latch4[3] = MEM_c[3];
                        latch4[4] = std::to_string(result4);
                        bypass_ID_MEM = latch4[4];
                        labeling1 = pipes[3][0];
                        labeling2 = latch3[4];
                    }
                    // for EX stage
                    EX_c = pipes[2];
                    int result3;
                    if (EX_c[0] == "NOP" || EX_c[0] == "empty"||EX_c[0]=="j"){
                        result3 = EX(latch2[0], latch2[2], latch2[3], 1);
                    }else{
                        result3 = EX(latch2[0], latch2[2], latch2[3], 0);
                        latch3[0] = EX_c[0];
                        latch3[1] = EX_c[1];
                        latch3[2] = EX_c[2];
                        latch3[3] = EX_c[3];
                        latch3[4] = std::to_string(result3);
                        bypass_ID_EX=latch3[4];
                    }
                    // for ID stage
                    ID_c = pipes[1];
                    if (ID_c[0] == "NOP" || ID_c[0] == "empty" || ID_c[0] == "j"){
                        ID(ID_c[0], 1);
                    }else{
                        if(ID_c[0]=="lw" || ID_c[0]=="sw"){
                            int lapren = ID_command[2].find("(");
                            std::string depend = ID_command[2].substr(0,lapren);
                            if (bypass2==1){
                                std::string depend = ID_command[2].substr(0,lapren);
                                std::string offset = depend+"("+bypass_ID_EX+")";
                                ID_c[2]=offset;
                            }else if(bypass4==1){
                                std::string depend = ID_command[2].substr(0,lapren);
                                std::string offset = depend+"("+bypass_ID_MEM+")";
                                ID_c[2]=offset;
                            }
                        }else{
                            if (bypass1==1){
                                ID_c[2]=bypass_ID_EX;
                            }else if(bypass3==1){
                                ID_c[2]=bypass_ID_MEM;
                            }
                            if(bypass2==1){
                                ID_c[3]=bypass_ID_EX;
                            }else if(bypass4==1){
                                ID_c[3]=bypass_ID_MEM;
                            }
                        }
                        ID(ID_c[0], 0),latch2[0] = ID_c[0],latch2[1] = ID_c[1];
                        latch2[2] = ID_c[2],latch2[3] = ID_c[3],latch2[4] = "NULL";
                    }
                    // for IF stage
                    IF_c = pipes[0];
                    if (IF_c[0] == "NOP" || IF_c[0] == "empty" || IF_c[0]=="j"){
                        IF(IF_c[0], 1);
                    }else{
                        IF(IF_c[0], 0);
                        latch1[0] = IF_c[0];
                        latch1[1] = IF_c[1];
                        latch1[2] = IF_c[2];
                        latch1[3] = IF_c[3];
                        latch1[4] = "NULL";
                    }if (IF_c[0] == "NOP"){
                        if (!(PCcurr == commands.size())){
                            pipes.insert(pipes.begin(), commands[PCcurr]);
                        }else{
                            pipes.insert(pipes.begin(), {"NOP"});
                        }
                    }else{
                        pipes.insert(pipes.begin(), {"NOP"});
                    }
                    printRegisters(clockCycle);
                
                }else if (label == "bne" || label == "beq" || label == "j"){
                    std::vector<std::string> IF_c, ID_c, EX_c, MEM_c;
                    IF_c = pipes[0], ID_c = pipes[1];
                    if (IF_c[0] == "NOP" && ID_c[0] == "NOP"){
                        std::string a=latch2[1],b=latch2[2];
                        if (branch(label, a, b)){
                            PCcurr = address[command[3]];
                        }else{
                            PCcurr = PCcurr + 1;
                        }
                    }else{
                        PCcurr = PCcurr;
                    }
                    MEM_c = pipes[3];
                    int result4;
                    if (MEM_c[0] == "NOP" || MEM_c[0] == "empty"){
                        result4 = MEM(latch3[0], latch3[1], latch3[4], 1);
                    }else{
                        result4 = MEM(latch3[0], latch3[1], latch3[4], 0);
                        latch4[0] = MEM_c[0];
                        latch4[1] = MEM_c[1];
                        latch4[2] = MEM_c[2];
                        latch4[3] = MEM_c[3];
                        latch4[4] = std::to_string(result4);
                        bypass_ID_MEM=latch4[4];
                        labeling1 = pipes[3][0];
                        labeling2 = latch3[4];
                    }
                    // for EX stage
                    EX_c = pipes[2];
                    int result3;
                    if (EX_c[0] == "NOP" || EX_c[0] == "empty"){
                        result3 = EX(latch2[0], latch2[2], latch2[3], 1);
                    }else{
                        result3 = EX(latch2[0], latch2[2], latch2[3], 0);
                        latch3[0] = EX_c[0];
                        latch3[1] = EX_c[1];
                        latch3[2] = EX_c[2];
                        latch3[3] = EX_c[3];
                        latch3[4] = std::to_string(result3);
                        bypass_ID_EX=latch3[4];
                    }
                    // for ID stage
                    ID_c = pipes[1];
                    if (ID_c[0] == "NOP" || ID_c[0] == "empty"){
                        ID(ID_c[0], 1);
                    }else{
                        if(ID_c[0]=="lw" || ID_c[0]=="sw"){
                            int lapren = ID_command[2].find("(");
                            std::string depend = ID_command[2].substr(0,lapren);
                            if (bypass2==1){
                                std::string depend = ID_command[2].substr(0,lapren);
                                std::string offset = depend+"("+bypass_ID_EX+")";
                                ID_c[2]=offset;
                            }else if(bypass4==1){
                                std::string depend = ID_command[2].substr(0,lapren);
                                std::string offset = depend+"("+bypass_ID_MEM+")";
                                ID_c[2]=offset;
                            }
                        }else if (ID_c[0]=="beq"||ID_c[0]=="bne"){
                            if (bypass1==1){
                                ID_c[1]=bypass_ID_EX;
                            }else if(bypass3==1){
                                ID_c[1]=bypass_ID_MEM;
                            }
                            if(bypass2==1){
                                ID_c[2]=bypass_ID_EX;
                            }else if(bypass4==1){
                                ID_c[2]=bypass_ID_MEM;
                            }
                        }else{
                            if (bypass1==1){
                                ID_c[2]=bypass_ID_EX;
                            }else if(bypass3==1){
                                ID_c[2]=bypass_ID_MEM;
                            }
                            if(bypass2==1){
                                ID_c[3]=bypass_ID_EX;
                            }else if(bypass4==1){
                                ID_c[3]=bypass_ID_MEM;
                            }
                        }
                        ID(ID_c[0], 0),latch2[0] = ID_c[0],latch2[1] = ID_c[1];
                        latch2[2] = ID_c[2],latch2[3] = ID_c[3],latch2[4] = "NULL";
                    }
                    // for IF stage
                    IF_c = pipes[0];
                    if (IF_c[0] == "NOP" || IF_c[0] == "empty"){
                        IF(IF_c[0], 1);
                    }else{
                        IF(IF_c[0], 0);
                        latch1[0] = IF_c[0];
                        latch1[1] = IF_c[1];
                        latch1[2] = IF_c[2];
                        latch1[3] = IF_c[3];
                        latch1[4] = "NULL";
                    }
                    if (IF_c[0] == "NOP" && ID_c[0] == "NOP"){
                        if (!(PCcurr == commands.size())){
                            pipes.insert(pipes.begin(), commands[PCcurr]);
                        }else{
                            pipes.insert(pipes.begin(), {"NOP"});
                        }
                    }else if (IF_c[0] == "NOP"){
                        pipes.insert(pipes.begin(), {"NOP"});
                    }else{
                        pipes.insert(pipes.begin(), {"NOP"});
                    }
                    printRegisters(clockCycle);
                }else{
                    // for MEM stage
                    std::vector<std::string> MEM_c = pipes[3];
                    int result4;
                    if (MEM_c[0] == "NOP" || MEM_c[0] == "empty"){
                        result4 = MEM(latch3[0], latch3[1], latch3[4], 1);
                    }else{
                        result4 = MEM(latch3[0], latch3[1], latch3[4], 0);
                        latch4[0] = MEM_c[0];
                        latch4[1] = MEM_c[1];
                        latch4[2] = MEM_c[2];
                        latch4[3] = MEM_c[3];
                        latch4[4] = std::to_string(result4);
                        bypass_ID_MEM=latch4[4];
                        labeling1 = pipes[3][0];
                        labeling2 = latch3[4];
                    }
                    // for EX stage
                    std::vector<std::string> EX_c = pipes[2];
                    int result3;
                    if (EX_c[0] == "NOP" || EX_c[0] == "empty"){
                        result3 = EX(latch2[0], latch2[2], latch2[3], 1);
                    }else{
                        result3 = EX(latch2[0], latch2[2], latch2[3], 0);
                        latch3[0] = EX_c[0];
                        latch3[1] = EX_c[1];
                        latch3[2] = EX_c[2];
                        latch3[3] = EX_c[3];
                        latch3[4] = std::to_string(result3);
                        bypass_ID_EX = latch3[4];
                    }
                    // for ID stage
                    std::vector<std::string> ID_c = pipes[1];
                    if (ID_c[0] == "NOP" || ID_c[0] == "empty"){
                        ID(ID_c[0], 1);
                    }else{
                        if(ID_c[0]=="lw" || ID_c[0]=="sw"){
                            int lapren = ID_command[2].find("(");
                            std::string depend = ID_command[2].substr(0,lapren);
                            if (bypass2==1){
                                std::string depend = ID_command[2].substr(0,lapren);
                                std::string offset = depend+"("+bypass_ID_EX+")";
                                ID_c[2]=offset;
                            }else if(bypass4==1){
                                std::string depend = ID_command[2].substr(0,lapren);
                                std::string offset = depend+"("+bypass_ID_MEM+")";
                                ID_c[2]=offset;
                            }
                        }else{
                            if (bypass1==1){
                                ID_c[2]=bypass_ID_EX;
                            }else if(bypass3==1){
                                ID_c[2]=bypass_ID_MEM;
                            }
                            if(bypass2==1){
                                ID_c[3]=bypass_ID_EX;
                            }else if(bypass4==1){
                                ID_c[3]=bypass_ID_MEM;
                            }
                        }
                        ID(ID_c[0], 0),latch2[0] = ID_c[0],latch2[1] = ID_c[1];
                        latch2[2] = ID_c[2],latch2[3] = ID_c[3],latch2[4] = "NULL";
                    }
                    // for IF stage
                    std::vector<std::string> IF_c = pipes[0];
                    if (IF_c[0] == "NOP" || IF_c[0] == "empty"){
                        IF(IF_c[0], 1);
                    }else{
                        IF(IF_c[0], 0);
                        latch1[0] = IF_c[0];
                        latch1[1] = IF_c[1];
                        latch1[2] = IF_c[2];
                        latch1[3] = IF_c[3];
                        latch1[4] = "NULL";
                    }
                    if (PCcurr + 1 == commands.size()){
                        pipes.insert(pipes.begin(), {"empty"});
                        printRegisters(clockCycle);
                    }else if(PCcurr==commands.size()){
                        pipes.insert(pipes.begin(),{"empty"});
                        printRegisters(clockCycle);

                    }else{
                        PCcurr += 1;
                        pipes.insert(pipes.begin(), commands[PCcurr]);
                        printRegisters(clockCycle);
                    }
                }
            }else{
                std::vector<std::string> IF_c,ID_c,EX_c, MEM_c;
                MEM_c = pipes[3];
                int result4;
                if (MEM_c[0] == "NOP" || MEM_c[0] == "empty"){
                    result4 = MEM(latch3[0], latch3[1], latch3[4], 1);
                }else{
                    result4 = MEM(latch3[0], latch3[1], latch3[4], 0);
                    latch4[0] = MEM_c[0];
                    latch4[1] = MEM_c[1];
                    latch4[2] = MEM_c[2];
                    latch4[3] = MEM_c[3];
                    latch4[4] = std::to_string(result4);
                    bypass_ID_MEM=latch4[4];
                    labeling1 = pipes[3][0];
                    labeling2 = latch3[4];
                }
                // for EX stage
                EX_c = pipes[2];
                int result3;
                if (EX_c[0] == "NOP" || EX_c[0] == "empty"){
                    result3 = EX(latch2[0], latch2[2], latch2[3], 1);
                }else{
                    result3 = EX(latch2[0], latch2[2], latch2[3], 0);
                    latch3[0] = EX_c[0];
                    latch3[1] = EX_c[1];
                    latch3[2] = EX_c[2];
                    latch3[3] = EX_c[3];
                    latch3[4] = std::to_string(result3);
                    bypass_ID_EX=latch3[4];
                }
                // for ID stage
                ID_c = pipes[1];
                if (ID_c[0] == "NOP" || ID_c[0] == "empty"){
                    ID(ID_c[0], 1);
                }else{
                    // std::cout<<"pipeline is stalled"<<"\n";
                }
                // for IF stage
                IF_c = pipes[0];
                if (IF_c[0] == "NOP" || IF_c[0] == "empty"){
                    IF(IF_c[0], 1);
                }else{
                    // std::cout<<"pipeline is stalled"<<"\n";
                }
                pipes.insert(pipes.begin() + 2, {"empty"});
                printRegisters(clockCycle);
            }
                // std::cout<<bypass1<<bypass2<<bypass3<<bypass4<<'\n';

            if (labeling1=="sw"){
                std::cout<<1<<" "<<labeling2<<" "<<data[stoi(labeling2)]<<'\n';
            }else{
                std::cout<<0<<'\n';
            }
        }

    }
    void printRegisters(int clockCycle){
        //std::cout << "Cycle number: " << clockCycle << '\n';
        for (int i = 0; i < 32; ++i)
            std::cout << registers[i] << ' ';
        std::cout << std::dec << '\n';
    }
};
#endif