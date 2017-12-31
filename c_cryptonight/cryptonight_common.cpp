/*
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  *
  * Additional permission under GNU GPL version 3 section 7
  *
  * If you modify this Program, or any covered work, by linking or combining
  * it with OpenSSL (or a modified version of that library), containing parts
  * covered by the terms of OpenSSL License and SSLeay License, the licensors
  * of this Program grant you additional permission to convey the resulting work.
  *
  */

#include "cryptonight.hpp"
#include "cryptonight_aesni.hpp"
#include "xmrstak/jconf.hpp"
#include <stdio.h>
#include <stdlib.h>

#ifdef __GNUC__
#include <mm_malloc.h>
#else
#include <malloc.h>
#endif // __GNUC__

#if defined(__APPLE__)
#include <mach/vm_statistics.h>
#endif

#include <sys/mman.h>
#include <errno.h>
#include <string.h>

#include <cassert>

#include "c_blake/do_blake_hash.hpp"
#include "c_skein/do_skein_hash.hpp"
#include "c_groestl/do_groestl_hash.hpp"
#include "c_jh/do_jh_hash.hpp"

void (* const extra_hashes[4])(const uint8_t *, size_t, uint8_t *) = {do_blake_hash, do_groestl_hash, do_jh_hash, do_skein_hash};

size_t cryptonight_init(size_t use_fast_mem, size_t use_mlock, alloc_msg* msg)
{
	return 1;
}

cryptonight_ctx* cryptonight_alloc_ctx(size_t use_fast_mem, size_t use_mlock, alloc_msg* msg)
{
	const size_t hashMemSize = MONERO_MEMORY;
	cryptonight_ctx* ptr = (cryptonight_ctx*)_mm_malloc(sizeof(cryptonight_ctx), 4096);

	if(use_fast_mem == 0)
	{
		// use 2MiB aligned memory
		ptr->long_state = (uint8_t*)_mm_malloc(hashMemSize, hashMemSize);
		ptr->ctx_info[0] = 0;
		ptr->ctx_info[1] = 0;
		return ptr;
	}

#if defined(__APPLE__)
	ptr->long_state  = (uint8_t*)mmap(0, hashMemSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, VM_FLAGS_SUPERPAGE_SIZE_2MB, 0);
#elif defined(__FreeBSD__)
	ptr->long_state = (uint8_t*)mmap(0, hashMemSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_ALIGNED_SUPER | MAP_PREFAULT_READ, -1, 0);
#else
	ptr->long_state = (uint8_t*)mmap(0, hashMemSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_POPULATE, 0, 0);
#endif

	if (ptr->long_state == MAP_FAILED)
	{
		_mm_free(ptr);
		msg->warning = "mmap failed";
		return NULL;
	}

	ptr->ctx_info[0] = 1;

	if(madvise(ptr->long_state, hashMemSize, MADV_RANDOM|MADV_WILLNEED) != 0)
		msg->warning = "madvise failed";

	ptr->ctx_info[1] = 0;
	if(use_mlock != 0 && mlock(ptr->long_state, hashMemSize) != 0)
		msg->warning = "mlock failed";
	else
		ptr->ctx_info[1] = 1;

	return ptr;
}

void cryptonight_free_ctx(cryptonight_ctx* ctx)
{
	const size_t hashMemSize = MONERO_MEMORY;

	if(ctx->ctx_info[0] != 0)
	{
		if(ctx->ctx_info[1] != 0)
			munlock(ctx->long_state, hashMemSize);
		munmap(ctx->long_state, hashMemSize);
	}
	else
		_mm_free(ctx->long_state);

	_mm_free(ctx);
}
