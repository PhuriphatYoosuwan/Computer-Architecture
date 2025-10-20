#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <cstdlib>
#include <algorithm>
#include <vector>
using namespace std;

// โครงสร้างข้อมูลสำหรับ 1 บรรทัด assembly
struct Instruction {
    string label, opcode, arg0, arg1, arg2;
};


// ฟังก์ชันไว้ตรวจสอบว่า string เป็นตัวเลขทั้งหมดหรือไม่
// ใช้เพื่อแยกว่า argument เป็น immediate หรือ label
bool isNumber(const string &s) {
    if (s.empty()) return false;
    char *p;
    strtol(s.c_str(), &p, 10);
    return (*p == 0);
}

// ฟังก์ชันขยายค่าจำนวนเต็มจาก 16 บิต ให้เป็น 32 บิต
// ใช้ในกรณีที่ค่าตัวเลขเป็นลบ เพื่อให้ผลยังคงถูกต้องหลังขยายบิต
int convertNum(int num) {
    if (num & (1 << 15)) num -= (1 << 16);
    return num;
}

// ฟังก์ชันสำหรับอ่านและแยกคำสั่ง Assembly ทีละบรรทัด ทำหน้าที่:
// 1.อ่านข้อความจากไฟล์ 1 บรรทัด
// 2.ตัดส่วนที่เป็นคอมเมนต์ออก (หลังเครื่องหมาย ';')
// 3.แยกข้อความออกเป็นคำ ๆ (label, opcode, และ argument ต่าง ๆ)
// 4.เก็บผลลัพธ์ลงในโครงสร้าง Instruction
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

// ฟังก์ชันหลักของโปรแกรม Assembler
// ทำหน้าที่แปลงไฟล์ Assembly ให้เป็น Machine Code
// โดยใช้กระบวนการ 2 รอบ (2-pass):
// Pass 1: อ่านไฟล์เพื่อเก็บตำแหน่งของ label แต่ละตัว
// Pass 2: แปลงคำสั่งทั้งหมดเป็นตัวเลข 32 บิต แล้วเขียนลงไฟล์ผลลัพธ์
void assembler(const string &inputFile, const string &outputFile) {
    // เปิดไฟล์ Assembly ที่จะอ่านข้อมูลเข้า (inputFile)
    ifstream inFile(inputFile);
        if (!inFile.is_open()) {                // ถ้าเปิดไฟล์ไม่ได้
            cerr << "error opening " << inputFile << endl;  // แสดงข้อความผิดพลาด
            exit(1);                            // และหยุดการทำงานทันที
        }

    // เปิดไฟล์ผลลัพธ์สำหรับเขียน Machine Code (outputFile)
    ofstream outFile(outputFile);
        if (!outFile.is_open()) {               // ถ้าเปิดไฟล์ไม่ได้
            cerr << "error opening " << outputFile << endl;
            exit(1);
        }

    // สร้างตัวแปรที่ใช้ภายใน assembler
    map<string, int> symbolTable;           // ตารางเก็บชื่อ label และตำแหน่ง address ของมัน
    vector<Instruction> instructions;       // เก็บคำสั่ง Assembly ทั้งหมดในรูปแบบที่แยกส่วนแล้ว
    Instruction inst;                       // ตัวแปรชั่วคราวไว้ใช้ตอนอ่านแต่ละบรรทัด
    int address = 0;                        // ตัวนับตำแหน่งคำสั่ง (เริ่มจากบรรทัด 0)


        // PASS 1 : สร้างตาราง symbol table
        // ขั้นตอนนี้จะอ่านไฟล์ Assembly ทีละบรรทัด
        // เพื่อเก็บชื่อ label และตำแหน่งบรรทัด (address) ของแต่ละคำสั่ง
        // ข้อมูลเหล่านี้จะถูกนำไปใช้ใน PASS 2 ตอนแปลงเป็น machine code
        while (readAndParse(inFile, inst)) {
            // ถ้าบรรทัดนี้มี label อยู่ข้างหน้า (เช่น "loop add 1 2 3")
            if (!inst.label.empty()) {
                // ตรวจว่ามี label นี้อยู่ในตารางแล้วหรือยัง
                // ถ้ามีซ้ำ → แสดง error แล้วหยุดการทำงาน
                if (symbolTable.count(inst.label)) {
                    cerr << "error: duplicate label " << inst.label << endl;
                    exit(1);
                }
                // ถ้าไม่ซ้ำ → บันทึก label และตำแหน่งปัจจุบันลง symbol table
                symbolTable[inst.label] = address; // เช่น "loop" → 3
            }
            // เก็บคำสั่งทั้งหมด (รวม label, opcode, arg) ลงใน vector instructions
            instructions.push_back(inst);

            // เพิ่ม address ทีละ 1 (นับจำนวนบรรทัดของคำสั่ง)
            address++;
        }

    
    // PASS 2 : แปลงแต่ละคำสั่งเป็น machine code
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


// main function : เรียก assembler
int main() {
    // เปลี่ยนชื่อไฟล์ตามที่ต้องการรัน
    assembler("assembly/Multiplication.txt", "machine_code/machine_code.txt");
    return 0;
}
