# BarleyMatch

PVZ（植物大战僵尸）自动化测试框架，支持通过 Lua 脚本配置测试环境，实现大规模植物强度测试。

同时支持手动模式，实现手动开局和观战。

## 功能特性

- **Lua 脚本配置**：通过 `MatchConfig.lua` 灵活定义测试规则
- **自动化测试**：支持多实例并行测试，自动循环执行
- **状态机驱动**：清晰的对局生命周期管理
- **数据输出**：可通过 lua 脚本自动输出测试结果

## 构建

项目使用 [vcpkg](https://github.com/microsoft/vcpkg) 管理依赖：

```bash
# 安装依赖
vcpkg install

# 使用 Visual Studio 打开 Match.slnx 构建
```

## 使用方法

### 1. 放置配置文件

将 `example` 目录下的配置文件复制到可执行文件所在目录：

```
YOUR_PVZ_DIR/
├── AutoTest-Setup.exe          // 测试启动软件
├── file.json                   // 测试配置文件
├── MatchCore.dll               // 测试内核
├── MatchConfig.lua             // 对局脚本
└── PlantsVsZombies.exe         // PVZ 主程序
```

PVZ 主程序必须命名为 `PlantsVsZombies.exe` 。

如果采用多文件夹分离方案，可通过 `file.json` 的 `script` 配置统一分发 `MatchConfig.lua`，无需在各游戏目录中分别维护脚本：

```
WORK_DIR/
├── AutoTest-Setup.exe
├── file.json
├── lua51.dll
├── MatchCore.dll
├── MatchConfig.lua
├── PVZ_1/
│   └── PlantsVsZombies.exe
├── PVZ_2/
│   └── PlantsVsZombies.exe
└── ...
```

### 2. 配置测试环境

按需修改 `MatchConfig.lua`，调整对局配置：

```lua
-- 基础配置
function isPoolEnabled()
    return false  -- 是否启用泳池（5行/6行）
end

function isAutoMode()
    return true  -- 是否自动模式
end

function getAccelerationFactor()
    return 250  -- 加速倍率
end

function shouldDrawBoard()
    return false  -- 是否绘制场景
end

-- 定义植物池、僵尸池等
```

### 3. 配置启动路径

编辑 `file.json`，设置各 PVZ 实例的路径：

```json
{
    "venv": ["PVZ安装路径"]
}
```

多实例时列出所有子文件夹路径，支持相对路径：

```json
{
    "script": "./MatchConfig.lua",
    "venv": [
        "./path/to/WORK_DIR/PVZ_1",
        "./path/to/WORK_DIR/PVZ_2"
    ]
}
```

`script` 支持相对路径和绝对路径。配置后，`AutoTest-Setup` 会在注入前将该脚本原样分发为每个游戏目录下的 `MatchConfig.lua`。省略 `script` 时，仍会直接使用各游戏目录中已有的 `MatchConfig.lua`。

### 4. 运行测试

1. 启动 PVZ 游戏
2. 运行 `AutoTest-Setup` 注入测试模块
3. 测试自动开始，结果由 Lua 脚本输出
  - 若在手动模式下，则通过快捷键 `Shift + K` 开始一场对局。

## Lua API 参考

### 配置函数

初始化时调用，必须实现：

| 函数 | 返回值 | 说明 |
|------|--------|------|
| `isPoolEnabled()` | `bool` | 是否启用泳池（true=6行，false=5行） |
| `isAutoMode()` | `bool` | 是否为自动对战模式 |
| `getAccelerationFactor()` | `number` | 加速倍率 |
| `shouldDrawBoard()` | `bool` | 是否绘制场景 |
| `isIZMode()` | `bool` | 是否为 IZ 模式 |

### 回调函数

在对应生命周期由 C++ 调用：

| 函数 | 参数 | 调用时机 |
|------|------|----------|
| `OnMatchInit()` | 无 | 对局初始化，用于布置植物阵型 |
| `OnPreMatch()` | 无 | 赛前准备阶段每帧调用 |
| `OnMatchUpdate()` | 无 | 比赛进行时每帧调用 |
| `OnTeamEliminated(row)` | `row`: 被淘汰的行号 | 小推车触发（普通模式）或脑子被吃（IZ 模式）时调用 |
| `OnTerminate(plant_won)` | `plant_won`: 植物方是否获胜 | 对局结束时调用 |

### 导出函数

可在 Lua 中直接调用：

| 函数 | 参数 | 说明 |
|------|------|------|
| `CreatePlant(type, row, column, options)` | 植物类型、行、列、可选配置 table | 在指定位置创建植物，可设置初始生命值和睡眠状态，详见 [植物配置](../example/CreatePlant.md) |
| `CreateZombie(type, row, column, x)` | 僵尸类型、行、列、X坐标 | 在指定位置创建僵尸 |
| `CreateIZBrain(row, column)` | 行、列（默认0） | 在指定位置创建 IZ 模式的脑子 |
| `ClearPlants()` | 无 | 清除场上所有植物 |
| `Terminate(plant_won)` | `plant_won`: 植物方是否获胜 | 终止当前对局，触发 `OnTerminate(plant_won)` |
| `HasZombie()` | 无 | `bool` | 判断场上是否有僵尸 |

### Match 属性

通过 `Match.xxx` 读写游戏状态：

| 属性 | 读写 | 说明 |
|------|------|------|
| `Match.StateCountdown` | 读写 | 阶段转换倒计时，归零时进入下一阶段 |
| `Match.PrimaryCounter` | 读写 | 游戏时间计数器（PlayingTime） |
| `Match.SecondaryCounter` | 读写 | 僵尸生成倒计时 |
| `Match.TertiaryCounter` | 读写 | 通用计时器 |
| `Match.Round` | 读写 | 已完成对局数 |
| `Match.RowsPerRound` | 只读 | 每回合行数（5或6） |
| `Match.ProcessIndex` | 只读 | 当前进程的测试顺序编号（从1开始） |
