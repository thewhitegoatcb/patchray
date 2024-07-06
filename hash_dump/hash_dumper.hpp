#pragma once
#include "config.hpp"

#include <cstdint>
#include <unordered_map>
#include <string>
#include <thread>
#include <fstream>

#include <mutex>
#include <condition_variable>

namespace patchray {
	using murmur64_t = uint64_t;
	using murmur32_t = uint32_t;

	class hash_dumper {
	public:
		hash_dumper(const hash_dumper&) = delete;
		void operator=(const hash_dumper&) = delete;
		
		bool init() noexcept;

		static __declspec(dllexport) const char* lookup_murmur(std::uint64_t hash) {
			auto& dumper = get_instance();
			auto key = dumper.lookup(hash);
			return key.data();
		}

		static hash_dumper& get_instance() {
			static hash_dumper instance;
			return instance;
		}
	private:
		hash_dumper() = default;
		~hash_dumper();

		bool save_dictionary(const std::string& save_name) const noexcept;
		bool load_dictionary(const std::string& save_name) noexcept;
		std::string_view lookup(murmur64_t hash) const noexcept;

		void save_worker() noexcept;
		void try_emplace_dict(std::string_view key, murmur64_t hash) noexcept;

		static uint64_t __fastcall murmur64_hook(const char* key, int len, uint64_t seed) noexcept;
		static bool is_valid_key(std::string_view key, int len, uint64_t seed) noexcept;


		std::unordered_map<patchray::murmur64_t, std::string> m_murmur64_dict;
		std::size_t m_dict_last_save_size = 0;
		std::condition_variable m_cv;
		std::thread m_save_worker_thread;
		bool m_quitting = false;

		mutable std::mutex m_dict_mutex;
	};
}
