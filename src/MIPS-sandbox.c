#define NKC_IMPLEMENTATION
#define NKC_DISABLE_DEFAULT_FONT
#include "../nuklear_cross/nuklear_cross.h"
#include <strings.h>

#define NOC_FILE_DIALOG_IMPLEMENTATION
#ifdef _WIN32
//tweak the formatting for windows
#define fontSize 19
#define fontPaddingExtra 0
#define NOC_FILE_DIALOG_WIN32
#else
//platform independent case independent string comparison
#define stricmp strcasecmp
//tweak the formatting for non-windows
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
//inttypes allows us to guarantee n-bit ints
#include <inttypes.h>
#include <stdbool.h>
#include "registers.h"
#include "opcodes.h"
#include "linkedList.h"

extern int screenWidth;
extern int screenHeight;

bool loadedFile = false;
static int check = nk_false;
//signed 32 bit max val = 2,147,483,647, or 10 digits; need 3 additional digits for optional - sign and \0
char registers[NUMREGISTERS][REGISTERLEN];

//for now, use a fixed size code window
#define NUMCODECHARS 999
char codeText[NUMCODECHARS];
char codeTextPrev[NUMCODECHARS];
char curFileName[260];
char modifiedFileName[261];
char consoleText[999];
char curOpcode[7];

/**
 * check if the specified register contents are invalid; if so, set register style to force red highlight
 * @param regNum: the register number to check
 * @param ctx: the current nuklear context
 * @returns: whether the specified register is invalid (true) or not (false)
 */
bool checkSetInvalidRegisterContents(int regNum, struct nk_context *ctx) {
	for (int i = 0; i < REGISTERLEN; ++i) {
		if (!((i == 0 && registers[regNum][i] == '-') || isdigit(registers[regNum][i]) || registers[regNum][i] == '\0')) {
			ctx->style.edit.normal.data.color =nk_rgb(255,0,0);
			ctx->style.edit.active.data.color =nk_rgb(255,0,0);
			ctx->style.edit.hover.data.color =nk_rgb(255,0,0);
			return true;
		}
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
	//copy file data into code text buffer, stripping windows \r's as we go
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
	//selected file; save to it
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
	//disallow saving unchanged file unless its empty
	if (strlen(codeText) > 0 && strcmp(codeText,codeTextPrev)==0) {
		return false;
	}
	//we are working on an existing file, so save to it
	FILE *fp = fopen(curFileName, "w");
	if (!fp)
		exitError("Error: unable to open input file");
	fputs(codeText, fp);
	fclose(fp);
	strcpy(codeTextPrev,codeText);
	return true;
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
	}
}

/**
 * set all registers to the value 0. This would not be guaranteed in practice, but should make fresh runs visually cleaner
 */
void clearRegisters() {
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
	//exit on empty code or initial value of null terminator
	if (codeText[0] == '\0' || (pc != -1 && codeText[pc] == '\0')) return -1;
	int startPc = pc;
	for (++pc; pc != startPc; ++pc) {
		//ignore whitespace
		for(; codeText[pc] != '\0' && (codeText[pc] == ' ' || codeText[pc] == '\n'); ++pc);
		if (codeText[pc] == '\0') {
			return -1;
		}
		//ignore comment lines
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
 * convert the string representation of curOpcode to its integer enum value
 * @returns: the integer value corresponding to the enum for curOpcode, or -1 if the opcode is not found
 */
int opcodeStrToInt() {
	for (int i = 0; i < NUMOPCODES; ++i) {
		if (stricmp(opcodeNames[i],curOpcode) == 0) {
			return i;
		}
	}
	return -1;
}

/**
 * execute the simulation on the current file. This is a slow temporary solution that iterates through code lines
 */
void runSimulation() {
	clearRegisters();

	writeConsole("Beginning Run\n");
	//init program counter pc
	int pc=-1;
	for(;;) {
		pc = incrementPc(pc);
		if (pc == -1) break;
		int spaceIndex;
		for (spaceIndex = pc; codeText[spaceIndex] != '\0' &&  codeText[spaceIndex] != ' ' && codeText[spaceIndex] != '\n';++spaceIndex);
		if (spaceIndex - pc > 6) {
			printf("Error: unrecognized opcode at position %d\n",pc);
		}
		else {
			*curOpcode = '\0';
			strncat(curOpcode,codeText+pc,spaceIndex-pc);
			printf("pc = %d opcode = %s\n",pc,curOpcode);
			int opcode = opcodeStrToInt();
			if (opcode == -1) {
				printf("Error: unrecognized opcode %s\n",curOpcode);
				break;
			}
			//char* args =
			switch(opcodeStrToInt()) {
			case ADDI:
				break;
			}
		}
		pc = spaceIndex;

	}
	writeConsole("Run Completed\n");
}

/**
 * infinite main loop for our program; runs the nuklear GUI as well as all program logic
 * @param nkcPointer: void* pointer to our nuklear nkc struct
 */
void mainLoop(void* nkcPointer){
    struct nk_context *ctx = ((struct nkc*)nkcPointer)->ctx;

    //reusable vars
    struct nk_rect bounds;
    struct nk_rect menubarBounds;
    struct nk_rect menubarOpenMenuBounds = menubarBounds;
    const struct nk_input *in = &ctx->input;
    int curHeight = 0;

    //poll events
    union nkc_event e = nkc_poll_events((struct nkc*)nkcPointer);
    if( (e.type == NKC_EWINDOW) && (e.window.param == NKC_EQUIT) )
        nkc_stop_main_loop((struct nkc*)nkcPointer);

    //hotkeys
    handleHotKeys(in);

    //layout constants
    int textEditWindowPaddingSize = 22; //number of pixels needed to prevent an unneeded scrollbar from forming in a textedit window

    //menubar window
    int menubarHeight = 30 + fontPaddingExtra;
    int window_flags = 0;
    if (nk_begin(ctx, "mainMenu", nk_rect(0,curHeight,screenWidth,menubarHeight), window_flags)) {
        menubarBounds = nk_window_get_bounds(ctx);
    	nk_menubar_begin(ctx);
		nk_layout_row_begin(ctx, NK_STATIC, 25, 5);
		nk_layout_row_push(ctx, 45);
		if (nk_menu_begin_label(ctx, "File", NK_TEXT_LEFT, nk_vec2(120, 125))) {
			menubarOpenMenuBounds = nk_window_get_bounds(ctx);
			nk_layout_row_dynamic(ctx, 25, 1);
			if (nk_menu_item_label(ctx, "Save", NK_TEXT_LEFT))
				saveFileData();
			if (nk_menu_item_label(ctx, "Save As", NK_TEXT_LEFT))
				saveFileDataAs();
			if (nk_menu_item_label(ctx, "Load", NK_TEXT_LEFT)) {
				loadFileData();
			}
			if (nk_menu_item_label(ctx, "Run", NK_TEXT_LEFT)) {
				runSimulation();
			}
			nk_menu_end(ctx);
		}

		nk_layout_row_push(ctx, 170);
		if (nk_menu_begin_label(ctx, "Options", NK_TEXT_LEFT, nk_vec2(120, 35))) {
			menubarOpenMenuBounds = nk_window_get_bounds(ctx);
			nk_layout_row_dynamic(ctx, 25, 1);
			nk_checkbox_label(ctx, "some tickbox", &check);
			nk_menu_end(ctx);
		}

		nk_layout_row_push(ctx, screenWidth-45-170-40);
		//add an asterisk to fileName when the file contents have been modified
		modifiedFileName[0] = strcmp(codeText,codeTextPrev)==0 ? '\0' : '*';
		modifiedFileName[1]='\0';
		strcat(modifiedFileName,curFileName);
		nk_label(ctx, curFileName[0]=='\0' ? "Untitled" : modifiedFileName, NK_TEXT_LEFT);
		nk_menubar_end(ctx);
		nk_end(ctx);
    }
    curHeight += menubarHeight;
    //register window
    window_flags = NK_WINDOW_BORDER | NK_WINDOW_BACKGROUND;
    if (nk_begin(ctx, "registers", nk_rect(0,curHeight,220,screenHeight - curHeight), window_flags)) {
    	nk_layout_row_dynamic(ctx, 25, 2);
    	nk_label(ctx, "Int Register", NK_TEXT_LEFT);
    	nk_label(ctx, "Value", NK_TEXT_LEFT);
    	for (int i = 0; i < NUMREGISTERS; ++i) {
			nk_layout_row_dynamic(ctx, 25, 2);
			checkSetInvalidRegisterContents(i,ctx);
			//tooltip
			bounds = nk_widget_bounds(ctx);
			if (nk_input_is_mouse_hovering_rect(in, bounds) && !(nk_input_is_mouse_hovering_rect(in,menubarOpenMenuBounds) || nk_input_is_mouse_hovering_rect(in, menubarBounds))) {
				nk_tooltip(ctx,registerTips[i]);
			}
			nk_label(ctx, registerNames[i], NK_TEXT_LEFT);
			//for edit modes, see https://github.com/vurtun/nuklear/blob/181cfd86c47ae83eceabaf4e640587b844e613b6/src/nuklear.h#L3132
			nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD | NK_EDIT_READ_ONLY *(i==0), registers[i], sizeof(registers[i]), nk_filter_decimal);
			//keep the cursor from passing the end of the register field (purely serves as a graphical improvement)
			if (ctx->current->edit.cursor > REGISTERLEN-1)
				ctx->current->edit.cursor = REGISTERLEN-1;
			clearRegisterStyle(ctx);
    	}
    	nk_end(ctx);
    }

    //code edit window
    window_flags = NK_WINDOW_BORDER;
    if (nk_begin(ctx, "code", nk_rect(220,curHeight,screenWidth-220,screenHeight-curHeight-200+19), window_flags)) {
    	if (loadedFile) {
    		//reset selection on file load to avoid potential out of bounds errors
    		ctx->current->edit.sel_start = ctx->current->edit.sel_end = 0;
    		loadedFile = false;
    	}
    	nk_layout_row_dynamic(ctx, screenHeight-curHeight-200-textEditWindowPaddingSize, 1);
    	nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX, codeText, sizeof(codeText), nk_filter_default);
    	//show line and char number
    	nk_layout_row_dynamic(ctx, 15, 2);
    	char cursorPosStr[100];
    	sprintf(cursorPosStr,"%d:%d",1+lineCountTo(ctx->current->edit.cursor),lineCharCount(ctx->current->edit.cursor));
    	nk_label(ctx,cursorPosStr,NK_TEXT_LEFT);
    	nk_end(ctx);
    }
    curHeight += screenHeight-curHeight-200+19;

    //console window
    if (nk_begin(ctx, "console", nk_rect(220,curHeight,screenWidth-220,200-19), window_flags)) {
		nk_layout_row_dynamic(ctx, 200-19-textEditWindowPaddingSize, 1);
		nk_edit_string_zero_terminated(ctx, NK_EDIT_SELECTABLE | NK_EDIT_MULTILINE | NK_EDIT_CLIPBOARD, consoleText, sizeof(consoleText), nk_filter_default);
		nk_end(ctx);
	}

	nkc_render((struct nkc*)nkcPointer, nk_rgb(60,60,60) );
}

int main(){
	//manual vbuf for output flushing on windows to fix Eclipse failing to display output
	#ifdef _WIN32
		setvbuf(stdout, NULL, _IONBF, 0);
		setvbuf(stderr, NULL, _IONBF, 0);
	#endif
	registers[0][0] = '0';
	registers[0][1] = '\0';
	curFileName[0]='\0';

    struct nkc nkcx;
    screenWidth = 1280;
    screenHeight = 720;
    if( nkc_init(&nkcx, "MIPS Simulator", screenWidth,screenHeight, NKC_WIN_NORMAL) ) {
    	//load font
    	struct nk_user_font *font = nkc_load_font_file(&nkcx, "ProggyClean.ttf", fontSize,0);
    	nkcx.ctx->style.font = font;
    	nkc_set_main_loop(&nkcx, mainLoop,(void*)&nkcx);
    }
    else
        printf("Can't init NKC\n");
    nkc_shutdown(&nkcx);
    return 0;
}
