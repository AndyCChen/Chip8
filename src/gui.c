#include "stdio.h"

#define NK_SDL_CLAMP_CLIP_RECT
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
#include "../includes/chip8.h"

static struct nk_context *ctx = NULL;

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

static int window_height = 0;
static int window_width = 0;

static struct nk_color RED;
static struct nk_color CYAN;

// gui widgets

static void widget_stack(float x_pos, float y_pos, float width, float height);
static void widget_memory(float x_pos, float y_pos, float width, float height);
static void widget_cpu_state(float x_pos, float y_pos, float width, float height);

void gui_close()
{
   nk_sdl_shutdown();
}

void gui_init()
{
   window = display_get_window();
   renderer = display_get_renderer();

   SDL_GetWindowSize(window, &window_width, &window_height);

   ctx = nk_sdl_init( window, renderer );

   // load fonts
   struct nk_font_atlas *atlas;
   struct nk_font_config config = nk_font_config(0);
   struct nk_font *font;

   nk_sdl_font_stash_begin(&atlas);
   font = nk_font_atlas_add_default(atlas, 13, &config);
   nk_sdl_font_stash_end();

   nk_style_set_font(ctx, &font->handle);

   // create nk colors
   RED = nk_rgb(247, 47, 47);
   CYAN = nk_rgb(44, 191, 191);
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

void gui_create_widgets()
{
   widget_stack(0, 0, GUI_STACK_WIDGET_W, (float) window_height);
   widget_memory(GUI_STACK_WIDGET_W, 0, GUI_MEMORY_WIDGET_W, (float) window_height);
   widget_cpu_state(GUI_STACK_WIDGET_W + GUI_MEMORY_WIDGET_W , 0, GUI_CPU_STATE_WIDGET_W, (float) window_height);
}

void gui_draw()
{
   nk_sdl_render(NK_ANTI_ALIASING_ON);
}

static void widget_stack(float x_pos, float y_pos, float width, float height)
{
   #define STACK_COUNT_LABEL_SIZE 3
   #define STACK_VALUE_LABEL_SIZE 5
   
   char stack_value_buffer[STACK_VALUE_LABEL_SIZE];
   char stack_level_buffer[STACK_COUNT_LABEL_SIZE];

   if ( nk_begin( ctx, "Stack", nk_rect(x_pos, y_pos, width, height), NK_WINDOW_BORDER|NK_WINDOW_TITLE|NK_WINDOW_MINIMIZABLE ) )
   {
      for (int stack_level = 0; stack_level < MAX_STACK_LEVEL; ++stack_level)
      {
         snprintf(stack_value_buffer, STACK_VALUE_LABEL_SIZE, "%04X", myChip8.stack[stack_level]);
         snprintf(stack_level_buffer, STACK_COUNT_LABEL_SIZE, "%d", stack_level);

         nk_layout_row_begin(ctx, NK_DYNAMIC, 15, 2);

         nk_layout_row_push(ctx, 0.4);
         nk_label_colored(ctx, stack_level_buffer, NK_TEXT_CENTERED, myChip8.sp == stack_level ? CYAN : RED );

         nk_layout_row_push(ctx, 0.6);
         nk_label(ctx, stack_value_buffer, NK_TEXT_LEFT);

         nk_layout_row_end(ctx);
      }
   }

   nk_end(ctx);

   #undef STACK_COUNT_LABEL_SIZE
   #undef STACK_VALUE_LABEL_SIZE
}

static void widget_memory(float x_pos, float y_pos, float width, float height)
{
   #define MEMORY_ADDRESS_LABEL_SIZE 5
   #define MEMORY_VALUE_LABEL_SIZE 3
   #define NUM_OF_COLS 9
   
   char memory_address_buffer[MEMORY_ADDRESS_LABEL_SIZE];
   char memory_value_buffer[MEMORY_VALUE_LABEL_SIZE];

   float widths[NUM_OF_COLS] = {40, 15, 15, 15, 15, 15, 15, 15, 15}; // widths for each of the 9 columns in a row

   if ( nk_begin( ctx, "Memory", nk_rect(x_pos, y_pos, width, height), NK_WINDOW_BORDER|NK_WINDOW_TITLE|NK_WINDOW_MINIMIZABLE ) )
   {
      nk_layout_row(ctx, NK_STATIC, 15, 9, widths);

      for (int address = 0; address < RAM_SIZE; address += 8)
      {
         snprintf(memory_address_buffer, MEMORY_ADDRESS_LABEL_SIZE, "%04X", address);

         nk_label_colored(ctx, memory_address_buffer, NK_TEXT_CENTERED, RED);

         for (int byte = 0; byte < 8; ++byte)
         {
            snprintf(memory_value_buffer, MEMORY_VALUE_LABEL_SIZE, "%02X", myChip8.ram[address + byte]);
            nk_label(ctx, memory_value_buffer, NK_TEXT_CENTERED);
         }
      }
   }

   nk_end(ctx);

   #undef MEMORY_ADDRESS_LABEL_SIZE
   #undef MEMORY_VALUE_LABEL_SIZE
   #undef NUM_OF_COLS
}

static void widget_cpu_state(float x_pos, float y_pos, float width, float height)
{
   #define V_REGISTER_LABEL_BUFFER_SIZE 4
   #define REGISTER_VALUE_BUFFER_SIZE 5

   char register_value_buffer[5];

   float widths[2] = {30, 35};
   float v_register_widths[4] = {30, 25, 30, 25};

   if ( nk_begin( ctx, "CPU State", nk_rect(x_pos, y_pos, width, height), NK_WINDOW_BORDER|NK_WINDOW_TITLE|NK_WINDOW_MINIMIZABLE ) )
   {
      nk_layout_row(ctx, NK_STATIC, 15, 2, widths);

      // program counter row
      snprintf(register_value_buffer, REGISTER_VALUE_BUFFER_SIZE, "%04X", myChip8.PC);
      nk_label_colored(ctx, "PC:", NK_TEXT_RIGHT, RED);
      nk_label(ctx, register_value_buffer, NK_TEXT_RIGHT);

      // address register I row
      snprintf(register_value_buffer, REGISTER_VALUE_BUFFER_SIZE, "%04X", myChip8.I);
      nk_label_colored(ctx, "I:", NK_TEXT_RIGHT, RED);
      nk_label(ctx, register_value_buffer, NK_TEXT_RIGHT);

      // stack pointer register row
      snprintf(register_value_buffer, REGISTER_VALUE_BUFFER_SIZE, "%02X", myChip8.sp);
      nk_label_colored(ctx, "SP:", NK_TEXT_RIGHT, RED);
      nk_label(ctx, register_value_buffer, NK_TEXT_RIGHT);

      nk_spacer(ctx);

      // register title
      nk_layout_row_static(ctx, 15, 87, 1);
      nk_label_colored(ctx, "V Registers", NK_TEXT_RIGHT, RED);

      // v register row
      nk_layout_row(ctx, NK_STATIC, 15, 4, v_register_widths);

      char v_register_label[V_REGISTER_LABEL_BUFFER_SIZE];

      for (int i = 0; i < V_REGISTERS; ++i)
      {
         snprintf(v_register_label, V_REGISTER_LABEL_BUFFER_SIZE, "V%01X:", i);
         snprintf(register_value_buffer, REGISTER_VALUE_BUFFER_SIZE, "%02X", myChip8.V[i]);

         nk_label_colored(ctx, v_register_label, NK_TEXT_RIGHT, RED);
         nk_label(ctx, register_value_buffer, NK_TEXT_CENTERED);
      }

      nk_spacer(ctx);

      // timers
      nk_layout_row(ctx, NK_STATIC, 15, 2, widths);

      snprintf(register_value_buffer, REGISTER_VALUE_BUFFER_SIZE, "%02X", myChip8.delay_timer);
      nk_label_colored(ctx, "DT:", NK_TEXT_RIGHT, RED);
      nk_label(ctx, register_value_buffer, NK_TEXT_RIGHT);

      snprintf(register_value_buffer, REGISTER_VALUE_BUFFER_SIZE, "%02X", myChip8.sound_timer);
      nk_label_colored(ctx, "ST:", NK_TEXT_RIGHT, RED);
      nk_label(ctx, register_value_buffer, NK_TEXT_RIGHT);
   }

   nk_end(ctx);

   #undef REGISTER_VALUE_BUFFER_SIZE
   #undef V_REGISTER_LABEL_BUFFER_SIZE
}