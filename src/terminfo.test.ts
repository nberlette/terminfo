import { describe, it, beforeEach } from "node:test";
import process from "node:process";

import ti from "./terminfo.ts";

let TERM = process.env.TERM || "xterm-256color";

beforeEach(() => ti.init(TERM));

describe(`terminfo [${TERM}]`, (t) => {
  it("terminfo.init()", (t) => {
    const assert: typeof t.assert = t.assert;
    const rc = ti.init(TERM = process.env.TERM || "xterm-256color");
    assert.ok(rc, "Expected successful init return code");
  });

  it("terminfo.info()", (t) => {
    const assert: typeof t.assert = t.assert;
    const i = ti.info();
    assert.ok(i.columns > 0 && i.lines > 0, "Expected positive dimensions");
    assert.strictEqual(
      i.termname,
      process.env.TERM,
      "Expected terminfo name to match TERM environment variable",
    );
  });

  it("terminfo.tigetnum()", (t) => {
    const assert: typeof t.assert = t.assert;
    assert.strictEqual(
      typeof ti.tigetnum,
      "function",
      "Expected tigetnum to be a function",
    );
    assert.strictEqual(
      ti.tigetnum.length,
      1,
      "Expected tigetnum to take one argument",
    );
    assert.strictEqual(
      ti.tigetnum.name,
      "tigetnum",
      "Expected tigetnum function name to be 'tigetnum'",
    );
    assert.strictEqual(
      ti.tigetnum.toString().includes("[native code]"),
      true,
      "Expected tigetnum to be a native function",
    );

    t.test("cols", (t) => {
      const assert: typeof t.assert = t.assert;
      const cols = ti.tigetnum("cols");
      assert.strictEqual(
        cols,
        ti.info().columns,
        "Expected columns to match info().columns",
      );
      assert.strictEqual(
        cols,
        process.stdout.columns,
        "Expected columns to match process.stdout.columns",
      );
    });

    t.test("lines", (t) => {
      const assert: typeof t.assert = t.assert;
      const lines = ti.tigetnum("lines");
      assert.strictEqual(
        lines,
        ti.info().lines,
        "Expected lines to match info().lines",
      );
      assert.strictEqual(
        lines,
        process.stdout.rows,
        "Expected lines to match process.stdout.rows",
      );
    });

    t.test("colors", (t) => {
      const assert: typeof t.assert = t.assert;
      const colors = ti.tigetnum("colors");
      assert.ok(colors >= 8, "Expected at least 8 colors supported");
      assert.strictEqual(
        colors,
        process.stdout.getColorDepth({ ...process.env, TERM }),
        "Expected colors to match process.stdout.getColorDepth()",
      );
    });

    t.test("pairs", (t) => {
      const assert: typeof t.assert = t.assert;
      const pairs = ti.tigetnum("pairs");
      assert.ok(pairs >= 64, "Expected at least 64 color pairs supported");
    });
  });

  const esc = {
    bold: "\x1b[1m",
    dim: "\x1b[2m",
    sgr1: "\x1b[22m",
    sitm: "\x1b[3m",
    ritm: "\x1b[23m",
    smul: "\x1b[4m",
    rmul: "\x1b[24m",
    sblink: "\x1b[5m",
    rblink: "\x1b[25m",
    smrev: "\x1b[7m",
    rmrev: "\x1b[27m",
    invis: "\x1b[8m",
    rminvis: "\x1b[28m",
    smso: "\x1b[9m",
    rmso: "\x1b[29m",
    sgr0: "\x1b[0m",
    cuu1: "\x1b[A",
    cud1: "\x1b[B",
    cuf1: "\x1b[C",
    cub1: "\x1b[D",
    hpa: "\x1b[%i%p1%dG",
    cup: "\x1b[%i%p1%d;%p2%dH",
    vpa: "\x1b[%i%p1%dd",
    ed: "\x1b[J",
    el: "\x1b[K",
    clear: "\x1b[H\x1b[2J",
    smkx: "\x1b[?1h\x1b=",
    rmkx: "\x1b[?1l\x1b>",
  } as const;

  it("terminfo.tigetstr()", (t) => {
    const assert: typeof t.assert = t.assert;
    assert.strictEqual(
      ti.init("xterm-256color"),
      0,
      "Expected successful init for xterm-256color",
    );

    assert.strictEqual(
      typeof ti.tigetstr,
      "function",
      "Expected tigetstr to be a function",
    );

    t.test("bold", (t) => {
      const assert: typeof t.assert = t.assert;
      const bold = ti.tigetstr("bold");
      assert.strictEqual(bold, esc.bold, "Expected bold capability to match");
    });
    t.test("dim", (t) => {
      const assert: typeof t.assert = t.assert;
      const dim = ti.tigetstr("dim");
      assert.strictEqual(dim, esc.dim, "Expected dim capability to match");
    });
    t.test("sgr1", (t) => {
      const assert: typeof t.assert = t.assert;
      const sgr1 = ti.tigetstr("sgr1");
      assert.strictEqual(sgr1, esc.sgr1, "Expected sgr1 capability to match");
    });
    t.test("sitm", (t) => {
      const assert: typeof t.assert = t.assert;
      const sitm = ti.tigetstr("sitm");
      assert.strictEqual(sitm, esc.sitm, "Expected sitm capability to match");
    });
    t.test("ritm", (t) => {
      const assert: typeof t.assert = t.assert;
      const ritm = ti.tigetstr("ritm");
      assert.strictEqual(ritm, esc.ritm, "Expected ritm capability to match");
    });
    t.test("smul", (t) => {
      const assert: typeof t.assert = t.assert;
      const smul = ti.tigetstr("smul");
      assert.strictEqual(smul, esc.smul, "Expected smul capability to match");
    });
    t.test("rmul", (t) => {
      const assert: typeof t.assert = t.assert;
      const rmul = ti.tigetstr("rmul");
      assert.strictEqual(rmul, esc.rmul, "Expected rmul capability to match");
    });
    t.test("sblink", (t) => {
      const assert: typeof t.assert = t.assert;
      const sblink = ti.tigetstr("sblink");
      assert.strictEqual(
        sblink,
        esc.sblink,
        "Expected sblink capability to match",
      );
    });
    t.test("rblink", (t) => {
      const assert: typeof t.assert = t.assert;
      const rblink = ti.tigetstr("rblink");
      assert.strictEqual(
        rblink,
        esc.rblink,
        "Expected rblink capability to match",
      );
    });
    t.test("smrev", (t) => {
      const assert: typeof t.assert = t.assert;
      const smrev = ti.tigetstr("smrev");
      assert.strictEqual(
        smrev,
        esc.smrev,
        "Expected smrev capability to match",
      );
    });
    t.test("rmrev", (t) => {
      const assert: typeof t.assert = t.assert;
      const rmrev = ti.tigetstr("rmrev");
      assert.strictEqual(
        rmrev,
        esc.rmrev,
        "Expected rmrev capability to match",
      );
    });
    t.test("invis", (t) => {
      const assert: typeof t.assert = t.assert;
      const invis = ti.tigetstr("invis");
      assert.strictEqual(
        invis,
        esc.invis,
        "Expected invis capability to match",
      );
    });
    t.test("rminvis", (t) => {
      const assert: typeof t.assert = t.assert;
      const rminvis = ti.tigetstr("rminvis");
      assert.strictEqual(
        rminvis,
        esc.rminvis,
        "Expected rminvis capability to match",
      );
    });
    t.test("smso", (t) => {
      const assert: typeof t.assert = t.assert;
      const smso = ti.tigetstr("smso");
      assert.strictEqual(smso, esc.smso, "Expected smso capability to match");
    });
    t.test("rmso", (t) => {
      const assert: typeof t.assert = t.assert;
      const rmso = ti.tigetstr("rmso");
      assert.strictEqual(rmso, esc.rmso, "Expected rmso capability to match");
    });
    t.test("sgr0", (t) => {
      const assert: typeof t.assert = t.assert;
      const sgr0 = ti.tigetstr("sgr0");
      assert.strictEqual(sgr0, esc.sgr0, "Expected sgr0 capability to match");
    });
    t.test("cuu1", (t) => {
      const assert: typeof t.assert = t.assert;
      const cuu1 = ti.tigetstr("cuu1");
      assert.strictEqual(cuu1, esc.cuu1, "Expected cuu1 capability to match");
    });
    t.test("cud1", (t) => {
      const assert: typeof t.assert = t.assert;
      const cud1 = ti.tigetstr("cud1");
      assert.strictEqual(cud1, esc.cud1, "Expected cud1 capability to match");
    });
    t.test("cuf1", (t) => {
      const assert: typeof t.assert = t.assert;
      const cuf1 = ti.tigetstr("cuf1");
      assert.strictEqual(cuf1, esc.cuf1, "Expected cuf1 capability to match");
    });
    t.test("cub1", (t) => {
      const assert: typeof t.assert = t.assert;
      const cub1 = ti.tigetstr("cub1");
      assert.strictEqual(cub1, esc.cub1, "Expected cub1 capability to match");
    });
    t.test("vpa", (t) => {
      const assert: typeof t.assert = t.assert;
      const vpa = ti.tigetstr("vpa");
      assert.strictEqual(vpa, esc.vpa, "Expected vpa capability to match");
    });
    t.test("cup", (t) => {
      const assert: typeof t.assert = t.assert;
      const cup = ti.tigetstr("cup");
      assert.strictEqual(cup, esc.cup, "Expected cup capability to match");
    });
    t.test("ed", (t) => {
      const assert: typeof t.assert = t.assert;
      const ed = ti.tigetstr("ed");
      assert.strictEqual(ed, esc.ed, "Expected ed capability to match");
    });
    t.test("el", (t) => {
      const assert: typeof t.assert = t.assert;
      const el = ti.tigetstr("el");
      assert.strictEqual(el, esc.el, "Expected el capability to match");
    });
    t.test("clear", (t) => {
      const assert: typeof t.assert = t.assert;
      const clear = ti.tigetstr("clear");
      assert.strictEqual(
        clear,
        esc.clear,
        "Expected clear capability to match",
      );
    });
    t.test("smkx", (t) => {
      const assert: typeof t.assert = t.assert;
      const smkx = ti.tigetstr("smkx");
      assert.strictEqual(smkx, esc.smkx, "Expected smkx capability to match");
    });
    t.test("rmkx", (t) => {
      const assert: typeof t.assert = t.assert;
      const rmkx = ti.tigetstr("rmkx");
      assert.strictEqual(rmkx, esc.rmkx, "Expected rmkx capability to match");
    });
  });

  it("terminfo.tparm()", (t) => {
    const assert: typeof t.assert = t.assert;
    const tparm_cup_5_10 = ti.tparm("cup", 5, 10);
    assert.strictEqual(
      tparm_cup_5_10,
      "\x1b[6;11H",
      "Expected tparm cup(5,10) to match",
    );

    const tparm_hpa_20 = ti.tparm("hpa", 20);
    assert.strictEqual(
      tparm_hpa_20,
      "\x1b[21G",
      "Expected tparm hpa(20) to match",
    );

    const tparm_vpa_15 = ti.tparm("vpa", 15);
    assert.strictEqual(
      tparm_vpa_15,
      "\x1b[16d",
      "Expected tparm vpa(15) to match",
    );
  });
});
