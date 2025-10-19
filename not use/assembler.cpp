#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cctype>
using namespace std;

// ประกาศตัวแปร
const string R_type[] = {"add", "nand"};
const string I_type[] = {"lw", "sw", "beq"};
const string J_type[] = {"jalr"};
const string O_type[] = {"halt", "noop"};
const int R_size = 2, I_size = 3, J_size = 1, O_size = 2;

// โครงสร้างเก็บ label
struct Symbol {
    string name;
    int address;
};
Symbol symbolTable[100];
int symbolCount = 0;

int getOpcode(const string& instr) {
    if (instr == "add") return 0;
    if (instr == "nand") return 1;
    if (instr == "lw") return 2;
    if (instr == "sw") return 3;
    if (instr == "beq") return 4;
    if (instr == "jalr") return 5;
    if (instr == "halt") return 6;
    if (instr == "noop") return 7;
    if (instr == ".fill") return -1;
    cerr << "Error: unknown instruction " << instr << "\n";
    exit(1);
}

bool isInList(const string& s, const string list[], int size) {
    for (int i = 0; i < size; i++)
        if (s == list[i]) return true;
    return false;
}

bool isNum(const string& s) {
    if (s.empty()) return false;
    size_t start = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    for (size_t i = start; i < s.size(); i++)
        if (!isdigit(s[i])) return false;
    return true;
}

bool isValidLabel(const string& label) {
    if (label.empty() || label.size() > 6) return false;
    if (!isalpha(label[0])) return false;
    for (char c : label)
        if (!isalnum(c)) return false;
    return true;
}

bool labelExists(const string& label) {
    for (int i = 0; i < symbolCount; i++)
        if (symbolTable[i].name == label) return true;
    return false;
}

int getAddress(const string& label) {
    for (int i = 0; i < symbolCount; i++)
        if (symbolTable[i].name == label) return symbolTable[i].address;
    cerr << "Error: undefined label '" << label << "'\n";
    exit(1);
}

void parseLine(const string& line, string& label, string& instr, string& f0, string& f1, string& f2) {
    label = instr = f0 = f1 = f2 = "";
    stringstream ss(line);
    string token;
    ss >> token;

    if (isalpha(token[0]) && !isInList(token, R_type, R_size) && 
        !isInList(token, I_type, I_size) && 
        !isInList(token, J_type, J_size) && 
        !isInList(token, O_type, O_size) && token != ".fill") {
        label = token;
        ss >> instr;
    } else instr = token;

    ss >> f0 >> f1 >> f2;
}

void assembler(const string& filename) {
    ifstream file(filename);
    if (!file) {
        cerr << "Error: Cannot open file\n";
        exit(1);
    }

    string lines[200];
    int lineCount = 0;
    string line;
    while (getline(file, line)) lines[lineCount++] = line;

    // Pass 1: Build symbol table
    for (int address = 0; address < lineCount; address++) {
        string label, instr, f0, f1, f2;
        parseLine(lines[address], label, instr, f0, f1, f2);
        if (!label.empty()) {
            if (!isValidLabel(label)) {
                cerr << "Error: Invalid label '" << label << "'\n";
                exit(1);
            }
            if (labelExists(label)) {
                cerr << "Error: Duplicate label '" << label << "'\n";
                exit(1);
            }
            symbolTable[symbolCount++] = {label, address};
        }
    }

    // Pass 2: Encode machine code
    for (int address = 0; address < lineCount; address++) {
        string label, instr, f0, f1, f2;
        parseLine(lines[address], label, instr, f0, f1, f2);
        int opcode = getOpcode(instr);
        int machineCode = 0;

        if (instr == ".fill") {
            if (isNum(f0)) machineCode = stoi(f0);
            else machineCode = getAddress(f0);
        } 
        else if (isInList(instr, R_type, R_size)) {
            machineCode = (opcode << 22) | (stoi(f0) << 19) | (stoi(f1) << 16) | stoi(f2);
        } 
        else if (isInList(instr, I_type, I_size)) {
            int offset = 0;
            if (isNum(f2)) offset = stoi(f2);
            else offset = (instr == "beq") ? getAddress(f2) - (address + 1) : getAddress(f2);
            if (offset < -32768 || offset > 32767) {
                cerr << "Error: offset out of range at line " << address << "\n";
                exit(1);
            }
            machineCode = (opcode << 22) | (stoi(f0) << 19) | (stoi(f1) << 16) | (offset & 0xFFFF);
        } 
        else if (isInList(instr, J_type, J_size)) {
            machineCode = (opcode << 22) | (stoi(f0) << 19) | (stoi(f1) << 16);
        } 
        else if (isInList(instr, O_type, O_size)) {
            machineCode = (opcode << 22);
        }

        cout << machineCode << endl;
    }
}

int main() {
    assembler("assembler_test.txt");
    return 0;
}
