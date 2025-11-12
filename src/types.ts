export interface Info {
  termname: string;
  longname: string;
  columns: number;
  lines: number;
  baudrate: number;
  erasechar: number;
  killchar: number;
  stdin_fd: number;
  stdout_fd: number;
  stderr_fd: number;
  num_pairs: number;
  num_colors: number;
  is_terminal_raw: boolean;
  is_terminal_cbreak: boolean;
  is_terminal_normal: boolean;
  can_change_color: boolean;
  has_colors: boolean;
  has_alt_screen: boolean;
  has_focus_events: boolean;
  has_mouse_events: boolean;
  has_x10_mouse: boolean;
  has_vt200_mouse: boolean;
  has_utf8_mouse: boolean;
  has_sgr_mouse: boolean;
  has_alt_scroll: boolean;
  has_urxvt_mouse: boolean;
  has_pixel_mouse: boolean;
}

export interface MouseParsed {
  x: number;
  y: number;
  button: number;
  action: number; // 0=up, 1=down
  modifiers: number; // bitfield
}

export type BoxCharStyle = 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7; // 0=light, 1=heavy

export const enum ModeStatus {
  Unrecognized = 0,
  Enabled      = 1,
  Disabled     = 2,
  PermEnabled  = 3,
  PermDisabled = 4,
}
export type CapName =
  | "clear_screen"
  | "smcup"
  | "rmcup"
  | "civis"
  | "cnorm"
  | "sc"
  | "rc"
  | "clear"
  | "enable_mouse"
  | "disable_mouse";

export interface Terminfo {
  init(term?: string): boolean;
  info(): Info;
  tigetnum(cap: string): number;
  tigetflag(cap: string): number;
  tigetstr(cap: string): string;
  tparm(cap: string, ...params: number[]): string;
  sgr(
    bold?: number,
    underline?: number,
    blink?: number,
    reverse?: number,
    fg?: number,
    bg?: number,
  ): string;
  box_char(style: BoxCharStyle, part: number): string;
  color_rgb(i: number): { ok: boolean; r: number; g: number; b: number };
  cap(name: CapName): string;
  has_colors(): boolean;
  num_colors(): number;
  num_pairs(): number;
  can_change_color(): boolean;
  keys(): { name: string; code: number }[];
  keycode(name: string): number;
  unctrl(input: string): string;
  resolve_key_code(seq: string): number;
  get_key_count(): number;
  get_key_modifiers(code: number): number;
  is_ctrl(code: number): boolean;
  is_alt(code: number): boolean;
  is_shift(code: number): boolean;
  sc(): string;
  rc(): string;
  hpa(col: number): string;
  hpr(col: number): string;
  vpa(row: number): string;
  vpr(row: number): string;
  cup(row: number, col: number): string;
  cuu(n: number): string;
  cud(n: number): string;
  cuf(n: number): string;
  cub(n: number): string;
  cuu1(): string;
  cud1(): string;
  cuf1(): string;
  cub1(): string;
  home(): string;
  civis(): string;
  cnorm(): string;
  smcup(): string;
  rmcup(): string;
  smkx(): string;
  rmkx(): string;
  kmous(): string;
  ech(n?: number): string;
  dch(n?: number): string;
  ich(n?: number): string;
  ed(n?: number): string;
  el(n?: number): string;
  cursor_move(row: number, col: number): string;
  cursor_home(): string;
  cursor_up(n?: number): string;
  cursor_down(n?: number): string;
  cursor_forward(n?: number): string;
  cursor_back(n?: number): string;
  cursor_right(n?: number): string;
  cursor_left(n?: number): string;
  cursor_save(): string;
  cursor_restore(): string;
  cursor_hide(): string;
  cursor_show(): string;
  clear_screen(): string;
  erase_screen(): string;
  erase_display(): string;
  erase_line(): string;
  clear_line(): string;
  erase_lines(n: number): string;
  clear_lines(n: number): string;
  erase_chars(n: number): string;
  delete_chars(n: number): string;
  insert_chars(n: number): string;
  // set_cursor_color(color: string): string;
  has_alt_screen(): boolean;
  enter_alt_screen(): string;
  exit_alt_screen(): string;
  is_terminal_raw(): boolean;
  is_terminal_cbreak(): boolean;
  is_terminal_normal(): boolean;
  enter_raw_mode(): boolean;
  enter_cbreak_mode(): boolean;
  exit_raw_mode(): boolean;
  exit_cbreak_mode(): boolean;
  restore(): boolean;
  isatty(fd: number): boolean;
  enable_mouse(): string;
  disable_mouse(): string;
  parse_mouse(seq: string, protocol?: number): MouseParsed | undefined;
  get_mouse_x(): number;
  get_mouse_y(): number;
  get_mouse_button(): number;
  get_mouse_action(): number;
  get_mouse_modifiers(): number;
  has_mouse_support(): boolean;
  has_mouse_events(): boolean;
  has_x10_mouse(): boolean;
  has_vt200_mouse(): boolean;
  has_utf8_mouse(): boolean;
  has_sgr_mouse(): boolean;
  has_urxvt_mouse(): boolean;
  has_pixel_mouse(): boolean;
  request_mode_status(mode: number, dec?: boolean): ModeStatus;
  is_mode_supported(mode: number, dec?: boolean): boolean;
  is_mode_settable(mode: number, dec?: boolean): boolean;
  is_mode_enabled(mode: number, dec?: boolean): boolean;
  is_mode_disabled(mode: number, dec?: boolean): boolean;
  is_mode_permanent(mode: number, dec?: boolean): boolean;
  get_stdin_fd(): number;
  get_stdout_fd(): number;
  get_stderr_fd(): number;
}
