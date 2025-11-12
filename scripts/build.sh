#!/usr/bin/env bash

## This build script is used to run the node-gyp build process for the
## terminfo native addon module, which provides terminal handling capabilities
## from ncurses and terminfo databases to Node.js applications.
##
## The terminfo addon source code can be found in the ./src/native directory.
##
## ---------------------------------------------------------------------------
## MIT License. Copyright (c) 2025 Nicholas Berlette. All rights reserved.
## See https://nick.mit-license.org/2025 for a full copy of the license text.
## ---------------------------------------------------------------------------
## Usage:
##      ./scripts/build.sh [--debug]
##      --debug : Build the addon in debug mode.
##
## (...or, from the project root directory, with Deno installed:)
##      deno task build
##

set -euo pipefail

## --- helpers --- ##
function print() {
  echo -en "$*" >&2
  return 0
}

function println() {
  echo -e "$*" >&2
  return 0
}

function ansiprint() {
  local color_code="${1:-"2;90"}"
  local label="${2:-info}"
  shift 2
  println "\033[${color_code}m\uE0B6\033[7m${label}\033[27m\uE0B4\033[0m ${*}"
}

function err() {
  [ "${SILENT-}" = "1" ] || \
    ansiprint "1;31" "error" "${*:-"Unknown error occurred."}"
  return 1
}

function fatal() {
  ansiprint "1;38;5;160" $'\e[107mfatal\e[49m' "${*:-"oh shit!"}"
  exit 1
}

function log() {
  local -a args=($(echo -n "${*}" | tr ' ' '\n'))
  local first="${args[0]}"
  args=("${args[@]:1}")

  [ "${SILENT-}" = "1" ] || ansiprint "1;32" "${first:-log}" "${args[@]}"
  return 0
}

function info() {
  [ "${SILENT-}" = "1" ] || ansiprint "1;34" "info" "${*:-"???"}"
  return 0
}

function warn() {
  [ "${SILENT-}" = "1" ] || ansiprint "1;33" "warning" "${*:-"?!?!"}"
  return 0
}

function debug() {
  if [[ "${DEBUG:-0}" == "1" ]]; then
    ansiprint "90" "debug" "${*:-"Debug message."}"
  fi
  return 0
}

function run() {
  local -a args=("${@}")
  debug "${args[*]}"
  "${args[@]}"
}

## --- main script --- ##
DEBUG=${DEBUG:-0}
BUILD_TYPE="--release"
VERBOSE=${VERBOSE:-0}
SILENT=${SILENT:-0}
BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE_DIR="${BASE_DIR}/src"
NATIVE_DIR="${SOURCE_DIR}/native"
BUILD_DIR="${NATIVE_DIR}/build"
RELEASE_DIR="${BUILD_DIR}/Release"
DEBUG_DIR="${BUILD_DIR}/Debug"
TERMINFO_C_PATH="${NATIVE_DIR}/terminfo.c"
ADDON_C_PATH="${NATIVE_DIR}/addon.cc"
TERMINFO_ADDON_NAME="terminfo.node"
TERMINFO_ADDON_PATH="${RELEASE_DIR}/${TERMINFO_ADDON_NAME}"
OUTPUT_DIR="${NATIVE_DIR}"

for arg in "$@"; do
  case "${arg}" in
    (-h|--help|"-?")
      println "Usage: ./scripts/build.sh [--debug]"
      println "  --debug : Build the addon in debug mode."
      exit 0;;
    (-d|--debug) DEBUG=1; BUILD_TYPE="--debug";;
    (-r|--release) BUILD_TYPE="--release";;
    (-o|--out-dir|--output|--output-dir)
      shift
      OUTPUT_DIR="${1:-"${OUTPUT_DIR}"}";;
    (-s|--silent|-q|--quiet) SILENT=1;;
    (-v|--verbose) VERBOSE=1;;
    (*)
      fatal "Unknown argument: ${arg}"
      ;;
  esac
done

if [[ "${DEBUG}" == "1" ]]; then
  TERMINFO_ADDON_PATH="${DEBUG_DIR}/${TERMINFO_ADDON_NAME}"
fi

info "building terminfo addon in ${BUILD_TYPE#--} mode...";

debug "paths collected:"$'\n'\
"     base dir:    ${BASE_DIR}"$'\n'\
"   source dir:    ${SOURCE_DIR}"$'\n'\
"   native dir:    ${NATIVE_DIR}"$'\n'\
"    build dir:    ${BUILD_DIR}"$'\n'\
"  release dir:    ${RELEASE_DIR}"$'\n'\
"    debug dir:    ${DEBUG_DIR}"$'\n'\
"   output dir:    ${OUTPUT_DIR}"$'\n'\
"   addon path:    ${TERMINFO_ADDON_PATH}"

run cd "${BASE_DIR:-.}"
# run cp binding.gyp "${NATIVE_DIR}/binding.gyp"
# run cd "${NATIVE_DIR}"

if [[ "${VERBOSE}" == "1" ]]; then
  run node-gyp rebuild ${BUILD_TYPE} --verbose
elif [[ "${SILENT}" == "1" ]]; then
  run node-gyp rebuild ${BUILD_TYPE} &>/dev/null
else
  run node-gyp rebuild ${BUILD_TYPE}
fi

run mkdir -p "${OUTPUT_DIR}"

run cp -r build "${OUTPUT_DIR}/"
run cp "${TERMINFO_ADDON_PATH}" "${OUTPUT_DIR}/"

log "finished building terminfo addon!"
