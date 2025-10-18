#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <cstdlib>
#include <algorithm>
#include <vector>
using namespace std;

struct Instruction {
    string label, opcode, arg0, arg1, arg2;
};

bool isNumber(const string &s) {
    if (s.empty()) return false;
    char *p;
    strtol(s.c_str(), &p, 10);
    return (*p == 0);
}

int convertNum(int num) {
    if (num & (1 << 15)) num -= (1 << 16);
    return num;
}

int readAndParse(ifstream &inFile, Instruction &inst) {
    string line;
    if (!getline(inFile, line)) return 0;
    inst = {"", "", "", "", ""};

    // remove comment
    size_t pos = line.find(';');
    if (pos != string::npos) line = line.substr(0, pos);

    stringstream ss(line);
    vector<string> parts;
    string temp;
    while (ss >> temp) parts.push_back(temp);
    if (parts.empty()) return 1; // empty line

    const vector<string> OPCODES = {"add", "nand", "lw", "sw", "beq", "jalr", "halt", "noop", ".fill"};

    // ถ้าคำแรกเป็น opcode → ไม่มี label
    if (find(OPCODES.begin(), OPCODES.end(), parts[0]) != OPCODES.end()) {
        inst.label = "";
        inst.opcode = parts[0];
        if (parts.size() > 1) inst.arg0 = parts[1];
        if (parts.size() > 2) inst.arg1 = parts[2];
        if (parts.size() > 3) inst.arg2 = parts[3];
    } else { // มี label
        inst.label = parts[0];
        if (parts.size() > 1) inst.opcode = parts[1];
        if (parts.size() > 2) inst.arg0 = parts[2];
        if (parts.size() > 3) inst.arg1 = parts[3];
        if (parts.size() > 4) inst.arg2 = parts[4];
    }

    return 1;
}


void assembler(const string &inputFile, const string &outputFile) {
    ifstream inFile(inputFile);
    if (!inFile.is_open()) {
        cerr << "error opening " << inputFile << endl;
        exit(1);
    }
    ofstream outFile(outputFile);
    if (!outFile.is_open()) {
        cerr << "error opening " << outputFile << endl;
        exit(1);
    }

    map<string, int> symbolTable;
    vector<Instruction> instructions;
    Instruction inst;
    int address = 0;

    // PASS 1
    while (readAndParse(inFile, inst)) {
        if (!inst.label.empty()) {
            if (symbolTable.count(inst.label)) {
                cerr << "error: duplicate label " << inst.label << endl;
                exit(1);
            }
            symbolTable[inst.label] = address;
        }
        instructions.push_back(inst);
        address++;
    }

    // PASS 2
    for (int i = 0; i < (int)instructions.size(); i++) {
        inst = instructions[i];
        int machineCode = 0;

        if (inst.opcode == "add")
            machineCode = (0 << 22) | (stoi(inst.arg0) << 19) | (stoi(inst.arg1) << 16) | stoi(inst.arg2);
        else if (inst.opcode == "nand")
            machineCode = (1 << 22) | (stoi(inst.arg0) << 19) | (stoi(inst.arg1) << 16) | stoi(inst.arg2);
        else if (inst.opcode == "lw" || inst.opcode == "sw" || inst.opcode == "beq") {
            int opcodeNum = (inst.opcode == "lw") ? 2 : (inst.opcode == "sw") ? 3 : 4;
            int offset;
            if (isNumber(inst.arg2)) offset = stoi(inst.arg2);
            else if (symbolTable.count(inst.arg2)) {
                offset = symbolTable[inst.arg2] - ((inst.opcode == "beq") ? (i + 1) : 0);
            } else {
                cerr << "error: undefined label " << inst.arg2 << endl;
                exit(1);
            }
            if (offset < -32768 || offset > 32767) {
                cerr << "error: offsetField out of range at line " << i << endl;
                exit(1);
            }
            offset &= 0xFFFF;
            machineCode = (opcodeNum << 22) | (stoi(inst.arg0) << 19) | (stoi(inst.arg1) << 16) | offset;
        }
        else if (inst.opcode == "jalr")
            machineCode = (5 << 22) | (stoi(inst.arg0) << 19) | (stoi(inst.arg1) << 16);
        else if (inst.opcode == "halt")
            machineCode = (6 << 22);
        else if (inst.opcode == "noop")
            machineCode = (7 << 22);
        else if (inst.opcode == ".fill") {
            if (isNumber(inst.arg0)) machineCode = stoi(inst.arg0);
            else if (symbolTable.count(inst.arg0)) machineCode = symbolTable[inst.arg0];
            else {
                cerr << "error: undefined label in .fill " << inst.arg0 << endl;
                exit(1);
            }
        }
        else {
            cerr << "error: unrecognized opcode " << inst.opcode << endl;
            exit(1);
        }

        outFile << machineCode << endl;
    }

    inFile.close();
    outFile.close();
    exit(0);
}

int main() {
    assembler("assembly/Combination.txt", "machine_code/machine_code.txt");
    return 0;
}
