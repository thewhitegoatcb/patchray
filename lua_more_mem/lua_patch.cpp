#include "lua_patch.hpp"
#include "utils.hpp"

#include <mem/pattern.h>


namespace patchray {
	bool patch_lua() {
		mem::pattern lua_memory("32 C9 B8 00 00 00 40");
		auto lua_memory_loc = util::scan_main_module(lua_memory);
		if (!lua_memory_loc)
			return false;

		util::byte_vector_t lua_memory_2gb_patch{ 
			0xB8, 0x00, 0x00, 0xFD, 0x7F  // mov eax, 0x7FFD0000
		}; 

		return util::patch(lua_memory_loc + 2, lua_memory_2gb_patch);
	}
}