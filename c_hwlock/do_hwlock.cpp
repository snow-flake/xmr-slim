//
// Created by Snow Flake on 12/30/17.
//

#include "do_hwlock.hpp"
#include "hwlocMemory.hpp"

void do_hwlock(size_t processing_unit_id) {
	bindMemoryToNUMANode(processing_unit_id);
}
