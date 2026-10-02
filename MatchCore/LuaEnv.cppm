module;

#define SOL_LUAJIT 1
#include <sol/sol.hpp>
#include <windows.h>
#include <cmath>
#include <limits>
#include <type_traits>

export module LuaEnv;

import index;
import Util;
import Global;

static sol::state lua;
static bool lua_initialized = false;

struct ZombieSpawnOptions
{
	sol::optional<float> X;
	sol::optional<int> BodyHealth;
	sol::optional<int> BodyMaxHealth;
	sol::optional<int> HelmHealth;
	sol::optional<int> HelmMaxHealth;
	sol::optional<int> ShieldHealth;
	sol::optional<int> ShieldMaxHealth;
	sol::optional<int> FlyingHealth;
	sol::optional<int> FlyingMaxHealth;
	sol::optional<bool> Hypnotized;
	sol::optional<int> AttributeCountdown;
};

template<typename T>
static T ReadZombieSpawnNumber(const sol::object& value, const char* field)
{
	if (value.get_type() != sol::type::number)
		throw sol::error(std::string("CreateZombie: ") + field + " must be a number");

	double number = value.as<double>();
	if (!std::isfinite(number)
		|| number < (std::numeric_limits<T>::lowest)()
		|| number > (std::numeric_limits<T>::max)())
		throw sol::error(std::string("CreateZombie: ") + field + " is out of range");

	if constexpr (std::is_integral_v<T>)
	{
		if (std::trunc(number) != number)
			throw sol::error(std::string("CreateZombie: ") + field + " must be an integer");
	}
	return static_cast<T>(number);
}

template<typename T>
static sol::optional<T> ReadZombieSpawnField(const sol::table& table, const char* field)
{
	sol::object value = table.raw_get<sol::object>(field);
	if (value.get_type() == sol::type::lua_nil)
		return sol::nullopt;

	if constexpr (std::is_same_v<T, bool>)
	{
		if (value.get_type() != sol::type::boolean)
			throw sol::error(std::string("CreateZombie: ") + field + " must be a boolean");
		return value.as<bool>();
	}
	else
		return ReadZombieSpawnNumber<T>(value, field);
}

static ZombieSpawnOptions ReadZombieSpawnOptions(sol::variadic_args args)
{
	ZombieSpawnOptions options;
	if (args.begin() == args.end())
		return options;

	sol::object value = *args.begin();
	if (value.get_type() == sol::type::lua_nil)
		return options;
	if (value.get_type() == sol::type::number)
	{
		options.X = ReadZombieSpawnNumber<float>(value, "X");
		return options;
	}
	if (value.get_type() != sol::type::table)
		throw sol::error("CreateZombie: fourth argument must be a number, table or nil");

	auto table = value.as<sol::table>();
	struct IntegerField
	{
		const char* name;
		sol::optional<int> ZombieSpawnOptions::* member;
	};
	static constexpr IntegerField integer_fields[] = {
		{ "BodyHealth", &ZombieSpawnOptions::BodyHealth },
		{ "BodyMaxHealth", &ZombieSpawnOptions::BodyMaxHealth },
		{ "HelmHealth", &ZombieSpawnOptions::HelmHealth },
		{ "HelmMaxHealth", &ZombieSpawnOptions::HelmMaxHealth },
		{ "ShieldHealth", &ZombieSpawnOptions::ShieldHealth },
		{ "ShieldMaxHealth", &ZombieSpawnOptions::ShieldMaxHealth },
		{ "FlyingHealth", &ZombieSpawnOptions::FlyingHealth },
		{ "FlyingMaxHealth", &ZombieSpawnOptions::FlyingMaxHealth },
		{ "AttributeCountdown", &ZombieSpawnOptions::AttributeCountdown }
	};
	for (const auto& entry : table)
	{
		bool known = false;
		if (entry.first.get_type() == sol::type::string)
		{
			auto key = entry.first.as<std::string>();
			known = key == "X" || key == "Hypnotized";
			for (const auto& field : integer_fields)
				if (key == field.name)
				{
					known = true;
					break;
				}
			if (!known)
				throw sol::error("CreateZombie: unknown option " + key);
		}
		else
			throw sol::error("CreateZombie: option keys must be strings");
	}

	options.X = ReadZombieSpawnField<float>(table, "X");
	options.Hypnotized = ReadZombieSpawnField<bool>(table, "Hypnotized");
	for (const auto& field : integer_fields)
		options.*(field.member) = ReadZombieSpawnField<int>(table, field.name);
	return options;
}

static void ApplyZombieSpawnOptions(PVZ::Zombie& zombie, const ZombieSpawnOptions& options)
{
	if (options.Hypnotized.has_value())
	{
		if (*options.Hypnotized)
			zombie.Hypnotize();
		else
			zombie.Hypnotized = false;
	}
	if (options.X.has_value()) zombie.X = *options.X;
	if (options.BodyHealth.has_value()) zombie.BodyHealth = *options.BodyHealth;
	if (options.BodyMaxHealth.has_value()) zombie.BodyMaxHealth = *options.BodyMaxHealth;
	if (options.HelmHealth.has_value()) zombie.HelmHealth = *options.HelmHealth;
	if (options.HelmMaxHealth.has_value()) zombie.HelmMaxHealth = *options.HelmMaxHealth;
	if (options.ShieldHealth.has_value()) zombie.ShieldHealth = *options.ShieldHealth;
	if (options.ShieldMaxHealth.has_value()) zombie.ShieldMaxHealth = *options.ShieldMaxHealth;
	if (options.FlyingHealth.has_value()) zombie.FlyingHealth = *options.FlyingHealth;
	if (options.FlyingMaxHealth.has_value()) zombie.FlyingMaxHealth = *options.FlyingMaxHealth;
	// Apply last so Hypnotize cannot overwrite the requested initial countdown.
	if (options.AttributeCountdown.has_value()) zombie.AttributeCountdown = *options.AttributeCountdown;
}

export void LuaEnvInit();
export void LuaCallOnMatchInit()
{
	lua["OnMatchInit"]();
}
export void LuaCallOnPreMatch()
{
	lua["OnPreMatch"]();
}
export void LuaCallOnMatchUpdate()
{
	lua["OnMatchUpdate"]();
}
export void LuaCallOnTeamEliminated(int row)
{
	lua["OnTeamEliminated"](row);
}
export void LuaCallOnTerminate(bool plant_won)
{
	auto challenge = PVZ::GetBoard().GetChallenge();
	challenge.State = ChallengeState::BARLEYMATCH_AFTERMATCH;
	challenge.AttributeCountdown = 1;
	lua["OnTerminate"](plant_won);
}

void LuaEnvInit()
{
	if (lua_initialized)
		return;

	lua.open_libraries(
		sol::lib::base,
		sol::lib::package,
		sol::lib::coroutine,
		sol::lib::string,
		sol::lib::os,
		sol::lib::math,
		sol::lib::table,
		sol::lib::debug,
		sol::lib::io
	);

	lua.set_function("CreatePlant", [](int type, int row, int column) {
		return Creator::CreatePlant(static_cast<SeedType::SeedType>(type), row, column);
	});

	lua.set_function("CreateZombie", [](int type, int row, int column, sol::variadic_args args) {
		auto options = ReadZombieSpawnOptions(args);
		auto zombie = Creator::CreateZombie(static_cast<ZombieType::ZombieType>(type), row, column);
		ApplyZombieSpawnOptions(zombie, options);
		return zombie;
	});

	lua.set_function("ClearPlants", []() {
		for (auto plant : PVZ::GetBoard().GetAllPlants())
			plant.Remove();
	});

	lua.set_function("CreateIZBrain", [](int row, sol::optional<int> column) {
		return Creator::CreateIZBrain(row, column.value_or(0));
	});

	lua.set_function("HasZombie", []() {
		return PVZ::GetBoard().ZombiesCount > 0;
	});

	lua.set_function("Terminate", [](sol::optional<bool> plant_won) {
		LuaCallOnTerminate(plant_won.value_or(false));
	});

	lua.set_function("exit", []() {
		ExitProcess(0);
	});

	struct MatchProxy {};
	auto ut = lua.new_usertype<MatchProxy>("Match", sol::no_constructor);
	ut["StateCountdown"] = sol::property(
		[](MatchProxy&) -> int {
			return PVZ::GetBoard().GetChallenge().AttributeCountdown;
		},
		[](MatchProxy&, int v) {
			PVZ::GetBoard().GetChallenge().AttributeCountdown = v;
		}
	);
	ut["PrimaryCounter"] = sol::property(
		[](MatchProxy&) -> int {
			return PVZ::GetBoard().PlayingTime;
		},
		[](MatchProxy&, int v) {
			PVZ::GetBoard().PlayingTime = v;
		}
	);
	ut["SecondaryCounter"] = sol::property(
		[](MatchProxy&) -> int {
			return PVZ::GetBoard().GetChallenge().ConveyorCountdown;
		},
		[](MatchProxy&, int v) {
			PVZ::GetBoard().GetChallenge().ConveyorCountdown = v;
		}
	);
	ut["TertiaryCounter"] = sol::property(
		[](MatchProxy&) -> int {
			return PVZ::GetBoard().GetChallenge().LevelProcess;
		},
		[](MatchProxy&, int v) {
			PVZ::GetBoard().GetChallenge().LevelProcess = v;
		}
	);
	ut["Round"] = sol::property(
		[](MatchProxy&) -> int {
			return PVZ::GetBoard().GetChallenge().Round;
		},
		[](MatchProxy&, int v) {
			PVZ::GetBoard().GetChallenge().Round = v;
		}
	);
	ut["RowsPerRound"] = sol::property(
		[](MatchProxy&) -> int {
			return row_per_round;
		}
	);
	ut["ProcessIndex"] = sol::property(
		[](MatchProxy&) -> int {
			return PVZ::Memory::ReadMemory<uint8_t>(PVZ::GetPVZApp().GetBaseAddress() + 0x88E);
		}
	);
	lua["Match"] = MatchProxy{};

	std::string config_path = GetWorkingDirName("MatchConfig.lua");
	lua.script_file(config_path);

	sol::protected_function_result result = lua["isPoolEnabled"]();
	if (!result.valid() || result.get_type() != sol::type::boolean)
	{
		sol::error err = result;
		MessageBoxA(NULL, err.what(), "isPoolEnabled 存在异常", MB_ICONERROR);
		ExitProcess(1);
	}
	isPoolEnabled = result.get<bool>();

	sol::protected_function_result result2 = lua["isAutoMode"]();
	if (!result2.valid() || result2.get_type() != sol::type::boolean)
	{
		sol::error err = result2;
		MessageBoxA(NULL, err.what(), "isAutoMode 存在异常", MB_ICONERROR);
		ExitProcess(1);
	}
	isAutoMode = result2.get<bool>();

	sol::protected_function_result result3 = lua["getAccelerationFactor"]();
	if (!result3.valid() || result3.get_type() != sol::type::number)
	{
		sol::error err = result3;
		MessageBoxA(NULL, err.what(), "getAccelerationFactor 存在异常", MB_ICONERROR);
		ExitProcess(1);
	}
	accelerationFactor = result3.get<uint32_t>();

	sol::protected_function_result result4 = lua["shouldDrawBoard"]();
	if (!result4.valid() || result4.get_type() != sol::type::boolean)
	{
		sol::error err = result4;
		MessageBoxA(NULL, err.what(), "shouldDrawBoard 存在异常", MB_ICONERROR);
		ExitProcess(1);
	}
	shouldDrawBoard = result4.get<bool>();

	sol::protected_function_result result5 = lua["isIZMode"]();
	if (!result5.valid() || result5.get_type() != sol::type::boolean)
	{
		sol::error err = result5;
		MessageBoxA(NULL, err.what(), "isIZMode 存在异常", MB_ICONERROR);
		ExitProcess(1);
	}
	isIZMode = result5.get<bool>();

	lua_initialized = true;
}
