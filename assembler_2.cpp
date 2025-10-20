#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <cstdlib>
#include <algorithm>
#include <vector>
using namespace std;

//-----------------------------
// โครงสร้างข้อมูลสำหรับ 1 บรรทัด assembly
//-----------------------------
struct Instruction {
    string label, opcode, arg0, arg1, arg2;
};

//-----------------------------
// ฟังก์ชันไว้ตรวจว่า string เป็นตัวเลขล้วนหรือไม่
// ใช้เพื่อแยกว่า argument เป็น immediate หรือ label
//-----------------------------
bool isNumber(const string &s) {
    if (s.empty()) return false;
    char *p;
    strtol(s.c_str(), &p, 10);
    return (*p == 0);
}

//-----------------------------
// ฟังก์ชัน sign-extend ค่าจาก 16 บิตเป็น 32 บิต
// (ไม่ได้ถูกใช้ในเวอร์ชันนี้ แต่เป็นยูทิลิตี้มาตรฐาน)
//-----------------------------
int convertNum(int num) {
    if (num & (1 << 15)) num -= (1 << 16);
    return num;
}

//-----------------------------
// ฟังก์ชันอ่านและแยก 1 บรรทัดจากไฟล์ Assembly
// - ตัดคอมเมนต์
// - แยกส่วน label, opcode, arg0–arg2
//-----------------------------
int readAndParse(ifstream &inFile, Instruction &inst) {
    string line;
    if (!getline(inFile, line)) return 0;   // end of file
    inst = {"", "", "", "", ""};            // เคลียร์ค่าเก่า

    // --- ลบคอมเมนต์ (เริ่มจาก ';') ---
    size_t pos = line.find(';');
    if (pos != string::npos) line = line.substr(0, pos);

    // --- แยก token ด้วยช่องว่าง ---
    stringstream ss(line);
    vector<string> parts;
    string temp;
    while (ss >> temp) parts.push_back(temp);

    if (parts.empty()) return 1; // บรรทัดว่าง → ข้ามได้

    // --- รายชื่อ opcode ที่รองรับ ---
    const vector<string> OPCODES = {
        "add", "nand", "lw", "sw", "beq", "jalr", "halt", "noop", ".fill"
    };

    // --- ถ้าคำแรกเป็น opcode → ไม่มี label ---
    if (find(OPCODES.begin(), OPCODES.end(), parts[0]) != OPCODES.end()) {
        inst.label = "";
        inst.opcode = parts[0];
        if (parts.size() > 1) inst.arg0 = parts[1];
        if (parts.size() > 2) inst.arg1 = parts[2];
        if (parts.size() > 3) inst.arg2 = parts[3];
    }
    // --- ถ้าไม่ใช่ opcode แสดงว่าคำแรกคือ label ---
    else {
        inst.label = parts[0];
        if (parts.size() > 1) inst.opcode = parts[1];
        if (parts.size() > 2) inst.arg0 = parts[2];
        if (parts.size() > 3) inst.arg1 = parts[3];
        if (parts.size() > 4) inst.arg2 = parts[4];
    }

    return 1;
}

//-----------------------------
// ฟังก์ชันหลัก: Assembler (2-pass assembler)
//-----------------------------
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

    // ตารางเก็บ label → address
    map<string, int> symbolTable;
    vector<Instruction> instructions;
    Instruction inst;
    int address = 0;

    //-----------------------------
    // PASS 1 : สร้าง symbol table
    //-----------------------------
    while (readAndParse(inFile, inst)) {
        // ถ้าพบบรรทัดที่มี label ให้เก็บตำแหน่ง address
        if (!inst.label.empty()) {
            if (symbolTable.count(inst.label)) {
                cerr << "error: duplicate label " << inst.label << endl;
                exit(1);
            }
            symbolTable[inst.label] = address; // map label → บรรทัดปัจจุบัน
        }

        instructions.push_back(inst);
        address++; // เพิ่ม address ทีละบรรทัด
    }

    //-----------------------------
    // PASS 2 : แปลงแต่ละคำสั่งเป็น machine code
    //-----------------------------
    for (int i = 0; i < (int)instructions.size(); i++) {
        inst = instructions[i];
        int machineCode = 0; // เก็บผลลัพธ์เลข 32 บิต

        //---------- R-type ----------
        if (inst.opcode == "add")
            machineCode = (0 << 22) | (stoi(inst.arg0) << 19)
                        | (stoi(inst.arg1) << 16) | stoi(inst.arg2);

        else if (inst.opcode == "nand")
            machineCode = (1 << 22) | (stoi(inst.arg0) << 19)
                        | (stoi(inst.arg1) << 16) | stoi(inst.arg2);

        //---------- I-type ----------
        else if (inst.opcode == "lw" || inst.opcode == "sw" || inst.opcode == "beq") {
            int opcodeNum = (inst.opcode == "lw") ? 2 :
                            (inst.opcode == "sw") ? 3 : 4;
            int offset;

            // ตรวจว่า arg2 เป็น immediate หรือ label
            if (isNumber(inst.arg2))
                offset = stoi(inst.arg2);
            else if (symbolTable.count(inst.arg2)) {
                // beq ใช้ PC-relative offset
                offset = symbolTable[inst.arg2] - ((inst.opcode == "beq") ? (i + 1) : 0);
            } else {
                cerr << "error: undefined label " << inst.arg2 << endl;
                exit(1);
            }

            // ตรวจช่วงของ offset (-32768 ถึง 32767)
            if (offset < -32768 || offset > 32767) {
                cerr << "error: offsetField out of range at line " << i << endl;
                exit(1);
            }

            offset &= 0xFFFF; // เก็บเฉพาะ 16 บิตล่าง

            machineCode = (opcodeNum << 22)
                        | (stoi(inst.arg0) << 19)
                        | (stoi(inst.arg1) << 16)
                        | offset;
        }

        //---------- J-type ----------
        else if (inst.opcode == "jalr")
            machineCode = (5 << 22)
                        | (stoi(inst.arg0) << 19)
                        | (stoi(inst.arg1) << 16);

        //---------- O-type ----------
        else if (inst.opcode == "halt")
            machineCode = (6 << 22);

        else if (inst.opcode == "noop")
            machineCode = (7 << 22);

        //---------- .fill directive ----------
        else if (inst.opcode == ".fill") {
            if (isNumber(inst.arg0))
                machineCode = stoi(inst.arg0);
            else if (symbolTable.count(inst.arg0))
                machineCode = symbolTable[inst.arg0];
            else {
                cerr << "error: undefined label in .fill " << inst.arg0 << endl;
                exit(1);
            }
        }

        //---------- ถ้าไม่รู้จัก opcode ----------
        else {
            cerr << "error: unrecognized opcode " << inst.opcode << endl;
            exit(1);
        }

        // เขียน machine code ลงไฟล์ output (หนึ่งบรรทัดต่อคำสั่ง)
        outFile << machineCode << endl;
    }

    // ปิดไฟล์ทั้งหมด
    inFile.close();
    outFile.close();

    // จบโปรแกรม
    exit(0);
}

//-----------------------------
// main function : เรียก assembler
//-----------------------------
int main() {
    // เปลี่ยนชื่อไฟล์ตามที่ต้องการรัน
    assembler("assembly/Multiplication.txt", "machine_code/machine_code.txt");
    return 0;
}
