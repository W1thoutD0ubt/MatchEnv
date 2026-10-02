-- MatchConfig.lua
-- 测试环境：全随机植物阵型 VS 每行随机僵尸

-- 基础配置
function isPoolEnabled()
    return false  -- 5行模式（无泳池）
end

function isAutoMode()
    return true  -- 自动模式
end

function getAccelerationFactor()
    return 250  -- 加速倍率
end

function shouldDrawBoard()
    return false  -- 是否绘制场景
end

function isIZMode()
    return false  -- 是否为 IZ 模式
end

-- 植物池：所有图鉴植物（0-45），排除玉米加农炮(46)和模仿者(47)
local plant_pool = {}
for i = 0, 45 do
    table.insert(plant_pool, i)
end

-- 僵尸池：排除气球(16)、矿工(17)、蹦极(20)、篮球(22)、小鬼(24)、僵王(25)
local zombie_pool = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,  -- 普通到盒子里僵尸
    18, 19,  -- 雪人和蹦极之间
    21,  -- 梯子
    23,  -- 巨人
    26, 27, 28, 29, 30, 31, 32  -- 植物僵尸到红眼巨人
}

-- 配置参数
local ZOMBIE_SPAWN_INTERVAL = 2500  -- 25秒 = 2500 cs（游戏时间单位）
local GARGANTUAR_CHANCE = 0.002  -- 0.2% 概率生成巨人

-- 淘汰队伍计数
local eliminated_count = 0

-- 波数计数
local wave_count = 0

-- 当前阵型记录（从列到行，45个数）
local current_layout = {}

-- 输出文件
local result_file = nil

-- 生成文件名（带时间戳）
local function generate_filename()
    local time = os.date("*t")
    return string.format("result_%04d%02d%02d_%02d%02d%02d.csv",
        time.year, time.month, time.day, time.hour, time.min, time.sec)
end

-- 初始化输出文件
local function init_result_file()
    if result_file == nil then
        local filename = generate_filename()
        result_file = io.open(filename, "w")
        if result_file then
            -- 写入表头
            local header = {}
            for col = 0, 8 do
                for row = 0, 4 do
                    table.insert(header, string.format("C%dR%d", col, row))
                end
            end
            table.insert(header, "Waves")
            result_file:write(table.concat(header, ",") .. "\n")
        end
    end
end

-- 初始化函数：随机生成植物阵型
function OnMatchInit()
    init_result_file()
    
    local rows = Match.RowsPerRound
    local cols = 9  -- 9列
    
    -- 清空当前阵型记录
    current_layout = {}
    
    -- 从列到行生成植物
    for col = 0, cols - 1 do
        for row = 0, rows - 1 do
            -- 随机选择植物
            local plant_index = math.random(1, #plant_pool)
            local plant_type = plant_pool[plant_index]
            
            -- 记录到阵型
            table.insert(current_layout, plant_type)
            
            -- 创建植物
            CreatePlant(plant_type, row, col)
        end
    end
    
    -- 初始化僵尸生成倒计时
    Match.SecondaryCounter = ZOMBIE_SPAWN_INTERVAL
    wave_count = 0
    eliminated_count = 0
end

-- 赛前准备（每帧调用，可用于延迟开赛）
function OnPreMatch()
    -- 默认立即开赛
end

-- 对局更新（每帧调用）
function OnMatchUpdate()
    -- 检查僵尸生成倒计时
    if Match.SecondaryCounter <= 0 then
        -- 重置倒计时
        Match.SecondaryCounter = ZOMBIE_SPAWN_INTERVAL
        wave_count = wave_count + 1
        
        -- 在每行生成僵尸
        local rows = Match.RowsPerRound
        for row = 0, rows - 1 do
            -- 决定僵尸类型
            local zombie_type
            if math.random() < GARGANTUAR_CHANCE then
                zombie_type = 23  -- 巨人
            else
                local index = math.random(1, #zombie_pool)
                zombie_type = zombie_pool[index]
            end
            
            -- 生成僵尸在列10，X坐标随机偏移
            local x = 780.0 + math.random() * 40
            CreateZombie(zombie_type, row, 10, { X = x })
        end
    end
end

-- 队伍被淘汰回调
function OnTeamEliminated(row)
    eliminated_count = eliminated_count + 1
    if eliminated_count >= 5 then
        Terminate(false)
    end
end

-- 比赛结束回调
function OnTerminate(plant_won)
    -- 输出当前阵型和坚持轮数
    if result_file then
        local line = {}
        for _, plant_type in ipairs(current_layout) do
            table.insert(line, tostring(plant_type))
        end
        table.insert(line, tostring(wave_count))
        result_file:write(table.concat(line, ",") .. "\n")
        result_file:flush()
    end
    
    -- 递增已完成对局数
    Match.Round = Match.Round + 1
end
