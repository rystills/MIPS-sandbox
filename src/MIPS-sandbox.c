#define NKC_IMPLEMENTATION
#include "../nuklear_cross/nuklear_cross.h"
#include <stdio.h>
//inttypes allows us to guarantee n-bit ints
#include <inttypes.h>
#define NUMREGISTERS 32

static int check = nk_false;
//signed 32 bit max val = 2,147,483,647, or 10 digits; need 3 additional digits for optional - sign and \0
char registers [NUMREGISTERS][12];
void mainLoop(void* nkcPointer){
    struct nk_context *ctx = ((struct nkc*)nkcPointer)->ctx;

    union nkc_event e = nkc_poll_events((struct nkc*)nkcPointer);
    if( (e.type == NKC_EWINDOW) && (e.window.param == NKC_EQUIT) )
        nkc_stop_main_loop((struct nkc*)nkcPointer);

    /* Nuklear GUI code */
    int window_flags = 0;
    //todo: find a more elegant way to fit the menubar horizontally than 9999999
    //nk_style_push_color(ctx, &s->window.background, nk_rgba(255,0,0,255));
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

    window_flags = NK_WINDOW_BORDER;
    if (nk_begin(ctx, "registers", nk_rect(0,36,640,480), window_flags)) {
    	for (int i = 0; i < NUMREGISTERS; ++i) {
			nk_layout_row_dynamic(ctx, 25, 1);
			nk_edit_string_zero_terminated (ctx, NK_EDIT_FIELD, registers[i], sizeof(registers[i]), nk_filter_default);
    	}
    	nk_end(ctx);
    }

    /* End Nuklear GUI */
    nkc_render((struct nkc*)nkcPointer, nk_rgb(60,60,60) );
}

int main(){
	//manual vbuf for output flushing on windows to fix Eclipse failing to display output
	#ifdef _WIN32
		setvbuf(stdout, NULL, _IONBF, 0);
		setvbuf(stderr, NULL, _IONBF, 0);
	#endif

    struct nkc nkcx;
    if( nkc_init(&nkcx, "Nuklear+ Example", 1280, 720, NKC_WIN_NORMAL) )
        nkc_set_main_loop(&nkcx, mainLoop,(void*)&nkcx);
    else
        printf("Can't init NKC\n");
    nkc_shutdown(&nkcx);
    return 0;
}
