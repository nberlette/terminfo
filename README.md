<div align="center">

# [@nick/terminfo]

**Native [terminfo] and [ncurses] bindings for [Deno], [Node.js], and [Bun].**

![early development](./.github/assets/early_development.svg)

</div>

---

## Overview

This project provides a Node.js native addon for the [terminfo database] and a
subset of [ncurses], exposing terminal capabilities and the ability to query or
control terminal behavior in Deno, Bun, and Node.js applications alike.

## Install

<img align="right" src="https://api.iconify.design/simple-icons/deno.svg?color=%23888" height="44" />

```bash
deno add jsr:@nick/terminfo
```

<img align="right" src="https://api.iconify.design/simple-icons/pnpm.svg?color=%23fa0&height=44" height="44" />

```bash
pnpm add jsr:@nick/terminfo
```

<img align="right" src="https://api.iconify.design/simple-icons/yarn.svg?color=%232C9FBF&height=44"/>

```bash
yarn add jsr:@nick/terminfo
```

<img align="right" src="https://api.iconify.design/logos/bun.svg?inline=1&height=42" />

```bash
bunx jsr add @nick/terminfo
```

<img align="right" src="https://api.iconify.design/simple-icons/npm.svg?color=%23CB3837&height=42&width=48&scale=10" />

```bash
npx jsr add @nick/terminfo
```

---

## Usage

#### 1. Import [@nick/terminfo] and initialize terminfo

```ts
import ti from "@nick/terminfo";

// You MUST call .init([termname]) before any other API, else seg faults occur!
ti.init(); // uses $TERM by default

// or specify a termname explicitly
ti.init("xterm-256color"); // or "xterm", "vt100", "linux", "tmux", "rxvt", ...
```

#### 2. Use terminfo APIs!

```ts
import ti from "@nick/terminfo";
import { env, stdin, stdout } from "node:process";
import assert from "node:assert";

ti.init("xterm-256color");

// verifying the terminfo validity against process info
assert.strictEqual(ti.termname(), "xterm-256color");
assert.strictEqual(ti.columns(), stdout.columns);
assert.strictEqual(ti.lines(), stdout.rows);
assert.strictEqual(ti.has_colors(), stdout.getColorDepth() >= 8);
assert.strictEqual(ti.is_terminal_raw(), stdin.isRaw);
```

##### Example: Clear screen and write text at specific position

```ts
import ti from "@nick/terminfo";
import { stdout } from "node:process";

// ensure terminfo is initialized (using $TERM by default)
ti.init();

const clear = ti.cap("clear");
stdout.write(ti.enter_alt_screen() + clear);
stdout.write(ti.cursor_save() + ti.cursor_hide());
stdout.write(ti.tparm("cup", 5, 10) + "Hello, Terminfo!");
```

##### Example: request terminal info

```ts
import ti from "@nick/terminfo";
import assert from "node:assert";

// ensure terminfo is initialized
ti.init("xterm-256color");

console.log(ti.info());

assert.
```

##### Example: query capability values

```ts
import ti from "@nick/terminfo";
import assert from "node:assert";

ti.init("xterm-256color");

assert.strictEqual(ti.has_colors(), true);
assert.strictEqual(ti.num_colors(), 256);
assert.strictEqual(ti.has_mouse_events(), true);

// .tigetstr("<capname>") retrieves capability strings by capname
assert.strictEqual(ti.tigetstr("kmous"), "\x1b[<");

// .cap("<capname>") is a shorthand for .tigetstr("<capname>")
assert.strictEqual(ti.cap("cub1"), "\x1b[D");

assert.strictEqual(ti.cap("cuf"), "\x1b[%dC");

assert.strictEqual(ti.cap("el"), "\x1b[K");
```

##### Example: use parameterized capabilities

```ts
import ti from "@nick/terminfo";
import { stdout } from "node:process";

ti.init();

const clear = ti.cap("clear");
stdout.write(ti.enter_alt_screen() + clear);
stdout.write(ti.cursor_save() + ti.cursor_hide());

// .tparm("<capname>", <params...>) is used to parameterize capabilities with
// a variadic number of parameters. It supports an arity of up to 9 arguments.
stdout.write(ti.tparm("cup", 5, 10) + "Hello, Terminfo!");
```

> [!NOTE]
>
> The parameters are 0-indexed. If a capability is noted as 1-indexed in the
> [terminfo database] for a given termname, it is adjusted automatically by the
> native `tparm()` implementation.
>
> ```ts
> import ti from "@nick/terminfo";
> import assert from "node:assert";
>
> ti.init("xterm-256color");
>
> // 0-indexed input, 0-indexed output
> assert.strictEqual(ti.tparm("dl", 3), "\x1b[3M");
>
> // 0-indexed input, 1-indexed output
> assert.strictEqual(ti.tparm("hpa", 15), "\x1b[16G");
> ```

---

## API

> [!TIP]
>
> **Full API documentation is available on [JSR](https://jsr.io/@nick/terminfo/doc).**

<!-- AUTO-GENERATED:START -->

    TODO: generate API docs here

<!-- AUTO-GENERATED:END -->

---

<div align="center">

**[MIT] © [Nicholas Berlette].** All rights reserved.

<small>

[github] · [issues] · [docs] · [jsr] · [@nick]

</small>
</div>

[MIT]: https://nick.mit-license.org/2025 "MIT © Nicholas Berlette. All rights reserved."
[Nicholas Berlette]: https://github.com/nberlette "Follow @nberlette on GitHub for more cool projects!"
[@nick]: https://jsr.io/@nick "View all packages by @nick on JSR.io"
[@nick/terminfo]: https://jsr.io/@nick/terminfo/doc "View @nick/terminfo on JSR.io"
[GitHub]: https://github.com/nberlette/terminfo#readme "Give this project a star GitHub! ⭐"
[issues]: https://github.com/nberlette/terminfo/issues "View issues for this project"
[docs]: https://jsr.io/@nick/terminfo/doc "View the documentation for @nick/terminfo on JSR.io"
[JSR]: https://jsr.io/@nick/terminfo "View @nick/terminfo on JSR.io"
[Deno]: https://deno.land "Deno: A modern runtime for JavaScript and TypeScript"
[Node.js]: https://nodejs.org "Node.js JavaScript runtime"
[Bun]: https://bun.sh "Bun: Fast all-in-one JavaScript runtime"
[terminfo database]: https://invisible-island.net/ncurses/terminfo.html "The terminfo database documentation"
[terminfo]: https://en.wikipedia.org/wiki/Terminfo "Terminfo Wikipedia article"
[ncurses]: https://invisible-island.net/ncurses/ "The ncurses library documentation"
