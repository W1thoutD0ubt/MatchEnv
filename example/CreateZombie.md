# CreateZombie 初始属性

```lua
CreateZombie(type, row, column)
CreateZombie(type, row, column, x)       -- 兼容旧写法
CreateZombie(type, row, column, options)
```

`options` 为可选 table，也可传入 `nil`。行、列沿用现有的从 0 开始的编号。

| 字段 | 类型 | 含义 |
| --- | --- | --- |
| `X` | number | 创建完成后的实际横坐标 |
| `BodyHealth` | integer | 本体当前生命值 |
| `BodyMaxHealth` | integer | 本体生命上限 |
| `HelmHealth` | integer | 头盔当前生命值 |
| `HelmMaxHealth` | integer | 头盔生命上限 |
| `ShieldHealth` | integer | 盾牌当前生命值 |
| `ShieldMaxHealth` | integer | 盾牌生命上限 |
| `FlyingHealth` | integer | 气球当前生命值 |
| `FlyingMaxHealth` | integer | 气球生命上限 |
| `Hypnotized` | boolean | 是否被魅惑 |
| `AttributeCountdown` | integer | 当前状态使用的属性倒计时 |

## 赋值规则

- 省略字段或填入 `nil` 时，保留创建时的默认值。
- 当前生命值和生命上限独立赋值，不自动联动，也不校验两者大小关系。
- 不检查部位是否适用；填写防具生命值不会添加或移除装备。
- `false` 和 `0` 均为有效值；整数接受底层 `int` 范围内的值，不限制正负。
- `Hypnotized = true` 调用游戏的魅惑方法；`false` 写入未魅惑标志。
- 先处理魅惑，再设置坐标及生命值，最后设置 `AttributeCountdown`。
- `AttributeCountdown` 的用途取决于僵尸类型和当前状态，不是召唤延迟或通用状态效果时长。它按游戏逻辑更新，不按现实时间计。
- `X` 是创建后的坐标覆盖，不保证飞行或跳跃僵尸后续的最终落点。
- table 仅在本次创建时读取，之后修改 table 不会改变已经创建的僵尸。
- 字段名区分大小写；未知字段、错误类型、非有限数值、超出存储范围的数值和非整数的整数属性会在创建前报错。

## 示例

```lua
-- 默认生成
CreateZombie(0, 2, 10)

-- 覆盖横坐标，与旧写法 CreateZombie(0, 2, 10, 850) 等价
CreateZombie(0, 2, 10, { X = 850 })

-- 满血的增强版普通僵尸
CreateZombie(0, 2, 10, {
    BodyHealth = 500,
    BodyMaxHealth = 500,
})

-- 受伤出生的铁桶僵尸
CreateZombie(4, 2, 10, {
    BodyHealth = 100,
    BodyMaxHealth = 500,
    HelmHealth = 300,
    HelmMaxHealth = 1500,
})

-- 创建魅惑僵尸
CreateZombie(0, 2, 10, {
    X = 700,
    Hypnotized = true,
})

-- 明确设置 false 和 0；倒计时的行为由该僵尸的当前状态决定
CreateZombie(0, 2, 10, {
    Hypnotized = false,
    AttributeCountdown = 0,
})
```
