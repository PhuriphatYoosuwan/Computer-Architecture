#include <iostream>
#include <fstream>
#include <vector>
#include <string>
using namespace std;

const int NUMREGS = 8;      // จำนวน register
const int MEMSIZE = 65536;  // ขนาด memory (พอสำหรับทุกกรณี)

// -----------------------------
// แปลง offset 16-bit -> 32-bit (sign-extend)
// -----------------------------
int convertNum(int num) {
    if (num & (1 << 15)) {  // ถ้า bit ที่ 15 เป็น 1 => เป็นลบ
        num -= (1 << 16);
    }
    return num;
}

// -----------------------------
// แสดงสถานะของเครื่อง (pc, register, memory)
// -----------------------------
void printState(int pc, const int reg[], const vector<int>& mem) {
    cout << "\n@@@\nstate:\n";
    cout << "\tpc " << pc << "\n";
    cout << "\tmemory:\n";
    for (int i = 0; i < mem.size(); i++) {
        cout << "\t\tmem[" << i << "] " << mem[i] << "\n";
    }
    cout << "\tregisters:\n";
    for (int i = 0; i < NUMREGS; i++) {
        cout << "\t\treg[" << i << "] " << reg[i] << "\n";
    }
    cout << "end state\n";
}

// -----------------------------
// ส่วนหลักของ Simulator
// -----------------------------
void simulator(const string& filename) {
    vector<int> mem;
    int reg[NUMREGS] = {0}; // initialize registers เป็นศูนย์
    int pc = 0;

    // --- อ่าน machine code จากไฟล์ ---
    ifstream fin(filename);
    if (!fin) {
        cerr << "Cannot open file: " << filename << "\n";
        return;
    }

    int value;
    while (fin >> value) {
        mem.push_back(value);
    }

    cout << "Machine code loaded: " << mem.size() << " words\n";

    // --- เริ่มจำลอง ---
    bool halted = false;
    int numInstructions = 0;

    while (!halted) {
        printState(pc, reg, mem);

        int instr = mem[pc];
        int opcode = (instr >> 22) & 0x7;
        int regA   = (instr >> 19) & 0x7;
        int regB   = (instr >> 16) & 0x7;
        int dest   = instr & 0x7;
        int offset = convertNum(instr & 0xFFFF); // ใช้กับ I-type

        switch (opcode) {
            case 0: // add
                reg[dest] = reg[regA] + reg[regB];
                pc++;
                break;

            case 1: // nand
                reg[dest] = ~(reg[regA] & reg[regB]);
                pc++;
                break;

            case 2: // lw
                reg[regB] = mem[reg[regA] + offset];
                pc++;
                break;

            case 3: // sw
                mem[reg[regA] + offset] = reg[regB];
                pc++;
                break;

            case 4: // beq
                if (reg[regA] == reg[regB])
                    pc = pc + 1 + offset;
                else
                    pc++;
                break;

            case 5: // jalr
                reg[regB] = pc + 1;
                pc = reg[regA];
                break;

            case 6: // halt
                halted = true;
                pc++;
                cout << "machine halted\n";
                cout << "total of " << numInstructions + 1 << " instructions executed\n";
                cout << "final state:\n";
                printState(pc, reg, mem);
                break;

            case 7: // noop
                pc++;
                break;

            default:
                cerr << "Unknown opcode: " << opcode << endl;
                halted = true;
                break;
        }

        numInstructions++;
    }
}

// -----------------------------
// main program
// -----------------------------
int main() {
    simulator("machine_code_1.txt");
    return 0;
}