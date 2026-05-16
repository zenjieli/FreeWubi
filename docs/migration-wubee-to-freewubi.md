# Migrating from Wubee to FreeWubi

If you previously installed Wubee and want to switch to the renamed FreeWubi:

## Steps

```bash
# 1. Build and install FreeWubi
cd fcitx5-plugin/build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make install

# 2. Remove old Wubee files
rm ~/.local/lib/fcitx5/wubee.so
rm ~/.local/share/fcitx5/addon/wubee.conf
rm ~/.local/share/fcitx5/inputmethod/wubee.conf

# 3. Remove old environment.d config (replaced by new install)
rm ~/.config/environment.d/fcitx5-wubee.conf

# 4. Update fcitx5 input method list
#    Either run fcitx5-configtool and replace Wubee with FreeWubi,
#    or edit ~/.config/fcitx5/profile directly.
#    Change all occurrences of "wubee" to "freewubi".

# 5. Reload fcitx5
fcitx5-remote -r
```

## Verify

```bash
fcitx5-remote -n
# Should print: freewubi
```
