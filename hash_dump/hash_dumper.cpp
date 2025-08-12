#include "hash_dumper.hpp"
#include "utils.hpp"
#include "MurmurHash2.h"

#include <algorithm>

#include <mem/pattern.h>


bool patchray::hash_dumper::is_valid_key(std::string_view key, int len, uint64_t seed) noexcept {
	if (seed != 0 || len <= 0) {
		return false;
	}
	//filter out hashes of hex values 
	if (3 <= len && len <= 18 && key[0] == '0' && key[1] == 'x') {
		return false;
	}

	return std::all_of(key.begin(), key.end(), key_char_fitler);
}

bool patchray::hash_dumper::save_dictionary(const std::string& save_name) const noexcept {
	std::ofstream file(save_name);
	if (!file.is_open())
		return false;

	for (auto&& [_, key] : m_murmur64_dict) {
		file << key << delimeter;
	}
	return true;
}

bool patchray::hash_dumper::load_dictionary(const std::string& save_name) noexcept {
	std::ifstream file(save_name);
	if (!file.is_open())
		return false;

	std::string key;
	while (std::getline(file, key, delimeter)) {
		m_murmur64_dict.try_emplace(MurmurHash64A(key.c_str(), key.size(), 0), key);
	}
	return true;
}

std::string_view patchray::hash_dumper::lookup(murmur64_t hash) const noexcept {
	std::unique_lock lock(m_dict_mutex);
	auto iter = m_murmur64_dict.find(hash);

	if(iter == m_murmur64_dict.end())
		return std::string_view();

	return iter->second;
}

void patchray::hash_dumper::save_worker() noexcept {
	while (!m_quitting) {
		std::this_thread::sleep_for(save_thread_delay);
		{
			std::unique_lock lock(m_dict_mutex);
			m_cv.wait(lock, [this] { return m_murmur64_dict.size() != m_dict_last_save_size || m_quitting; });
			m_dict_last_save_size = m_murmur64_dict.size();
			save_dictionary(save_path);
		}
	}
}

void patchray::hash_dumper::try_emplace_dict(std::string_view key, murmur64_t hash) noexcept {
	std::unique_lock lock(m_dict_mutex);
	const auto iter = m_murmur64_dict.find(hash);

	if (iter == m_murmur64_dict.end()) {
		m_murmur64_dict.emplace(hash, key);
		m_cv.notify_all();
	}
}

uint64_t __fastcall patchray::hash_dumper::murmur64_hook(const char* key, int len, uint64_t seed) noexcept {
	const auto hash = MurmurHash64A(key, len, seed);
	const auto key_str = std::string_view(key, len);

	if (is_valid_key(key_str, len, seed)) {
		auto& dumper = get_instance();
		dumper.try_emplace_dict(key_str, hash);
	}
	return hash;
}

bool patchray::hash_dumper::init() noexcept {
	m_murmur64_dict.reserve(reserve_dict);
	load_dictionary(save_path);
	m_dict_last_save_size = m_murmur64_dict.size();
	
	mem::pattern murmur64(murmur64_pattern);
	auto murmur64_loc = util::scan_main_module(murmur64);
	if (!murmur64_loc)
		return false;

	m_save_worker_thread = std::thread([this] { save_worker(); });

	//Due to compiler optimization on the game the murmur function was marked to have the standard volitile registers xmm1-5 - 
	//to be non-volitile, so we back them up and restore after our hook
	//https://learn.microsoft.com/en-us/cpp/build/x64-software-conventions?view=msvc-170
	auto murmur64_trampoline = util::generate_xmm_backup_call(&murmur64_hook);
	return util::patch(murmur64_loc, murmur64_trampoline);
}

patchray::hash_dumper::~hash_dumper() {
	m_quitting = true;
	m_cv.notify_all();
	if(m_save_worker_thread.joinable())
		m_save_worker_thread.join();
}