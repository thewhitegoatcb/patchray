#pragma once
#include <cstdint>
#include <chrono>

namespace patchray {
	constexpr const char delimeter = '\n';
	constexpr const auto key_char_fitler = [](char c) { return c != delimeter && c != '\0'; };
	constexpr const char save_path[] = "dict.txt";
	constexpr const auto save_thread_delay = std::chrono::milliseconds(1000);
	constexpr const std::size_t reserve_dict = 10000;
}