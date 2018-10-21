#define NKC_IMPLEMENTATION
#include "../nuklear_cross/nuklear_cross.h"
#include <stdio.h>

enum radioOptions {
    EASY,
    HARD
};

struct my_nkc_app {
    struct nkc* nkcHandle;

    /* some user data */
    float value;
    enum radioOptions op;
};

static int show_menu = nk_true;
static int show_app_about = nk_false;


void mainLoop(void* loopArg){
    struct my_nkc_app* myapp = (struct my_nkc_app*)loopArg;
    struct nk_context *ctx = nkc_get_ctx(myapp->nkcHandle);

    union nkc_event e = nkc_poll_events(myapp->nkcHandle);
    if( (e.type == NKC_EWINDOW) && (e.window.param == NKC_EQUIT) ){
        nkc_stop_main_loop(myapp->nkcHandle);
    }

    /* Nuklear GUI code */
    int window_flags = 0;
    if (nk_begin(ctx, "mainMenu", nk_rect(0,0,1280,34), window_flags)) {
        nk_menubar_begin(ctx);
        			nk_layout_row_begin(ctx, NK_STATIC, 25, 5);
                    nk_layout_row_push(ctx, 45);
                    if (nk_menu_begin_label(ctx, "File", NK_TEXT_LEFT, nk_vec2(120, 65)))
                    {
                        static size_t prog = 40;
                        static int slider = 10;
                        static int check = nk_true;
                        nk_layout_row_dynamic(ctx, 25, 1);
                        if (nk_menu_item_label(ctx, "Save", NK_TEXT_LEFT))
                            show_menu = nk_false;
                        if (nk_menu_item_label(ctx, "Load", NK_TEXT_LEFT))
                            show_app_about = nk_true;
                        nk_progress(ctx, &prog, 100, NK_MODIFIABLE);
                        nk_slider_int(ctx, 0, &slider, 16, 1);
                        nk_checkbox_label(ctx, "check", &check);
                        nk_menu_end(ctx);
                    }
		nk_menubar_end(ctx);
    }
    nk_end(ctx);
    /* End Nuklear GUI */

    nkc_render(myapp->nkcHandle, nk_rgb(40,40,40) );
}

int main(){
	//manual vbuf for output flushing on windows to fix Eclipse failing to display output
	#ifdef _WIN32
		setvbuf(stdout, NULL, _IONBF, 0);
		setvbuf(stderr, NULL, _IONBF, 0);
	#endif

	struct my_nkc_app myapp;
    struct nkc nkcx; /* Allocate memory for Nuklear+ handle */
    myapp.nkcHandle = &nkcx;
    /* init some user data */
    myapp.value = 0.4;
    myapp.op = HARD;

    if( nkc_init( myapp.nkcHandle, "Nuklear+ Example", 1280, 720, NKC_WIN_NORMAL) ){
        printf("Successfull init. Starting 'infinite' main loop...\n");
        nkc_set_main_loop(myapp.nkcHandle, mainLoop, (void*)&myapp );
    } else {
        printf("Can't init NKC\n");
    }
    printf("Value after exit = %f\n", myapp.value);
    nkc_shutdown( myapp.nkcHandle );
    return 0;
}
