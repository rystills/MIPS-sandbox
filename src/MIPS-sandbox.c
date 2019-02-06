#define NKC_IMPLEMENTATION
#define NKC_DISABLE_DEFAULT_FONT
#include "../nuklear_cross/nuklear_cross.h"
#include <strings.h>

#define NOC_FILE_DIALOG_IMPLEMENTATION
#ifdef _WIN32
// tweak the formatting for windows
#define fontSize 19
#define fontPaddingExtra 0
#define NOC_FILE_DIALOG_WIN32
#else
// platform independent case independent string comparison
#define stricmp strcasecmp
// tweak the formatting for non-windows
#define fontSize 13
#define fontPaddingExtra 3
#endif

#ifdef __APPLE__
#define NOC_FILE_DIALOG_OSX
#endif
#ifdef __linux__
#define NOC_FILE_DIALOG_GTK
#endif
#ifdef __FreeBSD__
#define NOC_FILE_DIALOG_GTK
#endif
#include "../noc/noc_file_dialog.h"
#include <stdio.h>
// inttypes allows us to guarantee n-bit ints
#include <inttypes.h>
#include <stdbool.h>
#include "registers.h"
#include "opcodes.h"
#include "linkedList.h"

extern int screenWidth;
extern int screenHeight;

bool loadedFile = false;
static int shouldZeroRegistersOnRun = nk_false;
static int singleStepMode = nk_false;
static int singleStepCompleted = nk_true;
int pc=-1;  // simulation program counter

// signed 32 bit max val = 2,147,483,647, or 10 digits; need 3 additional digits for optional - sign and \0
char registers[NUMREGISTERS][REGISTERLEN];

// for now, use a fixed size code window
#define NUMCODECHARS 999
char codeText[NUMCODECHARS];
char codeTextPrev[NUMCODECHARS];
char curFileName[260];
char modifiedFileName[261];
char consoleText[999];
int curOpcode;
char curOpcodeArg0[16];
char curOpcodeArg1[16];
char curOpcodeArg2[16];

/**check whether or not a string is a number
 * @param str: string to check
 * @param len: string length
 * @returns: whether str is a valid number (true) or not (false)
 */
bool stringIsNumber(char* str,int len) {
	for (int i = 0; i < len && str[i]!='\0'; ++i) {
		if (!((i == 0 && str[i] == '-') || isdigit(str[i]) || str[i] == '\0')) {
			return false;
		}
	}
	return true;
}

/**
 * check if the specified register contents are invalid; if so, set register style to force red highlight
 * @param regNum: the register number to check
 * @param ctx: the current nuklear context
 * @returns: whether the specified register is invalid (true) or not (false)
 */
bool checkSetInvalidRegisterContents(int regNum, struct nk_context *ctx) {
	if (!(stringIsNumber(registers[regNum],REGISTERLEN))) {
		ctx->style.edit.normal.data.color =nk_rgb(255,0,0);
		ctx->style.edit.active.data.color =nk_rgb(255,0,0);
		ctx->style.edit.hover.data.color =nk_rgb(255,0,0);
		return true;
	}
	return false;
}

/**
 * count the number of lines in the code text string up to the specified position
 * @param pos: the character number up to which to count newlines in the code text string
 * @returns: the number of lines in the code text string up to the specified position
 */
int lineCountTo(int pos) {
	int lines = 0;
	for (int i = 0; i < pos; ++i)
		if (codeText[i] == '\n') ++lines;
	return lines;
}

/**
 * count the number of characters on the same line up to the specified position
 * @param pos: the character number up to which to count characters in the code text string
 * @returns: the number of characters on the same line up to the specified position
 */
int lineCharCount(int pos) {
	int chars = 1;
	for (int i = pos-1; i >= 0 && codeText[i] != '\n'; --i,++chars);
	return chars;
}

/**
 * reset the 'edit' style (used for register input) to default
 * @param ctx: the current nuklear context
 */
void clearRegisterStyle(struct nk_context *ctx) {
	ctx->style.edit.normal.data.color = nk_rgb(38,38,38);
	ctx->style.edit.active.data.color = nk_rgb(38,38,38);
	ctx->style.edit.hover.data.color = nk_rgb(38,38,38);
}

/**
 * determine the number of characters present in the specified file
 * @param file: a file pointer returned by fopen
 * @returns: the number of characters in file
 */
long getFileLength(FILE *file) {
    fseek(file, 0L, SEEK_END);
    long length = ftell(file);
    rewind(file);
    return length;
}

/**
 * display an error message and terminate the application
 * @param msg: the message to display
 */
void exitError(char* msg) {
	fputs(msg,stderr);
	exit(1);
}

/**
 * open a file select dialogue and load the contents of the chosen file into fileData
 * @returns: whether a file was successfully chosen and read (true) or not (false)
 */
bool loadFileData() {
	const char* ret = noc_file_dialog_open(NOC_FILE_DIALOG_OPEN,"asm\0*.s;*.asm\0", NULL, NULL);
	if (ret == NULL) {
		return false;
	}
	FILE *inFile = fopen(ret,"rb");
	if (!inFile) {
		exitError("Error: unable to open input file");
	}
	long fileLen = getFileLength(inFile);
	char* fileData = malloc(fileLen+1);
	if (!fileData) {
		fclose(inFile);
		exitError("Error: unable to allocate memory for file buffer");
	}
	size_t readVal = fread(fileData, fileLen, 1, inFile);
	fileData[fileLen] = '\0';
	fclose(inFile);
	if(readVal != 1) {
		  free(fileData);
		  exitError("Error: unable to read all data from input file");
	}
	// copy file data into code text buffer, stripping windows \r's as we go
	memset(codeText,'\0',sizeof(codeText));
	int charsCopied = 0;
	const char delim[2] = "\r";
	char *token = strtok(fileData, delim);
	while(token != NULL) {
		strcpy(codeText+charsCopied,token);
		charsCopied += strlen(token);
		token = strtok(NULL, delim);
	}

	free(fileData);
	strcpy(curFileName, ret);
	strcpy(codeTextPrev,codeText);
	loadedFile = true;
	return true;
}

bool saveFileData();

/**
 * selects a file and saves the contents of the code window to it
 * @returns: whether a file was successfully chosen and written to (true) or not (false)
 */
bool saveFileDataAs() {
	const char* ret = noc_file_dialog_open(NOC_FILE_DIALOG_SAVE,"asm\0*.s;*.asm\0", NULL, NULL);
	if (ret == NULL) {
		return false;
	}
	// selected file; save to it
	strcpy(curFileName, ret);
	if (strcmp(curFileName+strlen(curFileName)-4,".asm")!=0) {
		strcat(curFileName,".asm");
	}
	codeTextPrev[0]='\0';
	return saveFileData();
}

/**
 * saves the contents of the code window to the last loaded file (if no file was loaded, triggers saveAs window)
 * @returns: whether a file was successfully chosen and written to (true) or not (false)
 */
bool saveFileData() {
	if (curFileName[0] == '\0') {
		return saveFileDataAs();
	}
	// disallow saving unchanged file unless its empty
	if (strlen(codeText) > 0 && strcmp(codeText,codeTextPrev)==0) {
		return false;
	}
	// we are working on an existing file, so save to it
	FILE *fp = fopen(curFileName, "w");
	if (!fp)
		exitError("Error: unable to open input file");
	fputs(codeText, fp);
	fclose(fp);
	strcpy(codeTextPrev,codeText);
	return true;
}

/**
 * set all registers to the value 0. This would not be guaranteed in practice, but should make fresh runs visually cleaner
 */
void zeroRegisters() {
	for (int i = 1; i < NUMREGISTERS; ++i) {
		registers[i][0] = '0';
		registers[i][1] = '\0';
	}
}

/**increment the program counter to the next instruction
 * @param pc: the index of the program counter in the code string
 * @returns: updated location of program counter
 */
int incrementPc(int pc) {
	// exit on empty code or initial value of null terminator
	if (codeText[0] == '\0' || (pc != -1 && codeText[pc] == '\0')) return -1;
	int startPc = pc;
	for (++pc; pc != startPc; ++pc) {
		// ignore whitespace
		for(; codeText[pc] != '\0' && (codeText[pc] == ',' || codeText[pc] == '\n'); ++pc);
		if (codeText[pc] == '\0') {
			return -1;
		}
		// ignore comment lines
		if (codeText[pc] == '#') {
			for(; codeText[pc] != '\0' && codeText[pc] != '\n'; ++pc);
			if (codeText[pc] == '\0') {
				return -1;
			}
		}
		break;
	}
	return pc;
}

/**
 * write a message to the output console
 * @param msg: the string message to write
 */
void writeConsole(char* msg) {
	int i;
	for (i = 0; consoleText[i] != '\0'; ++i);
	strcpy(consoleText+i,msg);
}

/**
 * convert the string representation of the specified register to its integer enum value
 * @param REGISTER: the register string to convert
 * @returns: the integer value corresponding to the enum for REGISTER, or -1 if the opcode is not found
 */
int registerStrToInt(char* REGISTER) {
	for (int i = 0; i < NUMREGISTERS; ++i) {
		if (stricmp(registerNames[i],REGISTER) == 0) {
			return i;
		}
	}
	return -1;
}

/**
 * convert the string representation of the specified opcode to its integer enum value
 * @param opcode: the opcode string to convert
 * @returns: the integer value corresponding to the enum for opcode, or -1 if the opcode is not found
 */
int opcodeStrToInt(char* opcode) {
	for (int i = 0; i < NUMOPCODES; ++i) {
		if (stricmp(opcodeNames[i],opcode) == 0) {
			return i;
		}
	}
	return -1;
}

/**
 * parses the arguments in the currently pointed to opcode, storing them in curOpcodeArg1-3
 * @param pc: current program counter location
 * @returns: whether or not the args were parsed successfully
 */
bool opcodeParseArgs(int *pc) {
	int curOpcodeNum = -1;
	int curOpcodeStartLoc;
	int curOpcodeEndLoc;
	for (;; ++*pc) {
		// navigate to the start of the current argument
		for (;codeText[*pc] != '\0' && (codeText[*pc] == ' ' || codeText[*pc] == '\t');++*pc,curOpcodeStartLoc = *pc);
		if (codeText[*pc] == ',' || codeText[*pc] == '\n' || codeText[*pc] == '\0') {
			// get the start and end of the current argument, and make sure the argument length is within a reasonable bound
			curOpcodeEndLoc = *pc;
			if (++curOpcodeNum > 2) {
				printf("Error: too many args detected\n");
				return false;
			}
			if (curOpcodeEndLoc - curOpcodeStartLoc > 15) {
				printf("Error: arg is too long, likely invalid\n");
				return false;
			}
			char curOpcodeStr[16];
			memset(&curOpcodeStr[0], 0, sizeof(curOpcodeStr));

			strncpy(curOpcodeStr,codeText+curOpcodeStartLoc,curOpcodeEndLoc-curOpcodeStartLoc);
			// check the current argument against our opcode's arglist
			printf("current opcode string? %s\n",curOpcodeStr);
			switch(opcodeArgs[curOpcode][curOpcodeNum]) {
				case REGISTER:
					if (registerStrToInt(curOpcodeStr) == -1) {
						printf("Error: opcode expects register for argument %d, but did not map to a known register\n",curOpcodeNum);
						return false;
					}
					break;
				case INTEGER:
					if (!stringIsNumber(curOpcodeStr,20)) {
						printf("Error: opcode expects integer for argument %d, but did not map to a valid integer\n",curOpcodeNum);
					}
					break;
				case LABEL:
					// TODO: fill me out
					break;
				case OFFSET:
					// TODO: fill me out
					break;
				case NONE:
					// TODO: fill me out
					break;
			}
			if (curOpcodeNum == 0) strcpy(curOpcodeArg0,curOpcodeStr);
			else if (curOpcodeNum == 1) strcpy(curOpcodeArg1,curOpcodeStr);
			else strcpy(curOpcodeArg2,curOpcodeStr);
			// continue on to the next argument
			curOpcodeStartLoc = curOpcodeEndLoc+1;
		}
		if (codeText[*pc] == '\n' || codeText[*pc] == '\0') {
			break;
		}
	}
	return true;
}

/**find and return the location of the specified label, if found
 * @param labelName: the name of the label to find
 * @returns the location in codeText of labelName, or -1 if not found
 */
int findLabel(char* labelName) {
	char fullLabel[100];
	sprintf(fullLabel,"%s:",labelName);
	printf("finding label %s\n",fullLabel);
	char* substr = strstr(codeText,fullLabel);
	return substr == NULL? -1 : substr - codeText-1;
}

/**
 * execute the simulation on the current file. This is a slow temporary solution that iterates through code lines
 */
void runSimulation() {
	if ((!singleStepMode) || singleStepCompleted) {
		// initialization
		if (shouldZeroRegistersOnRun) {
			zeroRegisters();
		}
		singleStepCompleted = nk_false;
		// init program counter pc
		pc=-1;

		writeConsole("Beginning Run\n");

	}
	for(;;) {
		pc = incrementPc(pc);
		if (pc == -1) break;
		// get the current opcode length
		int spaceIndex;
		for (spaceIndex = pc; codeText[spaceIndex] != '\0' && codeText[spaceIndex] != ' ' && codeText[spaceIndex] != '\n';++spaceIndex);
		// no opcode exceeds 6 characters in length
		if (spaceIndex - pc > 6) {
			// ignore labels
			// TODO: disallow malformed labels
			if (codeText[spaceIndex-1] != ':') {
				printf("Error: unrecognized opcode at position %d\n",pc);
			}
			else {
				printf("Ignoring label at position %d\n",pc);
			}
			pc = spaceIndex;
			continue;
		}
		else {
			// get the current opcode and check that its valid
			char opcode[7];
			*opcode = '\0';
			strncat(opcode,codeText+pc,spaceIndex-pc);
			printf("pc = %d opcode = %s\n",pc,opcode);
			curOpcode = opcodeStrToInt(opcode);
			if (curOpcode == -1) {
				// ignore labels
				// TODO: disallow malformed labels
				if (codeText[spaceIndex-1] != ':') {
					printf("Error: unrecognized opcode %s\n",opcode);
				}
				else {
					printf("Ignoring label %s\n",opcode);
				}
				pc = spaceIndex;
				continue;
			}
			else {
				// get the current arguments and check that they are valid and match opcode expected args
				int nextPc = spaceIndex;
				bool validArgs = opcodeParseArgs(&nextPc);
				printf("args = %s, %s, %s\n",curOpcodeArg0,curOpcodeArg1,curOpcodeArg2);
				if (!validArgs) {
					break;
				}
				else {
					// run the command corresponding to the current opcode
					switch(curOpcode) {
					// TODO: operate on int registers directly, not the GUI's int strings
					// arithmetic and logic
					// TODO: overflow vs no overflow (sign)
					case ADD:
						sprintf(registers[registerStrToInt(curOpcodeArg0)],"%d",atoi(registers[registerStrToInt(curOpcodeArg1)])+atoi(registers[registerStrToInt(curOpcodeArg2)]));
						break;
					case ADDU:
						sprintf(registers[registerStrToInt(curOpcodeArg0)],"%d",atoi(registers[registerStrToInt(curOpcodeArg1)])+atoi(registers[registerStrToInt(curOpcodeArg2)]));
						break;
					case ADDI:
						sprintf(registers[registerStrToInt(curOpcodeArg0)],"%d",atoi(registers[registerStrToInt(curOpcodeArg1)])+atoi(curOpcodeArg2));
						break;
					case ADDIU:
						sprintf(registers[registerStrToInt(curOpcodeArg0)],"%d",atoi(registers[registerStrToInt(curOpcodeArg1)])+atoi(curOpcodeArg2));
						break;
					case SUB:
						sprintf(registers[registerStrToInt(curOpcodeArg0)],"%d",atoi(registers[registerStrToInt(curOpcodeArg1)])-atoi(registers[registerStrToInt(curOpcodeArg2)]));
						break;
					case SUBU:
						sprintf(registers[registerStrToInt(curOpcodeArg0)],"%d",atoi(registers[registerStrToInt(curOpcodeArg1)])-atoi(registers[registerStrToInt(curOpcodeArg2)]));
						break;
					case AND:
						sprintf(registers[registerStrToInt(curOpcodeArg0)],"%d",atoi(registers[registerStrToInt(curOpcodeArg1)])&atoi(registers[registerStrToInt(curOpcodeArg2)]));
						break;
					case ANDI:
						sprintf(registers[registerStrToInt(curOpcodeArg0)],"%d",atoi(registers[registerStrToInt(curOpcodeArg1)])&atoi(registers[registerStrToInt(curOpcodeArg2)]));
						break;
					// TODO: div, mul -> require mfhi & mflo
					case NOR:
						sprintf(registers[registerStrToInt(curOpcodeArg0)],"%d",~(atoi(registers[registerStrToInt(curOpcodeArg1)])|atoi(registers[registerStrToInt(curOpcodeArg2)])));
						break;
					case OR:
						sprintf(registers[registerStrToInt(curOpcodeArg0)],"%d",atoi(registers[registerStrToInt(curOpcodeArg1)])|atoi(registers[registerStrToInt(curOpcodeArg2)]));
						break;
					case ORI:
						sprintf(registers[registerStrToInt(curOpcodeArg0)],"%d",atoi(registers[registerStrToInt(curOpcodeArg1)])|atoi(curOpcodeArg2));
						break;
					// branching
					// TODO: allow branch offset in addition to label
					case BEQ:
						if (strcmp(registers[registerStrToInt(curOpcodeArg0)],registers[registerStrToInt(curOpcodeArg1)]) == 0) {
							nextPc = findLabel(curOpcodeArg2);
						}
						break;
					case BGTZ:
						if (atoi(registers[registerStrToInt(curOpcodeArg0)]) > 0) {
							nextPc = findLabel(curOpcodeArg2);
						}
						break;
					case BLEZ:
						if (atoi(registers[registerStrToInt(curOpcodeArg0)]) <= 0) {
							nextPc = findLabel(curOpcodeArg2);
						}
						break;
					case BNE:
						if (strcmp(registers[registerStrToInt(curOpcodeArg0)],registers[registerStrToInt(curOpcodeArg1)]) != 0) {
							nextPc = findLabel(curOpcodeArg2);
						}
						break;
					// jumping
					case J:
						nextPc = findLabel(curOpcodeArg0);
						break;
					case JAL:
						sprintf(registers[31],"%d",pc);
						nextPc = findLabel(curOpcodeArg0);
						break;
					case JALR:
						sprintf(registers[31],"%d",pc);
						nextPc = atoi(registers[registerStrToInt(curOpcodeArg0)]);
						break;
					case JR:
						nextPc = atoi(registers[registerStrToInt(curOpcodeArg0)]);
						break;
					}
				}
				pc = nextPc;
			}
		}
		if (codeText[pc] == '\0') break;
		if (singleStepMode) return;
	}
	singleStepCompleted = nk_true;
	writeConsole("Run Completed\n");
}

/**
 * check and resolve custom hotkeys
 * @param in: the current frame's input
 */
void handleHotKeys(const struct nk_input *in) {
	for (int i = 0; i < NK_KEY_MAX; ++i) {
		if (i == NK_KEY_SAVE && nk_input_is_key_pressed(in, (enum nk_keys)i)) {
			if (in->keyboard.keys[NK_KEY_SHIFT].down)
				saveFileDataAs();
			else
				saveFileData();
		}
		else if (i == NK_KEY_OPEN && nk_input_is_key_pressed(in, (enum nk_keys)i)) {
			loadFileData();
		}
		else if (i == NK_KEY_RUN && nk_input_is_key_pressed(in, (enum nk_keys)i)) {
			runSimulation();
		}
	}
}

/**
 * infinite main loop for our program; runs the nuklear GUI as well as all program logic
 * @param nkcPointer: void* pointer to our nuklear nkc struct
 */
void mainLoop(void* nkcPointer){
    struct nk_context *ctx = ((struct nkc*)nkcPointer)->ctx;

    // reusable vars
    struct nk_rect bounds;
    struct nk_rect menubarBounds;
    struct nk_rect menubarOpenMenuBounds = menubarBounds;
    const struct nk_input *in = &ctx->input;
    int curHeight = 0;

    // poll events
    union nkc_event e = nkc_poll_events((struct nkc*)nkcPointer);
    if( (e.type == NKC_EWINDOW) && (e.window.param == NKC_EQUIT) )
        nkc_stop_main_loop((struct nkc*)nkcPointer);

    // hotkeys
    handleHotKeys(in);

    // layout constants
    int textEditWindowPaddingSize = 22;  // number of pixels needed to prevent an unneeded scrollbar from forming in a textedit window

    // menubar window
    int menubarHeight = 30 + fontPaddingExtra;
    int window_flags = 0;
    if (nk_begin(ctx, "mainMenu", nk_rect(0,curHeight,screenWidth,menubarHeight), window_flags)) {
        menubarBounds = nk_window_get_bounds(ctx);
    	nk_menubar_begin(ctx);
		nk_layout_row_begin(ctx, NK_STATIC, 25, 5);
		nk_layout_row_push(ctx, 45);
		if (nk_menu_begin_label(ctx, "File", NK_TEXT_LEFT, nk_vec2(205, 125))) {
			menubarOpenMenuBounds = nk_window_get_bounds(ctx);
			nk_layout_row_dynamic(ctx, 25, 1);
			if (nk_menu_item_label(ctx, "Save                 Ctrl+S", NK_TEXT_LEFT))
				saveFileData();
			if (nk_menu_item_label(ctx, "Save As        Ctrl+Shift+S", NK_TEXT_LEFT))
				saveFileDataAs();
			if (nk_menu_item_label(ctx, "Open                 Ctrl+O", NK_TEXT_LEFT)) {
				loadFileData();
			}
			if (nk_menu_item_label(ctx, "Run                  Ctrl+R", NK_TEXT_LEFT)) {
				runSimulation();
			}
			nk_menu_end(ctx);
		}

		nk_layout_row_push(ctx, 170);
		if (nk_menu_begin_label(ctx, "Options", NK_TEXT_LEFT, nk_vec2(205, 65))) {
			menubarOpenMenuBounds = nk_window_get_bounds(ctx);
			nk_layout_row_dynamic(ctx, 25, 1);
			nk_checkbox_label(ctx, "Zero Registers On Run", &shouldZeroRegistersOnRun);
			nk_checkbox_label(ctx, "Single Step Mode", &singleStepMode);
			nk_menu_end(ctx);
		}

		nk_layout_row_push(ctx, screenWidth-45-170-40);
		// add an asterisk to fileName when the file contents have been modified
		modifiedFileName[0] = strcmp(codeText,codeTextPrev)==0 ? '\0' : '*';
		modifiedFileName[1]='\0';
		strcat(modifiedFileName,curFileName);
		nk_label(ctx, curFileName[0]=='\0' ? "Untitled" : modifiedFileName, NK_TEXT_LEFT);
		nk_menubar_end(ctx);
		nk_end(ctx);
    }
    curHeight += menubarHeight;
    // register window
    window_flags = NK_WINDOW_BORDER | NK_WINDOW_BACKGROUND;
    if (nk_begin(ctx, "registers", nk_rect(0,curHeight,220,screenHeight - curHeight), window_flags)) {
    	nk_layout_row_dynamic(ctx, 25, 2);
    	nk_label(ctx, "Int Register", NK_TEXT_LEFT);
    	nk_label(ctx, "Value", NK_TEXT_LEFT);
    	for (int i = 0; i < NUMREGISTERS; ++i) {
			nk_layout_row_dynamic(ctx, 25, 2);
			checkSetInvalidRegisterContents(i,ctx);
			// tooltip
			bounds = nk_widget_bounds(ctx);
			if (nk_input_is_mouse_hovering_rect(in, bounds) && !(nk_input_is_mouse_hovering_rect(in,menubarOpenMenuBounds) || nk_input_is_mouse_hovering_rect(in, menubarBounds))) {
				nk_tooltip(ctx,registerTips[i]);
			}
			nk_label(ctx, registerNames[i], NK_TEXT_LEFT);
			// for edit modes, see https:// github.com/vurtun/nuklear/blob/181cfd86c47ae83eceabaf4e640587b844e613b6/src/nuklear.h#L3132
			nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD | NK_EDIT_READ_ONLY *(i==0), registers[i], sizeof(registers[i]), nk_filter_decimal);
			// keep the cursor from passing the end of the register field (purely serves as a graphical improvement)
			if (ctx->current->edit.cursor > REGISTERLEN-1)
				ctx->current->edit.cursor = REGISTERLEN-1;
			clearRegisterStyle(ctx);
    	}
    	nk_end(ctx);
    }

    // code edit window
    window_flags = NK_WINDOW_BORDER;
    if (nk_begin(ctx, "code", nk_rect(220,curHeight,screenWidth-220,screenHeight-curHeight-200+19), window_flags)) {
    	if (loadedFile) {
    		// reset selection on file load to avoid potential out of bounds errors
    		ctx->current->edit.sel_start = ctx->current->edit.sel_end = 0;
    		loadedFile = false;
    	}
    	nk_layout_row_dynamic(ctx, screenHeight-curHeight-200-textEditWindowPaddingSize, 1);
    	nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX, codeText, sizeof(codeText), nk_filter_default);
    	// show line and char number
    	nk_layout_row_dynamic(ctx, 15, 2);
    	char cursorPosStr[100];
    	sprintf(cursorPosStr,"%d:%d",1+lineCountTo(ctx->current->edit.cursor),lineCharCount(ctx->current->edit.cursor));
    	nk_label(ctx,cursorPosStr,NK_TEXT_LEFT);

    	// single step line indicator
    	if (singleStepMode && !singleStepCompleted) {
    		int triangleWidth = 10, triangleHeight = 10;
    		int ix = 220+5;
    		int iy = curHeight+12 + 18*lineCountTo(codeText[pc] == '\n' ? pc+1 : pc) - ctx->current->edit.scrollbar.y;
    		int minY = curHeight + triangleHeight + 1;
    		int maxY = screenHeight-curHeight-200+19 - triangleHeight;
    		nk_fill_triangle(&ctx->current->buffer, ix,min(max(iy,minY),maxY),ix,min(max(iy+triangleHeight,minY),maxY),ix+triangleWidth,min(max(iy+triangleHeight/2,minY),maxY), nk_red);
    	}
    	nk_end(ctx);
    }
    curHeight += screenHeight-curHeight-200+19;

    // console window
    if (nk_begin(ctx, "console", nk_rect(220,curHeight,screenWidth-220,200-19), window_flags)) {
		nk_layout_row_dynamic(ctx, 200-19-textEditWindowPaddingSize, 1);
		nk_edit_string_zero_terminated(ctx, NK_EDIT_SELECTABLE | NK_EDIT_MULTILINE | NK_EDIT_CLIPBOARD, consoleText, sizeof(consoleText), nk_filter_default);
		nk_end(ctx);
	}

	nkc_render((struct nkc*)nkcPointer, nk_rgb(60,60,60) );
}

/**
 * load settings from the config file settings.cfg, creating the file if not present
 */
void loadConfig() {
	FILE *file;
	if((file = fopen("settings.cfg","r"))==NULL) {
		// config file does not exist; create it and populate it with default values
		file = fopen("config.txt","a+");
		fprintf(file, "resolution: %d %d\n", 1280,720);
		fprintf(file, "zero registers on run: 1\n");
		rewind(file);
	}
	fscanf(file,"%*s%d %d\n %*s%*s%*s%*s%d",&screenWidth, &screenHeight, &shouldZeroRegistersOnRun);
	fclose(file);
}

int main(){
	// manual vbuf for output flushing on windows to fix Eclipse failing to display output
	#ifdef _WIN32
		setvbuf(stdout, NULL, _IONBF, 0);
		setvbuf(stderr, NULL, _IONBF, 0);
	#endif

	// load preferences from settings file
	loadConfig();

	registers[0][0] = '0';
	registers[0][1] = '\0';
	zeroRegisters();
	curFileName[0]='\0';

    struct nkc nkcx;
    if( nkc_init(&nkcx, "MIPS Simulator", screenWidth,screenHeight, NKC_WIN_NORMAL) ) {
    	// load font
    	struct nk_user_font *font = nkc_load_font_file(&nkcx, "ProggyClean.ttf", fontSize,0);
    	nkcx.ctx->style.font = font;
    	nkc_set_main_loop(&nkcx, mainLoop,(void*)&nkcx);
    }
    else
        printf("Can't init NKC\n");
    nkc_shutdown(&nkcx);
    return 0;
}
