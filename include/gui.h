#ifndef GUI_H
#define GUI_H

void gui_init(void);
void gui_run(void);
void gui_draw_desktop(void);
void gui_draw_taskbar(void);
void gui_draw_start_menu(void);
void gui_draw_window(unsigned int x, unsigned int y, unsigned int width, unsigned int height, const char *title);
void gui_draw_button(unsigned int x, unsigned int y, unsigned int width, unsigned int height, const char *label);
void gui_handle_click(int mouse_x, int mouse_y);
void gui_handle_key(char key);

#endif
