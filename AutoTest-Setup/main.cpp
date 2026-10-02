#include <windows.h>

import index;

import <filesystem>;
import <fstream>;
using std::ifstream;
using std::string;
using nlohmann::json;

uint8_t code[] =
{
	0x8B, 5, 0xC0, 0x9E, 0x6A, 0,
	0xC6, 0x80, 0x8E, 8, 0, 0, 0,

	0x68, 0, 0, 0, 0,
	0xFF, 0x15, 0xA4, 0x20, 0x65, 0,

	0x85, 0xC0,
	0x75, 9,

	0x6A, 0xFF,
	0xB8, 0x92, 0xDD, 0x61, 0,
	0xFF, 0xD0
};

char dll_pos[] = "../MatchCore.dll";

std::wstring Utf8ToWide(const string& value)
{
	const int size = ::MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
	std::wstring result(size, L'\0');
	::MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size);
	return result;
}

bool DistributeScript(const std::filesystem::path& source_path, const string& venv)
{
	const auto output_path = std::filesystem::path{Utf8ToWide(venv)} / "MatchConfig.lua";
	std::error_code error;
	if (std::filesystem::equivalent(source_path, output_path, error))
		return true;
	error.clear();
	if (std::filesystem::copy_file(source_path, output_path, std::filesystem::copy_options::overwrite_existing, error))
		return true;

	std::printf("Failed to copy script: %s\n", venv.c_str());
	return false;
}

int main(int argc, char* argv[])
{
	std::ifstream config{"file.json"};
	json j;
	config >> j;

	const bool should_distribute_script = j.contains("script");
	std::filesystem::path script_path;
	if (should_distribute_script)
		script_path = std::filesystem::path{Utf8ToWide(j.at("script").get_ref<const string&>())};

	for (uint8_t idx = 0; string str : j.at("venv"))
	{
		idx++;
		if (should_distribute_script && !DistributeScript(script_path, str))
			return 1;

		uint32_t pid = ProcessOpener::OpenByFilePath(Utf8ToWide(str).c_str(), L"PlantsVsZombies.exe");

		PVZ::InitPVZNoLock(pid);
		PVZ::Memory::immediateExecute = true;

		*(code + 12) = idx;
		auto pos = (int*)(code + 14);
		*pos = PVZ::Memory::AllocMemoryUnsafe();

		PVZ::Memory::WriteArrayUnsafe(*pos, dll_pos, sizeof(dll_pos));

		Injector inj(0x5513B5, 6, code, sizeof(code));

		PVZ::QuitPVZ();
	}
	std::system("pause");
	return 0;
}