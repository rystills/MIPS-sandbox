#define NKC_IMPLEMENTATION
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
//#define NKC_DISABLE_DEFAULT_FONT
#include "../nuklear_cross/nuklear_cross.h"

#define NOC_FILE_DIALOG_IMPLEMENTATION
#ifdef _WIN32
#define fontSize 19
#define NOC_FILE_DIALOG_WIN32
#else
#define fontSize 13
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
#include "linkedList.h"

#define NUMREGISTERS 32
#define REGISTERLEN 12

extern int screenWidth;
extern int screenHeight;

static int check = nk_false;
//signed 32 bit max val = 2,147,483,647, or 10 digits; need 3 additional digits for optional - sign and \0
char registers[NUMREGISTERS][REGISTERLEN];

char codeText[999];
char codeTextPrev[999];
char curFileName[260];
char modifiedFileName[261];
char consoleText[999];

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

    //poll events
    union nkc_event e = nkc_poll_events((struct nkc*)nkcPointer);
    if( (e.type == NKC_EWINDOW) && (e.window.param == NKC_EQUIT) )
        nkc_stop_main_loop((struct nkc*)nkcPointer);

    //hotkeys
    handleHotKeys(in);

    //layout constants
    int windowPaddingSize = 22; //number of pixels needed to prevent an unneeded scrollbar from forming

    //menubar window
    int menubarHeight = 30;
    int window_flags = 0;
    if (nk_begin(ctx, "mainMenu", nk_rect(0,0,screenWidth,menubarHeight), window_flags)) {
        menubarBounds = nk_window_get_bounds(ctx);
    	nk_menubar_begin(ctx);
		nk_layout_row_begin(ctx, NK_STATIC, 25, 5);
		nk_layout_row_push(ctx, 45);
		if (nk_menu_begin_label(ctx, "File", NK_TEXT_LEFT, nk_vec2(120, 95))) {
			menubarOpenMenuBounds = nk_window_get_bounds(ctx);
			nk_layout_row_dynamic(ctx, 25, 1);
			if (nk_menu_item_label(ctx, "Save", NK_TEXT_LEFT))
				saveFileData();
			if (nk_menu_item_label(ctx, "Save As", NK_TEXT_LEFT))
				saveFileDataAs();
			if (nk_menu_item_label(ctx, "Load", NK_TEXT_LEFT)) {
				loadFileData();
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
    //register window
    window_flags = NK_WINDOW_BORDER | NK_WINDOW_BACKGROUND;
    if (nk_begin(ctx, "registers", nk_rect(0,menubarHeight,220,screenHeight - menubarHeight), window_flags)) {
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
    if (nk_begin(ctx, "code", nk_rect(220,menubarHeight,screenWidth-220,screenHeight-menubarHeight-200), window_flags)) {
    	nk_layout_row_dynamic(ctx, screenHeight-menubarHeight-200-windowPaddingSize, 1);
    	nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX, codeText, sizeof(codeText), nk_filter_default);
    	nk_end(ctx);
    }

    //console window
    if (nk_begin(ctx, "console", nk_rect(220,screenHeight-200,screenWidth-220,200), window_flags)) {
		nk_layout_row_dynamic(ctx, 200-windowPaddingSize, 1);
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
