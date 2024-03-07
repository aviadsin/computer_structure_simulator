#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

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
int dskCycle;
int dskBuffer;
int dskCmd;
int dskSector;
int inIrq;
int changedPC;
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




int init(int argc, char const *argv[])
{
    //TODO: put every variable to zero, call functions to read from input files,
    // open trace files - trace.txt hwregtrace.txt leds.txt display7seg.tx 
    /*
    put every variable to zero
    call functions to read from input files - readimemin more reads and shit
    open trace files - global shit
    
    */
    int i,j;
    
    for(i=0;i<16;i++){
        registers[i]=0;
    }
    for(i=0;i<23;i++){
        deviceRegisters[i]=0;
    }
    for(i=0;i<4096;i++){
        instructions[i]=0;
    }
    for(i=0;i<4096;i++){
        memory[i]=0;
    }
    for(i=0;i<4096;i++){
        for(j=0;j<256;j++){
            monitor[i][j]=0;
        }
    }
    for(i=0;i<16384;i++){
        disk[i]=0;
    }
    readimemin(argv[1]);
    readdmemin(argv[2]);
    readdiskin(argv[3]);
    readirq2in(argv[4]);
    traceFile = fopen(argv[7], "r+");
    hwRegTraceFile = fopen(argv[8], "r+");
    ledFile = fopen(argv[10], "r+");
    display7segFile = fopen(argv[11], "r+");
    LEDS=0;
    irq2List = NULL;
    cycle =0;
    PC=0;
    dskCycle=0;
    dskCmd=0;
    inIrq=0;
    changedPC=0;

    return 0;
}

int simClockCycle()
{
    //TODO: decide in which order we need to call the functions
    /*
    do disk shit - r/w sector (from dskCmd),reset busy diskcmd(registor) , activate irqstatus1 
    update_tracefile()
    timer
    interrupts
    call parse instruction function !!! - {$$ben}
    call opcode(if not jump)
    check and handle monitor/disk - busy and dskCmd u saves clockcycle+1024/LEDS/7seg - write7Seg
    hwtracefile() ,
    increment PC(when not jumping)
    halt - return -1 else 0 error 1
    */
    bool irq = (deviceRegisters[3]==1 && deviceRegisters[0]==1)||(deviceRegisters[4]==1 && deviceRegisters[1]==1)||(deviceRegisters[5]==1 && deviceRegisters[2]==1);
    int *inst; 
    changedPC = 0;
    update_traceFile();
    if(cycle==dskCycle && deviceRegisters[17]==1){
        handleDisk();
    }
    incrementTimer();
    if(inIrq == 0 && irq){
        if(deviceRegisters[3]==1 && deviceRegisters[0]==1){
            deviceRegisters[3] = 0;
        }
        else{
            if(deviceRegisters[4]==1 && deviceRegisters[1]==1){
                deviceRegisters[4] = 0;
            }
            else{
                if(deviceRegisters[5]==1 && deviceRegisters[2]==1){
                    deviceRegisters[5] = 0;
                }
            }
        }
        deviceRegisters[7] = PC;
        PC = deviceRegisters[6];
        changedPC = 1;
        inIrq = 1;

    }else{
        //inst = parse instruction function
        inst = parseInstruction(instructions[PC]);
        if(inst[0] == 0){
            addCmd(inst[1],inst[2],inst[3],inst[4]);
        }
        if(inst[0] == 1){
            subCmd(inst[1],inst[2],inst[3],inst[4]);
        }
        if(inst[0] == 2){
            macCmd(inst[1],inst[2],inst[3],inst[4]);
        }
        if(inst[0] == 3){
            andCmd(inst[1],inst[2],inst[3],inst[4]);
        }
        if(inst[0] == 4){
            orCmd(inst[1],inst[2],inst[3],inst[4]);
        }
        if(inst[0] == 5){
            xorCmd(inst[1],inst[2],inst[3],inst[4]);
        }
        if(inst[0] == 6){
            sllCmd(inst[1],inst[2],inst[3]);
        }
        if(inst[0] == 7){
            sraCmd(inst[1],inst[2],inst[3]);
        }
        if(inst[0] == 8){
            srlCmd(inst[1],inst[2],inst[3]);
        }
        if(inst[0] == 9){
            beqCmd(inst[2],inst[3],inst[4]);
        }
        if(inst[0] == 10){
            bneCmd(inst[2],inst[3],inst[4]);
        }
        if(inst[0] == 11){
            bltCmd(inst[2],inst[3],inst[4]);
        }
        if(inst[0] == 12){
            bgtCmd(inst[2],inst[3],inst[4]);
        }
        if(inst[0] == 13){
            bleCmd(inst[2],inst[3],inst[4]);
        }
        if(inst[0] == 14){
            bgeCmd(inst[2],inst[3],inst[4]);
        }
        if(inst[0] == 15){
            jalCmd(inst[1],inst[4]);
        }
        if(inst[0] == 16){
            lwCmd(inst[1],inst[2],inst[3],inst[4]);
        }
        if(inst[0] == 17){
            swCmd(inst[1],inst[2],inst[3],inst[4]);
        }
        if(inst[0] == 18){
            retiCmd();
        }
        if(inst[0] == 19){
            inCmd(inst[1],inst[2],inst[3]);
            update_hwRegTraceFile("READ", (int)(inst[2]+inst[3]) , inst[1]);
        }
        if(inst[0] == 20){
            outCmd(inst[2],inst[3],inst[4]);
            update_hwRegTraceFile("WRITE",(int)(inst[2]+inst[3]),inst[4]);
            if(inst[2]+inst[3] == 22 && inst[4] !=0){
                writeToMonitor();
            }
            if(inst[2]+inst[3] == 14 && inst[4] !=0){
                dskCycle = cycle+1024;
                dskCmd = inst[4];
                dskBuffer= deviceRegisters[16];
                dskSector = deviceRegisters[15];
                deviceRegisters[17]=1;
            }
            if(inst[2]+inst[3] == 9){
                writeLeds(ledFile);
            }
            if(inst[2]+inst[3] == 9){
                write7Seg(display7segFile);
            }

        }
        if(changedPC==0){
            PC+=1;
        }
        if(inst[0]==21){
            return -1;
        }
        cycle++;
        deviceRegisters[8] = (deviceRegisters[8]+1)%0xffffffff;
        return 0;

    }

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
    init(argc,argv);
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

int orCmd(int rd, int rs,int rt, int rm){
    if(rd<0||rd>=16){
        return 0;
    }
    if(rd<=2){
        return 1;
    }
    registers[rd] = registers[rs] | registers[rt] | registers[rm];
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
        changedPC = 1;
    }
    return 1;
}

int bneCmd(int rs, int rt,int rm){ //////// i don't know if we should c=increment the value of PC by 1 in simCycle() after this function is called
    if(registers[rs]!=registers[rt]){
        PC = (registers[rm]<<21)>>21;
        changedPC = 1;
    }
    return 1;
}

int bltCmd(int rs, int rt,int rm){ //////// i don't know if we should c=increment the value of PC by 1 in simCycle() after this function is called
    if(registers[rs]<registers[rt]){
        PC = (registers[rm]<<21)>>21;
        changedPC = 1;
    }
    return 1;
}

int bgtCmd(int rs, int rt,int rm){ //////// i don't know if we should c=increment the value of PC by 1 in simCycle() after this function is called
    if(registers[rs]>registers[rt]){
        PC = (registers[rm]<<21)>>21;
        changedPC = 1;
    }
    return 1;
}

int bleCmd(int rs, int rt,int rm){ //////// i don't know if we should c=increment the value of PC by 1 in simCycle() after this function is called
    if(registers[rs]<=registers[rt]){
        PC = (registers[rm]<<21)>>21;
        changedPC = 1;
    }
    return 1;
}

int bgeCmd(int rs, int rt,int rm){ //////// i don't know if we should c=increment the value of PC by 1 in simCycle() after this function is called
    if(registers[rs]<=registers[rt]){
        PC = (registers[rm]<<21)>>21;
        changedPC = 1;
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
    changedPC = 1;
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

int retiCmd(void){ //////// i don't know if we should c=increment the value of PC by 1 in simCycle() after this function is called
    PC = deviceRegisters[7];
    changedPC = 1;
    inIrq = 0;
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
    if(registers[rs]+registers[rt]!=17){
        deviceRegisters[registers[rs]+registers[rt]] = registers[rm];
    }
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


int* parseInstruction(uint64_t inst)
{
    /*
    ret[0] = opcode
    ret[1] = rd
    ret[2] = rs
    ret[3] = rt
    ret[4] = rm
    */

    int ret[5];
    ret[0] = inst>>40;
    ret[1] = (inst >>36) & 0xf;
    ret[2] = (inst>>32) & 0xf;
    ret[3] = (inst>>28) & 0xf;
    ret[4] = (inst>>24) & 0xf;

    // update $imm1, $imm2 values
    registers[1] = (inst>>12) & 0xfff; // $imm1
    registers[2] = inst & 0xfff; // $imm2

    return ret;
}

int handleDisk()
{

    if(dskCmd == 1)
    {
        //read sector -> copy sector from disk to address deviceRegisters[16]
        int i=0;
        for(;i<128;i++)
        {
            if((dskBuffer + i) < 4096)
                {memory[dskBuffer + i] = disk[dskSector * 128 + i];}
        }
        
    }
    if(dskCmd == 2)
    {
        // write sector - > copy data from address deviceRegisters[16] to disk
        int i=0;
        for(;i<128;i++)
        {
            if((dskBuffer + i) < 4096)
                {disk[dskSector * 128 + i] = memory[dskBuffer + i];}
        }

    }

    //reset regs
    deviceRegisters[14] = 0;
    deviceRegisters[17] = 0;
    dskCmd = 0;


    return 0;
}