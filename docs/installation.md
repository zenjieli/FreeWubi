[**English**](installation.md) | [中文](installation.zh-CN.md)

# Installation

## Quick install (recommended)

```bash
git clone https://github.com/zenjieli/FreeWubi.git
cd FreeWubi
./install.sh
```

`install.sh` runs through this whole page for you: it checks your OS,
architecture, and glibc version; installs Fcitx5 via `apt` if it's missing
(asking for confirmation before it uses `sudo`, and before it changes your
default input framework); downloads the latest release; extracts it to
`~/.local`; and adds `FCITX_ADDON_DIRS` to `~/.xinputrc` (backing up the file
first if it already exists). It's safe to re-run — it detects and skips steps
that are already done, and asks before overwriting an existing install.

It only automates the `apt`-based path; on Fedora/Arch/openSUSE it will tell
you to install Fcitx5 yourself, then continue from there. The last step —
adding FreeWubi as an active input method in `fcitx5-configtool` — is a GUI
action and always manual; see [Verify](#verify) below.

If it doesn't fit your setup, use the manual steps below instead.

## Install from GitHub Release

Pre-built binaries are available on the [Releases](https://github.com/zenjieli/FreeWubi/releases) page.

1. Download the latest `freewubi-v*-linux-x86_64.tar.gz`
2. Extract to your home directory:

```bash
tar xzf freewubi-v0.1.0-linux-x86_64.tar.gz -C ~/.local
```

This installs to user-local dirs (no sudo):
- `~/.local/lib/fcitx5/freewubi.so`
- `~/.local/share/fcitx5/addon/freewubi.conf`
- `~/.local/share/fcitx5/inputmethod/freewubi.conf`
- `~/.local/share/fcitx5/data/` (dictionary and pinyin data)

### Register the addon library path

Fcitx5 does not search `~/.local/lib/fcitx5/` by default. Set `FCITX_ADDON_DIRS` so it finds the plugin.

If `~/.xinputrc` already exists (created by `im-config -n fcitx5`), add the export before the `run_im` line:

```bash
# ~/.xinputrc
export FCITX_ADDON_DIRS=/usr/lib/x86_64-linux-gnu/fcitx5:$HOME/.local/lib/fcitx5
run_im fcitx5
```

If `~/.xinputrc` does not exist, create it:

```bash
cat > ~/.xinputrc << 'EOF'
export FCITX_ADDON_DIRS=/usr/lib/x86_64-linux-gnu/fcitx5:$HOME/.local/lib/fcitx5
run_im fcitx5
EOF
```

### Install Fcitx5 (if not already installed)

```bash
sudo apt install -y fcitx5 fcitx5-chinese-addons \
  fcitx5-frontend-gtk3 fcitx5-frontend-gtk4 \
  fcitx5-frontend-qt5 fcitx5-config-qt im-config

im-config -n fcitx5  # Set fcitx5 as default input method framework
```

Log out and back in for the changes to take effect.

### Verify

1. Run `fcitx5-configtool` — FreeWubi should appear in the available input methods
2. Add **FreeWubi** to your active input methods
3. Open any text editor, switch to FreeWubi (Ctrl+Space)
4. Press `a` — should show candidates like `1.工 2.戈`
5. Press `1` to commit `工`, or `Space` to commit the top candidate

---

## Build from Source

### Install dev dependencies

```bash
sudo apt install -y libfcitx5core-dev libfcitx5config-dev libfcitx5utils-dev \
  extra-cmake-modules build-essential cmake
```

### Build and install

```bash
cd fcitx5-plugin
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make
make install
```

Then follow the [Register the addon library path](#register-the-addon-library-path) and [Verify](#verify) steps above.

---

## Debugging

For plugin debugging in VS Code, see `.vscode/launch.json` for the launch configuration.
