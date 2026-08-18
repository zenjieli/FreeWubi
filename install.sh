#!/usr/bin/env bash
# Installs FreeWubi: checks prerequisites, downloads the latest release,
# extracts it to ~/.local, and points Fcitx5 at the plugin directory.
#
# Safe to re-run. Prompts (via /dev/tty, so this also works when piped
# through `curl | bash`) before anything that needs sudo, changes a system
# default, or touches an existing file.
set -euo pipefail

REPO="zenjieli/FreeWubi"
INSTALL_PREFIX="$HOME/.local"
ADDON_DIR="$INSTALL_PREFIX/lib/fcitx5"
XINPUTRC="$HOME/.xinputrc"
ADDON_DIRS_LINE='export FCITX_ADDON_DIRS=/usr/lib/x86_64-linux-gnu/fcitx5:$HOME/.local/lib/fcitx5'

step() { printf '\n==> %s\n' "$1"; }
info() { printf '    %s\n' "$1"; }
ok()   { printf '    [ok] %s\n' "$1"; }
warn() { printf '    [warn] %s\n' "$1" >&2; }
die()  { printf '    [error] %s\n' "$1" >&2; exit 1; }

# Prompts on /dev/tty directly so this still works under `curl ... | bash`,
# where stdin is the script itself, not a terminal. Declines (rather than
# hangs) if no terminal is reachable at all.
confirm() {
  local prompt="$1" reply
  if ! read -r -p "    $prompt [y/N] " reply 2>/dev/null < /dev/tty; then
    warn "No terminal available; skipping: $prompt"
    return 1
  fi
  case "$reply" in
    [yY] | [yY][eE][sS]) return 0 ;;
    *) return 1 ;;
  esac
}

WORKDIR=""
cleanup() { [ -n "$WORKDIR" ] && rm -rf "$WORKDIR"; }
trap cleanup EXIT

fetch_stdout() {
  if [ "$DOWNLOADER" = curl ]; then
    curl -fsSL "$1"
  else
    wget -qO- "$1"
  fi
}

fetch_file() {
  if [ "$DOWNLOADER" = curl ]; then
    curl -fsSL -o "$2" "$1"
  else
    wget -qO "$2" "$1"
  fi
}

step "Checking system requirements"

os="$(uname -s)"
[ "$os" = "Linux" ] || die "FreeWubi requires Linux (detected: $os)."
ok "OS: Linux"

arch="$(uname -m)"
[ "$arch" = "x86_64" ] || die "Pre-built binaries are linux-x86_64 only (detected: $arch). Build from source instead: docs/installation.md#build-from-source."
ok "Architecture: x86_64"

if command -v ldd >/dev/null 2>&1; then
  glibc_ver="$(ldd --version 2>/dev/null | head -1 | grep -oE '[0-9]+\.[0-9]+$' || true)"
  if [ -n "$glibc_ver" ]; then
    major="${glibc_ver%%.*}"
    minor="${glibc_ver#*.}"
    if [ "$major" -lt 2 ] || { [ "$major" -eq 2 ] && [ "$minor" -lt 35 ]; }; then
      warn "glibc $glibc_ver detected; the release binary needs glibc 2.35+. If it fails to load, build from source instead."
    else
      ok "glibc $glibc_ver"
    fi
  fi
fi

DOWNLOADER=""
if command -v curl >/dev/null 2>&1; then
  DOWNLOADER="curl"
elif command -v wget >/dev/null 2>&1; then
  DOWNLOADER="wget"
else
  die "Need curl or wget to download the release."
fi
ok "Downloader: $DOWNLOADER"

command -v tar >/dev/null 2>&1 || die "Need tar to extract the release archive."
ok "tar available"

step "Checking for Fcitx5"
if command -v fcitx5 >/dev/null 2>&1; then
  ok "Fcitx5 already installed"
else
  info "Fcitx5 not found."
  if command -v apt >/dev/null 2>&1; then
    if confirm "Install fcitx5 and related packages via apt (requires sudo)?"; then
      sudo apt update
      sudo apt install -y fcitx5 fcitx5-chinese-addons \
        fcitx5-frontend-gtk3 fcitx5-frontend-gtk4 \
        fcitx5-frontend-qt5 fcitx5-config-qt im-config
      ok "Fcitx5 installed"
      if confirm "Set fcitx5 as the default input method framework (im-config -n fcitx5)?"; then
        im-config -n fcitx5
        ok "Default input method framework set to fcitx5"
      else
        warn "Skipped im-config; set fcitx5 as the default input framework yourself, or FreeWubi may not load."
      fi
    else
      die "Fcitx5 is required. Install it, then re-run this script."
    fi
  else
    pm=""
    for candidate in dnf pacman zypper; do
      command -v "$candidate" >/dev/null 2>&1 && pm="$candidate" && break
    done
    if [ -n "$pm" ]; then
      die "No apt found (detected '$pm'). Install fcitx5 with your package manager, then re-run this script."
    else
      die "No known package manager found. Install fcitx5 manually (see docs/installation.md), then re-run this script."
    fi
  fi
fi

step "Downloading latest FreeWubi release"
WORKDIR="$(mktemp -d)"

api_url="https://api.github.com/repos/$REPO/releases/latest"
release_json="$(fetch_stdout "$api_url" || true)"
[ -n "$release_json" ] || die "Could not reach GitHub to find the latest release."

tag="$(printf '%s' "$release_json" | grep -m1 '"tag_name"' | sed -E 's/.*"tag_name": *"([^"]+)".*/\1/' || true)"
asset_url="$(printf '%s' "$release_json" |
  grep -o '"browser_download_url": *"[^"]*linux-x86_64\.tar\.gz"' |
  head -1 |
  sed -E 's/.*"(https:[^"]+)"$/\1/' || true)"
[ -n "$tag" ] || die "Could not determine the latest release tag from GitHub."
[ -n "$asset_url" ] || die "No linux-x86_64 release asset found for $tag."
info "Latest release: $tag"

archive="$WORKDIR/freewubi.tar.gz"
fetch_file "$asset_url" "$archive"
ok "Downloaded $tag"

step "Installing to $INSTALL_PREFIX"
if [ -f "$ADDON_DIR/freewubi.so" ]; then
  info "An existing FreeWubi install was found at $ADDON_DIR/freewubi.so"
  confirm "Overwrite it with $tag?" || die "Left the existing install untouched."
fi
mkdir -p "$INSTALL_PREFIX"
tar xzf "$archive" -C "$INSTALL_PREFIX"
ok "Extracted to $INSTALL_PREFIX"

step "Configuring Fcitx5 addon path"
if [ -f "$XINPUTRC" ] && grep -q 'FCITX_ADDON_DIRS' "$XINPUTRC" && grep -q "$ADDON_DIR" "$XINPUTRC"; then
  ok "$XINPUTRC already points at $ADDON_DIR"
elif [ -f "$XINPUTRC" ]; then
  if grep -q '^run_im ' "$XINPUTRC"; then
    backup="$XINPUTRC.bak.$(date +%Y%m%d%H%M%S)"
    cp "$XINPUTRC" "$backup"
    info "Backed up existing $XINPUTRC to $backup"
    awk -v line="$ADDON_DIRS_LINE" '/^run_im /{print line} {print}' "$XINPUTRC" >"$WORKDIR/xinputrc.new"
    mv "$WORKDIR/xinputrc.new" "$XINPUTRC"
    ok "Added FCITX_ADDON_DIRS to $XINPUTRC"
  else
    warn "$XINPUTRC exists but has no 'run_im' line; leaving it untouched."
    warn "Add this line to it manually: $ADDON_DIRS_LINE"
  fi
else
  printf '%s\nrun_im fcitx5\n' "$ADDON_DIRS_LINE" >"$XINPUTRC"
  ok "Created $XINPUTRC"
fi

step "Verifying install"
all_found=1
for f in "$ADDON_DIR/freewubi.so" \
  "$INSTALL_PREFIX/share/fcitx5/addon/freewubi.conf" \
  "$INSTALL_PREFIX/share/fcitx5/inputmethod/freewubi.conf"; do
  if [ -f "$f" ]; then
    ok "Found $f"
  else
    warn "Missing $f"
    all_found=0
  fi
done
[ "$all_found" -eq 1 ] || die "Install looks incomplete; see warnings above."

step "Almost done"
info "1. Log out and back in (needed for the addon path / default input framework to take effect)."
info "2. Run 'fcitx5-configtool', click 'Add Input Method', search for 'FreeWubi', and add it."
info "3. Switch to FreeWubi (Ctrl+Space) and press 'a' -- you should see candidates like '1.工 2.戈'."
info "See docs/usage.md for the full key binding reference."
