#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef struct LinkedList_int{
    struct LinkedList *next;
    int cycleVal;
}LinkedList_int;

struct LinkedList_int *irq2List;
int cycle;
int PC; //program counter
uint32_t registers[16];
uint32_t deviceRegisters[23];
uint64_t instructions[4096];
uint32_t memory[4096];
uint32_t LEDS;
uint8_t monitor[256][256];
uint32_t disk[16384];
FILE* traceFile;
FILE* hwRegTraceFile;
FILE* ledFile;
FILE* display7segFile;
const char* hwRegistersNames[] = {
    // Usage:
    // hwRegisterNames[register number] -> "regname"

    "irq0enable", "irq1enable", "irq2enable",
    "irq0status", "irq1status", "irq2status",
    "irqhandler", "irqreturn",
    "clks", "leds", "display7seg", 
    "timerenable", "timercurrent", "timermax",
    "diskcmd", "disksector", "diskbuffer", "diskstatus",
    "reserved", "monitoraddr", "monitordata", "monitorcmd"
};

//TODO: devices - timer , LED lights, Monitor, Disk Drive
//TODO: init 
//TODO: functions reading from input files - imemin.txt dmemin.txt diskin.txt irq2in.txt {$Aviad}

//TODO: symCycle
//TODO: create parse instruction function.

//TODO: byebye
//TODO: functions writing to output files - dmemout.txt 
//      regout.txt cycles.txt leds.txt display7seg.txt diskout.txt monitor.txtmonitor.yuv {$Ben}

// test: write a function for each opcode - Aviad
// test: traceFile and hwRegTraceFile - Ben



//ATTENTION1: i did not create a haltCmd() functon because i think this should just return the simCycle() function.
//ATTENTION2: i do not know if ther is a maximal clock cycle for irq2in.txt 
//           | i also do not know if to read the hold file at once in the beginning
//             or read it line by line as the program runs
//             i chose to read all of the memory at once
//ATTENTION3: i did not deal with diskin.txt 
//            becuase we did not design a memory structure for it
//ATTENTION4: all register need to be initialized to 0




int init()
{
    //TODO: put every variable to zero, call functions to read from input files,
    // open trace files - trace.txt hwregtrace.txt leds.txt display7seg.tx 
    /*
    put every variable to zero
    call functions to read from input files - readimemin more reads and shit
    open trace files - global shit
    
    */
    return 0;
}

int simClockCycle()
{
    //TODO: decide in which order we need to call the functions
    /*
    update_tracefile()
    timer
    interrupts
    call parse instruction function !!! - {$$ben}
    call opcode(if not jump)
    check and handle monitor/disk/LEDS/7seg - write7Seg
    hwtracefile() ,
    increment PC(when not jumping)
    halt - return -1 else 0 error 1
    */
    update_traceFile();
    
    //TODO: interrupts,handle trace files - update_tracefile() , call parse instruction function , call opcode(if not jump),
    //  check and handle monitor/disk ,
    // handle trace files - hwtracefile() , 
    //increment PC(when not jumping)
    return 0;
}

int byebye()
{
    //TODO: call the output file functions, close all files
    // handle file updates
    return 0;
}


int main(int argc, char const *argv[])
{
    init();
    while(simClockCycle()==0){
    }
    byebye();
    return 0;
}

int writeToMonitor(void){
    int line = deviceRegisters[20]/256;
    int index = deviceRegisters[20]%256;
    monitor[line][index] = deviceRegisters[21];
    deviceRegisters[22]=0;
    return 0;
}

int writeToLeds(FILE *ledsFileName){
    LEDS = deviceRegisters[9];
    fprintf(ledsFileName, "%d %u\n", cycle,LEDS);
    return 0;
}

int incrementTimer(void){
    deviceRegisters[12]+=1;
    if(deviceRegisters[12]==deviceRegisters[13]){
        deviceRegisters[12] =0;
        deviceRegisters[3]=1;
    }
    return 0;
}

int readimemin(char *imeminFileName){
    FILE *imemin;
    char line[13];
    int cnt = 0;
    uint64_t current_instruction;
    uint64_t letter;
    int i;

    imemin = fopen(imeminFileName, "r");
     if (imemin == NULL) {
        return 1;
    }

    while (fgets(line, 12, imemin) != NULL) {
        current_instruction = 0;
        for(i=0;i<12;i++){
            letter = hex_to_bin64(line[i]);
            if(letter==UINT64_MAX){
                return 1;
            }
            current_instruction+=letter;
            current_instruction<<4;
        }
        instructions[cnt] = current_instruction;
        cnt++;
    }
    fclose(imemin);
    return 0;
}

int readdmemin(char *dmeminFileName){
    FILE *dmemin;
    char line[9];
    int cnt = 0;
    uint32_t current_memory_line;
    uint32_t letter;
    int i;

    dmemin = fopen(dmeminFileName, "r");
     if (dmemin == NULL) {
        return 1;
    }

    while (fgets(line, 8, dmemin) != NULL) {
        current_memory_line = 0;
        for(i=0;i<8;i++){
            letter = hex_to_bin32(line[i]);
            if(letter==UINT32_MAX){
                return 1;
            }
            current_memory_line+=letter;
            current_memory_line<<4;
        }
        memory[cnt] = current_memory_line;
        cnt++;
    }
    fclose(dmemin);
    return 0;
}

int readdiskin(char *diskinFileName){
    FILE *diskin;
    char line[9];
    int cnt = 0;
    uint32_t current_disk_line;
    uint32_t letter;
    int i;

    diskin = fopen(diskinFileName, "r");
     if (diskin == NULL) {
        return 1;
    }

    while (fgets(line, 8, diskin) != NULL) {
        current_disk_line = 0;
        for(i=0;i<8;i++){
            letter = hex_to_bin32(line[i]);
            if(letter==UINT32_MAX){
                return 1;
            }
            current_disk_line+=letter;
            current_disk_line<<4;
        }
        disk[cnt] = current_disk_line;
        cnt++;
    }
    fclose(diskin);
    return 0;
}

int readirq2in(char irq2inFileName){
    FILE *irq2in;
    char *line[20];
    int cycle;
    int i;
    LinkedList_int *lastNode;
    LinkedList_int *curNode;

    irq2List = NULL;
    irq2in = fopen(irq2inFileName, "r");
     if (irq2in == NULL) {
        return 1;
    }

    while (fgets(line, 20, irq2in) != NULL) {
        cycle = atoi(line);
        curNode = (LinkedList_int*)malloc(sizeof(LinkedList_int));
        if(curNode == NULL){
            return 1;
        }
        curNode->next = NULL;
        curNode->cycleVal = cycle;
        if(irq2List == NULL){
            irq2List = curNode;
            lastNode = curNode;
        }
        else{
            lastNode->next = curNode;
            lastNode = curNode;
        }
    }
    fclose(irq2in);
    return 0;
}

uint64_t hex_to_bin64(char hex_char) {
    if (hex_char >= '0' && hex_char <= '9')
        return hex_char - '0';
    else if (hex_char >= 'a' && hex_char <= 'f')
        return hex_char - 'a' + 10;
    else if (hex_char >= 'A' && hex_char <= 'F')
        return hex_char - 'A' + 10;
    else
        return UINT64_MAX;
}

uint32_t hex_to_bin32(char hex_char) {
    if (hex_char >= '0' && hex_char <= '9')
        return hex_char - '0';
    else if (hex_char >= 'a' && hex_char <= 'f')
        return hex_char - 'a' + 10;
    else if (hex_char >= 'A' && hex_char <= 'F')
        return hex_char - 'A' + 10;
    else
        return UINT32_MAX;
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

int sllCmd(int rd, int rs, int rt){
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

int inCmd(int rd, int rs,int rt) { 
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

int update_traceFile() {
    
    // make sure that traceFile is open
    if(traceFile == NULL)
    {
    printf("traceFile not opened properly!\n");
    return 1;
    }
    
    fprintf(traceFile, "%08X %08X", PC, instructions[PC]); // add "PC INST" to file (in hex)
        for (int i = 0; i < 16; i++) {
            fprintf(traceFile, " %08X", registers[i]); // add " R[i]" for 0<=i<=15
        }
        fprintf(traceFile, "\n"); 
        fflush(traceFile); // save immediately

    return 0;
}

int update_hwRegTraceFile(const char* action, int regNum, uint32_t data){
    
    // Usage:
    // update_hwRegTraceFile("READ", 11, deviceRegisters[11]);
    // update_hwRegTraceFile("WRITE", 14, data); (where data is what you wrote to device register 14 in that clock cycle)

    // The file format is:
    // CYCLE READ/WRITE NAME DATA

    if(hwRegTraceFile == NULL)
    {
        printf("hwRegTraceFile not opened properly! \n");
        return 1;
    }

    if (hwRegTraceFile != NULL && regNum < sizeof(hwRegistersNames)/sizeof(hwRegistersNames[0])) {
        // Print the cycle number, read/write action, register name, and data in hex format
        fprintf(hwRegTraceFile, "%d %s %s %08X\n", cycle, action, hwRegistersNames[regNum], data);
        fflush(hwRegTraceFile); 
    }

    return 0;

}


int writeDmemout(FILE* dmemoutFile)
{
    // Assuming dmemoutFile is open and valid
        for (size_t i = 0; i < sizeof(memory)/sizeof(memory[0]); i++) {
        fprintf(dmemoutFile, "%08X\n", memory[i]);
    }
    fprintf(dmemoutFile, "\n"); 

    return 0;
}

int writeRegout(FILE* regoutFile)
{
    if(regoutFile==NULL)
    {
        printf("regout.txt not opened properly.\n");
        return 1;
    }
    int i=3;
    for(;i<16;i++)
    {
        fprintf(regoutFile, "08X\n", registers[i]);
    }
    
    fprintf(regoutFile, "\n"); 
    return 0;
}

int writeCycles(FILE* cyclesFile)
{
    if(cyclesFile == NULL)
    {
        printf("cycles.txt not opened properly.\n");
        return 1;
    }
    fprintf(cyclesFile, "%u", cycle);
    return 0;
}

int writeLeds(FILE* ledFile)
{
    // call this function when a led changes
    if(ledFile == NULL)
    {
        printf("leds.txt not opened properly.\n");
        return 1;
    }

    fprintf(ledFile, "%u %08X\n", cycle, LEDS);
    return 0;

}

int write7Seg(FILE* sevenSegmentFile)
{
    if (sevenSegmentFile == NULL)
    {
        printf("display7seg.txt not opened properly.\n");
        return 1;
    }

    fprintf(sevenSegmentFile, "%u %08X\n", cycle, deviceRegisters[10]);
    return 0;
}

int writeDiskOut(FILE* diskoutFile)
{
    // Assuming we're writing one 32 bit word (8 hex digits) in each line, and between sectors (128 words)
    // there's an extra "\n"
    if(diskoutFile == NULL)
    {
        printf("diskout.txt not opened properly.\n");
        return 1;
    }

    int sector = 0;
    int i=0;
    for(;sector<128;sector++)
    {
        for(;i<128;i++)
        {
            fprintf(diskoutFile, "%08X\n", disk[128*sector + i]); // max{128*sector+i} = 128*127+127 = 16383 as required :)
        }
        fprintf(diskoutFile, "\n"); /////// MAKE SURE THIS IS THE REQUIRED FORMAT BEFORE SUBMISSION
        i=0;
    }
    return 0;
}

int writeMonitorTxt(FILE* monitorFile)
{
    if(monitorFile == NULL)
    {
        printf("monitor.txt not opened properly.\n");
        return 1;
    }

    int row = 0, col=0;
    for(; row<128; row++)
    {
        for(;col<128;col++)
        {
            fprintf(monitorFile, "%02X\n", monitor[row][col]); /////// MAKE SURE THIS IS TOP TO BOTTOM AND THEN LEFT TO RIGHT
        }

        col = 0;
    }

    return 0;
}

uint8_t hexStringToByte(const char* hexString) {
    // Helper function

    int hi = hexCharToValue(hexString[0]);
    int lo = hexCharToValue(hexString[1]);
    
    if (hi == -1 || lo == -1) {
        fprintf(stderr, "Invalid hexadecimal digit encountered.\n");
        exit(EXIT_FAILURE);
    }

    return (uint8_t)((hi << 4) | lo);
}

int writeMonitorFiles(FILE* txtMonitorFile, FILE* binMonitorFile)
{

    writeMonitorTxt(txtMonitorFile); // write monitor contents to the .txt file
    fseek(txtMonitorFile, 0, SEEK_SET); // move the .txt file pointer to point at the beginning of the file.
    char hexString[3]; // To store two hexadecimal digits and a null terminator (contents of each line)
    while (fscanf(txtMonitorFile, "%2s", hexString) == 1) {
        uint8_t byte = hexStringToByte(hexString); // ignore the null terminator and convert each line to 2 hex digits (a.k.a 2*4 bits = 8 bits)
        fwrite(&byte, sizeof(byte), 1, binMonitorFile); 
    }

    return 0;
}