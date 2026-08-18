[English](development.md) | **中文**

# 开发

## 前置条件

```bash
sudo apt install -y libfcitx5core-dev libfcitx5config-dev libfcitx5utils-dev \
  extra-cmake-modules build-essential cmake
```

## 构建

所有命令在 `fcitx5-plugin/` 目录下运行：

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
cmake --install build   # 安装到 ~/.local
```

## 测试

```bash
cmake -B build -DBUILD_TESTING=ON
cmake --build build --target test_engine   # 仅构建测试
cd build && ctest --output-on-failure       # 运行测试
```

运行单个测试用例：
```bash
./build/tests/test_engine "Literal mode: Enter commits"
```

## 代码风格

使用 clang-format 强制格式化（Google 风格，120 列）。CI 在每个 PR 上运行
`clang-format --dry-run --Werror`，未格式化的代码会导致检查失败。

```bash
# 检查格式
clang-format --dry-run --Werror $(git ls-files '*.cpp' '*.h')

# 修复格式
clang-format -i $(git ls-files '*.cpp' '*.h')
```

`clang-format` 的报错行号可能不是你直接改动的那一行：它会对齐连续多行的行尾注释，
在附近加一条更长的注释会改变相邻行所需的对齐方式。请对整个文件重新检查，而不只是
diff 里的那几行。

为了在提交到 CI 之前就发现问题，可以安装本仓库的 pre-commit hook（检查已 stage 的
`.cpp`/`.h` 文件，格式不通过则阻止提交）：

```bash
ln -sf ../../scripts/pre-commit .git/hooks/pre-commit
```

### 命名规范

遵循 [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
命名规则：

| 元素 | 规则 | 示例 |
|---|---|---|
| 文件 | 小写 + 下划线 | `engine_logic.cpp` |
| 类型（类/结构体/枚举） | CapWords | `EngineLogic`, `Modifiers` |
| 函数 / 方法 | camelCase | `processKey`, `pageSize()` |
| 变量（局部变量、参数） | snake_case | `page_offset`, `text` |
| 类数据成员 | snake_case + 末尾 `_` | `dict_`, `pageSize_` |
| 结构体数据成员 | 纯 snake_case | `slashBuffer` |
| 常量与枚举值 | `k` + CapWords | `kMaxCodeLen`, `kNone` |
| 命名空间 | 小写 | `keys` |
| `keys::` keysym 常量 | CapWords（与 X11 一致） | `keys::Control_L` |

两个刻意的偏离 —— 请勿"修正"：
- 函数/方法使用 **camelCase**，而非 Google 的 CapWords
- `keys::` 常量保持 CapWords，以对应 X11 keysym 名称（`XK_Control_L`）

完整规范参考请参见 [CLAUDE.md](../CLAUDE.md)。

## 架构

```
Fcitx5 适配层          freewubi.cpp, fcitx_output.h
    ↓
引擎逻辑              engine_logic.cpp, engine_types.h   （无 fcitx5 依赖）
    ↓
字典                  wubi_dict.cpp, pinyin_dict.cpp      （无 fcitx5 依赖）
```

**按键事件流：** Fcitx5 按键事件 → `FreeWubiEngine::keyEvent()`（freewubi.cpp）
→ `EngineLogic::processKey()`（engine_logic.cpp）→ 字典查找 → 候选列表
→ `IEngineOutput` 接口 → FcitxOutput 或 TestOutput。

### 三层设计

- **Fcitx5 适配层** —— 将 Fcitx5 按键事件转换为可移植的 `KeyInput`，通过 Fcitx5 API 渲染候选词和预编辑文本
- **引擎逻辑** —— 纯 C++ 状态机，无 Fcitx5 依赖，通过 `IEngineOutput` 接口完全可测试
- **字典** —— 加载 YAML/TSV 数据文件，提供按编码查找功能

### 两级字典

- `dict_`：完整五笔 86 字典（约 8.9 万条目）
- `dict_common_`：仅常用字（由 `scripts/build_common_dict.py` 生成）

`EngineLogic::activeDict()` 默认返回 `dict_common_`，生僻字模式开启时返回 `dict_`。

## CI 工作流

| 工作流 | 触发条件 | 功能 |
|---|---|---|
| `format-check.yml` | 推送到 `main`，PR | 对所有 `.cpp`/`.h` 运行 `clang-format --dry-run --Werror` |
| `test.yml` | 推送到 `main`，PR | 使用 `-DBUILD_TESTING=ON` 构建，运行 `ctest` |
| `release.yml` | 推送 `v*` 标签 | Release 构建，打包 tarball，创建 GitHub Release |

## 贡献

1. Fork 本仓库并创建分支
2. 进行修改，使用 `clang-format -i` 格式化
3. 推送并创建 Pull Request
4. CI 必须通过（`clang-format` ✅ + `cpp-tests` ✅）后方可合并
