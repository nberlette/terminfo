/*!
 * @file terminfo.c
 * @brief Terminfo library for exposing terminal capabilities to JS via WASM.
 * @author Nicholas Berlette
 * @date 2025-06-21
 * @copyright 2025 Nicholas Berlette. MIT License (https://nick.mit-license.org)
 */
#include <curses.h>
#include <term.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>

#define BUF_SIZE 256

// ---- INIT ----

int init_terminfo(const char* termname) {
  // prevent abrupt abort on failure
  if (setupterm(termname, 1, NULL) != OK) {
    return -1;
  }
  return 0;
}

// ---- BASIC INFO ----

int get_term_cols() { return columns; }

int get_term_lines() { return lines; }

const char* get_termname() { return termname(); }

const char* get_longname() { return longname(); }

int get_baudrate() { return baudrate(); }

int get_erasechar() { return erasechar(); }

int get_killchar() { return killchar(); }

int can_change_color_support() { return can_change_color(); }

// ---- CAPABILITIES ----

int get_tigetnum(const char* cap) {
  return tigetnum(cap);
}

int get_tigetflag(const char* cap) {
  return tigetflag(cap);
}

const char* get_tigetstr(const char* cap) {
  char* s = tigetstr(cap);
  return s ? s : "";
}

// ---- FORMAT ----

const char* format_cap(const char* cap, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9) {
  static char buf[BUF_SIZE];
  const char* capstr = tigetstr(cap);
  if (!capstr) return "";
  const char* result = tparm(capstr, a1, a2, a3, a4, a5, a6, a7, a8, a9);
  strncpy(buf, result, BUF_SIZE - 1);
  buf[BUF_SIZE - 1] = '\0';
  return buf;
}

// ---- COMPOSED SGR ----

const char* render_sgr(int bold, int underline, int blink, int reverse, int fg, int bg) {
  static char buf[BUF_SIZE];
  char* sgr = tigetstr("sgr");
  if (!sgr) return "";
  const char* result = tparm(sgr, bold, underline, reverse, blink, 0, 0, fg, bg, 0);
  strncpy(buf, result, BUF_SIZE - 1);
  buf[BUF_SIZE - 1] = '\0';
  return buf;
}

// ---- BOX DRAWING ----

const char* get_boxchar(int style, int part) {
  static const char* slim[] = {
    "┌", "┐", "└", "┘", "─", "│", "├", "┤", "┬", "┴", "┼",
  };
  static const char* thicc[] = {
    "┏", "┓", "┗", "┛", "━", "┃", "┣", "┫", "┳", "┻", "╋",
  };
  static const char* dubs[] = {
    "╔", "╗", "╚", "╝", "═", "║", "╠", "╣", "╦", "╩", "╬",
  };
  static const char* round[] = {
    "╭", "╮", "╰", "╯", "─", "│", "├", "┤", "┬", "┴", "┼",
  };
  static const char* slimthicc[] = {
    "┎", "┒", "┖", "┚", "─", "┃", "┠", "┨", "┰", "┸", "╂",
  };
  static const char* thiccslim[] = {
    "┍", "┑", "┕", "┙", "━", "│", "┝", "┥", "┯", "┷", "┿",
  };
  static const char* slimdubs[] = {
    "╒", "╕", "╘", "╛", "═", "│", "╞", "╡", "╤", "╧", "╪",
  };
  static const char* dubsslim[] = {
    "╓", "╖", "╙", "╜", "─", "║", "╟", "╢", "╥", "╨", "╫",
  };
  const char** tbl = NULL;
  switch (style) {
    case 0: tbl = slim; break;
    case 1: tbl = thicc; break;
    case 2: tbl = dubs; break;
    case 3: tbl = round; break;
    case 4: tbl = slimthicc; break;
    case 5: tbl = thiccslim; break;
    case 6: tbl = slimdubs; break;
    case 7: tbl = dubsslim; break;
  }
  return (part >= 0 && part < 11) ? tbl[part] : "?";
}

// ---- COLOR ----

int get_color_rgb(short i, short* r, short* g, short* b) {
  return color_content(i, r, g, b);
}

int get_num_pairs() {
  return tigetnum("pairs");
}

int get_num_colors() {
  return tigetnum("colors");
}

#ifdef has_colors
int has_colors_support() {
  return has_colors() || (get_num_colors() > 0);
}
#else
int has_colors_support() {
  return get_num_colors() > 0;
}
#endif

// ---- COMMON SEQUENCES ----

int has_alt_screen() {
  return tigetstr("smcup") != NULL && tigetstr("rmcup") != NULL;
}

const char* enter_alt_screen() {
  return tigetstr("smcup") ?: "";
}

const char* exit_alt_screen() {
  return tigetstr("rmcup") ?: "";
}

const char* erase_screen() {
  return tigetstr("clear") ?: "";
}

const char* erase_line(int mode) {
  static char buf[BUF_SIZE];
  const char* cap = tigetstr("el");
  if (!cap) return "";
  const char* result = tparm(cap, mode);
  strncpy(buf, result, BUF_SIZE - 1);
  buf[BUF_SIZE - 1] = '\0';
  return buf;
}

const char* erase_lines(int n) {
  static char buf[BUF_SIZE];
  const char* cap = tigetstr("el");
  if (!cap) return "";
  buf[0] = '\0';
  for (int i = 0; i < n; i++) {
    const char* result = tparm(cap, 0); // mode 0: clear to end of line
    strncat(buf, result, BUF_SIZE - strlen(buf) - 1);
    if (i < n - 1) {
      const char* cud1 = tigetstr("cud1");
      if (cud1) {
        strncat(buf, cud1, BUF_SIZE - strlen(buf) - 1);
      }
    }
  }
  return buf;
}

const char* cursor_hide() {
  return tigetstr("civis") ?: "";
}

const char* cursor_show() {
  return tigetstr("cnorm") ?: "";
}

const char* cursor_save() {
  return tigetstr("sc") ?: "";
}

const char* cursor_restore() {
  return tigetstr("rc") ?: "";
}

const char* enable_keyboard_action_mode() {
  return tigetstr("smkx") ?: "";
}

const char* disable_keyboard_action_mode() {
  return tigetstr("rmkx") ?: "";
}

const char* enable_mouse() {
  const char* smkx = tigetstr("smkx");
  const char* kmous = tigetstr("kmous");
  return smkx ? smkx : (kmous ? kmous : "");
}

const char* disable_mouse() {
  const char* rmkx = tigetstr("rmkx");
  return rmkx ?: "";
}

// ---- CURSOR MOVE (alias of cup) ----

const char* cursor_move(int row, int col) {
  static char buf[BUF_SIZE];
  const char* cap = tigetstr("cup");
  if (!cap) return "";
  const char* result = tparm(cap, row, col);
  strncpy(buf, result, BUF_SIZE - 1);
  buf[BUF_SIZE - 1] = '\0';
  return buf;
}

#define CAP_FN0(name, capname) \
const char* name() { \
  static char buf[BUF_SIZE]; \
  const char* cap = tigetstr(capname); \
  if (!cap) return ""; \
  const char* result = tparm(cap); \
  strncpy(buf, result, BUF_SIZE - 1); \
  buf[BUF_SIZE - 1] = '\0'; \
  return buf; \
}

#define CAP_FN1(name, capname, param1) \
const char* name(int p1) { \
  static char buf[BUF_SIZE]; \
  const char* cap = tigetstr(capname); \
  if (!cap) return ""; \
  const char* result = tparm(cap, p1); \
  strncpy(buf, result, BUF_SIZE - 1); \
  buf[BUF_SIZE - 1] = '\0'; \
  return buf; \
}

#define CAP_FN2(name, capname, param1, param2) \
const char* name(int p1, int p2) { \
  static char buf[BUF_SIZE]; \
  const char* cap = tigetstr(capname); \
  if (!cap) return ""; \
  const char* result = tparm(cap, p1, p2); \
  strncpy(buf, result, BUF_SIZE - 1); \
  buf[BUF_SIZE - 1] = '\0'; \
  return buf; \
}

CAP_FN1(hpa, "hpa", col)
CAP_FN1(hpr, "hpr", col)

CAP_FN1(vpa, "vpa", row)
CAP_FN1(vpr, "vpr", row)

CAP_FN0(home, "home")
CAP_FN2(cup, "cup", row, col)

/** Cursor Down (n=1) */
CAP_FN0(cud1, "cud1")
/** Cursor Down */
CAP_FN1(cud, "cud", n)

/** Cursor Up (n=1) */
CAP_FN0(cuu1, "cuu1")
/** Cursor Up */
CAP_FN1(cuu, "cuu", n)

/** Cursor Forward (n=1) */
CAP_FN0(cuf1, "cuf1")
/** Cursor Forward */
CAP_FN1(cuf, "cuf", n)

/** Cursor Backward (n=1) */
CAP_FN0(cub1, "cub1")
/** Cursor Backward */
CAP_FN1(cub, "cub", n)

// ------------------------------
// Insert/Delete/Erase Characters
// ------------------------------
CAP_FN1(ech,  "ech", n)
CAP_FN1(dch, "dch", n)
CAP_FN1(ich, "ich", n)

// ------------------------------
// Stdin/Stdout/Stderr
// ------------------------------
int get_stdin_fd() {
  return fileno(stdin);
}

int get_stdout_fd() {
  return fileno(stdout);
}

int get_stderr_fd() {
  return fileno(stderr);
}

int is_fd_atty(int fd) {
  return isatty(fd);
}

// ------------------------------
// Section: Key Symbol Table
// ------------------------------

typedef struct {
  // numeric key code
  int code;
  // human-readable key name
  const char* name;
  // optional sequence field (without leading ESC)
  const char* seq;
} KeyEntry;

static KeyEntry key_table[] = {
  { KEY_UP,        "KEY_UP",         "[A"  },
  { KEY_DOWN,      "KEY_DOWN",       "[B"  },
  { KEY_LEFT,      "KEY_LEFT",       "[D"  },
  { KEY_RIGHT,     "KEY_RIGHT",      "[C"  },
  { KEY_BACKSPACE, "KEY_BACKSPACE",  "[D"  },
  { KEY_HOME,      "KEY_HOME",       "[H"  },
  { KEY_SR,        "KEY_SCROLLUP",   "[1~" },
  { KEY_SF,        "KEY_SCROLLDN",   "[4~" },
  { KEY_END,       "KEY_END",        "[F"  },
  { KEY_IC,        "KEY_INSERT",     "[2~" },
  { KEY_DC,        "KEY_DELETE",     "[3~" },
  { KEY_PPAGE,     "KEY_PGUP",       "[5~" },
  { KEY_NPAGE,     "KEY_PGDN",       "[6~" },
  { KEY_RESIZE,    "KEY_RESIZE",     "[8~" },
  { KEY_ENTER,     "KEY_ENTER",      "[M"  },
  { KEY_ENTER,     "KEY_RETURN",     "\r"  },
  { 27,            "KEY_ESC",        "\x1B"},
  { KEY_BTAB,      "KEY_SHIFT_TAB",  "[Z"  },
  { KEY_BTAB,      "KEY_BTAB",       "[Z"  },
  { KEY_F(1),      "KEY_F1",         "[11~" },
  { KEY_F(2),      "KEY_F2",         "[12~" },
  { KEY_F(3),      "KEY_F3",         "[13~" },
  { KEY_F(4),      "KEY_F4",         "[14~" },
  { KEY_F(5),      "KEY_F5",         "[15~" },
  { KEY_F(6),      "KEY_F6",         "[16~" },
  { KEY_F(7),      "KEY_F7",         "[17~" },
  { KEY_F(8),      "KEY_F8",         "[18~" },
  { KEY_F(9),      "KEY_F9",         "[19~" },
  { KEY_F(10),     "KEY_F10",        "[20~" },
  { KEY_F(11),     "KEY_F11",        "[21~" },
  { KEY_F(12),     "KEY_F12",        "[22~" },
  { 0,             NULL,             "\0" },
  { 0,             "NUL",            "\0" }, // alias for \0 NULL
  { 1,             "SOH",            "\x01" },
  { 2,             "STX",            "\x02" },
  { 3,             "ETX",            "\x03" },
  { 4,             "EOT",            "\x04" },
  { 5,             "ENQ",            "\x05" },
  { 6,             "ACK",            "\x06" },
  { 7,             "BEL",            "\x07" },
  { 8,             "BS",             "\b"   },
  { 9,             "TAB",            "\t"   },
  { 10,            "LF",             "\n"   },
  { 11,            "VT",             "\v"   },
  { 12,            "FF",             "\f"   },
  { 13,            "CR",             "\r"   },
  { 14,            "SO",             "\x0E" },
  { 15,            "SI",             "\x0F" },
  { 16,            "DLE",            "\x10" },
  { 17,            "DC1",            "\x11" },
  { 18,            "DC2",            "\x12" },
  { 19,            "DC3",            "\x13" },
  { 20,            "DC4",            "\x14" },
  { 21,            "NAK",            "\x15" },
  { 22,            "SYN",            "\x16" },
  { 23,            "ETB",            "\x17" },
  { 24,            "CAN",            "\x18" },
  { 25,            "EM",             "\x19" },
  { 26,            "SUB",            "\x1A" },
  { 27,            "ESC",            "\x1B" },
  { 28,            "FS",             "\x1C" },
  { 29,            "GS",             "\x1D" },
  { 30,            "RS",             "\x1E" },
  { 31,            "US",             "\x1F" },
  { 32,            "SP",             "\x20" },
  { 33,            "!",              "\x21" },
  { 34,            "\"",             "\x22" },
  { 35,            "#",              "\x23" },
  { 36,            "$",              "\x24" },
  { 37,            "%",              "\x25" },
  { 38,            "&",              "\x26" },
  { 39,            "'",              "\x27" },
  { 40,            "(",              "\x28" },
  { 41,            ")",              "\x29" },
  { 42,            "*",              "\x2a" },
  { 43,            "+",              "\x2b" },
  { 44,            ",",              "\x2c" },
  { 45,            "-",              "\x2d" },
  { 46,            ".",              "\x2e" },
  { 47,            "/",              "\x2f" },
  { 48,            "0",              "\x30" },
  { 49,            "1",              "\x31" },
  { 50,            "2",              "\x32" },
  { 51,            "3",              "\x33" },
  { 52,            "4",              "\x34" },
  { 53,            "5",              "\x35" },
  { 54,            "6",              "\x36" },
  { 55,            "7",              "\x37" },
  { 56,            "8",              "\x38" },
  { 57,            "9",              "\x39" },
  { 58,            ":",              "\x3a" },
  { 59,            ";",              "\x3b" },
  { 60,            "<",              "\x3c" },
  { 61,            "=",              "\x3d" },
  { 62,            ">",              "\x3e" },
  { 63,            "?",              "\x3f" },
  { 64,            "@",              "\x40" },
  { 65,            "A",              "\x41" },
  { 66,            "B",              "\x42" },
  { 67,            "C",              "\x43" },
  { 68,            "D",              "\x44" },
  { 69,            "E",              "\x45" },
  { 70,            "F",              "\x46" },
  { 71,            "G",              "\x47" },
  { 72,            "H",              "\x48" },
  { 73,            "I",              "\x49" },
  { 74,            "J",              "\x4a" },
  { 75,            "K",              "\x4b" },
  { 76,            "L",              "\x4c" },
  { 77,            "M",              "\x4d" },
  { 78,            "N",              "\x4e" },
  { 79,            "O",              "\x4f" },
  { 80,            "P",              "\x50" },
  { 81,            "Q",              "\x51" },
  { 82,            "R",              "\x52" },
  { 83,            "S",              "\x53" },
  { 84,            "T",              "\x54" },
  { 85,            "U",              "\x55" },
  { 86,            "V",              "\x56" },
  { 87,            "W",              "\x57" },
  { 88,            "X",              "\x58" },
  { 89,            "Y",              "\x59" },
  { 90,            "Z",              "\x5a" },
  { 91,            "[",              "\x5b" },
  { 92,            "\\",             "\x5c" },
  { 93,            "]",              "\x5d" },
  { 94,            "^",              "\x5e" },
  { 95,            "_",              "\x5f" },
  { 96,            "`",              "\x60" },
  { 97,            "a",              "\x61" },
  { 98,            "b",              "\x62" },
  { 99,            "c",              "\x63" },
  { 100,           "d",              "\x64" },
  { 101,           "e",              "\x65" },
  { 102,           "f",              "\x66" },
  { 103,           "g",              "\x67" },
  { 104,           "h",              "\x68" },
  { 105,           "i",              "\x69" },
  { 106,           "j",              "\x6a" },
  { 107,           "k",              "\x6b" },
  { 108,           "l",              "\x6c" },
  { 109,           "m",              "\x6d" },
  { 110,           "n",              "\x6e" },
  { 111,           "o",              "\x6f" },
  { 112,           "p",              "\x70" },
  { 113,           "q",              "\x71" },
  { 114,           "r",              "\x72" },
  { 115,           "s",              "\x73" },
  { 116,           "t",              "\x74" },
  { 117,           "u",              "\x75" },
  { 118,           "v",              "\x76" },
  { 119,           "w",              "\x77" },
  { 120,           "x",              "\x78" },
  { 121,           "y",              "\x79" },
  { 122,           "z",              "\x7a" },
  { 123,           "{",              "\x7b" },
  { 124,           "|",              "\x7c" },
  { 125,           "}",              "\x7d" },
  { 126,           "~",              "\x7e" },
  { 127,           "DEL",            "\x7f" },
};

int get_known_key_count() {
  int i = 0;
  while (key_table[i].name) i++;
  return i;
}

int get_key_code_by_index(int index) {
  return (index >= 0 && key_table[index].name) ? key_table[index].code : -1;
}

const char* get_key_name_by_index(int index) {
  return (index >= 0 && key_table[index].name) ? key_table[index].name : "";
}

const char* get_key_seq_by_index(int index) {
  return (index >= 0 && key_table[index].name) ? key_table[index].seq : "";
}

int resolve_key_code(const char* name) {
  if (!name) return -1;
  for (int i = 0; key_table[i].name; i++) {
    if (strcmp(key_table[i].name, name) == 0)
      return key_table[i].code;
  }
  return -1;
}

// ------------------------------
// Section: UTF-8 Safe unctrl
// ------------------------------

const char* get_unctrl_utf8(const char* str) {
  static char buf[16];
  unsigned char c = (unsigned char)str[0];
  const char* u = unctrl(c);
  strncpy(buf, u, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  return buf;
}

// ------------------------------
// Section: Modifier Decoding
// ------------------------------

#define MOD_SHIFT  0x01
#define MOD_ALT    0x02
#define MOD_CTRL   0x04

int decode_modifiers(int code) {
  int mods = 0;
  if (code >= 1 && code <= 26) mods |= MOD_CTRL;
  if (code >= 128) mods |= MOD_ALT;
  // add heuristic for shift if needed
  return mods;
}

int is_ctrl_key(int code) {
  return decode_modifiers(code) & MOD_CTRL;
}

int is_alt_key(int code) {
  return decode_modifiers(code) & MOD_ALT;
}

int is_shift_key(int code) {
  return decode_modifiers(code) & MOD_SHIFT;
}

// ------------------------------
// Section: Mouse Parser (SGR)
// ------------------------------


#define SET_X10_MOUSE             9
#define SET_VT200_MOUSE           1000
#define SET_VT200_HIGHLIGHT_MOUSE 1001
#define SET_BTN_EVENT_MOUSE       1002
#define SET_ANY_EVENT_MOUSE       1003
#define SET_FOCUS_EVENT_MOUSE     1004
#define SET_UTF8_EXT_MODE_MOUSE   1005
#define SET_SGR_EXT_MODE_MOUSE    1006
#define SET_ALT_SCROLL_MOUSE      1007
#define SET_URXVT_EXT_MODE_MOUSE  1015
#define SET_PIXEL_POSITION_MOUSE  1016
#define SET_ALT_SCREEN_MODE       1049
#define SET_BRACKETED_PASTE_MODE  2004

typedef enum {
  MOUSE_BUTTON_LEFT        = 0,
  MOUSE_BUTTON_MIDDLE      = 1,
  MOUSE_BUTTON_RIGHT       = 2,
  MOUSE_BUTTON_RELEASE     = 3,
  MOUSE_BUTTON_WHEEL_UP    = 4,
  MOUSE_BUTTON_WHEEL_DOWN  = 5,
  MOUSE_BUTTON_WHEEL_LEFT  = 6,
  MOUSE_BUTTON_WHEEL_RIGHT = 7,
} MouseButton;

typedef enum {
  MOUSE_MOD_NONE   = 0,
  // convert keyboard mods -> mouse mods by shifting 2 bits to the left
  MOUSE_MOD_SHIFT  = MOD_SHIFT << 2,
  MOUSE_MOD_ALT    = MOD_ALT   << 2,
  MOUSE_MOD_CTRL   = MOD_CTRL  << 2,
} MouseModifier;

typedef enum {
  // emitted on button release
  // (except when using SET_X10_MOUSE protocol)
  MOUSE_ACTION_RELEASE     = 0,
  // emitted on button press
  MOUSE_ACTION_PRESS       = 1,
  // emitted on any motion event
  // (with SET_ANY_EVENT_MOUSE enabled)
  MOUSE_ACTION_MOVE        = 2,
  // emitted on motion with button held down
  // (with SET_BTN_EVENT_MOUSE or SET_ANY_EVENT_MOUSE enabled)
  MOUSE_ACTION_DRAG        = 3,
  // emitted on wheel scroll up
  MOUSE_ACTION_WHEEL_UP    = 4,
  // emitted on wheel scroll down
  MOUSE_ACTION_WHEEL_DOWN  = 5,
  // emitted on wheel scroll left
  MOUSE_ACTION_WHEEL_LEFT  = 6,
  // emitted on wheel scroll right
  MOUSE_ACTION_WHEEL_RIGHT = 7,
} MouseAction;

typedef enum {
  MOUSE_PROTOCOL_UNKNOWN = 0,
  MOUSE_PROTOCOL_X10     = 9,
  MOUSE_PROTOCOL_VT200   = 1000,
  MOUSE_PROTOCOL_URXVT   = 1015,
  MOUSE_PROTOCOL_SGR     = 1006,
  MOUSE_PROTOCOL_UTF8    = 1005,
} MouseProtocol;

typedef struct {
  int x;
  int y;
  int button;
  int action;
  int modifiers;
} MouseEvent;

static MouseEvent mouse_result;

MouseEvent* parse_x10_mouse_sequence(const char* seq) {
  // Expected: \x1B [ M b x y
  if (!seq || strncmp(seq, "\x1B[M", 4) != 0) return NULL;

  int b = (unsigned char)seq[4] - 32;
  int x = (unsigned char)seq[5] - 32;
  int y = (unsigned char)seq[6] - 32;

  mouse_result.x = x;
  mouse_result.y = y;
  mouse_result.button = b & 0b11;
  mouse_result.action = 1; // X10 does not distinguish press/release
  mouse_result.modifiers = (b >> 2) & 0b111;

  return &mouse_result;
}

MouseEvent* parse_vt200_mouse_sequence(const char* seq) {
  // Expected: \x1B [ M b x y
  if (!seq || strncmp(seq, "\x1B[M", 4) != 0) return NULL;

  int b = (unsigned char)seq[4] - 32;
  int x = (unsigned char)seq[5] - 32;
  int y = (unsigned char)seq[6] - 32;

  mouse_result.x = x;
  mouse_result.y = y;
  mouse_result.button = b & 0b11;
  mouse_result.action = (b & 0b11) == 3 ? 0 : 1; // release if button == 3
  mouse_result.modifiers = (b >> 2) & 0b111;

  return &mouse_result;
}

MouseEvent* parse_urxvt_mouse_sequence(const char* seq) {
  // Expected: \x1B [ b ; x ; y M
  if (!seq || strncmp(seq, "\x1B[", 3) != 0) return NULL;
  int b = 0, x = 0, y = 0;
  char final = 0;
  sscanf(seq, "\x1B[%d;%d;%d%c", &b, &x, &y, &final);

  mouse_result.x = x;
  mouse_result.y = y;
  mouse_result.button = b & 0b11;
  mouse_result.action = (final == 'M' || final == 'T') ? MOUSE_ACTION_PRESS : (final == 'm' || final == 't') ? MOUSE_ACTION_RELEASE : -1;
  mouse_result.modifiers = (b >> 2) & 0b111;

  return &mouse_result;
}

MouseEvent* parse_sgr_mouse_sequence(const char* seq) {
  // Expected: \x1B [ < b ; x ; y (M or m)
  if (!seq || strncmp(seq, "\x1B[<", 3) != 0) return NULL;

  int b = 0, x = 0, y = 0;
  char final = 0;
  sscanf(seq, "\x1B[<%d;%d;%d%c", &b, &x, &y, &final);

  mouse_result.x = x;
  mouse_result.y = y;
  mouse_result.button = b & 0x3;
  mouse_result.action = (final == 'm') ? 0 : 1;
  mouse_result.modifiers = (b >> 3) & 0xF;

  return &mouse_result;
}

MouseEvent* parse_utf8_mouse_sequence(const char* seq) {
  // Expected: \x1B [ M b x y (UTF-8 encoded)
  if (!seq || strncmp(seq, "\x1B[M", 4) != 0) return NULL;

  // Decode UTF-8 values for b, x, y
  int b = 0, x = 0, y = 0;
  const unsigned char* p = (const unsigned char*)seq + 4;

  // Decode b
  if (*p & 0x80) {
    b = ((*p & 0x3F) << 6);
    p++;
    b |= (*p & 0x3F);
  } else {
    b = *p;
  }
  p++;

  // Decode x
  if (*p & 0x80) {
    x = ((*p & 0x3F) << 6);
    p++;
    x |= (*p & 0x3F);
  } else {
    x = *p;
  }
  p++;

  // Decode y
  if (*p & 0x80) {
    y = ((*p & 0x3F) << 6);
    p++;
    y |= (*p & 0x3F);
  } else {
    y = *p;
  }

  mouse_result.x = x;
  mouse_result.y = y;
  mouse_result.button = b & 0b11;
  mouse_result.action = (b & 0b11) == MOUSE_BUTTON_RELEASE ? MOUSE_ACTION_RELEASE : MOUSE_ACTION_PRESS;
  mouse_result.modifiers = (b >> 2) & 0b111;

  return &mouse_result;
}

MouseEvent* parse_mouse_sequence(const char* seq, int protocol) {
  switch (protocol) {
    case SET_X10_MOUSE:
      return parse_x10_mouse_sequence(seq);
    case SET_VT200_MOUSE:
      return parse_vt200_mouse_sequence(seq);
    case SET_URXVT_EXT_MODE_MOUSE:
      return parse_urxvt_mouse_sequence(seq);
    case SET_SGR_EXT_MODE_MOUSE:
      return parse_sgr_mouse_sequence(seq);
    case SET_UTF8_EXT_MODE_MOUSE:
      return parse_utf8_mouse_sequence(seq);
    default: {
      const char* kmous = tigetstr("kmous");
      if (kmous && strstr(kmous, "\x1B[<")) {
        return parse_sgr_mouse_sequence(seq);
      } else if (kmous) {
        return parse_x10_mouse_sequence(seq);
      } else {
        return NULL;
      }
    }
  }
}

int get_mouse_x() { return mouse_result.x; }
int get_mouse_y() { return mouse_result.y; }
int get_mouse_button() { return mouse_result.button; }
int get_mouse_action() { return mouse_result.action; }
int get_mouse_modifiers() { return mouse_result.modifiers; }

int has_mouse_support() {
  return tigetstr("kmous") != NULL;
}

int has_mouse_events() {
  return has_mouse_support();
}

// check if terminal supports a given mode (for SM/DECSET)
// this requires a two-way communication with the terminal,
// writing a request sequence to stdout/stderr, and reading
// a response sequence from stdin, then parsing the response.
//
// the request sequence is usually of the form "ESC [ ? Ps $p" for
// DECSET parameters, where Ps is the parameter number. for SM
// parameters, it is usually "ESC [ Ps $p".
//
// the response sequence is usually of the form "ESC [ ? Ps ; Rn $y",
// or "ESC [ Ps ; Rn $y" for DECSET and SM parameters respectively,
// where Ps is the parameter number, and Rn is one of the following:
//   - 0 : feature is unsupported / unrecognized
//   - 1 : feature is supported and enabled
//   - 2 : feature is supported but disabled
//   - 3 : feature is supported and permanently enabled
//   - 4 : feature is supported but permanently disabled
#ifndef O_NONBLOCK
  #define O_NONBLOCK 2048
#endif
#ifndef F_GETFL
  #define F_GETFL 3
#endif
#ifndef F_SETFL
  #define F_SETFL 4
#endif

int request_mode_status(int param, int dec) {
  // read current terminal settings
  struct termios oldt;
  tcgetattr(STDIN_FILENO, &oldt);

  // enable non-blocking read on stdin
  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

  // Flush input/output buffers
  tcflush(STDIN_FILENO, TCIFLUSH);
  tcflush(STDOUT_FILENO, TCOFLUSH);

  // enable raw mode
  enter_raw_mode();

  // set a timeout for reading response
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 200000; // 200 ms

  // set up file descriptor set for select
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(STDIN_FILENO, &readfds);
  select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);

  // input is available
  if (FD_ISSET(STDIN_FILENO, &readfds)) {
    // send request sequence
    char request[32], response[64];
    snprintf(request, sizeof(request), dec ? "\x1B[?%d$p" : "\x1B[%d$p", param);
    write(STDOUT_FILENO, request, strlen(request));

    // read response sequence
    ssize_t n = read(STDIN_FILENO, response, sizeof(response) - 1);

    tcsetattr(STDIN_FILENO, TCSANOW,  &oldt); // restore termios

    if (n > 0) {
      // null-terminate the response string
      response[n] = '\0';

      // parse response sequence
      int resp_param = 0, resp_value = 0;
      if (dec) {
        sscanf(response, "\x1B[?%d;%d$y", &resp_param, &resp_value);
      } else {
        sscanf(response, "\x1B[%d;%d$y", &resp_param, &resp_value);
      }

      if (resp_param == param) {
        return resp_value;
      }
    }
  }
  // restore terminal settings
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  // timeout or no input available
  return -1;
}

int is_mode_supported(int param, int dec) {
  int status = request_mode_status(param, dec);
  return (status > 0 && status < 5);
}

int is_mode_settable(int param, int dec) {
  int status = request_mode_status(param, dec);
  return (status == 1 || status == 2);
}

int is_mode_permanent(int param, int dec) {
  int status = request_mode_status(param, dec);
  return (status == 3 || status == 4);
}

int is_mode_enabled(int param, int dec) {
  int status = request_mode_status(param, dec);
  return (status == 1 || status == 3);
}

int is_mode_disabled(int param, int dec) {
  int status = request_mode_status(param, dec);
  return (status == 2 || status == 4);
}

// feature-specific support checks
int has_bracketed_paste() {
  return is_mode_supported(SET_BRACKETED_PASTE_MODE, 1);
}

int has_x10_mouse() {
  return is_mode_supported(SET_X10_MOUSE, 1);
}

int has_vt200_mouse() {
  return is_mode_supported(SET_VT200_MOUSE, 1);
}

int has_vt200_highlight_mouse() {
  return is_mode_supported(SET_VT200_HIGHLIGHT_MOUSE, 1);
}

int has_btn_event_mouse() {
  return is_mode_supported(SET_BTN_EVENT_MOUSE, 1);
}

int has_any_event_mouse() {
  return is_mode_supported(SET_ANY_EVENT_MOUSE, 1);
}

int has_focus_events() {
  return is_mode_supported(SET_FOCUS_EVENT_MOUSE, 1);
}

int has_utf8_mouse() {
  return is_mode_supported(SET_UTF8_EXT_MODE_MOUSE, 1);
}

int has_sgr_mouse() {
  return is_mode_supported(SET_SGR_EXT_MODE_MOUSE, 1);
}

int has_alt_scroll() {
  return is_mode_supported(SET_ALT_SCROLL_MOUSE, 1);
}

int has_urxvt_mouse() {
  return is_mode_supported(SET_URXVT_EXT_MODE_MOUSE, 1);
}

int has_pixel_mouse() {
  return is_mode_supported(SET_PIXEL_POSITION_MOUSE, 1);
}

int has_alt_screen_mode() {
  return is_mode_supported(SET_ALT_SCREEN_MODE, 1);
}

// ------------------------------
// Section: Termios Raw/CBreak
// ------------------------------

static struct termios original_termios;

int enter_raw_mode() {
  struct termios raw;
  if (tcgetattr(STDIN_FILENO, &original_termios) == -1) return -1;
  raw = original_termios;
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  return tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int exit_raw_mode() {
  struct termios current;
  if (tcgetattr(STDIN_FILENO, &current) == -1) return -1;
  // enable local echo, canonical mode, extended input processing, and signals
  current.c_lflag |= (ECHO | ICANON | IEXTEN | ISIG);
  // enable input processing flags
  // (IXON: start/stop output control, ICRNL: map CR to NL, BRKINT: signal interrupt on break, INPCK: enable input parity checking, ISTRIP: strip 8th bit)
  current.c_iflag |= (IXON | ICRNL | BRKINT | INPCK | ISTRIP);
  // enable output processing (post-processing of output)
  current.c_oflag |= (OPOST);
  // set character size to 8 bits per byte
  current.c_cflag &= ~(CS8);
  return tcsetattr(STDIN_FILENO, TCSAFLUSH, &current);
}

int enter_cbreak_mode() {
  struct termios cb;
  if (tcgetattr(STDIN_FILENO, &original_termios) == -1) return -1;
  cb = original_termios;
  // disable local echo and canonical mode (cbreak mode)
  cb.c_lflag &= ~(ICANON | ECHO);
  // enable signal generation (like Ctrl+C)
  cb.c_lflag |= ISIG;
  // disable software flow control
  cb.c_iflag &= ~(IXON);
  // set character size to 8 bits per byte
  cb.c_cflag |= (CS8);
  // minimum number of bytes for non-canonical read
  cb.c_cc[VMIN] = 1;
  // timeout (in deciseconds) for non-canonical read
  cb.c_cc[VTIME] = 0;

  return tcsetattr(STDIN_FILENO, TCSAFLUSH, &cb);
}

int exit_cbreak_mode() {
  struct termios current;
  if (tcgetattr(STDIN_FILENO, &current) == -1) return -1;
  // enable local echo, canonical mode, and signal generation
  current.c_lflag |= (ECHO | ICANON | ISIG);
  // enable software flow control
  current.c_iflag |= IXON;

  return tcsetattr(STDIN_FILENO, TCSAFLUSH, &current);
}

int restore_terminal_mode() {
  return tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

int is_terminal_raw() {
  struct termios current;
  if (tcgetattr(STDIN_FILENO, &current) == -1) return -1;
  return (current.c_lflag & (ECHO | ICANON | IEXTEN | ISIG)) == 0;
}

int is_terminal_cbreak() {
  struct termios current;
  if (tcgetattr(STDIN_FILENO, &current) == -1) return -1;
  return (current.c_lflag & ICANON) == 0;// && (current.c_lflag & ISIG);
}

int is_terminal_normal() {
  struct termios current;
  if (tcgetattr(STDIN_FILENO, &current) == -1) return -1;
  return (current.c_lflag & (ECHO | ICANON | IEXTEN | ISIG)) == (ECHO | ICANON | IEXTEN | ISIG);
}
