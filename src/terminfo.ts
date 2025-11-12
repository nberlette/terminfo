/* eslint-disable @typescript-eslint/no-explicit-any */
// deno-lint-ignore-file no-explicit-any
import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
import process from "node:process";
import _native from "./native/terminfo.node";

import type { MouseParsed, Info, Terminfo } from "./types.ts";

const require = createRequire(import.meta.url);

function loadNative(): Terminfo {
  const __filename = fileURLToPath(import.meta.url);
  const __dirname = dirname(__filename);
  const candidates = [
    join(__dirname, "native", "terminfo.node"),
    join(__dirname, "native", "build", "Release", "terminfo.node"),
    join(__dirname, "native", "build", "Debug", "terminfo.node"),
  ];
  let lastError: unknown = null;
  for (const p of candidates) {
    try {
      return require(p);
    } catch (e) {
      lastError = e;
    }
  }
  throw lastError ?? new Error("Failed to load terminfo addon");
}

const native: Terminfo = _native ?? loadNative();

function attempt<T, A extends readonly any[] = any[]>(
  closure: (...args: A) => T,
  onerror: (...args: A | []) => T,
  ...args: A
): T;
function attempt<T, A extends readonly any[] = any[]>(
  closure: (...args: A) => T,
  fallback: T,
  ...args: A
): T;
function attempt<T, A extends readonly any[] = any[]>(
  closure: (...args: A) => T,
  fallback: ((...args: A | []) => T) | T,
  ...args: A
): T {
  try {
    return closure(...args);
  } catch {
    if (typeof fallback === "function") {
      return (fallback as (...a: A) => T)(...args);
    } else {
      return fallback;
    }
  }
}


export const terminfo: Terminfo = {
  ...native,
  init(term?: string): boolean {
    try {
      return +native.init(term ??= process.env.TERM || "xterm") === 0;
    } catch (cause) {
      throw new TypeError(`Failed to initialize terminfo for terminal "${term}"`, { cause });
    }
  },
  info(): Info {
    return native.info();
  },
  tigetnum(cap: string): number {
    return attempt(native.tigetnum, -1, cap);
  },
  tigetflag(cap: string): number {
    return attempt(native.tigetflag, 0, cap);
  },
  tigetstr(cap: string): string {
    return attempt(native.tigetstr, "", cap);
  },
  tparm(cap: string, ...args: number[]): string {
    return native.tparm(cap, ...args.slice(0, 9));
  },
  sgr(
    bold = 0,
    underline = 0,
    blink = 0,
    reverse = 0,
    fg = -1,
    bg = -1,
  ): string {
    return native.sgr(bold, underline, blink, reverse, fg, bg);
  },
  box_char(style: 0 | 1, part: number): string {
    return native.box_char(style, part);
  },
  color_rgb(i: number): { ok: boolean; r: number; g: number; b: number } {
    return native.color_rgb(i);
  },
  cap(
    name:
      | "smcup"
      | "rmcup"
      | "civis"
      | "cnorm"
      | "sc"
      | "rc"
      | "clear"
      | "enable_mouse"
      | "disable_mouse",
  ): string {
    return attempt(native.cap, "", name);
  },
  cup(row: number, col: number): string {
    return native.cup(row, col);
  },
  keys(): { name: string; code: number }[] {
    return native.keys();
  },
  keycode(name: string): number {
    return attempt(native.keycode, -1, name);
  },
  unctrl(input: string): string {
    return native.unctrl(input);
  },
  get_key_modifiers(code: number): number {
    return attempt(native.get_key_modifiers, 0, code);
  },
  is_ctrl(code: number): boolean {
    return attempt(native.is_ctrl, false, code);
  },
  is_alt(code: number): boolean {
    return attempt(native.is_alt, false, code);
  },
  is_shift(code: number): boolean {
    return attempt(native.is_shift, false, code);
  },
  parse_mouse(seq: string): MouseParsed | undefined {
    const out = native.parse_mouse(seq);
    return out?.x != null ? out : undefined;
  },
  get_mouse_x(): number {
    return native.get_mouse_x();
  },
  get_mouse_y(): number {
    return native.get_mouse_y();
  },
  get_mouse_button(): number {
    return native.get_mouse_button();
  },
  get_mouse_action(): number {
    return native.get_mouse_action();
  },
  get_mouse_modifiers(): number {
    return native.get_mouse_modifiers();
  },
  enter_raw_mode(): boolean {
    return +native.enter_raw_mode() === 0;
  },
  exit_raw_mode(): boolean {
    return +native.exit_raw_mode() === 0;
  },
  is_terminal_raw(): boolean {
    return +native.is_terminal_raw() > 0;
  },
  enter_cbreak_mode(): boolean {
    return +native.enter_cbreak_mode() === 0;
  },
  exit_cbreak_mode(): boolean {
    return +native.exit_cbreak_mode() === 0;
  },
  is_terminal_cbreak(): boolean {
    return +native.is_terminal_cbreak() > 0;
  },
  is_terminal_normal(): boolean {
    return +native.is_terminal_normal() > 0;
  },
  restore(): boolean {
    return +native.restore() === 0;
  },
};

export type * from "./types.ts";

export default terminfo;
