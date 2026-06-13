[**English**](README.md) | [中文](README.zh-CN.md)

# FreeWubi

A versatile Wubi 86 input method engine for Fcitx5 on Linux. Easy to set up, easy
to use — type Chinese via Wubi codes with a handful of convenient shortcuts
for pinyin lookup, temporary English, and more. Every trigger key is
configurable.

## Table of Contents

- [Introduction](#introduction)
- [Get Started](#get-started)
- [Documentation](#documentation)
- [For Developers](#for-developers)
- [Supported Platforms](#supported-platforms)
- [License](#license)

## Introduction

FreeWubi is an easy-to-use Wubi 86 input method for Linux — ideal for
fast Chinese typing with seamless En/CN switching and occasional pinyin
lookup:

- **Wubi 86 input** — frequency-based candidate ordering, prefix matching, and
  automatic commit when a code is unambiguous

  ![Wubi input](docs/images/wubi-input.png)

- **Temp pinyin lookup** — forgot a character's Wubi code? Type pinyin to find
  it, with the Wubi code shown next to each candidate

  ![Temp pinyin lookup](docs/images/temp-pinyin.png)

- **Temp English mode** — type a short English phrase inline without toggling
  the entire input method
- **Slash mode** — quickly type paths and commands (`/usr/bin`) without
  leaving Chinese mode
- **Custom phrases** — define your own code-to-phrase shortcuts
- **Z-key repeat** — press `z` to re-commit the last Wubi character instantly
- **Two-tier dictionary** — common characters by default; press `` ` `` mid-
  composition to reveal rare characters
- **Chinese punctuation** — comma, period, quotes, and more map to their Chinese
  equivalents automatically
- **Fully configurable keys** — all trigger keys (temp pinyin, temp English,
  slash mode, etc.) can be remapped in Fcitx5 settings

> For the full key binding reference, see the [Usage Guide](docs/usage.md).

## Get Started

### 1. Install Fcitx5 (if not already installed)

```bash
sudo apt install -y fcitx5 fcitx5-chinese-addons \
  fcitx5-frontend-gtk3 fcitx5-frontend-gtk4 \
  fcitx5-frontend-qt5 fcitx5-config-qt im-config

im-config -n fcitx5
```

Log out and back in.

### 2. Download and install FreeWubi

Grab the latest tarball from the [Releases](https://github.com/zli/FreeWubi/releases)
page and extract it:

```bash
tar xzf freewubi-v0.1.0-linux-x86_64.tar.gz -C ~/.local
```

### 3. Tell Fcitx5 where to find the plugin

Fcitx5 does not search `~/.local/lib/fcitx5/` by default. Create or update
`~/.xinputrc`:

```bash
cat > ~/.xinputrc << 'EOF'
export FCITX_ADDON_DIRS=/usr/lib/x86_64-linux-gnu/fcitx5:$HOME/.local/lib/fcitx5
run_im fcitx5
EOF
```

### 4. Enable FreeWubi

1. Run `fcitx5-configtool`
2. Click **Add Input Method** → search for **FreeWubi** → add it
3. Switch to FreeWubi with `Ctrl+Space`
4. Press `a` — you should see candidates like `1.工 2.戈`

> See the [Installation Guide](docs/installation.md) for more details,
> troubleshooting, and building from source.

## Documentation

| Document | Description |
|---|---|
| [Installation Guide](docs/installation.md) | Install from release, build from source, troubleshooting |
| [Usage Guide](docs/usage.md) | All key bindings, modes, and Chinese punctuation tables |

## For Developers

FreeWubi uses a three-layer architecture (Fcitx5 adapter → engine logic →
dictionaries) so the core logic is fully testable without Fcitx5.

```bash
# Clone and build
git clone https://github.com/zli/FreeWubi.git
cd FreeWubi/fcitx5-plugin
cmake -B build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
cd build && ctest --output-on-failure
```

See the [Development Guide](docs/development.md) for architecture details, code
style, CI workflows, and contributing guidelines.

## Supported Platforms

| Distro | Minimum Version |
|---|---|
| Ubuntu | 22.04 |
| Debian | 12 |
| Fedora | 38 |
| Arch Linux | rolling |
| openSUSE | Leap 15.5 / Tumbleweed |

Any distribution that ships Fcitx5 and GCC 12+ should work.

## License

[Apache 2.0](LICENSE)

## Acknowledgments

The Wubi 86 dictionary is based on the widely used
[rime-wubi86-jidian](https://github.com/KyleBing/rime-wubi86-jidian) by KyleBing
(Apache 2.0).
