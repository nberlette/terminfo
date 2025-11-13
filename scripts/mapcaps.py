#!/usr/bin/env python3
"""Generate a mapping of termcap codes to terminfo metadata and values."""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass
from typing import Dict, Iterable, List, Optional, Set, Tuple

__version__ = "1.1.0"

FORMAT_COMMANDS = {"d", "o", "x", "X", "c", "s"}
FORMAT_FLAG_CHARS = set(" #+-'.0123456789")
FORMAT_KIND_MAP = {
  "d": "number",
  "o": "number",
  "x": "number",
  "X": "number",
  "c": "char",
  "s": "string",
}
FORMAT_LABELS = {
  "d": "decimal",
  "o": "octal",
  "x": "hex",
  "X": "HEX",
  "c": "char",
  "s": "string",
}

@dataclass
class CapabilityEntry:
  name: str
  type: str
  value: Optional[object]

def run_infocmp(term: str, extra: List[str]) -> str:
  cmd = ["infocmp"] + extra + [term]
  try:
    res = subprocess.run(
      cmd, check=True, capture_output=True, text=True
    )
  except FileNotFoundError as err:
    raise RuntimeError("infocmp executable is required") from err
  except subprocess.CalledProcessError as err:
    detail = err.stderr.strip() or err.stdout.strip()
    raise RuntimeError(detail or f"infocmp failed: {cmd}") from err
  return res.stdout

def normalize_string_literal(value: str) -> str:
  if not value:
    return value
  return value.replace("\\E", "\\x1B")

def parse_capability_token(token: str) -> Optional[CapabilityEntry]:
  if not token or token.endswith("@"):
    return None
  if "=" in token:
    name, value = token.split("=", 1)
    return CapabilityEntry(name.strip(), "string", normalize_string_literal(value))
  if "#" in token:
    name, value = token.split("#", 1)
    try:
      num = int(value, 0)
    except ValueError:
      num = value
    return CapabilityEntry(name.strip(), "number", num)
  return CapabilityEntry(token.strip(), "boolean", True)

def parse_capability_values(term: str) -> Dict[str, CapabilityEntry]:
  text = run_infocmp(term, ["-I", "-1"])
  values: Dict[str, CapabilityEntry] = {}
  for line in text.splitlines():
    stripped = line.strip()
    if not stripped or stripped.startswith("#") or not line.startswith((" ", "\t")):
      continue
    if stripped.endswith(","):
      stripped = stripped[:-1]
    for token in (part.strip() for part in stripped.split(",") if part.strip()):
      entry = parse_capability_token(token)
      if entry:
        values[entry.name] = entry
  return values

def normalize_value(value: Optional[object], kind: str) -> Optional[object]:
  if value is None:
    return None
  if kind == "boolean":
    return bool(value)
  if kind == "number":
    return int(value)
  return str(value)

def clone_value(value: Dict[str, object]) -> Dict[str, object]:
  return {
    "params": set(value.get("params", set())),
    "derived": bool(value.get("derived")),
  }

def pop_value(stack: List[Dict[str, object]]) -> Dict[str, object]:
  if stack:
    return stack.pop()
  return {"params": set(), "derived": False}

def push_const(stack: List[Dict[str, object]]) -> None:
  stack.append({"params": set(), "derived": False})

def note_parameter_usage(
  usage: Dict[int, Dict[str, object]],
  operand: Dict[str, object],
  fmt: str,
) -> None:
  params: Set[int] = set(operand.get("params", set()))
  if not params:
    return
  derived = bool(operand.get("derived") or len(params) > 1)
  for idx in sorted(params):
    meta = usage.setdefault(idx, {
      "index": idx,
      "formats": set(),
      "count": 0,
      "derived": False,
    })
    meta["count"] += 1
    meta["formats"].add(fmt)
    if derived:
      meta["derived"] = True

def read_format_command(value: str, start: int) -> Tuple[Optional[str], int]:
  i = start
  while i < len(value) and value[i] in FORMAT_FLAG_CHARS:
    i += 1
  if i < len(value) and value[i] in FORMAT_COMMANDS:
    cmd = value[i]
    i += 1
    return cmd, i
  return None, start

def analyze_string_parameters(value: str) -> List[Dict[str, object]]:
  if not value or "%p" not in value:
    return []
  stack: List[Dict[str, object]] = []
  variables: Dict[str, Dict[str, object]] = {}
  usage: Dict[int, Dict[str, object]] = {}
  offsets: Dict[int, int] = {}
  binary_ops = set("+-*/m&|^=><AO")
  unary_ops = set("!~")
  i = 0
  length = len(value)
  while i < length:
    ch = value[i]
    if ch != "%":
      i += 1
      continue
    i += 1
    if i >= length:
      break
    cmd = value[i]
    i += 1
    if cmd == "%":
      continue
    if cmd == "p":
      if i < length and value[i].isdigit():
        idx = int(value[i])
        i += 1
        stack.append({"params": {idx}, "derived": False})
      continue
    if cmd == "P":
      if i < length:
        var = value[i]
        i += 1
        var_value = clone_value(pop_value(stack))
        variables[var] = var_value
      continue
    if cmd == "g":
      if i < length:
        var = value[i]
        i += 1
        stack.append(clone_value(variables.get(var, {"params": set(), "derived": True})))
      continue
    if cmd == "{":
      while i < length and value[i] != "}":
        i += 1
      if i < length and value[i] == "}":
        i += 1
      push_const(stack)
      continue
    if cmd == "'":
      while i < length:
        if value[i] == "\\" and i + 1 < length:
          i += 2
          continue
        if value[i] == "'":
          i += 1
          break
        i += 1
      push_const(stack)
      continue
    if cmd == "i":
      for idx in (1, 2):
        offsets[idx] = offsets.get(idx, 0) + 1
      continue
    if cmd == "l":
      val = pop_value(stack)
      stack.append({"params": set(val.get("params", set())), "derived": True})
      continue
    if cmd in binary_ops:
      right = pop_value(stack)
      left = pop_value(stack)
      stack.append({
        "params": set(left.get("params", set())) | set(right.get("params", set())),
        "derived": True,
      })
      continue
    if cmd in unary_ops:
      operand = pop_value(stack)
      stack.append({"params": set(operand.get("params", set())), "derived": True})
      continue
    if cmd in ("?", "t", "e", ";"):
      continue
    if cmd == "r":
      if len(stack) >= 2:
        stack[-1], stack[-2] = stack[-2], stack[-1]
      continue
    if cmd in FORMAT_COMMANDS:
      note_parameter_usage(usage, pop_value(stack), cmd)
      continue
    if cmd in FORMAT_FLAG_CHARS:
      fmt_cmd, i = read_format_command(value, i)
      if fmt_cmd:
        note_parameter_usage(usage, pop_value(stack), fmt_cmd)
      continue
  result: List[Dict[str, object]] = []
  for idx in sorted(usage):
    meta = usage[idx]
    formats = sorted(FORMAT_LABELS.get(ch, ch) for ch in meta["formats"])
    kinds = {FORMAT_KIND_MAP.get(ch, "unknown") for ch in meta["formats"]}
    kind = "mixed" if len(kinds) > 1 else kinds.pop()
    entry: Dict[str, object] = {
      "index": idx,
      "kind": kind,
      "formats": formats,
      "count": meta["count"],
    }
    if meta.get("derived"):
      entry["derived"] = True
    offset = offsets.get(idx)
    if offset:
      entry["offset"] = offset
    result.append(entry)
  return result

def describe_string_parameters(value: Optional[str]) -> Optional[List[Dict[str, object]]]:
  if not value:
    return None
  params = analyze_string_parameters(value)
  return params or None

def parse_capability_listing(term: str, flag: str) -> Dict[str, List[str]]:
  text = run_infocmp(term, [flag, "-1"])
  sections = {"boolean": [], "number": [], "string": []}
  for line in text.splitlines():
    stripped = line.strip()
    if not stripped or stripped == "," or stripped.startswith("#") or "|" in stripped:
      continue
    if stripped.endswith(","):
      stripped = stripped[:-1]
    if "=" in stripped:
      kind = "string"
      token = stripped.split("=", 1)[0]
    elif "#" in stripped:
      kind = "number"
      token = stripped.split("#", 1)[0]
    else:
      kind = "boolean"
      token = stripped
    token = token.strip()
    if token:
      sections[kind].append(token)
  return sections

def parse_termcap_capabilities(term: str) -> Dict[str, CapabilityEntry]:
  text = run_infocmp(term, ["-C", "-r"])
  buffer = []
  for line in text.splitlines():
    stripped = line.strip()
    if not stripped or stripped.startswith("#"):
      continue
    if "|" in stripped:
      _, _, stripped = stripped.partition(":")
    if stripped.endswith("\\"):
      stripped = stripped[:-1]
    buffer.append(stripped)
  joined = ":".join(part for part in buffer if part)
  capabilities: Dict[str, CapabilityEntry] = {}
  for token in joined.split(":"):
    token = token.strip()
    if not token:
      continue
    entry = parse_capability_token(token)
    if entry:
      capabilities[entry.name] = entry
  return capabilities

def resolve_termcap_code(
  name: str,
  entry: Optional[CapabilityEntry],
  termcap_values: Dict[str, CapabilityEntry],
) -> str:
  if name in termcap_values:
    return name
  if name.startswith("OT"):
    short = name[2:]
    if short in termcap_values:
      return short
  if entry and entry.value is not None and entry.type != "boolean":
    matches = [
      code for code, candidate in termcap_values.items()
      if candidate.type == entry.type and candidate.value == entry.value
    ]
    if len(matches) == 1:
      return matches[0]
  return name

def build_mapping(term: str) -> Dict[str, Dict[str, object]]:
  short_names = parse_capability_listing(term, "-I")
  human_names = parse_capability_listing(term, "-L")
  values = parse_capability_values(term)
  termcap_values = parse_termcap_capabilities(term)
  mapping: Dict[str, Dict[str, object]] = {}

  def hydrate(kind: str) -> None:
    names = short_names.get(kind, [])
    humans = human_names.get(kind, [])
    limit = min(len(names), len(humans))
    for idx in range(limit):
      name = names[idx]
      human = humans[idx] if idx < len(humans) else ""
      entry = values.get(name)
      code = resolve_termcap_code(name, entry, termcap_values)
      value = normalize_value(entry.value if entry and entry.type == kind else None, kind)
      payload: Dict[str, object] = {
        "termcap": code,
        "terminfo": name or None,
        "human": human or None,
        "type": kind,
        "value": value,
      }
      if kind == "string" and entry and entry.value:
        params = describe_string_parameters(entry.value)
        if params:
          payload["parameters"] = params
      mapping[code] = payload

  hydrate("boolean")
  hydrate("number")
  hydrate("string")
  return mapping

TYPE_ORDER = {"boolean": 0, "number": 1, "string": 2}

def sort_capabilities(data: Dict[str, Dict[str, object]], mode: str) -> Dict[str, Dict[str, object]]:
  items = list(data.items())
  def sort_key_code(item):
    return item[0] or ""
  def sort_key_terminfo(item):
    payload = item[1]
    return (payload.get("terminfo") or "", item[0])
  def sort_key_human(item):
    payload = item[1]
    return (payload.get("human") or "", item[0])
  def sort_key_type(item):
    payload = item[1]
    return (TYPE_ORDER.get(payload.get("type"), 99), item[0])
  selectors = {
    "code": sort_key_code,
    "terminfo": sort_key_terminfo,
    "human": sort_key_human,
    "type": sort_key_type,
  }
  key_func = selectors.get(mode, sort_key_code)
  return {key: payload for key, payload in sorted(items, key=key_func)}

def yaml_scalar(value: object) -> str:
  if value is None:
    return "null"
  if isinstance(value, bool):
    return "true" if value else "false"
  if isinstance(value, (int, float)):
    return str(value)
  if isinstance(value, str):
    if not value:
      return "''"
    if re.fullmatch(r"[A-Za-z0-9_.:/@+-]+", value):
      return value
    return json.dumps(value, ensure_ascii=False)
  return json.dumps(value, ensure_ascii=False)

def emit_yaml(obj: object, indent: int = 0) -> List[str]:
  space = "  " * indent
  if isinstance(obj, dict):
    lines: List[str] = []
    for key, val in obj.items():
      key_str = key if re.fullmatch(r"[A-Za-z0-9_]+", key) else json.dumps(key, ensure_ascii=False)
      if isinstance(val, (dict, list)):
        lines.append(f"{space}{key_str}:")
        lines.extend(emit_yaml(val, indent + 1))
      else:
        lines.append(f"{space}{key_str}: {yaml_scalar(val)}")
    return lines or [f"{space}{{}}"]
  if isinstance(obj, list):
    lines: List[str] = []
    for item in obj:
      if isinstance(item, (dict, list)):
        lines.append(f"{space}-")
        lines.extend(emit_yaml(item, indent + 1))
      else:
        lines.append(f"{space}- {yaml_scalar(item)}")
    return lines or [f"{space}[]"]
  return [f"{space}{yaml_scalar(obj)}"]

def render_yaml(data: Dict[str, object]) -> str:
  return "\n".join(emit_yaml(data))

def toml_scalar(value: object) -> str:
  if value is None:
    return "null"
  if isinstance(value, bool):
    return "true" if value else "false"
  if isinstance(value, (int, float)):
    return str(value)
  if isinstance(value, list):
    return "[" + ", ".join(toml_scalar(item) for item in value) + "]"
  return json.dumps(str(value), ensure_ascii=False)

def render_toml(data: Dict[str, object]) -> str:
  lines: List[str] = [f'term = {toml_scalar(data.get("term"))}']
  caps = data.get("capabilities")
  if isinstance(caps, dict) and caps:
    lines.append("")
    for name, payload in caps.items():
      lines.append(f"[capabilities.{name}]")
      for key, val in payload.items():
        if key == "parameters" and isinstance(val, list):
          if not val:
            continue
          for param in val:
            lines.append(f"[[capabilities.{name}.parameters]]")
            if isinstance(param, dict):
              for param_key, param_val in param.items():
                lines.append(f"{param_key} = {toml_scalar(param_val)}")
        else:
          lines.append(f"{key} = {toml_scalar(val)}")
      if lines[-1] != "":
        lines.append("")
    if lines[-1] == "":
      lines.pop()
  return "\n".join(lines)

def render_json(data: Dict[str, object]) -> str:
  return json.dumps(data, indent=2, ensure_ascii=False)

def render_jsonc(data: Dict[str, object]) -> str:
  comment = f'// terminfo capability mapping for {data.get("term")}'
  return f"{comment}\n{render_json(data)}"

def compile_patterns(patterns: Iterable[str]) -> Optional[List[re.Pattern]]:
  compiled = []
  for pattern in patterns:
    try:
      compiled.append(re.compile(pattern))
    except re.error as err:
      raise argparse.ArgumentTypeError(f"invalid regex '{pattern}': {err}") from err
  return compiled or None

def parse_args() -> argparse.Namespace:
  parser = argparse.ArgumentParser(
    description="Create a mapping of termcap codes to terminfo capability metadata.\n\n"
  )
  parser.add_argument(
    "-t", "--term",
    default=os.environ.get("TERM"),
    help="terminal / terminfo entry name"
  )
  parser.add_argument(
    "-C", "--caps",
    action="store_true",
    help="include capabilities mapping in the output"
  )
  parser.add_argument(
    "-f", "--format",
    default="json",
    choices=("json", "jsonc", "yaml", "toml"),
  )
  parser.add_argument(
    "-o", "--output",
    type=argparse.FileType("w", encoding="utf-8"),
    default=sys.stdout,
  )
  parser.add_argument(
    "-S", "--sort",
    default="code",
    choices=("code", "terminfo", "human", "type"),
    help="cap sorting strategy (default: code)"
  )
  parser.add_argument(
    "-F", "--filter",
    action="append",
    default=[],
    metavar="REGEXP",
    help="include only entries with matching code/name"
  )
  parser.add_argument(
    "-X", "--exclude",
    action="append",
    default=[],
    metavar="REGEXP",
    help="exclude any entries with matching code/name"
  )
  parser.add_argument(
    "-c", "--comments",
    action="store_true",
    help="include comments in output"
  )
  parser.add_argument(
    "-v", "--version",
    action="version",
    version=f"terminfo_capmap {__version__}"
  )
  return parser.parse_args()

def main() -> None:
  args = parse_args()
  patterns = compile_patterns(args.filter)
  exclude_patterns = compile_patterns(args.exclude)
  if not shutil.which("infocmp"):
    print("infocmp is not available on PATH", file=sys.stderr)
    sys.exit(1)
  if not args.term:
    print("No term specified and $TERM is unset", file=sys.stderr)
    sys.exit(1)
  try:
    capabilities = build_mapping(args.term)
  except RuntimeError as err:
    print(err, file=sys.stderr)
    sys.exit(1)
  if patterns or exclude_patterns:
    filtered = {}
    for code, payload in capabilities.items():
      name = payload.get("terminfo") or ""
      human = payload.get("human") or ""
      matches_include = True
      if patterns:
        matches_include = any(
          pattern.search(code) or pattern.search(name) or pattern.search(human)
          for pattern in patterns
        )
      matches_exclude = False
      if exclude_patterns:
        matches_exclude = any(
          pattern.search(code) or pattern.search(name) or pattern.search(human)
          for pattern in exclude_patterns
        )
      if matches_include and not matches_exclude:
        filtered[code] = payload
    capabilities = filtered
  capabilities = sort_capabilities(capabilities, args.sort)
  payload = {"term": args.term, "capabilities": capabilities}
  renderers = {
    "json": render_json,
    "jsonc": render_jsonc,
    "yaml": render_yaml,
    "toml": render_toml,
  }
  renderer = renderers[args.format]
  if args.format == "json" and args.comments:
    renderer = render_jsonc
  data = renderer(payload)
  output = args.output
  output.write(data)
  output.write("\n")

if __name__ == "__main__":
  main()
