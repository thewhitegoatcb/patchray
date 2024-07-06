#pragma once
#include <mem/module.h>
#include <mem/pattern.h>
#include <mem/protect.h>

#include <type_traits>


namespace patchray::util{
	using byte_vector_t = std::vector<std::uint8_t>;
	using ba_t = std::initializer_list<std::uint8_t>;

	using patch_queue_t = std::vector<std::pair<mem::pointer, const byte_vector_t>>;

	template<typename T,
		typename = std::enable_if<std::is_trivially_copyable_v<T>>>
	inline byte_vector_t& operator<<(byte_vector_t& v, const T& val) {
		v.insert(v.end(),
			reinterpret_cast<const std::uint8_t*>(&val),
			reinterpret_cast<const std::uint8_t*>(&val) + sizeof(T));
		return v;
	}

	inline byte_vector_t& operator<<(byte_vector_t& v, byte_vector_t&& val) {
		v.insert(v.end(), val.cbegin(), val.cend());
		return v;
	}

	inline byte_vector_t& operator<<(byte_vector_t& v, ba_t&& val) {
		v.insert(v.end(), val.begin(), val.end());
		return v;
	}

	inline byte_vector_t generate_abs_jmp(mem::pointer target) {
		byte_vector_t abs_jmp {
			0x49, 0xBA, // mov r10, target
		};
		abs_jmp << target.as<void*>() << ba_t {
			0x41, 0xFF, 0xE2  // jmp r10
		};
		return abs_jmp;
	}

	inline byte_vector_t generate_abs_call(mem::pointer target) {
		byte_vector_t abs_call {
			0x48, 0xB8, // mov rax, target
		};
		abs_call << target.as<void*>() << ba_t {
			0xFF, 0xD0, // call rax
		};
		return abs_call;
	}

	inline byte_vector_t generate_xmm_backup_call(mem::pointer target) {
		//sub    rsp,0x68
		//movdqu XMMWORD PTR [rsp],xmm0
		//movdqu XMMWORD PTR [rsp+0x10],xmm1
		//movdqu XMMWORD PTR [rsp+0x20],xmm2
		//movdqu XMMWORD PTR [rsp+0x30],xmm3
		//movdqu XMMWORD PTR [rsp+0x40],xmm4
		//movdqu XMMWORD PTR [rsp+0x50],xmm5
		//movabs rax, target
		//call   rax
		//movdqu xmm5,XMMWORD PTR [rsp+0x50]
		//movdqu xmm4,XMMWORD PTR [rsp+0x40]
		//movdqu xmm3,XMMWORD PTR [rsp+0x30]
		//movdqu xmm2,XMMWORD PTR [rsp+0x20]
		//movdqu xmm1,XMMWORD PTR [rsp+0x10]
		//movdqu xmm0,XMMWORD PTR [rsp]
		//add    rsp,0x68
		//ret
		byte_vector_t xmm_backup_call {
			0x48, 0x83, 0xEC, 0x68,
			0xF3, 0x0F, 0x7F, 0x04, 0x24,
			0xF3, 0x0F, 0x7F, 0x4C, 0x24, 0x10,
			0xF3, 0x0F, 0x7F, 0x54, 0x24, 0x20,
			0xF3, 0x0F, 0x7F, 0x5C, 0x24, 0x30,
			0xF3, 0x0F, 0x7F, 0x64, 0x24, 0x40,
			0xF3, 0x0F, 0x7F, 0x6C, 0x24, 0x50,
		};
		xmm_backup_call << generate_abs_call(target) << ba_t {
			0xF3, 0x0F, 0x6F, 0x6C, 0x24, 0x50,
			0xF3, 0x0F, 0x6F, 0x64, 0x24, 0x40,
			0xF3, 0x0F, 0x6F, 0x5C, 0x24, 0x30,
			0xF3, 0x0F, 0x6F, 0x54, 0x24, 0x20,
			0xF3, 0x0F, 0x6F, 0x4C, 0x24, 0x10,
			0xF3, 0x0F, 0x6F, 0x04, 0x24, 0x48,
			0x83, 0xC4, 0x68,
			0xC3,
		};
		return xmm_backup_call;
	}

	inline void append_nop(byte_vector_t& patch, std::size_t count) {
		patch.insert(patch.end(), count, 0x90);
	}

	inline bool patch(mem::pointer address, const byte_vector_t& patch) {
		mem::prot_flags old_flags;

		if (!mem::protect_modify(address.as<void*>(), patch.size(), mem::prot_flags::RWX, &old_flags))
			return false;

		std::memcpy(address.as<void*>(), patch.data(), patch.size());
		mem::protect_modify(address.as<void*>(), patch.size(), old_flags);
		return true;
	}

	inline mem::pointer scan_module(mem::module& target, const mem::pattern& pattern) {
		mem::default_scanner scanner(pattern);
		mem::pointer found_address;
		
		target.enum_segments([&scanner, &found_address](mem::region range, mem::prot_flags prot) {
			if ((mem::prot_flags::RX & prot) != mem::prot_flags::RX)
				return false;

			if (found_address = scanner(range))
				return true;

			return false;
			});
		return found_address;
	}

	inline mem::pointer scan_main_module(const mem::pattern& pattern) {
		auto main = mem::module::main();

		return scan_module(main, pattern);
	}
}