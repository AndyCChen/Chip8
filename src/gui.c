#include "stdio.h"
#include "stdint.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "../nuklear/nuklear.h"
#include "../includes/nuklear_sdl_renderer.h"

#include "../includes/gui.h"
#include "../includes/display.h"
#include "../includes/chip8.h"

static struct nk_context *ctx = NULL;

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

static int window_height = 0;
static int window_width = 0;

static struct nk_color RED = { 247, 47, 47, 255 };
static struct nk_color CYAN = { 44, 191, 191, 255 };

static struct nk_colorf bg_color = { 0, 0, 0, 1 };
static struct nk_colorf fg_color = { 1, 1, 1, 1 };

static int viewport_width;

// gui widgets

static void widget_stack(float x_pos, float y_pos, float width, float height);
static void widget_memory(float x_pos, float y_pos, float width, float height);
static void widget_cpu_state(float x_pos, float y_pos, float width, float height);
static void widget_keypad(float x_pos, float y_pos, float width, float height);
static void widget_debug(float x_pos, float y_pos, float width, float height);
static void widget_general(float x_pos, float y_pos, float width, float height);

void gui_close()
{
   nk_sdl_shutdown();
}

void gui_init()
{
   window = display_get_window();
   renderer = display_get_renderer();

   SDL_GetWindowSize(window, &window_width, &window_height);

   display_get_viewport_size( &viewport_width, NULL );

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
   widget_stack( 0, 0, GUI_STACK_WIDGET_W, (float) window_height );
   widget_memory( GUI_STACK_WIDGET_W, 0, GUI_MEMORY_WIDGET_W, (float) window_height );

   const float WIDGET_CPU_STATE_HEIGHT = (float) window_height * 0.6;
   const float WIDGET_KEYPAD_HEIGHT = (float) window_height * 0.4;

   widget_cpu_state( GUI_STACK_WIDGET_W + GUI_MEMORY_WIDGET_W, 0, GUI_CPU_STATE_WIDGET_W, WIDGET_CPU_STATE_HEIGHT );
   widget_keypad( GUI_STACK_WIDGET_W + GUI_MEMORY_WIDGET_W, WIDGET_CPU_STATE_HEIGHT, GUI_KEYPAD_W, WIDGET_KEYPAD_HEIGHT );

   const float WIDGET_DEBUG_WIDTH = viewport_width * 0.3;
   const float WIDGET_GENERAL_WIDTH = viewport_width * 0.7;

   widget_debug( GUI_STACK_WIDGET_W + GUI_MEMORY_WIDGET_W + GUI_CPU_STATE_WIDGET_W, 0, WIDGET_DEBUG_WIDTH, GUI_DEBUG_H );
   widget_general( GUI_STACK_WIDGET_W + GUI_MEMORY_WIDGET_W + GUI_CPU_STATE_WIDGET_W + WIDGET_DEBUG_WIDTH, 0, WIDGET_GENERAL_WIDTH, GUI_GENERAL_H );
}

void gui_draw()
{
   nk_sdl_render(NK_ANTI_ALIASING_ON);
}

static void widget_stack(float x_pos, float y_pos, float width, float height)
{
   #define STACK_COUNT_LABEL_SIZE 3
   #define STACK_VALUE_LABEL_SIZE 5
   
   static char stack_value_buffer[STACK_VALUE_LABEL_SIZE];
   static char stack_level_buffer[STACK_COUNT_LABEL_SIZE];

   if ( nk_begin( ctx, "Stack", nk_rect(x_pos, y_pos, width, height), NK_WINDOW_BORDER|NK_WINDOW_TITLE ) )
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
   
   static char memory_address_buffer[MEMORY_ADDRESS_LABEL_SIZE];
   static char memory_value_buffer[MEMORY_VALUE_LABEL_SIZE];

   static int64_t y_scroll = 0;
   static bool end_reached = false;

   static float widths[NUM_OF_COLS] = { 45, 15, 15, 15, 15, 15, 15, 15, 15 }; // widths for each of the 9 columns in a row

   if ( nk_begin( ctx, "Memory", nk_rect(x_pos, y_pos, width, height), NK_WINDOW_BORDER|NK_WINDOW_TITLE|NK_WINDOW_NO_SCROLLBAR ) )
   {
      if ( nk_window_has_focus(ctx) )
      {
         if ( (y_scroll - ctx->input.mouse.scroll_delta.y) < 0)
         {
            y_scroll = 0;
         }
         else
         {
            // scroll up
            if ( (int) ctx->input.mouse.scroll_delta.y == 1 ) 
            {
               if (end_reached)
               {
                  end_reached = false;
               }
               y_scroll -= ctx->input.mouse.scroll_delta.y;
            }
            // scroll down
            else if ( (int) ctx->input.mouse.scroll_delta.y == -1 )
            {
               if (!end_reached)
               {
                  y_scroll -= ctx->input.mouse.scroll_delta.y;
               }
            }
         }
      }

      nk_layout_row(ctx, NK_STATIC, 15, 9, widths);

      int num_of_rows = ( nk_window_get_height(ctx) / 20 );
      
      int address = y_scroll * 8;
      int address_row = address + (num_of_rows * 8);
      for (address; address < address_row; address += 8)
      {
         if (address == RAM_SIZE)
         {  
            end_reached = true;
            break;
         }

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

   static char register_value_buffer[5];

   static float widths[2] = {30, 35};
   static float v_register_widths[4] = {30, 25, 30, 25};

   if ( nk_begin( ctx, "CPU State", nk_rect(x_pos, y_pos, width, height), NK_WINDOW_BORDER|NK_WINDOW_TITLE ) )
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

      static char v_register_label[V_REGISTER_LABEL_BUFFER_SIZE];

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

static void widget_keypad(float x_pos, float y_pos, float width, float height)
{
   if ( nk_begin( ctx, "Keypad", nk_rect(x_pos, y_pos, width, height), NK_WINDOW_BORDER|NK_WINDOW_TITLE ) )
   {
      static const char keypad_labels[] = "123C456D789EA0BF"; // char text associated with each keypad button

      // numerical value associated with each keypad button
      static const char keypad_values[] = { 
         0x1, 0x2, 0x3, 0xC, 
         0x4, 0x5, 0x6, 0xD, 
         0x7, 0x8, 0x9, 0xE,
         0xA, 0x0, 0xB, 0xF 
      };

      nk_button_set_behavior(ctx, NK_BUTTON_REPEATER);

      struct nk_style_button button_style = ctx->style.button;
      button_style.active.data.color = RED;
      button_style.hover.data.color = RED;

      // 16 bit keypad state retrieved from cpu
      uint16_t keypad_states = chip8_get_keypad();

       // 16 bit int where each bit represents on or off state of the gui keypad button
      static uint16_t gui_button_states = 0;

      nk_layout_row_static(ctx, 35, 25, 4);
      for (int i = 0; i < 16; ++i)
      {
         // if key is in on state, toggle color to red
         uint8_t key = ( keypad_states & ( 1 << keypad_values[i] ) ) >> keypad_values[i];

         if ( key ) 
            button_style.normal.data.color = RED;
         else 
            button_style.normal.data.color = ctx->style.button.normal.data.color;

         if ( nk_button_text_styled(ctx, &button_style, &keypad_labels[i], 1) )
         {
            gui_button_states = gui_button_states | ( 1 << keypad_values[i] );
            chip8_set_key_down(keypad_values[i]);
         }  
         else
         {
            uint8_t button  = ( gui_button_states & ( 1 << keypad_values[i] ) ) >> keypad_values[i];
            
            // only register a key up event if the button was previously in the on state
            if (button == 1)
            {
               chip8_set_key_up(keypad_values[i]);
            }

            gui_button_states = gui_button_states & ~( 1 << keypad_values[i] );
         }
      }

      nk_button_set_behavior(ctx, NK_BUTTON_DEFAULT);
   }

   nk_end(ctx);
}

static void widget_debug(float x_pos, float y_pos, float width, float height)
{
   if ( nk_begin( ctx, "Debug", nk_rect(x_pos, y_pos, width, height), NK_WINDOW_BORDER|NK_WINDOW_TITLE ) )
   {
      struct nk_style_button button_style = ctx->style.button;
      button_style.hover.data.color = RED;
      button_style.active.data.color = RED;
      button_style.normal.data.color = myChip8.pause_flag ? RED : ctx->style.button.normal.data.color;

      nk_layout_row_static(ctx, 15, 50, 1);
      nk_label_colored(ctx, "Status:", NK_TEXT_LEFT, RED);
      
      float col_widths[] = { 50, 100 };

      nk_layout_row(ctx, NK_STATIC, 20, 2, col_widths);

      nk_label_colored(ctx, "Paused", NK_TEXT_LEFT, myChip8.pause_flag ? CYAN : RED);
      if ( nk_button_label_styled(ctx, &button_style, "Pause") )
      {
         myChip8.pause_flag = !myChip8.pause_flag;

         if (myChip8.pause_flag) 
            printf("Paused, press space to step through a single instruction or press f5 again to resume.\n"); 
      }

      button_style.normal.data.color = ctx->style.button.normal.data.color;

      nk_label_colored(ctx, "Tick", NK_TEXT_LEFT, RED);
      if ( nk_button_label_styled(ctx, &button_style,"Cycle Step") )
      {
         if (myChip8.pause_flag) myChip8.cycle_step_flag = true;
      }
   }

   nk_end(ctx);
}

static void widget_general(float x_pos, float y_pos, float width, float height)
{
   if ( nk_begin( ctx, "General", nk_rect(x_pos, y_pos, width, height), NK_WINDOW_BORDER|NK_WINDOW_TITLE ) )
   {
      float width = nk_window_get_width(ctx);
      float col_widths[] = { width * 0.20, width * 0.25 };
      static int clock_rate, volume = DEFAULT_VOLUME;

      nk_layout_row(ctx, NK_STATIC, 20, 2, col_widths);

      nk_button_set_behavior(ctx, NK_BUTTON_REPEATER);

      nk_label_colored(ctx, "Clock Rate: ", NK_TEXT_LEFT, RED);
      clock_rate = myChip8.clock_rate;
      nk_property_int(ctx, "Clock Rate:", 1, &clock_rate, 2000, 1, 1);
      myChip8.clock_rate = clock_rate;
      
      nk_button_set_behavior(ctx, NK_BUTTON_DEFAULT);

      nk_label_colored(ctx, "Volume: ", NK_TEXT_LEFT, RED);
      if ( nk_slider_int(ctx, 0, &volume, 5000, 1) )
      {
         display_set_volume(volume);
      }
      
      nk_label_colored(ctx, "Background Color:", NK_TEXT_LEFT, RED);
      if ( nk_combo_begin_color( ctx, nk_rgb_cf(bg_color), nk_vec2( nk_widget_width( ctx ), 400 ) ) )
      {
         nk_layout_row_dynamic(ctx, 120, 1);
         bg_color = nk_color_picker(ctx, bg_color, NK_RGB);

         nk_layout_row_dynamic(ctx, 25, 1);
         bg_color.r = (float) nk_propertyi(ctx, "#R:", 0, bg_color.r * 255, 255, 1, 1) / 255;
         bg_color.g = (float) nk_propertyi(ctx, "#G:", 0, bg_color.g * 255, 255, 1, 1) / 255;
         bg_color.b = (float) nk_propertyi(ctx, "#B:", 0, bg_color.b * 255, 255, 1, 1) / 255;

         nk_combo_end(ctx);

         display_set_bg_color(bg_color.r, bg_color.g, bg_color.b);
      }

      nk_label_colored(ctx, "Foreground Color:", NK_TEXT_LEFT, RED);
      if ( nk_combo_begin_color( ctx, nk_rgb_cf(fg_color), nk_vec2( nk_widget_width( ctx ), 400 ) ) )
      {
         nk_layout_row_dynamic(ctx, 120, 1);
         fg_color = nk_color_picker(ctx, fg_color, NK_RGB);

         nk_layout_row_dynamic(ctx, 25, 1);
         fg_color.r = (float) nk_propertyi(ctx, "#R:", 0, fg_color.r * 255, 255, 1, 1) / 255;
         fg_color.g = (float) nk_propertyi(ctx, "#G:", 0, fg_color.g * 255, 255, 1, 1) / 255;
         fg_color.b = (float) nk_propertyi(ctx, "#B:", 0, fg_color.b * 255, 255, 1, 1) / 255;

         nk_combo_end(ctx);

         display_set_fg_color(fg_color.r, fg_color.g, fg_color.b);
      }
   }

   nk_end(ctx);
}