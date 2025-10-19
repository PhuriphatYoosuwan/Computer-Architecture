#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
using namespace std;

const int NUMMEMORY = 65536;
const int NUMREGS = 8;

struct State {
    int pc;
    vector<int> mem;
    vector<int> reg;
};

void printState(const State &state) {
    cout << "\n@@@\nstate:\n";
    cout << "\tpc " << state.pc << "\n";
    cout << "\tmemory:\n";
    for (size_t i = 0; i < state.mem.size(); i++)
        cout << "\t\tmem[ " << i << " ] " << state.mem[i] << "\n";
    cout << "\tregisters:\n";
    for (int i = 0; i < NUMREGS; i++)
        cout << "\t\treg[ " << i << " ] " << state.reg[i] << "\n";
    cout << "end state\n";
}

int convertNum(int num) {
    if (num & (1 << 15))
        num -= (1 << 16);
    return num;
}

int simulator(const string &filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "error: can't open file " << filename << endl;
        return 1;
    }

    State state;
    state.pc = 0;
    state.reg = vector<int>(NUMREGS, 0);
    state.mem.clear();

    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        state.mem.push_back(stoi(line));
        // cout << "memory[" << state.mem.size() - 1 << "]=" << state.mem.back() << endl;
    }

    int instrCount = 0;

    while (true) {
        // printState(state);

        instrCount++;

        if (state.pc < 0 || state.pc >= (int)state.mem.size()) {
            cerr << "error: pc out of bounds" << endl;
            return 1;
        }

        int instr = state.mem[state.pc];
        int opcode = (instr >> 22) & 0x7;
        int regA = (instr >> 19) & 0x7;
        int regB = (instr >> 16) & 0x7;
        int offset = convertNum(instr & 0xFFFF);

        switch (opcode) {
            case 0: // add
                state.reg[instr & 0x7] = state.reg[regA] + state.reg[regB];
                state.pc++;
                break;
            case 1: // nand
                state.reg[instr & 0x7] = ~(state.reg[regA] & state.reg[regB]);
                state.pc++;
                break;
            case 2: // lw
                state.reg[regB] = state.mem[state.reg[regA] + offset];
                state.pc++;
                break;
            case 3: // sw
                state.mem[state.reg[regA] + offset] = state.reg[regB];
                state.pc++;
                break;
            case 4: // beq
                if (state.reg[regA] == state.reg[regB])
                    state.pc = state.pc + 1 + offset;
                else
                    state.pc++;
                break;
            case 5: { // jalr
                int temp = state.pc + 1;
                state.pc = state.reg[regA];
                state.reg[regB] = temp;
                break;
            }
            case 6: // halt
                cout << "machine halted\n";
                cout << "total of " << instrCount << " instructions executed\n";
                cout << "final state of machine:\n";
                printState(state);
                return 0;
            case 7: // noop
                state.pc++;
                break;
            default:
                cerr << "error: invalid opcode " << opcode << endl;
                return 1;
        }
    }
}

int main() {
    simulator("machine_code/machine_code.txt");
    return 0;
}
