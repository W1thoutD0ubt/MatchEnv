# CreatePlant

```lua
CreatePlant(type, row, column, options)
```

在指定位置创建植物，返回 `PVZ::Plant`。行、列从 0 开始编号。

`options` 是可选 table，也可以传入 `nil`。省略、传入 `nil` 或 `{}` 时保留游戏原始初始化结果，不额外设置任何属性。

## 初始属性

| 字段 | 类型 | 说明 |
|------|------|------|
| `Hp` | 整数 | 当前生命值 |
| `MaxHp` | 整数 | 最大生命值 |
| `Sleeping` | 布尔值 | `true` 表示睡眠，`false` 表示清醒，通过底层 `SetSleeping()` 设置 |

仅应用明确提供的字段。字段缺失或为 `nil` 时不赋值，也不使用默认参数覆盖游戏结果。`Hp` 与 `MaxHp` 独立设置，不自动同步；`Sleeping = false` 是显式设置，不会被当作未提供。

生命值只要求为底层 32 位有符号整数可表示的整数，不增加业务范围限制。创建前完成配置校验：未知字段、非字符串键、错误类型、小数、非有限数值、超出整数表示范围的数值会触发 Lua 错误，不创建植物。

创建后先应用显式提供的睡眠状态，再写入显式提供的生命值。这些配置只在生成时应用一次。

## 示例

```lua
CreatePlant(0, 0, 0)       -- 使用游戏默认情况
CreatePlant(0, 0, 1, nil)  -- 使用游戏默认情况
CreatePlant(0, 0, 2, {})   -- 使用游戏默认情况

CreatePlant(0, 1, 0, { Hp = 1000, MaxHp = 1000 })
CreatePlant(0, 1, 1, { Hp = 500 })  -- 仅修改当前生命值，最大生命值保持默认
CreatePlant(8, 2, 0, { Sleeping = false }) -- 仅设置为清醒
CreatePlant(8, 2, 1, { Hp = 1000, MaxHp = 1000, Sleeping = true })
```
