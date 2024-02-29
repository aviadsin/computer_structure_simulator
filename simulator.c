#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>




int cycle;
int PC; //program counter
uint32_t registers[16];
uint32_t deviceRegisters[23];
uint64_t instructions[4096];
uint32_t memory[4096];
uint32_t LEDS;
uint8_t monitor[256][256];
FILE* traceFile;
FILE* hwRegTraceFile;
FILE* ledFile;

// FILE* sevenSegFile;


// TODO: write a function for each opcode - Aviad
// TODO: LED, Timer, Monitor - nagia ahar kach
// TODO: traceFile and hwRegTraceFile - Ben



//ATTENTION: i do not know if the PC should be incremented in simCycle after a PC related Cmd
//ATTENTION: i did not create a halCmd() functon because i think this should just return the simCycle() function. 


int init()
{
    return 0;
}

int simClockCycle()
{
    return 0;
}

int exit()
{
    // handle file updates
    return 0;
}


int main(int argc, char const *argv[])
{
    init();
    simClockCycle();
    exit();
    return 0;
}

int addCmd(int rd, int rs,int rt, int rm){
    if(rd<0||rd>=16){
        return 0;
    }
    if(rd<=2){
        return 1;
    }
    registers[rd] = registers[rs]+registers[rt]+registers[rm];
    return 1;
}

int subCmd(int rd, int rs,int rt, int rm){
    if(rd<0||rd>=16){
        return 0;
    }
    if(rd<=2){
        return 1;
    }
    registers[rd] = registers[rs]-registers[rt]-registers[rm];
    return 1;
}

int macCmd(int rd, int rs,int rt, int rm){
    if(rd<0||rd>=16){
        return 0;
    }
    if(rd<=2){
        return 1;
    }
    registers[rd] = registers[rs]*registers[rt]+registers[rm];
    return 1;
}


int andCmd(int rd, int rs,int rt, int rm){
    if(rd<0||rd>=16){
        return 0;
    }
    if(rd<=2){
        return 1;
    }
    registers[rd] = registers[rs]&registers[rt]&registers[rm];
    return 1;
}

int xorCmd(int rd, int rs,int rt, int rm){
    if(rd<0||rd>=16){
        return 0;
    }
    if(rd<=2){
        return 1;
    }
    registers[rd] = registers[rs] ^ registers[rt] ^ registers[rm];
    return 1;
}

int sllCmd(int rd, int rs,int rt){
    if(rd<0||rd>=16){
        return 0;
    }
    if(rd<=2){
        return 1;
    }
    registers[rd] = registers[rs] << registers[rt];
    return 1;
}

int sraCmd(int rd, int rs,int rt){
    uint32_t result;
    int i;
    if(rd<0||rd>=16){
        return 0;
    }
    if(rd<=2){
        return 1;
    }
    result = registers[rs] >> registers[rt];
    if(registers[rs]&(1<<31)!=0){
        for(i=0;i<registers[rt];i++){
            result = result | (1<<(31-i));
        }
    }
    registers[rd] = result;
    return 1;
}

int srlCmd(int rd, int rs,int rt){
    if(rd<0||rd>=16){
        return 0;
    }
    if(rd<=2){
        return 1;
    }
    registers[rd] = registers[rs] >> registers[rt];
    return 1;
}

int beqCmd(int rs, int rt,int rm){ //////// i don't know if we should c=increment the value of PC by 1 in simCycle() after this function is called
    if(registers[rs]==registers[rt]){
        PC = (registers[rm]<<21)>>21;
    }
    return 1;
}

int bneCmd(int rs, int rt,int rm){ //////// i don't know if we should c=increment the value of PC by 1 in simCycle() after this function is called
    if(registers[rs]!=registers[rt]){
        PC = (registers[rm]<<21)>>21;
    }
    return 1;
}

int bltCmd(int rs, int rt,int rm){ //////// i don't know if we should c=increment the value of PC by 1 in simCycle() after this function is called
    if(registers[rs]<registers[rt]){
        PC = (registers[rm]<<21)>>21;
    }
    return 1;
}

int bgtCmd(int rs, int rt,int rm){ //////// i don't know if we should c=increment the value of PC by 1 in simCycle() after this function is called
    if(registers[rs]>registers[rt]){
        PC = (registers[rm]<<21)>>21;
    }
    return 1;
}

int bleCmd(int rs, int rt,int rm){ //////// i don't know if we should c=increment the value of PC by 1 in simCycle() after this function is called
    if(registers[rs]<=registers[rt]){
        PC = (registers[rm]<<21)>>21;
    }
    return 1;
}

int bgeCmd(int rs, int rt,int rm){ //////// i don't know if we should c=increment the value of PC by 1 in simCycle() after this function is called
    if(registers[rs]<=registers[rt]){
        PC = (registers[rm]<<21)>>21;
    }
    return 1;
}

int jalCmd(int rd,int rm){ //////// i don't know if we should c=increment the value of PC by 1 in simCycle() after this function is called
    if(rd<0||rd>=16){
        return 0;
    }
    if(rd<=2){
        return 1;
    }
    registers[rd] = (uint32_t)(PC+1);
    PC = (registers[rm]<<21)>>21;
    return 1;
}

int lwCmd(int rd, int rs,int rt, int rm){ 
    if(rd<0||rd>=16){
        return 0;
    }
    if(rd<=2){
        return 1;
    }
    registers[rd] = memory[registers[rs]+registers[rt]]+registers[rm];
    return 1;
}

int swCmd(int rd, int rs,int rt, int rm){ 
    if(rd<0||rd>=16){
        return 0;
    }
    if(rd<=2){
        return 1;
    }
    memory[registers[rs]+registers[rt]] = registers[rm] + registers[rd];
    return 1;
}

int retiCmd(){ //////// i don't know if we should c=increment the value of PC by 1 in simCycle() after this function is called
    PC = deviceRegisters[7];
    return 1;
}

int inCmd(int rd, int rs,int rt){ 
    if(rd<0||rd>=16){
        return 0;
    }
    if(rd<=2){
        return 1;
    }
    registers[rd] = deviceRegisters[registers[rs]+registers[rt]];
    return 1;
}

int outCmd(int rs,int rt, int rm){ 
    deviceRegisters[registers[rs]+registers[rt]] = registers[rm];
    return 1;
}