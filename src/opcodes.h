#define NUMOPCODES 50

enum argTypes {
	REGISTER, //any register type formatted as a register ($r, $s, $a, etc..)
	INTEGER, //any immediate value formatted as a number
	LABEL, //instruction label used for jumps etc.. formatted as labelName: command
	OFFSET, //address offset formatted as off(b) where b is a register containing some memory address
	NONE //no arg (most opcodes use 3 args, but those who do not will fill the remaining 1-2 slots with None)
};

enum opcodes {
	ADD,
	ADDU,
	ADDI,
	ADDIU,
	AND,
	ANDI,
	DIV,
	DIVU,
	MULT,
	MULTU,
	NOR,
	OR,
	ORI,
	SLL,
	SLLV,
	SRA,
	SRAV,
	SRL,
	SRLV,
	SUB,
	SUBU,
	XOR,
	XORI,
	LHI,
	LLO,
	SLT,
	SLTU,
	SLTI,
	SLTIU,
	BEQ,
	BGTZ,
	BLEZ,
	BNE,
	J,
	JAL,
	JALR,
	JR,
	LB,
	LBU,
	LH,
	LHU,
	LW,
	SB,
	SH,
	SW,
	MFHI,
	MFLO,
	MTHI,
	MTLO,
	TRAP
};

const char * opcodeNames[NUMOPCODES] = {
	"ADD",
	"ADDU",
	"ADDI",
	"ADDIU",
	"AND",
	"ANDI",
	"DIV",
	"DIVU",
	"MULT",
	"MULTU",
	"NOR",
	"OR",
	"ORI",
	"SLL",
	"SLLV",
	"SRA",
	"SRAV",
	"SRL",
	"SRLV",
	"SUB",
	"SUBU",
	"XOR",
	"XORI",
	"LHI",
	"LLO",
	"SLT",
	"SLTU",
	"SLTI",
	"SLTIU",
	"BEQ",
	"BGTZ",
	"BLEZ",
	"BNE",
	"J",
	"JAL",
	"JALR",
	"JR",
	"LB",
	"LBU",
	"LH",
	"LHU",
	"LW",
	"SB",
	"SH",
	"SW",
	"MFHI",
	"MFLO",
	"MTHI",
	"MTLO",
	"TRAP"
};

const int opcodeArgs[NUMOPCODES][3] = {
	{REGISTER,REGISTER,REGISTER}, //ADD
	{REGISTER,REGISTER,REGISTER}, //ADDU
	{REGISTER,REGISTER,INTEGER}, //ADDI
	{REGISTER,REGISTER,INTEGER}, //ADDIU
	{REGISTER,REGISTER,REGISTER}, //AND
	{REGISTER,REGISTER,INTEGER}, //ANDI
	{REGISTER,REGISTER,NONE}, //DIV
	{REGISTER,REGISTER,NONE}, //DIVU
	{REGISTER,REGISTER,NONE}, //MULT
	{REGISTER,REGISTER,NONE}, //MULTU
	{REGISTER,REGISTER,REGISTER}, //NOR
	{REGISTER,REGISTER,REGISTER}, //OR
	{REGISTER,REGISTER,INTEGER}, //ORI
	{REGISTER,REGISTER,REGISTER}, //SLL
	{REGISTER,REGISTER,REGISTER}, //SLLV
	{REGISTER,REGISTER,REGISTER}, //SRA
	{REGISTER,REGISTER,REGISTER}, //SRAV
	{REGISTER,REGISTER,REGISTER}, //SRL
	{REGISTER,REGISTER,REGISTER}, //SRLV
	{REGISTER,REGISTER,REGISTER}, //SUB
	{REGISTER,REGISTER,REGISTER}, //SUBU
	{REGISTER,REGISTER,REGISTER}, //XOR
	{REGISTER,REGISTER,INTEGER}, //XORI
	{REGISTER,INTEGER,NONE}, //LHI
	{REGISTER,INTEGER,NONE}, //LLO
	{REGISTER,REGISTER,REGISTER}, //SLT
	{REGISTER,REGISTER,REGISTER}, //SLTU
	{REGISTER,REGISTER,INTEGER}, //SLTI
	{REGISTER,REGISTER,INTEGER}, //SLTIU
	{REGISTER,REGISTER,LABEL}, //BEQ
	{REGISTER,LABEL,NONE}, //BGTZ
	{REGISTER,LABEL,NONE}, //BLEZ
	{REGISTER,REGISTER,LABEL}, //BNE
	{LABEL,NONE,NONE}, //J
	{LABEL,NONE,NONE}, //JAL
	{LABEL,NONE,NONE}, //JALR
	{LABEL,NONE,NONE}, //JR
	{REGISTER,OFFSET,NONE}, //LB
	{REGISTER,OFFSET,NONE}, //LBU
	{REGISTER,OFFSET,NONE}, //LH
	{REGISTER,OFFSET,NONE}, //LHU
	{REGISTER,OFFSET,NONE}, //LW
	{REGISTER,OFFSET,NONE}, //SB
	{REGISTER,OFFSET,NONE}, //SH
	{REGISTER,OFFSET,NONE}, //SW
	{REGISTER,NONE,NONE}, //MFHI
	{REGISTER,NONE,NONE}, //MFLO
	{REGISTER,NONE,NONE}, //MTHI
	{REGISTER,NONE,NONE}, //MTLO
	{INTEGER,NONE,NONE} //TRAP
};
