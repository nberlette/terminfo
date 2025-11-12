// addon.cc
#define NAPI_VERSION 10

#include <node_api.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

// Pull in curses for color_content fallback (so we don't rely on a custom shim)
#include <curses.h>
#include <term.h>

// ---- FFI (C symbols from terminfo.c) ----
#define MOD_SHIFT                 1
#define MOD_ALT                   2
#define MOD_CTRL                  4
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

extern "C" {
  int init_terminfo(const char* termname);
  int get_term_cols();
  int get_term_lines();
  int get_baudrate();
  int get_erasechar();
  int get_killchar();
  int can_change_color_support();

  int get_num_pairs();
  int get_num_colors();

  int get_stdin_fd();
  int get_stdout_fd();
  int get_stderr_fd();
  int is_fd_atty(int fd);

  int get_tigetnum(const char* cap);
  int get_tigetflag(const char* cap);
  const char* get_tigetstr(const char* cap);

  const char* get_termname();
  const char* get_longname();

  const char* format_cap(const char* cap,
                         int, int, int, int, int, int, int, int, int);
  const char* render_sgr(int bold, int underline, int blink, int reverse,
                         int fg, int bg);
  const char* get_boxchar(int style, int part);

  int has_alt_screen();
  const char* enter_alt_screen();
  const char* exit_alt_screen();

  const char* enable_mouse();
  const char* disable_mouse();

  const char* enable_keyboard_action_mode();
  const char* disable_keyboard_action_mode();

  const char* cursor_hide();
  const char* cursor_show();
  const char* cursor_save();
  const char* cursor_restore();
  const char* cursor_move(int row, int col);

  const char* erase_screen();
  const char* erase_line();
  const char* erase_lines(int n);
  const char* ech(int n);
  const char* dch(int n);
  const char* ich(int n);

  const char* hpa(int col);
  const char* hpr(int col);
  const char* vpa(int row);
  const char* vpr(int row);
  const char* home();
  const char* cup(int row, int col);
  const char* cuu(int n);
  const char* cud(int n);
  const char* cuf(int n);
  const char* cub(int n);
  const char* cuu1();
  const char* cud1();
  const char* cuf1();
  const char* cub1();

  int get_known_key_count();
  int get_key_code_by_index(int index);
  const char* get_key_name_by_index(int index);
  const char* get_key_seq_by_index(int index);
  int resolve_key_code(const char* name);

  const char* get_unctrl_utf8(const char* str);

  int decode_modifiers(int code);
  int is_ctrl_key(int code);
  int is_alt_key(int code);
  int is_shift_key(int code);

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
    MOUSE_PROTOCOL_X10     = SET_X10_MOUSE,
    MOUSE_PROTOCOL_VT200   = SET_VT200_MOUSE,
    MOUSE_PROTOCOL_URXVT   = SET_URXVT_EXT_MODE_MOUSE,
    MOUSE_PROTOCOL_SGR     = SET_SGR_EXT_MODE_MOUSE,
    MOUSE_PROTOCOL_UTF8    = SET_UTF8_EXT_MODE_MOUSE,
  } MouseProtocol;

  typedef struct {
    int x;
    int y;
    int button;
    int action;
    int modifiers;
  } MouseEvent;

  MouseEvent* parse_x10_mouse_sequence(const char* seq);
  MouseEvent* parse_vt200_mouse_sequence(const char* seq);
  MouseEvent* parse_urxvt_mouse_sequence(const char* seq);
  MouseEvent* parse_sgr_mouse_sequence(const char* seq);
  MouseEvent* parse_utf8_mouse_sequence(const char* seq);
  MouseEvent* parse_mouse_sequence(const char* seq, int protocol);

  int get_mouse_x();
  int get_mouse_y();
  int get_mouse_button();
  int get_mouse_action();
  int get_mouse_modifiers();

  int has_colors_support();
  int has_mouse_support();
  int has_mouse_events();
  int has_any_event_mouse();
  int has_any_event_mouse();
  int has_focus_events();
  int has_x10_mouse();
  int has_vt200_mouse();
  int has_vt200_highlight_mouse();
  int has_utf8_mouse();
  int has_sgr_mouse();
  int has_alt_scroll();
  int has_urxvt_mouse();
  int has_pixel_mouse();
  int has_bracketed_paste();
  int has_alt_screen_mode();

  int request_mode_status(int mode, int dec);
  int is_mode_supported(int mode, int dec);
  int is_mode_settable(int mode, int dec);
  int is_mode_permanent(int mode, int dec);
  int is_mode_enabled(int mode, int dec);
  int is_mode_disabled(int mode, int dec);

  int enter_raw_mode();
  int exit_raw_mode();
  int is_terminal_raw();

  int enter_cbreak_mode();
  int exit_cbreak_mode();
  int is_terminal_cbreak();

  int restore_terminal_mode();
  int is_terminal_normal();
}

// type alias: str -> const char*
using str = const char*;

// ---- Small N-API helpers ----

static inline napi_value make_int(napi_env env, int32_t v) {
  napi_value out;
  assert(napi_create_int32(env, v, &out) == napi_ok);
  return out;
}

static inline napi_value make_bool(napi_env env, bool v) {
  napi_value out;
  assert(napi_get_boolean(env, v, &out) == napi_ok);
  return out;
}

static inline napi_value make_str(napi_env env, const char* s) {
  if (!s) s = "";
  napi_value out;
  assert(napi_create_string_utf8(env, s, NAPI_AUTO_LENGTH, &out) == napi_ok);
  return out;
}

static inline const char* get_str(napi_env env, napi_value v, char* buf, size_t buflen) {
  if (!buf || buflen == 0) return "";
  size_t len = 0;
  if (napi_get_value_string_utf8(env, v, NULL, 0, &len) != napi_ok) { buf[0] = 0; return buf; }
  if (len + 1 > buflen) len = buflen - 1;
  if (napi_get_value_string_utf8(env, v, buf, buflen, &len) != napi_ok) { buf[0] = 0; return buf; }
  buf[len] = 0;
  return buf;
}

static inline int32_t get_int(napi_env env, napi_value v) {
  int32_t x = 0;
  (void)napi_get_value_int32(env, v, &x);
  return x;
}

static inline void set_prop(napi_env env, napi_value obj, const char* key, napi_value val) {
  assert(napi_set_named_property(env, obj, key, val) == napi_ok);
}

static inline napi_value make_obj(napi_env env) {
  napi_value o;
  assert(napi_create_object(env, &o) == napi_ok);
  return o;
}

static inline void load_args(napi_env env, napi_callback_info info,
                             size_t max, size_t* argc, napi_value* argv) {
  *argc = max;
  assert(napi_get_cb_info(env, info, argc, argv, NULL, NULL) == napi_ok);
}

// ----------------------------------
// Macros for simple functions
// ----------------------------------

/**
 * Helper macros for defining templated function wrappers around
 * functions with fixed parameter counts from terminfo.c. This is
 * mainly to reduce boilerplate, as these functions are very
 * similar in structure.
 *
 * Usage:
 *    FN0(fn_name, c_fn, ret_t)
 *    FN1(fn_name, c_fn, p1, p1_t, p1_d, ret_t)
 *    FN2(fn_name, c_fn, p1, p1_t, p1_d, p2, p2_t, p2_d, ret_t)
 *    ...
 *
 * Where:
 * >  - fn_name: Name of the generated N-API function
 * >  - c_fn: Target C function defined in terminfo.c
 * >  - pN: Name of the N-th parameter, used for naming variables
 * >  - pN_t: either 'int', 'str', or 'bool' (without quotes)
 * >      - used to coerce JavaScript values to C types
 * >  - pN_d: default value for the N-th parameter if not provided
 * >      - must be a literal or valid C expression of the appropriate type
 * >  - ret_t: either 'int', 'str', or 'bool' (without quotes)
 * >      - determines the type of the value returned to JavaScript
 *
 * Example:
 *   FN0(fn_cursor_home, cursor_home, str)
 * ... expands to ...
 *    static napi_value fn_cursor_home(napi_env env, napi_callback_info info) {
 *       (void)info;
 *       return make_str(env, cursor_home());
 *    }
 *
 *   FN2(fn_cursor_move, cursor_move, row, int, col, int, str)
 * ... expands to ...
 *    static napi_value fn_cursor_move(napi_env env, napi_callback_info info) {
 *       napi_value argv[2]; size_t argc = 2;
 *       load_args(env, info, 2, &argc, argv);
 *       int row = argc > 0 ? get_int(env, argv[0]) : 0;
 *       int col = argc > 1 ? get_int(env, argv[1]) : 0;
 *       return make_str(env, cursor_move(row, col));
 *    }
 */
#define FN0(fn_name, c_fn, ret_t)                                  \
static napi_value fn_name(napi_env env, napi_callback_info info) { \
  (void)info; return make_##ret_t(env, c_fn());                    \
}

#define FN1(fn_name, c_fn, p1, p1_t, p1_d, ret_t)                  \
static napi_value fn_name(napi_env env, napi_callback_info info) { \
  napi_value argv[1]; size_t argc = 1;                             \
  load_args(env, info, 1, &argc, argv);                            \
  p1_t p1 = argc > 0 ? get_##p1_t(env, argv[0]) : p1_d;            \
  return make_##ret_t(env, c_fn(p1));                              \
}

#define FZ1(fn_name, c_fn, p1, p1_t, p1_d, ret_t)                  \
static napi_value fn_name(napi_env env, napi_callback_info info) { \
  napi_value argv[1]; size_t argc = 1;                             \
  load_args(env, info, 1, &argc, argv);                            \
  p1_t p1 = argc > 0 ? get_##p1_t(env, argv[0]) : p1_d;            \
  return make_##ret_t(env, c_fn(p1) != 0);                         \
}

/*
 * Like `FN1`, but specifically for functions with string arguments,
 * which need a temporary buffer for conversion.
 */
#define FS1(fn_name, c_fn, p1, p1_d, b1_len, ret_t)                \
static napi_value fn_name(napi_env env, napi_callback_info info) { \
  napi_value argv[1]; size_t argc = 1;                             \
  load_args(env, info, 1, &argc, argv);                            \
  static char buf1[b1_len];                                        \
  if (argc > 0) {                                                  \
    get_str(env, argv[0], buf1, sizeof(buf1));                     \
  } else {                                                         \
    strncpy(buf1, p1_d, sizeof(buf1) - 1);                         \
    buf1[sizeof(buf1) - 1] = '\0';                                 \
  }                                                                \
  return make_##ret_t(env, c_fn(buf1));                            \
}

#define FN2(fn_name, c_fn, p1, p1_t, p1_d, p2, p2_t, p2_d, ret_t)  \
static napi_value fn_name(napi_env env, napi_callback_info info) { \
  napi_value argv[2]; size_t argc = 2;                             \
  load_args(env, info, 2, &argc, argv);                            \
  p1_t p1 = argc > 0 ? get_##p1_t(env, argv[0]) : p1_d;            \
  p2_t p2 = argc > 1 ? get_##p2_t(env, argv[1]) : p2_d;            \
  return make_##ret_t(env, c_fn(p1, p2));                          \
}

#define FZ2(fn_name, c_fn, p1, p1_t, p1_d, p2, p2_t, p2_d, ret_t)  \
static napi_value fn_name(napi_env env, napi_callback_info info) { \
  napi_value argv[2]; size_t argc = 2;                             \
  load_args(env, info, 2, &argc, argv);                            \
  p1_t p1 = argc > 0 ? get_##p1_t(env, argv[0]) : p1_d;            \
  p2_t p2 = argc > 1 ? get_##p2_t(env, argv[1]) : p2_d;            \
  return make_##ret_t(env, c_fn(p1, p2) != 0);                     \
}

// ----------------------------------
// Bindings
// ----------------------------------

static napi_value fn_init(napi_env env, napi_callback_info info) {
  napi_value argv[1]; size_t argc = 1;
  load_args(env, info, 1, &argc, argv);

  const char* name_c = NULL;
  char buf[256];
  if (argc >= 1) {
    napi_valuetype t;
    napi_typeof(env, argv[0], &t);
    if (t == napi_string) name_c = get_str(env, argv[0], buf, sizeof(buf));
  }
  return make_int(env, init_terminfo(name_c));
}

static napi_value fn_info(napi_env env, napi_callback_info info) {
  (void)info;
  napi_value o = make_obj(env);
  set_prop(env, o, "termname",           make_str(env,  get_termname()));
  set_prop(env, o, "longname",           make_str(env,  get_longname()));
  set_prop(env, o, "columns",            make_int(env,  get_term_cols()));
  set_prop(env, o, "lines",              make_int(env,  get_term_lines()));
  set_prop(env, o, "baudrate",           make_int(env,  get_baudrate()));
  set_prop(env, o, "erasechar",          make_int(env,  get_erasechar()));
  set_prop(env, o, "killchar",           make_int(env,  get_killchar()));
  set_prop(env, o, "stdin_fd",           make_int(env,  get_stdin_fd()));
  set_prop(env, o, "stdout_fd",          make_int(env,  get_stdout_fd()));
  set_prop(env, o, "stderr_fd",          make_int(env,  get_stderr_fd()));
  set_prop(env, o, "num_pairs",          make_int(env,  get_num_pairs()));
  set_prop(env, o, "num_colors",         make_int(env,  get_num_colors()));
  set_prop(env, o, "is_terminal_raw",    make_bool(env, is_terminal_raw()));
  set_prop(env, o, "is_terminal_cbreak", make_bool(env, is_terminal_cbreak()));
  set_prop(env, o, "is_terminal_normal", make_bool(env, is_terminal_normal()));
  set_prop(env, o, "can_change_color",   make_bool(env, can_change_color_support()));
  set_prop(env, o, "has_colors",         make_bool(env, has_colors_support()));
  set_prop(env, o, "has_alt_screen",     make_bool(env, has_alt_screen()));
  set_prop(env, o, "has_focus_events",   make_bool(env, has_focus_events()));
  set_prop(env, o, "has_mouse_events",   make_bool(env, has_mouse_events()));
  set_prop(env, o, "has_x10_mouse",      make_bool(env, has_x10_mouse()));
  set_prop(env, o, "has_vt200_mouse",    make_bool(env, has_vt200_mouse()));
  set_prop(env, o, "has_utf8_mouse",     make_bool(env, has_utf8_mouse()));
  set_prop(env, o, "has_sgr_mouse",      make_bool(env, has_sgr_mouse()));
  set_prop(env, o, "has_alt_scroll",     make_bool(env, has_alt_scroll()));
  set_prop(env, o, "has_urxvt_mouse",    make_bool(env, has_urxvt_mouse()));
  set_prop(env, o, "has_pixel_mouse",    make_bool(env, has_pixel_mouse()));
  return o;
}

static napi_value fn_tigetnum(napi_env env, napi_callback_info info) {
  napi_value argv[1]; size_t argc = 1;
  load_args(env, info, 1, &argc, argv);
  if (argc < 1) return make_int(env, -1);
  char cap[128];
  get_str(env, argv[0], cap, sizeof(cap));
  return make_int(env, get_tigetnum(cap));
}

static napi_value fn_tigetflag(napi_env env, napi_callback_info info) {
  napi_value argv[1]; size_t argc = 1;
  load_args(env, info, 1, &argc, argv);
  if (argc < 1) return make_int(env, -1);
  char cap[128];
  get_str(env, argv[0], cap, sizeof(cap));
  return make_int(env, get_tigetflag(cap));
}

static napi_value fn_tigetstr(napi_env env, napi_callback_info info) {
  napi_value argv[1]; size_t argc = 1;
  load_args(env, info, 1, &argc, argv);
  if (argc < 1) return make_str(env, "");
  char cap[128];
  get_str(env, argv[0], cap, sizeof(cap));
  return make_str(env, get_tigetstr(cap));
}

static napi_value fn_tparm(napi_env env, napi_callback_info info) {
  napi_value argv[10]; size_t argc = 10;
  load_args(env, info, 10, &argc, argv);
  if (argc < 1) return make_str(env, "");
  char cap[128];
  get_str(env, argv[0], cap, sizeof(cap));
  int a[9] = {0};
  for (size_t i = 1; i < argc && i <= 9; i++) a[i - 1] = get_int(env, argv[i]);
  return make_str(env, format_cap(cap, a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8]));
}

static napi_value fn_sgr(napi_env env, napi_callback_info info) {
  napi_value argv[6]; size_t argc = 6;
  load_args(env, info, 6, &argc, argv);
  int bold = argc > 0 ? get_int(env, argv[0]) : 0;
  int underline = argc > 1 ? get_int(env, argv[1]) : 0;
  int blink = argc > 2 ? get_int(env, argv[2]) : 0;
  int reverse = argc > 3 ? get_int(env, argv[3]) : 0;
  int fg = argc > 4 ? get_int(env, argv[4]) : -1;
  int bg = argc > 5 ? get_int(env, argv[5]) : -1;
  return make_str(env, render_sgr(bold, underline, blink, reverse, fg, bg));
}

static napi_value fn_box_char(napi_env env, napi_callback_info info) {
  napi_value argv[2]; size_t argc = 2;
  load_args(env, info, 2, &argc, argv);
  int style = argc > 0 ? get_int(env, argv[0]) : 0;
  int part  = argc > 1 ? get_int(env, argv[1]) : 0;
  return make_str(env, get_boxchar(style, part));
}

// Use curses directly to avoid relying on a custom C shim that your compiler denies exists.
static inline int color_rgb_bridge(short i, short* r, short* g, short* b) {
  return color_content(i, r, g, b); // OK (0) or ERR (-1)
}

static napi_value fn_color_rgb(napi_env env, napi_callback_info info) {
  napi_value argv[1]; size_t argc = 1;
  load_args(env, info, 1, &argc, argv);
  short idx = argc > 0 ? (short)get_int(env, argv[0]) : 0;
  short r=0,g=0,b=0;
  int rc = color_rgb_bridge(idx, &r, &g, &b);

  napi_value o = make_obj(env);
  set_prop(env, o, "ok", make_bool(env, rc == 0));
  set_prop(env, o, "r", make_int(env, (int)r));
  set_prop(env, o, "g", make_int(env, (int)g));
  set_prop(env, o, "b", make_int(env, (int)b));
  return o;
}

static napi_value fn_cap(napi_env env, napi_callback_info info) {
  napi_value argv[1]; size_t argc = 1;
  load_args(env, info, 1, &argc, argv);
  if (argc < 1) return make_str(env, "");
  char what[64]; get_str(env, argv[0], what, sizeof(what));
  if (strcmp(what, "smcup") == 0)         return make_str(env, enter_alt_screen());
  if (strcmp(what, "rmcup") == 0)         return make_str(env, exit_alt_screen());
  if (strcmp(what, "civis") == 0)         return make_str(env, cursor_hide());
  if (strcmp(what, "cnorm") == 0)         return make_str(env, cursor_show());
  if (strcmp(what, "sc") == 0)            return make_str(env, cursor_save());
  if (strcmp(what, "rc") == 0)            return make_str(env, cursor_restore());
  if (strcmp(what, "clear") == 0)         return make_str(env, erase_screen());
  if (strcmp(what, "home") == 0)          return make_str(env, home());
  if (strcmp(what, "cursor_home") == 0)   return make_str(env, home());
  if (strcmp(what, "enable_mouse") == 0)  return make_str(env, enable_mouse());
  if (strcmp(what, "disable_mouse") == 0) return make_str(env, disable_mouse());
  if (strcmp(what, "smkx") == 0)          return make_str(env, enable_keyboard_action_mode());
  if (strcmp(what, "rmkx") == 0)          return make_str(env, disable_keyboard_action_mode());
  if (strcmp(what, "cup") == 0)           return make_str(env, cup(0, 0));

  return make_str(env, get_tigetstr(what));
}

FN0(fn_ed, erase_screen,                                 str);
FN1(fn_el,  erase_lines,    n,  int, 1,                  str);
FN1(fn_ech,         ech,    n,  int, 1,                  str);
FN1(fn_dch,         dch,    n,  int, 1,                  str);
FN1(fn_ich,         ich,    n,  int, 1,                  str);
FN2(fn_cup,         cup,    r,  int, 0,    c, int, 0,    str);
FN1(fn_cuu,         cuu,    n,  int, 1,                  str);
FN1(fn_cud,         cud,    n,  int, 1,                  str);
FN1(fn_cuf,         cuf,    n,  int, 1,                  str);
FN1(fn_cub,         cub,    n,  int, 1,                  str);
FN1(fn_hpa,         hpa,  col,  int, 1,                  str);
FN1(fn_hpr,         hpr,  col,  int, 1,                  str);
FN1(fn_vpa,         vpa,  row,  int, 1,                  str);
FN1(fn_vpr,         vpr,  row,  int, 1,                  str);
FN0(fn_cuu1,       cuu1,                                 str);
FN0(fn_cud1,       cud1,                                 str);
FN0(fn_cuf1,       cuf1,                                 str);
FN0(fn_cub1,       cub1,                                 str);
FN0(fn_home,       home,                                 str);

FN0(fn_cursor_home,        home,                         str);
FN0(fn_cursor_hide,        cursor_hide,                  str);
FN0(fn_cursor_show,        cursor_show,                  str);
FN0(fn_cursor_save,        cursor_save,                  str);
FN0(fn_cursor_restore,     cursor_restore,               str);

FN0(fn_exit_alt_screen,      exit_alt_screen,            str);
FN0(fn_enter_alt_screen,     enter_alt_screen,           str);
FN0(fn_has_alt_screen,       has_alt_screen,             bool);
FN0(fn_has_colors,           has_colors_support,         bool);
FN0(fn_get_num_pairs,        get_num_pairs,              int);
FN0(fn_get_num_colors,       get_num_colors,             int);
FN0(fn_can_change_color,     can_change_color_support,   bool);
FN0(fn_enable_mouse,         enable_mouse,               str);
FN0(fn_disable_mouse,        disable_mouse,              str);
FN0(fn_erase_screen,         erase_screen,               str);
FN0(fn_erase_line,           erase_line,                 str);
FN1(fn_erase_lines,          erase_lines, n, int, 1,     str);
FN0(fn_get_mouse_x,          get_mouse_x,                int);
FN0(fn_get_mouse_y,          get_mouse_y,                int);
FN0(fn_get_mouse_button,     get_mouse_button,           int);
FN0(fn_get_mouse_action,     get_mouse_action,           int);
FN0(fn_get_mouse_modifiers,  get_mouse_modifiers,        int);
FN0(fn_has_mouse_support,    has_mouse_support,          bool);
FN0(fn_has_bracketed_paste,  has_bracketed_paste,        bool);
FN0(fn_has_focus_events,     has_focus_events,           bool);
FN0(fn_has_mouse_events,     has_mouse_events,           bool);
FN0(fn_has_x10_mouse,        has_x10_mouse,              bool);
FN0(fn_has_vt200_mouse,      has_vt200_mouse,            bool);
FN0(fn_has_utf8_mouse,       has_utf8_mouse,             bool);
FN0(fn_has_sgr_mouse,        has_sgr_mouse,              bool);
FN0(fn_has_alt_scroll,       has_alt_scroll,             bool);
FN0(fn_has_urxvt_mouse,      has_urxvt_mouse,            bool);
FN0(fn_has_pixel_mouse,      has_pixel_mouse,            bool);

FN2(fn_request_mode_status,  request_mode_status,        mode, int, 0,  dec, int, 0, int);
FN2(fn_is_mode_supported,    is_mode_supported,          mode, int, 0,  dec, int, 0, bool);
FN2(fn_is_mode_settable,     is_mode_settable,           mode, int, 0,  dec, int, 0, bool);
FN2(fn_is_mode_enabled,      is_mode_enabled,            mode, int, 0,  dec, int, 0, bool);
FN2(fn_is_mode_disabled,     is_mode_disabled,           mode, int, 0,  dec, int, 0, bool);
FN2(fn_is_mode_permanent,    is_mode_permanent,          mode, int, 0,  dec, int, 0, bool);

FN0(fn_is_terminal_raw,      is_terminal_raw,            bool);
FN0(fn_enter_raw_mode,       enter_raw_mode,             bool);
FN0(fn_exit_raw_mode,        exit_raw_mode,              bool);

FN0(fn_is_terminal_cbreak,   is_terminal_cbreak,         bool);
FN0(fn_enter_cbreak_mode,    enter_cbreak_mode,          bool);
FN0(fn_exit_cbreak_mode,     exit_cbreak_mode,           bool);

FN0(fn_is_terminal_normal,   is_terminal_normal,         bool);
FN0(fn_restore,              restore_terminal_mode,      bool);

FN1(fn_isatty,               is_fd_atty,  fd, int, 0,    bool);

FN0(fn_get_key_count,        get_known_key_count,        bool);
FS1(fn_resolve_key_code,     resolve_key_code,           name,  "", 128,  int);
FN1(fn_get_key_modifiers,    decode_modifiers,           code, int,   0,  int);
FZ1(fn_is_ctrl,              is_ctrl_key,                code, int,   0,  bool);
FZ1(fn_is_alt,               is_alt_key,                 code, int,   0,  bool);
FZ1(fn_is_shift,             is_shift_key,               code, int,   0,  bool);

FN0(fn_disable_keyboard_action_mode,   disable_keyboard_action_mode,      str);
FN0(fn_enable_keyboard_action_mode,    enable_keyboard_action_mode,       str);

static napi_value fn_keys(napi_env env, napi_callback_info info) {
  (void)info;
  int n = get_known_key_count();
  if (n < 0) n = 0;
  napi_value arr;
  assert(napi_create_array_with_length(env, (size_t)n, &arr) == napi_ok);
  for (int i = 0; i < n; i++) {
    napi_value item = make_obj(env);
    set_prop(env, item, "name", make_str(env, get_key_name_by_index(i)));
    set_prop(env, item, "code", make_int(env, get_key_code_by_index(i)));
    set_prop(env, item, "seq",  make_str(env, get_key_seq_by_index(i)));
    assert(napi_set_element(env, arr, (uint32_t)i, item) == napi_ok);
  }
  return arr;
}

static napi_value fn_keycode(napi_env env, napi_callback_info info) {
  napi_value argv[1]; size_t argc = 1;
  load_args(env, info, 1, &argc, argv);
  if (argc < 1) return make_int(env, -1);
  char name[128];
  get_str(env, argv[0], name, sizeof(name));
  return make_int(env, resolve_key_code(name));
}

static napi_value fn_unctrl(napi_env env, napi_callback_info info) {
  napi_value argv[1]; size_t argc = 1;
  load_args(env, info, 1, &argc, argv);
  if (argc < 1) return make_str(env, "");
  char s[8];
  get_str(env, argv[0], s, sizeof(s));
  return make_str(env, get_unctrl_utf8(s));
}

static napi_value fn_parse_mouse(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value argv[argc];
  load_args(env, info, 1, &argc, argv);
  napi_value out = make_obj(env);
  if (argc < 1) return out;
  char seq[256];
  int protocol = 0;
  get_str(env, argv[0], seq, sizeof(seq));
  if (argc > 1) {
    protocol = get_int(env, argv[1]);
  } else {
    // Auto-detect protocol
    if (has_sgr_mouse() && parse_sgr_mouse_sequence(seq)) {
      protocol = 3;
    } else if (has_x10_mouse() && parse_x10_mouse_sequence(seq)) {
      protocol = 1;
    } else if (has_vt200_mouse() && parse_vt200_mouse_sequence(seq)) {
      protocol = 2;
    } else if (has_urxvt_mouse() && parse_urxvt_mouse_sequence(seq)) {
      protocol = 4;
    } else if (has_utf8_mouse() && parse_utf8_mouse_sequence(seq)) {
      protocol = 5;
    } else {
      protocol = 0; // Unknown
    }
  }

  set_prop(env, out, "protocol",  make_int(env, protocol));

  MouseEvent* m = parse_mouse_sequence(seq, protocol);
  if (!m) return out;
  set_prop(env, out, "x",         make_int(env, get_mouse_x()));
  set_prop(env, out, "y",         make_int(env, get_mouse_y()));
  set_prop(env, out, "button",    make_int(env, get_mouse_button()));
  set_prop(env, out, "action",    make_int(env, get_mouse_action()));
  set_prop(env, out, "modifiers", make_int(env, get_mouse_modifiers()));
  return out;
}


// ----------------------------------
// Module init
// ----------------------------------

static void define(napi_env env, napi_value exports,
                   const char* name, napi_callback cb) {
  napi_value fn;
  assert(napi_create_function(env, name, NAPI_AUTO_LENGTH, cb, NULL, &fn) == napi_ok);
  assert(napi_set_named_property(env, exports, name, fn) == napi_ok);
}

#define DEFINE_FN(name, cb) define(env, exports, name, cb)

NAPI_MODULE_INIT() {
  DEFINE_FN("init",                 fn_init);
  DEFINE_FN("info",                 fn_info);
  DEFINE_FN("tigetnum",             fn_tigetnum);
  DEFINE_FN("tigetflag",            fn_tigetflag);
  DEFINE_FN("tigetstr",             fn_tigetstr);
  DEFINE_FN("tparm",                fn_tparm);
  DEFINE_FN("sgr",                  fn_sgr);
  DEFINE_FN("box_char",             fn_box_char);
  DEFINE_FN("color_rgb",            fn_color_rgb);
  DEFINE_FN("cap",                  fn_cap);

  DEFINE_FN("has_colors",           fn_has_colors);
  DEFINE_FN("num_colors",           fn_get_num_colors);
  DEFINE_FN("num_pairs",            fn_get_num_pairs);
  DEFINE_FN("can_change_color",     fn_can_change_color);

  DEFINE_FN("keys",                 fn_keys);
  DEFINE_FN("keycode",              fn_keycode);
  DEFINE_FN("unctrl",               fn_unctrl);
  DEFINE_FN("resolve_key_code",     fn_resolve_key_code);
  DEFINE_FN("get_key_count",        fn_get_key_count);
  DEFINE_FN("get_key_modifiers",    fn_get_key_modifiers);

  DEFINE_FN("is_ctrl",              fn_is_ctrl);
  DEFINE_FN("is_alt",               fn_is_alt);
  DEFINE_FN("is_shift",             fn_is_shift);

  DEFINE_FN("sc",                   fn_cursor_save);
  DEFINE_FN("rc",                   fn_cursor_restore);
  DEFINE_FN("hpa",                  fn_hpa);
  DEFINE_FN("hpr",                  fn_hpr);
  DEFINE_FN("vpa",                  fn_vpa);
  DEFINE_FN("vpr",                  fn_vpr);
  DEFINE_FN("cup",                  fn_cup);
  DEFINE_FN("cuu",                  fn_cuu);
  DEFINE_FN("cud",                  fn_cud);
  DEFINE_FN("cuf",                  fn_cuf);
  DEFINE_FN("cub",                  fn_cub);
  DEFINE_FN("cuu1",                 fn_cuu1);
  DEFINE_FN("cud1",                 fn_cud1);
  DEFINE_FN("cuf1",                 fn_cuf1);
  DEFINE_FN("cub1",                 fn_cub1);
  DEFINE_FN("home",                 fn_home);
  DEFINE_FN("civis",                fn_cursor_hide);
  DEFINE_FN("cnorm",                fn_cursor_show);
  DEFINE_FN("smcup",                fn_enter_alt_screen);
  DEFINE_FN("rmcup",                fn_exit_alt_screen);
  DEFINE_FN("smkx",                 fn_enable_keyboard_action_mode);
  DEFINE_FN("rmkx",                 fn_disable_keyboard_action_mode);
  DEFINE_FN("kmous",                fn_enable_mouse);
  DEFINE_FN("ech",                  fn_ech);
  DEFINE_FN("dch",                  fn_dch);
  DEFINE_FN("ich",                  fn_ich);
  DEFINE_FN("ed",                   fn_ed);
  DEFINE_FN("el",                   fn_el);

  DEFINE_FN("cursor_move",          fn_cup);
  DEFINE_FN("cursor_home",          fn_cursor_home);
  DEFINE_FN("cursor_up",            fn_cuu1);
  DEFINE_FN("cursor_down",          fn_cud1);
  DEFINE_FN("cursor_forward",       fn_cuf1);
  DEFINE_FN("cursor_back",          fn_cub1);
  DEFINE_FN("cursor_right",         fn_cuf1);
  DEFINE_FN("cursor_left",          fn_cub1);
  DEFINE_FN("cursor_save",          fn_cursor_save);
  DEFINE_FN("cursor_restore",       fn_cursor_restore);
  DEFINE_FN("cursor_hide",          fn_cursor_hide);
  DEFINE_FN("cursor_show",          fn_cursor_show);

  DEFINE_FN("clear_screen",         fn_erase_screen);
  DEFINE_FN("erase_screen",         fn_erase_screen);
  DEFINE_FN("erase_display",        fn_erase_screen);
  DEFINE_FN("erase_line",           fn_erase_line);
  DEFINE_FN("clear_line",           fn_erase_line);
  DEFINE_FN("erase_lines",          fn_erase_lines);
  DEFINE_FN("clear_lines",          fn_erase_lines);
  DEFINE_FN("erase_chars",          fn_ech);
  DEFINE_FN("delete_chars",         fn_dch);
  DEFINE_FN("insert_chars",         fn_ich);

  DEFINE_FN("has_alt_screen",       fn_has_alt_screen);
  DEFINE_FN("enter_alt_screen",     fn_enter_alt_screen);
  DEFINE_FN("exit_alt_screen",      fn_exit_alt_screen);

  DEFINE_FN("is_terminal_raw",      fn_is_terminal_raw);
  DEFINE_FN("is_terminal_cbreak",   fn_is_terminal_cbreak);
  DEFINE_FN("is_terminal_normal",   fn_is_terminal_normal);
  DEFINE_FN("enter_raw_mode",       fn_enter_raw_mode);
  DEFINE_FN("enter_cbreak_mode",    fn_enter_cbreak_mode);
  DEFINE_FN("exit_raw_mode",        fn_exit_raw_mode);
  DEFINE_FN("exit_cbreak_mode",     fn_exit_cbreak_mode);
  DEFINE_FN("restore",              fn_restore);
  DEFINE_FN("isatty",               fn_isatty);

  DEFINE_FN("enable_mouse",         fn_enable_mouse);
  DEFINE_FN("disable_mouse",        fn_disable_mouse);
  DEFINE_FN("parse_mouse",          fn_parse_mouse);
  DEFINE_FN("get_mouse_x",          fn_get_mouse_x);
  DEFINE_FN("get_mouse_y",          fn_get_mouse_y);
  DEFINE_FN("get_mouse_button",     fn_get_mouse_button);
  DEFINE_FN("get_mouse_action",     fn_get_mouse_action);
  DEFINE_FN("get_mouse_modifiers",  fn_get_mouse_modifiers);

  DEFINE_FN("has_mouse_support",    fn_has_mouse_support);
  DEFINE_FN("has_bracketed_paste",  fn_has_bracketed_paste);
  DEFINE_FN("has_focus_events",     fn_has_focus_events);
  DEFINE_FN("has_mouse_events",     fn_has_mouse_events);
  DEFINE_FN("has_x10_mouse",        fn_has_x10_mouse);
  DEFINE_FN("has_vt200_mouse",      fn_has_vt200_mouse);
  DEFINE_FN("has_utf8_mouse",       fn_has_utf8_mouse);
  DEFINE_FN("has_sgr_mouse",        fn_has_sgr_mouse);
  DEFINE_FN("has_alt_scroll",       fn_has_alt_scroll);
  DEFINE_FN("has_urxvt_mouse",      fn_has_urxvt_mouse);
  DEFINE_FN("has_pixel_mouse",      fn_has_pixel_mouse);

  DEFINE_FN("request_mode_status",  fn_request_mode_status);
  DEFINE_FN("is_mode_supported",    fn_is_mode_supported);
  DEFINE_FN("is_mode_settable",     fn_is_mode_settable);
  DEFINE_FN("is_mode_enabled",      fn_is_mode_enabled);
  DEFINE_FN("is_mode_disabled",     fn_is_mode_disabled);
  DEFINE_FN("is_mode_permanent",    fn_is_mode_permanent);

  return exports;
}
// terminfo.c
