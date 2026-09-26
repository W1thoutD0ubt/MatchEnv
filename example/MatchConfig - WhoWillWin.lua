-- MatchConfig - WhoWillWin.lua
-- 测试环境：谁会赢？测试

local zombie_win_round = 0
local total = 10000

-- 基础配置
function isPoolEnabled()
	return false  -- 5行模式（无泳池）
end

function isAutoMode()
	return true  -- 自动模式
end

function getAccelerationFactor()
	return 10  -- 加速倍率
end

function shouldDrawBoard()
	return true  -- 是否绘制场景
end

function isIZMode()
	return true  -- 是否为 IZ 模式
end

-- 输出文件
local result_file = nil

-- 生成文件名（带时间戳）
local function generate_filename()
	local time = os.date("*t")
	return string.format("result_%04d%02d%02d_%02d%02d%02d-%d.jsonl",
		time.year, time.month, time.day, time.hour, time.min, time.sec, Match.ProcessIndex)
end

-- 初始化输出文件
local function init_result_file()
	if result_file == nil then
		local filename = generate_filename()
		result_file = io.open(filename, "w")
	end
end

-- 初始化函数：随机生成植物阵型
function OnMatchInit()
	init_result_file()
	
	for col = 0, 4 do
		CreatePlant(34, 2, col)
	end
	CreateZombie(32, 2, 8)
	CreateIZBrain(2)
end

-- 赛前准备（每帧调用，可用于延迟开赛）
function OnPreMatch()
	-- 默认立即开赛
end

-- 对局更新（每帧调用）
function OnMatchUpdate()
end

-- 队伍被淘汰回调
function OnTeamEliminated(row)
	Terminate(false)
end

-- 比赛结束回调
function OnTerminate(plant_won)
	-- 输出当前阵型和坚持轮数
	if not plant_won then
		zombie_win_round = zombie_win_round + 1
	end
	
	-- 递增已完成对局数
	Match.Round = Match.Round + 1
	if Match.Round == total then
		result_file:write(string.format("{\"formation\":\"P34|P34|P34|P34|P34|_|_|_|Z32\",\"zombie_wins\":%d,\"trials\":%d}"
			, zombie_win_round, total))
		result_file:flush()

		exit()
	end
end
