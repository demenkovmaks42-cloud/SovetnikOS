
#include "gui.h"
#include "keyboard.h"
#include "vga.h"
#include "types.h"
#include "ramfs.h"
#include "ata.h"
#include "mem.h"
#include "pit.h"
#include "string.h"

#define DESKTOP       0x202A38
#define PANEL         0x151B25
#define PANEL_LIGHT   0x252F40
#define BLUE          0x4287F5
#define BLUE_DARK     0x2862C4
#define WHITE         0xF5F7FA
#define TEXT          0x252A34
#define MUTED         0x9CAAC0
#define WINDOW_BG     0xE8EDF4
#define GREEN         0x42C985
#define RED           0xEF6464
#define YELLOW        0xF3C969
#define BLACK         0x090D13

#define FONT_SCALE 2
#define FONT_W 5
#define FONT_H 7
#define CHAR_GAP 2

static uint32_t sw(void)
{
    uint32_t w = vga_width();
    return w ? w : 1024;
}

static uint32_t sh(void)
{
    uint32_t h = vga_height();
    return h ? h : 768;
}

/* 5x7 bitmap font: A-Z and 0-9. */
static const uint8_t font[36][7] = {
    {14,17,17,31,17,17,17}, /* A */
    {30,17,17,30,17,17,30}, /* B */
    {14,17,16,16,16,17,14}, /* C */
    {30,17,17,17,17,17,30}, /* D */
    {31,16,16,30,16,16,31}, /* E */
    {31,16,16,30,16,16,16}, /* F */
    {14,17,16,23,17,17,15}, /* G */
    {17,17,17,31,17,17,17}, /* H */
    {14,4,4,4,4,4,14},      /* I */
    {7,2,2,2,18,18,12},     /* J */
    {17,18,20,24,20,18,17}, /* K */
    {16,16,16,16,16,16,31}, /* L */
    {17,27,21,21,17,17,17}, /* M */
    {17,25,21,19,17,17,17}, /* N */
    {14,17,17,17,17,17,14}, /* O */
    {30,17,17,30,16,16,16}, /* P */
    {14,17,17,17,21,18,13}, /* Q */
    {30,17,17,30,20,18,17}, /* R */
    {15,16,16,14,1,1,30},   /* S */
    {31,4,4,4,4,4,4},       /* T */
    {17,17,17,17,17,17,14}, /* U */
    {17,17,17,17,17,10,4},  /* V */
    {17,17,17,21,21,21,10}, /* W */
    {17,17,10,4,10,17,17},  /* X */
    {17,17,10,4,4,4,4},     /* Y */
    {31,1,2,4,8,16,31},      /* Z */

    {14,17,19,21,25,17,14}, /* 0 */
    {4,12,4,4,4,4,14},       /* 1 */
    {14,17,1,2,4,8,31},      /* 2 */
    {30,1,1,14,1,1,30},      /* 3 */
    {2,6,10,18,31,2,2},      /* 4 */
    {31,16,16,30,1,1,30},    /* 5 */
    {6,8,16,30,17,17,14},    /* 6 */
    {31,1,2,4,8,8,8},        /* 7 */
    {14,17,17,14,17,17,14},  /* 8 */
    {14,17,17,15,1,2,12}     /* 9 */
};

static void glyph(uint32_t x, uint32_t y, char c,
                  uint32_t color, uint32_t scale)
{
    const uint8_t *g = 0;
    uint8_t rows[7] = {0,0,0,0,0,0,0};
    uint32_t r, col;

    if (c >= 'a' && c <= 'z')
        c = (char)(c - 'a' + 'A');

    if (c >= 'A' && c <= 'Z')
        g = font[c - 'A'];
    else if (c >= '0' && c <= '9')
        g = font[26 + c - '0'];
    else {
        switch (c) {
            case '-': rows[3]=14; break;
            case '_': rows[6]=31; break;
            case '.': rows[6]=4; break;
            case ':': rows[2]=4; rows[5]=4; break;
            case '/': rows[0]=1; rows[1]=2; rows[2]=2;
            rows[3]=4; rows[4]=8; rows[5]=8;
            rows[6]=16; break;
            case '>': rows[1]=16; rows[2]=8; rows[3]=4;
            rows[4]=8; rows[5]=16; break;
            case '<': rows[1]=1; rows[2]=2; rows[3]=4;
            rows[4]=2; rows[5]=1; break;
            case '!': rows[0]=4; rows[1]=4; rows[2]=4;
            rows[3]=4; rows[5]=4; break;
            case '?': rows[0]=14; rows[1]=17; rows[2]=1;
            rows[3]=2; rows[4]=4; rows[6]=4; break;
            case '+': rows[2]=4; rows[3]=14; rows[4]=4; break;
            case '=': rows[2]=14; rows[4]=14; break;
            case '(': rows[0]=2; rows[1]=4; rows[2]=8;
            rows[3]=8; rows[4]=8; rows[5]=4;
            rows[6]=2; break;
            case ')': rows[0]=8; rows[1]=4; rows[2]=2;
            rows[3]=2; rows[4]=2; rows[5]=4;
            rows[6]=8; break;
            default: break;
        }
        g = rows;
    }

    for (r = 0; r < 7; r++) {
        for (col = 0; col < 5; col++) {
            if (g[r] & (1u << (4 - col))) {
                vga_draw_rect(x + col * scale,
                              y + r * scale,
                              scale, scale, color);
            }
        }
    }
}

static void text(uint32_t x, uint32_t y, const char *s,
                 uint32_t color, uint32_t scale)
{
    if (!s) return;

    while (*s) {
        if (*s == '\n') {
            y += (FONT_H + 3) * scale;
            x = 0;
        } else if (*s == ' ') {
            x += (FONT_W + CHAR_GAP) * scale;
        } else {
            glyph(x, y, *s, color, scale);
            x += (FONT_W + CHAR_GAP) * scale;
        }
        s++;
    }
}

static void centered_text(uint32_t cx, uint32_t y,
                          const char *s, uint32_t color)
{
    uint32_t len = 0;
    const char *p = s;

    while (p && *p++) len++;

    uint32_t width = len * (FONT_W + CHAR_GAP) * FONT_SCALE;
    text(cx > width / 2 ? cx - width / 2 : 0,
         y, s, color, FONT_SCALE);
}

static void logo(uint32_t x, uint32_t y, uint32_t size)
{
    uint32_t t = size / 5;
    if (t < 2) t = 2;

    vga_draw_rect(x, y, size, t, WHITE);
    vga_draw_rect(x, y, t, size / 2, WHITE);
    vga_draw_rect(x, y + size / 2 - t, size, t, WHITE);
    vga_draw_rect(x + size - t, y + size / 2, t, size / 2, WHITE);
    vga_draw_rect(x, y + size - t, size, t, WHITE);
}

static void icon(uint32_t x, uint32_t y, int type)
{
    uint32_t c = BLUE;

    if (type == 1) c = GREEN;
    if (type == 2) c = 0xD7A8FF;

    vga_draw_rect(x, y, 54, 54, c);

    if (type == 0) {
        /* Computer */
        vga_draw_rect(x + 8, y + 8, 38, 27, WHITE);
        vga_draw_rect(x + 11, y + 11, 32, 21, PANEL);
        vga_draw_rect(x + 25, y + 35, 5, 7, WHITE);
        vga_draw_rect(x + 17, y + 42, 21, 4, WHITE);
    } else if (type == 1) {
        /* Folder */
        vga_draw_rect(x + 7, y + 15, 40, 29, YELLOW);
        vga_draw_rect(x + 7, y + 10, 20, 9, YELLOW);
        vga_draw_rect(x + 10, y + 21, 34, 19, 0xF9D98A);
    } else {
        /* Terminal */
        vga_draw_rect(x + 7, y + 9, 40, 36, PANEL);
        vga_draw_rect(x + 12, y + 17, 12, 3, GREEN);
        vga_draw_rect(x + 12, y + 24, 18, 3, WHITE);
        vga_draw_rect(x + 12, y + 31, 14, 3, GREEN);
    }
}

void gui_draw_desktop(void)
{
    uint32_t w = sw(), h = sh();

    vga_draw_rect(0, 0, w, h, DESKTOP);

    /* Desktop accent */
    vga_draw_rect(w > 500 ? w - 500 : 0, 58,
                  500, h > 112 ? h - 112 : 1, 0x222F43);

    vga_draw_rect(0, 0, w, 48, PANEL);
    vga_draw_rect(0, 48, w, 2, BLUE);

    logo(16, 9, 30);
    text(58, 17, "SOVETNIK OS", WHITE, 2);

    /* Desktop shortcuts */
    icon(28, 86, 0);
    text(24, 150, "COMPUTER", WHITE, 1);

    icon(28, 190, 1);
    text(35, 254, "FILES", WHITE, 1);

    icon(28, 294, 2);
    text(24, 358, "TERMINAL", WHITE, 1);

    /* Welcome panel */
    uint32_t ww = w > 800 ? 520 : w - 40;
    uint32_t wh = 210;
    uint32_t wx = (w - ww) / 2;
    uint32_t wy = h > 500 ? (h - wh) / 2 - 10 : 70;

    vga_draw_rect(wx + 6, wy + 6, ww, wh, BLACK);
    vga_draw_rect(wx, wy, ww, wh, PANEL);
    vga_draw_rect(wx, wy, ww, 42, PANEL_LIGHT);

    logo(wx + 12, wy + 7, 27);
    text(wx + 50, wy + 16, "WELCOME", WHITE, 2);

    centered_text(w / 2, wy + 67, "SOVETNIK OS", WHITE);
    centered_text(w / 2, wy + 94, "YOUR SYSTEM IS READY", MUTED);

    vga_draw_rect(wx + 35, wy + 132, ww - 70, 3, BLUE);

    vga_draw_rect(wx + 35, wy + 153, 12, 12, GREEN);
    text(wx + 55, wy + 155, "KERNEL RUNNING", WHITE, 1);

    vga_draw_rect(wx + ww - 150, wy + 153, 12, 12, GREEN);
    text(wx + ww - 130, wy + 155, "GUI READY", WHITE, 1);
}

void gui_draw_taskbar(void)
{
    uint32_t w = sw(), h = sh();
    if (h < 54) return;

    vga_draw_rect(0, h - 54, w, 54, PANEL);
    vga_draw_rect(0, h - 54, w, 2, PANEL_LIGHT);

    vga_draw_rect(10, h - 46, 140, 38, BLUE_DARK);
    logo(20, h - 38, 22);
    text(50, h - 32, "START", WHITE, 1);

    /* Status */
    vga_draw_rect(w - 155, h - 34, 10, 10, GREEN);
    text(w - 138, h - 34, "SYSTEM OK", WHITE, 1);
}

void gui_draw_start_menu(void)
{
    uint32_t h = sh();
    uint32_t x = 12;
    uint32_t y = h > 500 ? h - 54 - 330 : 10;

    vga_draw_rect(x + 5, y + 5, 300, 320, BLACK);
    vga_draw_rect(x, y, 300, 320, PANEL);
    vga_draw_rect(x, y, 300, 54, PANEL_LIGHT);

    logo(x + 14, y + 12, 30);
    text(x + 56, y + 20, "SOVETNIK OS", WHITE, 1);

    icon(x + 18, y + 75, 0);
    text(x + 86, y + 95, "COMPUTER", WHITE, 1);

    icon(x + 18, y + 145, 1);
    text(x + 86, y + 165, "FILE MANAGER", WHITE, 1);

    icon(x + 18, y + 215, 2);
    text(x + 86, y + 235, "TERMINAL", WHITE, 1);

    vga_draw_rect(x + 12, y + 285, 276, 1, PANEL_LIGHT);
    text(x + 20, y + 300, "VERSION 0.7", MUTED, 1);
}

void gui_draw_window(uint32_t x, uint32_t y,
                     uint32_t width, uint32_t height,
                     const char *title)
{
    vga_draw_rect(x + 6, y + 6, width, height, BLACK);
    vga_draw_rect(x, y, width, height, WINDOW_BG);
    vga_draw_rect(x, y, width, 42, PANEL);

    text(x + 14, y + 16, title, WHITE, 1);

    vga_draw_rect(x + width - 32, y + 9, 22, 22, RED);
    vga_draw_rect(x + width - 27, y + 14, 12, 12, RED);
}

void gui_draw_button(uint32_t x, uint32_t y,
                     uint32_t width, uint32_t height,
                     const char *label)
{
    vga_draw_rect(x, y, width, height, BLUE_DARK);
    vga_draw_rect(x, y, width, 2, BLUE);
    if (label)
        text(x + 12, y + height / 2 - 3, label, WHITE, 1);
}

/* Простая модель окон и меню. Движение мыши не перерисовывает экран. */

#define WIN_NONE     0
#define WIN_COMPUTER 1
#define WIN_FILES    2
#define WIN_TERMINAL 3
#define TERM_LINES 12
#define TERM_COLS 74
#define TERM_INPUT 96
#define TERM_HISTORY 8
#define TERM_MAX_VISIBLE (TERM_COLS - 5)

static int active_window = WIN_NONE;
static int start_open = 0;
static int gui_ready = 0;
static char term_lines[TERM_LINES][TERM_COLS + 1];
static uint32_t term_line_count = 0;
static char term_input[TERM_INPUT];
static uint32_t term_input_len = 0;
static uint32_t term_cursor = 0;
static char term_history[TERM_HISTORY][TERM_INPUT];
static uint32_t term_history_count = 0;
static int term_history_pos = -1;
static int files_selected = -1;
static char file_message[64] = "SELECT A FILE OR DIRECTORY";
static char file_view[RAMFS_MAX_FILE_SIZE + 1];
static uint32_t file_view_size = 0;
static int file_view_open = 0;

static uint32_t window_x(void) { return sw() > 700 ? (sw() - 640) / 2 : 10; }
static uint32_t window_y(void) { return sh() > 500 ? (sh() - 420) / 2 : 52; }

static void str_copy(char *d, const char *s, uint32_t cap)
{
    uint32_t i=0;
    if (!cap) return;
    if (!s) s="";
    while (s[i] && i+1<cap) { d[i]=s[i]; i++; }
    d[i]=0;
}
static void term_add(const char *s)
{
    uint32_t i=0;
    if (term_line_count < TERM_LINES) {
        str_copy(term_lines[term_line_count], s, TERM_COLS+1);
        term_line_count++;
    } else {
        for (i=1;i<TERM_LINES;i++) str_copy(term_lines[i-1],term_lines[i],TERM_COLS+1);
        str_copy(term_lines[TERM_LINES-1],s,TERM_COLS+1);
    }
}
static int starts_with(const char *s,const char *prefix)
{
    while (*prefix) { if (*s++ != *prefix++) return 0; }
    return 1;
}
static void skip_spaces(char **p) { while (**p==' ') ++*p; }
static void term_exec(void)
{
    char cmd[TERM_INPUT], *arg, *sp;
    uint32_t i;
    str_copy(cmd,term_input,sizeof(cmd));
    arg=cmd; skip_spaces(&arg);
    sp=arg; while (*sp && *sp!=' ') ++sp;
    if (*sp) { *sp++=0; skip_spaces(&sp); } else sp="";
    if (!*arg) return;
    { char *q; for (q=arg; *q; ++q) if (*q >= 'A' && *q <= 'Z') *q=(char)(*q-'A'+'a'); }
    if (strcmp(arg,"help")==0) {
        char *q; for (q=sp; *q; ++q) if (*q >= 'A' && *q <= 'Z') *q=(char)(*q-'A'+'a');
        if (!*sp) {
            term_add("COMMANDS (type HELP NAME for details):");
            term_add("help [NAME] - command reference");
            term_add("ls - list files; pwd - show current path");
            term_add("cd PATH - change directory; cat NAME - view file");
            term_add("mkdir NAME - create directory; touch NAME - create file");
            term_add("rm NAME - delete file; clear - clear terminal output");
            term_add("mem - memory info; ticks - timer ticks");
            term_add("ata-info - disk info");
        } else if (strcmp(sp,"help")==0) term_add("help [NAME]: show all commands or details for NAME");
        else if (strcmp(sp,"ls")==0) term_add("ls: list files and directories in current directory");
        else if (strcmp(sp,"pwd")==0) term_add("pwd: print current directory path");
        else if (strcmp(sp,"cd")==0) term_add("cd PATH: change directory; cd with no path goes to /");
        else if (strcmp(sp,"cat")==0) term_add("cat NAME: display a text file from RAMFS");
        else if (strcmp(sp,"mkdir")==0) term_add("mkdir NAME: create a directory inside current directory");
        else if (strcmp(sp,"touch")==0) term_add("touch NAME: create an empty file");
        else if (strcmp(sp,"rm")==0) term_add("rm NAME: delete a file or empty directory from RAMFS");
        else if (strcmp(sp,"clear")==0) term_add("clear: clear terminal output; command history remains");
        else if (strcmp(sp,"mem")==0) term_add("mem: show heap address and memory statistics");
        else if (strcmp(sp,"ticks")==0) term_add("ticks: show PIT timer tick count");
        else if (strcmp(sp,"ata-info")==0) term_add("ata-info: show detected ATA disk model and sector count");
        else term_add("No help for that command. Type HELP to list commands.");
    } else if (strcmp(arg,"clear")==0) {
        term_line_count=0;
    } else if (strcmp(arg,"pwd")==0) {
        term_add(ramfs_cwd());
    } else if (strcmp(arg,"ls")==0) {
        int shown=0;
        const char *cwd=ramfs_cwd();
        uint32_t n=(uint32_t)strlen(cwd);
        for (i=0;i<(uint32_t)ramfs_count();i++) {
            const char *name=ramfs_name_at((int)i), *rest;
            char line[TERM_COLS+1];
            if (!name || !starts_with(name,cwd)) continue;
            if (n>1 && (name[n]!='/' && name[n]!=0)) continue;
            rest=name+n;
            if (*rest=='/') rest++;
            if (!*rest) continue;
            { const char *q=rest; while (*q && *q!='/') q++; if (*q) continue; }
            str_copy(line,ramfs_type_at((int)i)==RAMFS_TYPE_DIR?"[DIR] ":"[FILE] ",sizeof(line));
            { uint32_t k=(uint32_t)strlen(line); str_copy(line+k,rest,sizeof(line)-k); }
            term_add(line); shown++;
        }
        if (!shown) term_add("(empty)");
    } else if (strcmp(arg,"cd")==0) {
        char path[RAMFS_PATH_MAX];
        if (!*sp) ramfs_set_cwd("/");
        else if (ramfs_resolve(sp,path)==0 && ramfs_is_dir(path)) ramfs_set_cwd(path);
        else term_add("cd: directory not found");
    } else if (strcmp(arg,"cat")==0) {
        char path[RAMFS_PATH_MAX]; uint32_t size=0;
        const char *data;
        if (!*sp || ramfs_resolve(sp,path)!=0 || !(data=ramfs_read(path,&size))) term_add("cat: file not found");
        else {
            char line[TERM_COLS+1]; uint32_t pos=0;
            for (i=0;i<size;i++) {
                char c=data[i];
                if (c=='\n' || pos>=TERM_COLS) { line[pos]=0; term_add(line); pos=0; if (c=='\n') continue; }
                if (c>=32 && c<127) line[pos++]=c;
            }
            if (pos) { line[pos]=0; term_add(line); }
        }
    } else if (strcmp(arg,"mkdir")==0 || strcmp(arg,"touch")==0) {
        char path[RAMFS_PATH_MAX];
        int dir=strcmp(arg,"mkdir")==0;
        if (!*sp || ramfs_resolve(sp,path)!=0) term_add("usage: mkdir/touch NAME");
        else if (ramfs_exists(path)) term_add("already exists");
        else if (ramfs_create(path,dir?RAMFS_TYPE_DIR:RAMFS_TYPE_FILE)==0) term_add("created");
        else term_add("create failed (parent must exist)");
    } else if (strcmp(arg,"rm")==0) {
        char path[RAMFS_PATH_MAX];
        if (!*sp || ramfs_resolve(sp,path)!=0 || !ramfs_exists(path)) term_add("rm: not found");
        else if (ramfs_delete(path)==0) term_add("deleted");
        else term_add("rm: delete failed (directory not empty?)");
    } else if (strcmp(arg,"mem")==0) {
        term_add("RAM / heap information:");
        term_add("see boot log for total RAM");
        term_add("heap start:");
        { char b[TERM_COLS+1]=""; uint32_t v=(uint32_t)mem_heap_start; const char *hex="0123456789ABCDEF"; int k;
          str_copy(b,"0x",sizeof(b)); for(k=7;k>=0;k--){uint32_t n=(v>>(k*4))&15; uint32_t l=(uint32_t)strlen(b); b[l]=hex[n]; b[l+1]=0;} term_add(b); }
    } else if (strcmp(arg,"ticks")==0) {
        char b[TERM_COLS+1]="TICKS: "; uint32_t v=pit_ticks(), tmp[10], n=0, k=(uint32_t)strlen(b);
        do { tmp[n++]=v%10; v/=10; } while(v && n<10);
        while(n) { b[k++]=(char)('0'+tmp[--n]); } b[k]=0; term_add(b);
    } else if (strcmp(arg,"ata-info")==0) {
        term_add("ATA MODEL:");
        term_add(ata_model());
        term_add("SECTORS:");
        { uint32_t v=ata_sectors(); char b[TERM_COLS+1]; uint32_t tmp[10],n=0,k=0; do{tmp[n++]=v%10;v/=10;}while(v&&n<10); while(n)b[k++]=(char)('0'+tmp[--n]); b[k]=0; term_add(b); }
    } else {
        term_add("unknown command - type help");
    }
}
static void draw_terminal(uint32_t x,uint32_t y)
{
    uint32_t i;
    vga_draw_rect(x+12,y+58,616,346,0x101820);
    for(i=0;i<term_line_count;i++) text(x+24,y+70+i*20,term_lines[i],0x8BE9A2,1);
    text(x+24,y+326,"SOV>",0x7DB7FF,1);
    text(x+72,y+326,term_input,WHITE,1);
    vga_draw_rect(x+72+term_cursor*7,y+342,6,2,WHITE);
}
static int path_is_child(const char *cwd,const char *name,const char **rest)
{
    uint32_t n=(uint32_t)strlen(cwd);
    if (!starts_with(name,cwd)) return 0;
    if (n>1 && name[n] && name[n]!='/') return 0;
    *rest=name+n; if (**rest=='/') ++*rest;
    if (!**rest) return 0;
    { const char *p=*rest; while(*p && *p!='/') ++p; if(*p) return 0; }
    return 1;
}
static void draw_files(uint32_t x,uint32_t y)
{
    int i,row=0;
    const char *cwd=ramfs_cwd();
    text(x+22,y+60,"PATH:",TEXT,1); text(x+72,y+60,cwd,BLUE_DARK,1);
    gui_draw_button(x+22,y+82,100,28,"UP");
    gui_draw_button(x+132,y+82,100,28,"REFRESH");
    gui_draw_button(x+242,y+82,100,28,"OPEN");
    gui_draw_button(x+352,y+82,100,28,"DELETE");
    vga_draw_rect(x+20,y+120,600,230,WHITE);
    for(i=0;i<ramfs_count() && row<10;i++) {
        const char *name=ramfs_name_at(i),*rest;
        char line[100];
        if(!name || !path_is_child(cwd,name,&rest)) continue;
        str_copy(line,ramfs_type_at(i)==RAMFS_TYPE_DIR?"[DIR] ":"[FILE] ",sizeof(line));
        { uint32_t n=(uint32_t)strlen(line); str_copy(line+n,rest,sizeof(line)-n); }
        if(i==files_selected) vga_draw_rect(x+24,y+125+(uint32_t)row*21,590,20,0xC9DDFB);
        text(x+30,y+128+(uint32_t)row*21,line,TEXT,1);
        row++;
    }
    text(x+22,y+358,file_message,TEXT,1);
    if(file_view_open) {
        vga_draw_rect(x+30,y+128,580,215,0x101820);
        text(x+42,y+138,"TEXT FILE (ESC TO CLOSE)",WHITE,1);
        for(i=0;i<9 && (uint32_t)i*50<file_view_size;i++) {
            char line[51]; uint32_t j=0,off=(uint32_t)i*50;
            while(j<50 && off+j<file_view_size && file_view[off+j]!='\n') { char c=file_view[off+j]; line[j]=(c>=32&&c<127)?c:' '; j++; }
            line[j]=0; text(x+42,y+160+(uint32_t)i*19,line,0x8BE9A2,1);
        }
    }
}
static void draw_active_window(void)
{
    uint32_t x=window_x(),y=window_y();
    const char *title=active_window==WIN_COMPUTER?"COMPUTER":active_window==WIN_FILES?"FILE MANAGER":"TERMINAL";
    gui_draw_window(x,y,640,420,title);
    if(active_window==WIN_COMPUTER) {
        uint32_t ram=mem_total_ram()/1024u/1024u;
        text(x+24,y+65,"SOVETNIKOS SYSTEM",TEXT,1);
        text(x+24,y+95,"ARCHITECTURE: X86 32-BIT",TEXT,1);
        text(x+24,y+125,"RAM MB:",TEXT,1); vga_draw_rect(x+110,y+120,80,18,WINDOW_BG);
        { char b[12]; uint32_t tmp[10],n=0,k=0; do{tmp[n++]=ram%10;ram/=10;}while(ram&&n<10);while(n)b[k++]=(char)('0'+tmp[--n]);b[k]=0;text(x+110,y+125,b,TEXT,1); }
        text(x+24,y+155,"ATA DISK:",TEXT,1); text(x+110,y+155,ata_model(),TEXT,1);
        text(x+24,y+185,"SECTORS:",TEXT,1);
        { uint32_t v=ata_sectors(),tmp[10],n=0,k=0; char b[16]; do{tmp[n++]=v%10;v/=10;}while(v&&n<10);while(n)b[k++]=(char)('0'+tmp[--n]);b[k]=0;text(x+110,y+185,b,TEXT,1); }
        text(x+24,y+215,"RAMFS ENTRIES:",TEXT,1);
        { uint32_t v=(uint32_t)ramfs_count(),tmp[10],n=0,k=0; char b[16]; do{tmp[n++]=v%10;v/=10;}while(v&&n<10);while(n)b[k++]=(char)('0'+tmp[--n]);b[k]=0;text(x+170,y+215,b,TEXT,1); }
        gui_draw_button(x+24,y+270,190,38,"OPEN FILES");
        gui_draw_button(x+230,y+270,190,38,"OPEN TERMINAL");
    } else if(active_window==WIN_FILES) draw_files(x,y);
    else draw_terminal(x,y);
}
static void gui_redraw(void)
{
    gui_draw_desktop(); gui_draw_taskbar();
    if(active_window!=WIN_NONE) draw_active_window();
    if(start_open) gui_draw_start_menu();
}

/* Redraw only the active window while typing; keep desktop/taskbar untouched. */
static void gui_redraw_active_window(void)
{
    if (active_window != WIN_NONE)
        draw_active_window();
}

static void term_remember_command(const char *cmd)
{
    uint32_t i;
    if (!cmd || !*cmd) return;
    if (term_history_count && strcmp(term_history[term_history_count - 1], cmd) == 0) {
        term_history_pos = -1;
        return;
    }
    if (term_history_count < TERM_HISTORY) {
        str_copy(term_history[term_history_count++], cmd, TERM_INPUT);
    } else {
        for (i = 1; i < TERM_HISTORY; ++i)
            str_copy(term_history[i - 1], term_history[i], TERM_INPUT);
        str_copy(term_history[TERM_HISTORY - 1], cmd, TERM_INPUT);
    }
    term_history_pos = -1;
}

static void term_history_move(int direction)
{
    if (direction < 0) {
        if (!term_history_count) return;
        if (term_history_pos < 0) term_history_pos = (int)term_history_count - 1;
        else if (term_history_pos > 0) term_history_pos--;
        else return;
        str_copy(term_input, term_history[term_history_pos], TERM_INPUT);
    } else {
        if (term_history_pos < 0) return;
        if ((uint32_t)(term_history_pos + 1) < term_history_count) {
            term_history_pos++;
            str_copy(term_input, term_history[term_history_pos], TERM_INPUT);
        } else {
            term_history_pos = -1;
            term_input[0] = 0;
        }
    }
    term_input_len = (uint32_t)strlen(term_input);
    term_cursor = term_input_len;
}
void gui_init(void)
{
    if(!vga_is_framebuffer()) return;
    active_window=WIN_NONE; start_open=0; gui_ready=1;
    term_line_count=0; term_input_len=0; term_cursor=0; term_input[0]=0;
    term_history_count=0; term_history_pos=-1;
    term_add("SOVETNIKOS TERMINAL READY"); term_add("TYPE HELP FOR COMMANDS");
    gui_redraw();
}
void gui_run(void) { if(!vga_is_framebuffer()) return; if(!gui_ready) gui_init(); }
void gui_handle_click(int mx,int my)
{
    uint32_t h,sy,wx,wy;
    if(!gui_ready||mx<0||my<0)return;
    h=sh(); wx=window_x(); wy=window_y();
    if((uint32_t)my>=h-((h>=54)?54:h)&&mx>=10&&mx<=150) { start_open=!start_open; gui_redraw(); return; }
    if(start_open) {
        sy=h>500?h-54-330:10;
        if(mx>=20&&mx<=290&&(uint32_t)my>=sy+65&&(uint32_t)my<=sy+135) active_window=WIN_COMPUTER;
        else if(mx>=20&&mx<=290&&(uint32_t)my>=sy+136&&(uint32_t)my<=sy+205) active_window=WIN_FILES;
        else if(mx>=20&&mx<=290&&(uint32_t)my>=sy+206&&(uint32_t)my<=sy+275) active_window=WIN_TERMINAL;
        else {start_open=0;gui_redraw();return;}
        start_open=0; gui_redraw(); return;
    }
    if(active_window!=WIN_NONE) {
        if((uint32_t)mx>=wx+608&&(uint32_t)mx<=wx+640&&(uint32_t)my>=wy+5&&(uint32_t)my<=wy+42) {active_window=WIN_NONE;file_view_open=0;gui_redraw();return;}
        if(active_window==WIN_COMPUTER) {
            if((uint32_t)my>=wy+270&&(uint32_t)my<=wy+308) {
                if((uint32_t)mx>=wx+24&&(uint32_t)mx<=wx+214) active_window=WIN_FILES;
                else if((uint32_t)mx>=wx+230&&(uint32_t)mx<=wx+420) active_window=WIN_TERMINAL;
            }
        } else if(active_window==WIN_FILES) {
            if(file_view_open) {file_view_open=0;gui_redraw();return;}
            if((uint32_t)my>=wy+82&&(uint32_t)my<=wy+110) {
                if((uint32_t)mx>=wx+22&&(uint32_t)mx<=wx+122) {
                    char p[RAMFS_PATH_MAX]; str_copy(p,ramfs_cwd(),sizeof(p)); uint32_t n=(uint32_t)strlen(p);
                    while (n > 1 && p[n - 1] != '/') n--;
                    if (n > 1) p[n - 1] = 0; else p[1] = 0;
                    ramfs_set_cwd(p);
                } else if((uint32_t)mx>=wx+352&&(uint32_t)mx<=wx+452) {
                    if(files_selected>=0 && files_selected<ramfs_count()) { const char *nm=ramfs_name_at(files_selected); if(nm && strcmp(nm,"/")!=0 && ramfs_delete(nm)==0) str_copy(file_message,"DELETED",sizeof(file_message)); else str_copy(file_message,"DELETE FAILED",sizeof(file_message)); files_selected=-1; }
                } else if((uint32_t)mx>=wx+242&&(uint32_t)mx<=wx+342) {
                    if(files_selected>=0&&files_selected<ramfs_count()) { const char *nm=ramfs_name_at(files_selected); if(nm&&ramfs_is_dir(nm)) ramfs_set_cwd(nm); else if(nm) {uint32_t sz=0;const char *d=ramfs_read(nm,&sz);if(d){if(sz>RAMFS_MAX_FILE_SIZE)sz=RAMFS_MAX_FILE_SIZE;for(uint32_t j=0;j<sz;j++)file_view[j]=d[j];file_view[sz]=0;file_view_size=sz;file_view_open=1;} } }
                }
            } else if((uint32_t)my>=wy+120&&(uint32_t)my<wy+350) {
                int row=((uint32_t)my-(wy+125))/21, found=-1, r=0;
                for(int i=0;i<ramfs_count();i++){const char *nm=ramfs_name_at(i),*rest;if(nm&&path_is_child(ramfs_cwd(),nm,&rest)){if(r++==row){found=i;break;}}}
                files_selected=found;
                if(found>=0) str_copy(file_message,ramfs_name_at(found),sizeof(file_message));
            }
        }
        gui_redraw(); return;
    }
    if(mx>=20&&mx<=100&&my>=78&&my<=165) active_window=WIN_COMPUTER;
    else if(mx>=20&&mx<=100&&my>=182&&my<=270) active_window=WIN_FILES;
    else if(mx>=20&&mx<=100&&my>=286&&my<=374) active_window=WIN_TERMINAL;
    else return;
    gui_redraw();
}
void gui_handle_key(char key)
{
    uint32_t i;
    if(!gui_ready)return;
    if(key==27) {
        if(file_view_open) file_view_open=0;
        else {active_window=WIN_NONE;start_open=0;}
        gui_redraw(); return;
    }
    if(active_window==WIN_TERMINAL) {
        if(key=='\r'||key=='\n') {
            term_input[term_input_len]=0;
            { char line[TERM_COLS+1]="SOV> "; str_copy(line+5,term_input,sizeof(line)-5); term_add(line); }
            term_remember_command(term_input);
            term_exec(); term_input_len=0; term_cursor=0; term_input[0]=0; term_history_pos=-1;
            gui_redraw_active_window(); return;
        }
        if(key==KEYCODE_LEFT) { if(term_cursor>0) term_cursor--; gui_redraw_active_window(); return; }
        if(key==KEYCODE_RIGHT) { if(term_cursor<term_input_len) term_cursor++; gui_redraw_active_window(); return; }
        if(key==KEYCODE_UP) { term_history_move(-1); gui_redraw_active_window(); return; }
        if(key==KEYCODE_DOWN) { term_history_move(1); gui_redraw_active_window(); return; }
        if(key=='\b'||key==127) {
            if(term_cursor>0) {
                for(i=term_cursor;i<term_input_len;i++) term_input[i-1]=term_input[i];
                term_input_len--; term_cursor--; term_input[term_input_len]=0;
                term_history_pos=-1; gui_redraw_active_window();
            }
            return;
        }
        if((unsigned char)key>=32&&(unsigned char)key<127&&term_input_len<TERM_MAX_VISIBLE) {
            for(i=term_input_len;i>term_cursor;i--) term_input[i]=term_input[i-1];
            term_input[term_cursor]=key; term_input_len++; term_cursor++; term_input[term_input_len]=0;
            term_history_pos=-1; gui_redraw_active_window();
        }
    }
}
