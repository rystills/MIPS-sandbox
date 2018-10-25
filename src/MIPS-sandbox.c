#define NKC_IMPLEMENTATION
#include "../nuklear_cross/nuklear_cross.h"
#include <stdio.h>
//inttypes allows us to guarantee n-bit ints
#include <inttypes.h>
#include <stdbool.h>
#define NUMREGISTERS 32
#define REGISTERLEN 12

static int check = nk_false;
//signed 32 bit max val = 2,147,483,647, or 10 digits; need 3 additional digits for optional - sign and \0
char registers [NUMREGISTERS][REGISTERLEN];
char registerNames [NUMREGISTERS][6] = {
		"$zero",
		"$at",
		"$v0",
		"$v1",
		"$a0",
		"$a1",
		"$a2",
		"$a3",
		"$t0",
		"$t1",
		"$t2",
		"$t3",
		"$t4",
		"$t5",
		"$t6",
		"$t7",
		"$s0",
		"$s1",
		"$s2",
		"$s3",
		"$s4",
		"$s5",
		"$s6",
		"$s7",
		"$t8",
		"$t9",
		"$k0",
		"$k1",
		"$gp",
		"$sp",
		"$fp",
		"$ra"
};

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
 * infinite main loop for our program; runs the nuklear GUI as well as all program logic
 * @param nkcPointer: void* pointer to our nuklear nkc struct
 */
void mainLoop(void* nkcPointer){
	printf("%d,%d\n",screenWidth,screenHeight);
    struct nk_context *ctx = ((struct nkc*)nkcPointer)->ctx;

    union nkc_event e = nkc_poll_events((struct nkc*)nkcPointer);
    if( (e.type == NKC_EWINDOW) && (e.window.param == NKC_EQUIT) )
        nkc_stop_main_loop((struct nkc*)nkcPointer);

    //todo: find a more elegant way to fit the menubar horizontally than 9999999
    //menubar window
    int window_flags = 0;
    if (nk_begin(ctx, "mainMenu", nk_rect(0,0,9999999,34), window_flags)) {
        nk_menubar_begin(ctx);
		nk_layout_row_begin(ctx, NK_STATIC, 25, 5);
		nk_layout_row_push(ctx, 45);
		if (nk_menu_begin_label(ctx, "File", NK_TEXT_LEFT, nk_vec2(120, 300))) {
			nk_layout_row_dynamic(ctx, 25, 1);
			if (nk_menu_item_label(ctx, "Save", NK_TEXT_LEFT))
				; //TODO: save file
			if (nk_menu_item_label(ctx, "Load", NK_TEXT_LEFT))
				; //TODO: load file
			nk_menu_end(ctx);
		}

		nk_layout_row_push(ctx, 45);
		if (nk_menu_begin_label(ctx, "Options", NK_TEXT_LEFT, nk_vec2(120, 300))) {
			nk_layout_row_dynamic(ctx, 25, 1);
			nk_checkbox_label(ctx, "some tickbox", &check);
			nk_menu_end(ctx);
		}
		nk_menubar_end(ctx);
		nk_end(ctx);
    }
    //register window
    window_flags = NK_WINDOW_BORDER | NK_WINDOW_BACKGROUND;
    if (nk_begin(ctx, "registers", nk_rect(0,34,220,1000), window_flags)) {
    	nk_layout_row_dynamic(ctx, 25, 2);
    	nk_label(ctx, "Integer Register", NK_TEXT_LEFT);
    	nk_label(ctx, "Value", NK_TEXT_LEFT);
    	for (int i = 0; i < NUMREGISTERS; ++i) {
			nk_layout_row_dynamic(ctx, 25, 2);
			checkSetInvalidRegisterContents(i,ctx);
			nk_label(ctx, registerNames[i], NK_TEXT_LEFT);
			nk_edit_string_zero_terminated(ctx, NK_EDIT_SIMPLE, registers[i], sizeof(registers[i]), nk_filter_decimal);
			clearRegisterStyle(ctx);
    	}
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

    struct nkc nkcx;
    screenWidth = 1280;
    screenHeight = 720;
    if( nkc_init(&nkcx, "MIPS Simulator", screenWidth,screenHeight, NKC_WIN_NORMAL) )
        nkc_set_main_loop(&nkcx, mainLoop,(void*)&nkcx);
    else
        printf("Can't init NKC\n");
    nkc_shutdown(&nkcx);
    return 0;
}
