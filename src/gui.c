#include "stdio.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "nuklear.h"
#include "../includes/nuklear_sdl_renderer.h"

#include "../includes/gui.h"
#include "../includes/display.h"

static struct nk_context *ctx = NULL;
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

static void chip8_gui_stack(struct nk_context *ctx);

void gui_close()
{
   nk_sdl_shutdown();
}

void gui_init()
{
   window = display_get_window();
   renderer = display_get_renderer();

   ctx = nk_sdl_init( window, renderer );

   // load fonts
   struct nk_font_atlas *atlas;
   struct nk_font_config config = nk_font_config(0);
   struct nk_font *font;

   nk_sdl_font_stash_begin(&atlas);
   font = nk_font_atlas_add_default(atlas, 13, &config);
   nk_sdl_font_stash_end();

   nk_style_set_font(ctx, &font->handle);
}

void gui_input_begin()
{
   nk_input_begin(ctx);
}

void gui_input_end()
{
   nk_input_end(ctx);
}

void gui_handle_event(SDL_Event *event)
{
   nk_sdl_handle_event(event);
}

void gui_create_and_update()
{
   chip8_gui_stack(ctx);

   SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
   SDL_RenderClear(renderer);

   display_update();
   
   nk_sdl_render(NK_ANTI_ALIASING_ON);
   
   SDL_RenderPresent(renderer);
}

// create gui window to display chip8 stack
static void chip8_gui_stack(struct nk_context *ctx)
{
   
   if ( nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250), NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
   {
      nk_layout_row_static(ctx, 30, 80, 1);
      if (nk_button_label(ctx, "press me"))
         printf("button pressed\n");
   }

   nk_end(ctx);
}
