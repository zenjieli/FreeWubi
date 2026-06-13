[English](installation.md) | **中文**

# 安装

## 从 GitHub Release 安装

预编译二进制文件可在 [Releases](https://github.com/zli/FreeWubi/releases) 页面获取。

1. 下载最新的 `freewubi-v*-linux-x86_64.tar.gz`
2. 解压到用户主目录：

```bash
tar xzf freewubi-v0.1.0-linux-x86_64.tar.gz -C ~/.local
```

安装到用户本地目录（无需 sudo）：
- `~/.local/lib/fcitx5/freewubi.so`
- `~/.local/share/fcitx5/addon/freewubi.conf`
- `~/.local/share/fcitx5/inputmethod/freewubi.conf`
- `~/.local/share/fcitx5/data/`（字典和拼音数据）

### 注册插件库路径

Fcitx5 默认不会搜索 `~/.local/lib/fcitx5/`。需要设置 `FCITX_ADDON_DIRS` 让 Fcitx5 找到插件。

如果 `~/.xinputrc` 已存在（由 `im-config -n fcitx5` 创建），在 `run_im` 行之前添加 export：

```bash
# ~/.xinputrc
export FCITX_ADDON_DIRS=/usr/lib/x86_64-linux-gnu/fcitx5:$HOME/.local/lib/fcitx5
run_im fcitx5
```

如果 `~/.xinputrc` 不存在，创建它：

```bash
cat > ~/.xinputrc << 'EOF'
export FCITX_ADDON_DIRS=/usr/lib/x86_64-linux-gnu/fcitx5:$HOME/.local/lib/fcitx5
run_im fcitx5
EOF
```

### 安装 Fcitx5（如尚未安装）

```bash
sudo apt install -y fcitx5 fcitx5-chinese-addons \
  fcitx5-frontend-gtk3 fcitx5-frontend-gtk4 \
  fcitx5-frontend-qt5 fcitx5-config-qt im-config

im-config -n fcitx5  # 设置 fcitx5 为默认输入法框架
```

注销并重新登录以使更改生效。

### 验证

1. 运行 `fcitx5-configtool` —— FreeWubi 应出现在可用输入法列表中
2. 将 **FreeWubi** 添加到当前输入法
3. 打开任意文本编辑器，切换到 FreeWubi（Ctrl+Space）
4. 按下 `a` —— 应显示候选词，如 `1.工 2.戈`
5. 按下 `1` 上屏 `工`，或按 `Space` 上屏第一个候选词

---

## 从源码构建

### 安装开发依赖

```bash
sudo apt install -y libfcitx5core-dev libfcitx5config-dev libfcitx5utils-dev \
  extra-cmake-modules build-essential cmake
```

### 构建并安装

```bash
cd fcitx5-plugin
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make
make install
```

然后按照上述[注册插件库路径](#注册插件库路径)和[验证](#验证)步骤操作。

---

## 调试

如需在 VS Code 中调试插件，请参见 `.vscode/launch.json` 中的启动配置。
