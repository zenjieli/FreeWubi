# FreeWubi

A Wubi 86 input method engine for Fcitx5 on Linux.

- **Wubi 86 input** with frequency-based candidate ordering and prefix matching
- **Temp pinyin lookup** — type pinyin to find a character's wubi code
- **Two-tier dictionary** — common characters by default, rare characters on demand

## Quick Install

Download the latest release from [Releases](https://github.com/zli/FreeWubi/releases):

```bash
tar xzf freewubi-v0.1.0-linux-x86_64.tar.gz -C ~/.local
```

See the full [Installation Guide](docs/installation.md) for setup details and
[building from source](docs/installation.md#build-from-source).

## Documentation

- [Installation Guide](docs/installation.md) — install from release or build from source
- [Usage Guide](docs/usage.md) — key bindings, modes, Chinese punctuation
- [Development](docs/development.md) — build, test, code style, architecture

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

MIT
