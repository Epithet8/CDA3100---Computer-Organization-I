#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdbool.h>

//This is the individule work of Gabriel Clyce.
int text[105];
int regFile[32];
int dataMem[32];
int pc=0,cycle=1,stall=0,branch=0,mispred=0;
typedef struct
{
	int op;
	int rs, rt,rd;
	int imm, brt,funct,shamt;
}Inst;
Inst inst[105];
typedef struct
{
	Inst in;
	int ins;
	int pc4;
}IFID;

typedef struct 
{
	Inst in;
	int ins;
	int pc4;
	int rd1,rd2;
}IDEX;

typedef struct
{
	Inst in;
	int ins;
	int alu, write;
	int reg;
}EXMEM;

typedef struct
{
	Inst in;
	int ins;
	int mem,alu;
	int reg;
}MEMWB;

typedef struct 
{
	int pc;
	IFID ifid;
	IDEX idex;
	EXMEM exmem;
	MEMWB memeb;
}state;

const int SNT=0, WNT=1, WT=2, ST=3;

typedef struct 
{
	int pc, target, state;
}BP;

BP bp[102];

int getOp(int ins)
{
	unsigned int newIns = ins >> 26;
	if(newIns ==0) // for R types
	{
		newIns = ins & 0x3F;
	}
	if(newIns == -29)
		newIns = 35;
	if(newIns == -21)
		 newIns = 43;
	return newIns;
}

int getRs(int ins)
{
	int newIns = ins & 0x03FFFFFF;
	newIns = newIns >>21;
	return newIns;
}

int getRt(int ins)
{
	int newIns = ins & 0x001FFFFF;
	newIns = newIns >>16;
	return newIns;
}

int getRd(int ins)
{
	int newIns = ins & 0x0000FFFF;
	newIns = newIns >>11;
	return newIns;
}

int getImm(int ins)
{
	int newIns = ins & 0x0000FFFF;
	short imm = (short)newIns;
	//printf("%d\t",imm);
	return imm;
}
int getBrTar(int ins, int pc)
{
	int target = ins &0x0000FFFF;
	short offset = (short) target;
	offset = offset * 4;
	return offset + pc;
}
int getFunct(int ins)
{
	int newIns = ins &0x0000003F;
	return newIns;
}
int getShamt(int ins)
{
	int newIns = ins&0x000007F0;
	newIns = newIns >> 6;
	return newIns;
}

void getReg(int newIns,char rs[] )
{
	if(newIns == 0)
		strcpy(rs,"0");
	else if(newIns ==1)
		strcpy(rs,"1");
	else if(newIns >1 && newIns <=3)
	{
		rs[0]='v';
		rs[1] = newIns-2 + 48;
		rs[2]='\0';
	}
	else if(newIns <=7)
	{
		rs[0]='a';
		rs[1] = newIns-4 + 48;
		rs[2]='\0';
	}
	else if(newIns <=15)
	{
		rs[0]='t';
		rs[1] = newIns-8 + 48;
		rs[2]='\0';
	}
	else if(newIns <=23)
	{
		rs[0]='s';
		rs[1] = newIns-16 + 48;
		rs[2]='\0';
	}
	else if(newIns <=25)
	{
		rs[0]='t';
		rs[1] = newIns-16 + 48;
		rs[2]='\0';
	}
	else if(newIns <=27)
	{
		rs[0]='k';
		rs[1] = newIns-26 + 48;
		rs[2]='\0';
	}	
	else if(newIns == 28)
		strcpy(rs,"gp");
	else if(newIns == 29)
		strcpy(rs,"sp");
	else if(newIns == 30)
		strcpy(rs,"fp");
	else if(newIns == 31)
		strcpy(rs,"ra");
}
void printInst(Inst in,int ins)
{
	char out[100] ="";
	char rs[5],rt[5],rd[5];
	int func;
	getReg(in.rs,rs);
	getReg(in.rt,rt);
	getReg(in.rd,rd);
	switch(in.op)
	{
		case 35: sprintf(out,"lw $%s,%d($%s)",rt,in.imm,rs);
				 break;
		case 43: sprintf(out,"sw $%s,%d($%s)",rt,in.imm,rs);
				break;
		case 8:sprintf(out,"addi $%s,$%s,%d",rt,rs,in.imm);
				break;
		case 13: sprintf(out,"ori $%s,$%s,%d",rt,rs,in.imm);
				break;
		case 4: sprintf(out,"beq $%s,$%s,%d",rs,rt,in.imm);
				break;
		case 1: sprintf(out,"halt");
				break;
		case 32: sprintf(out,"add $%s,$%s,$%s",rd,rs,rt);
				break;
		case 34: sprintf(out,"sub $%s,$%s,$%s",rd,rs,rt);
				break;
		case 0: if(ins==0)
					sprintf(out,"NOOP");
				else sprintf(out,"sll, $%s,$%s,%d",rd,rt,in.shamt);
				break;
	}
	printf("%s\n",out);
}
void getInst(int num, Inst *in, int pc)
{
	in->op = getOp(num);
	in->rs = getRs(num);
	in->rt = getRt(num);
	in->rd = getRd(num);
	in->imm = getImm(num);
	in->brt = getBrTar(num,pc);
	in->funct = getFunct(num);
	in->shamt = getShamt(num);
}
void printState(state st)
{
	int i;
	char rs[5],rt[5],rd[5];
	printf("********************\n");
	printf("State at the beginning of cycle %d\n",cycle);
	printf("\tPC = %d\n",pc);
	printf("\tData Memory:\n");
	for(i=0; i<16; i++)
	{
		printf("\t\tdataMem[%d] = %d\t\tdataMem[%d] = %d\n",i,dataMem[i],i+16,dataMem[i+16]);
	}
	printf("\tRegisters:\n");
	for(i=0; i<16; i++)
	{
		printf("\t\tregFile[%d] = %d\t\tregFile[%d] = %d\n",i,regFile[i],i+16,regFile[i+16]);
	}
	printf("\tIF/ID:\n");
	printf("\t\tInstruction: ");
	printInst(st.ifid.in,st.ifid.ins);
	printf("\t\tPCPlus4: %d\n",st.ifid.pc4);
	printf("\tID/EX:\n");
	printf("\t\tInstruction: ");
	printInst(st.idex.in,st.idex.ins);
	printf("\t\tPCPlus4: %d\n",st.idex.pc4);
	printf("\t\tbranchTarget: %d\n",st.idex.in.brt);
	printf("\t\treadData1: %d\n\t\treadData2: %d\n",st.idex.rd1,st.idex.rd2);
	printf("\t\timmed: %d\n",st.idex.in.imm);
	getReg(st.idex.in.rs,rs);
	getReg(st.idex.in.rt,rt);
	getReg(st.idex.in.rd,rd);
	printf("\t\trs: %s\n\t\trd: %s\n\t\trd: %s\n",rs,rt,rd);
	printf("\tEX/MEM\n");
	printf("\t\tInstruction: ");
	printInst(st.exmem.in,st.exmem.ins);
	getReg(st.exmem.reg,rs);
	printf("\t\taluResult: %d\n\t\twriteDataReg: %d\n\t\twriteReg: %s\n",st.exmem.alu,st.exmem.write,rs);
	printf("\tMEM/WB\n");
	printf("\t\tInstruction: ");
	printInst(st.memeb.in,st.memeb.ins);
	getReg(st.memeb.reg,rs);
	printf("\t\twriteDataMem: %d\n\t\twriteDataALU: %d\n\t\twriteReg: %s\n",st.memeb.mem,st.memeb.alu,rs);
}	
int read1(Inst in)
{
	return regFile[in.rs];
}
int read2(Inst in)
{
	if(in.op == 32 || in.op == 34 || in.op == 0 || in.op ==1 || in.op==5 )
		return regFile[in.rt];
	else return in.imm;
}
void init(state *st)
{
	// initialize your simulator and pipleine registers here
	// Initialize registers values' to 0x0
	for (int i = 0; i < 32; i++){
		regFile[i] = 0x0;
		dataMem[i] = -1;
	}
	for (int i = 0; i < 32; i++){
		text[i] = -1;
	}
}

int getDec(char *bin) {

	int  b, k, m, n;
	int  len, sum = 0;

	// Length - 1 to accomodate for null terminator
	len = strlen(bin) - 1;

	// Iterate the string
	for(k = 0; k <= len; k++) {

		// Convert char to numeric value
		n = (bin[k] - '0');

		// Check the character is binary
		if ((n > 1) || (n < 0))  {
			return 0;
		}

		for(b = 1, m = len; m > k; m--)
			b *= 2;

		// sum it up
		sum = sum + n * b;
	}

	return sum;
}
char *trimInstruction(char* str)
{
    char *end;

    while(isspace((unsigned char)*str)) str++;

    if(*str == 0)
        return str;

    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    end[1] = '\0';

    return str;
}

//rec start
typedef struct {
		char opstr[8];
		int op;
	}Ops_t;
Ops_t oparr[] = {
					{"addi", 8},
					{"add", 32},
					{"sub", 34}
				};

int getOpNum(char *op){
	for(int i=0; i<sizeof(oparr)/sizeof(oparr[0]); i++){
		if(strcmp(op, oparr[i].opstr) == 0){
			return oparr[i].op;
		}
	}
	return 0;
}
//rec end

int main()
{
	// declare some variables here
	state curState,newState;
	init(&curState);
	
	// read and parse the input into text and data segments
	// Open Input file
	FILE *fp;
	fp = fopen("test2.asm", "r");
	if (fp == NULL) {
		printf("Error opening input file.\n");
		exit(1);
	}


	// Initialize variables for parsing
	char line[100];
	char *p;
	int i = 0, line_num = 0;

	// Copy .text section to memory
	while (fgets(line, sizeof(line), fp) != NULL) {
		line_num++;
        
        p = strchr(line, '\n');
			if (p != NULL)
				*p = '\0';
        strcpy(line, trimInstruction(line));
//rec		line[i] = line; //not sure if correct
    
// 		text[i] = line;
		text[i] = getOpNum(line); //rec
		
		
		// If 'nop' found, move to 0x2000 in memory and break
		if (strcmp(line, "halt") == 0) {
			i = 0x800;
			break;
		}

		else
			i++;
	}

	// Seek fp to first instruction in .data
	char data[100];
	int bytes = 33 * line_num;
	fseek(fp, bytes, SEEK_SET);

	// Copy .data section to memory
	while (fgets(line, sizeof(data), fp) != NULL) {
		// Remove '\n' from 'line'
		
        p = strchr(line, '\n');
			if (p != NULL)
				*p = '\0';
        strcpy(line, trimInstruction(line));
        puts(line);
// 		dataMem[i] = line;
		i++;
	}


	
	
	pc=0;
	cycle=1;
	printState(curState);
	
// 	 while(/* halt is not yet in wb*/)
// 	{
// 		// simulate the pipeline
// 		pc+=4;
// 		cycle++;
// 		printState(newState);
// 		curState=newState;
// 	}
	printf("********************\n");
	printf("Total number of cycles executed: %d\n",cycle);
	printf("Total number of stalls: %d\n",stall);
	printf("Total number of branches: %d\n",branch);
	printf("Total number of mispredicted branches: %d\n",mispred);
	return 0;
}
