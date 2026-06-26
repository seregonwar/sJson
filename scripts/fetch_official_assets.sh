#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ASSET_DIR="$ROOT_DIR/benchmarks/assets"
SUITE_DIR="$ROOT_DIR/tests/JSONTestSuite"

mkdir -p "$ASSET_DIR" "$SUITE_DIR"

fetch() {
  local url="$1"
  local out="$2"
  if command -v curl >/dev/null 2>&1; then
    curl -L --fail --retry 3 -o "$out" "$url"
  elif command -v wget >/dev/null 2>&1; then
    wget -O "$out" "$url"
  else
    echo "curl or wget is required" >&2
    exit 1
  fi
}

echo "Fetching nativejson-benchmark datasets..."
fetch "https://raw.githubusercontent.com/miloyip/nativejson-benchmark/master/data/canada.json" "$ASSET_DIR/canada.json"
fetch "https://raw.githubusercontent.com/miloyip/nativejson-benchmark/master/data/citm_catalog.json" "$ASSET_DIR/citm_catalog.json"
fetch "https://raw.githubusercontent.com/miloyip/nativejson-benchmark/master/data/twitter.json" "$ASSET_DIR/twitter.json"

echo "Fetching JSONTestSuite corpus..."
if command -v git >/dev/null 2>&1; then
  tmp="$(mktemp -d)"
  trap 'rm -rf "$tmp"' EXIT
  git clone --depth 1 https://github.com/nst/JSONTestSuite.git "$tmp/JSONTestSuite"
  rm -rf "$SUITE_DIR/test_parsing"
  mkdir -p "$SUITE_DIR"
  cp -R "$tmp/JSONTestSuite/test_parsing" "$SUITE_DIR/"
else
  echo "git is required to fetch JSONTestSuite" >&2
  exit 1
fi

echo "Assets ready:"
echo "  $ASSET_DIR"
echo "  $SUITE_DIR/test_parsing"
