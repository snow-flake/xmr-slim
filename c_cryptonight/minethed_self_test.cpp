//
// Created by Snow Flake on 1/23/18.
//

#include "minethed_self_test.h"
#include "c_cryptonight/cryptonight.hpp"
#include "c_cryptonight/cryptonight_aesni.hpp"
#include <iostream>
#include <array>


namespace minethed_self_test {

	typedef void (*cn_hash_fun)(const void*, size_t, void*, cryptonight_ctx*);
	typedef void (*cn_hash_fun_multi)(const void*, size_t, void*, cryptonight_ctx**);

	cryptonight_ctx * minethd_alloc_ctx() {
		alloc_msg msg = { 0 };
		cryptonight_ctx * ctx = cryptonight_alloc_ctx(1, 1, &msg);
		if (msg.warning != NULL) {
			std::cout << __FILE__ << ":" << __LINE__<< "MEMORY ALLOC FAILED: " << msg.warning << std::endl;
		}
		if (ctx == NULL) {
			ctx = cryptonight_alloc_ctx(0, 0, NULL);
		}
		return ctx;
	}


	static constexpr size_t MAX_N = 5;

	bool test_func_selector() {
		std::array<cryptonight_ctx *, MAX_N> ctx;
		ctx.fill(nullptr);

		for (int i = 0; i < ctx.size(); i++) {
			if ((ctx[i] = minethd_alloc_ctx()) == nullptr) {
				for (int j = 0; j < i; j++) {
					cryptonight_free_ctx(ctx[j]);
				}
				return false;
			}
		}

		const std::array<cn_hash_fun, 4> func_table = {
			cryptonight_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, false, false>,
			cryptonight_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, false, true>,
			cryptonight_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, true, false>,
			cryptonight_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, true, true>
		};

		bool bResult = true;

		std::array<unsigned char, 32 * MAX_N> out;
		for (int i = 0; i < func_table.size(); i++) {
			const auto hashf = func_table[i];
			hashf("This is a test", 14, &out[0], ctx[0]);
			bResult &= memcmp(&out[0], "\xa0\x84\xf0\x1d\x14\x37\xa0\x9c\x69\x85\x40\x1b\x60\xd4\x35\x54\xae\x10\x58\x02\xc5\xf5\xd8\xa9\xb3\x25\x36\x49\xc0\xbe\x66\x05", 32) == 0;
			if (!bResult) {
				std::cout << __FILE__ << ":" << __LINE__ << ": Failed self test on i=" << i << std::endl;
			} else {
				std::cout << __FILE__ << ":" << __LINE__ << ": Passed self test on i=" << i << std::endl;
			}
		}

		for (int i = 0; i < ctx.size(); i++) {
			cryptonight_free_ctx(ctx[i]);
		}

		if(!bResult) {
			std::cout << __FILE__ << ":" << __LINE__ << ": Cryptonight hash self-test failed. This might be caused by bad compiler optimizations." << std::endl;
		} else {
			std::cout << __FILE__ << ":" << __LINE__ << ": Cryptonight hash self-test passed" << std::endl;
		}

		return bResult;

	}



	bool test_func_multi_selector() {
		std::array<cryptonight_ctx *, MAX_N> ctx;
		ctx.fill(nullptr);

		for (int i = 0; i < ctx.size(); i++) {
			if ((ctx[i] = minethd_alloc_ctx()) == nullptr) {
				for (int j = 0; j < i; j++) {
					cryptonight_free_ctx(ctx[j]);
				}
				return false;
			}
		}

		const std::array<cn_hash_fun, 4> func_table_1x = {
				cryptonight_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, false, false>,
				cryptonight_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, false, true>,
				cryptonight_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, true, false>,
				cryptonight_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, true, true>
		};
		const std::array<cn_hash_fun_multi, 4> func_table_2x = {
				cryptonight_double_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, false, false>,
				cryptonight_double_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, false, true>,
				cryptonight_double_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, true, false>,
				cryptonight_double_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, true, true>,
		};
		const std::array<cn_hash_fun_multi, 4> func_table_3x = {
			cryptonight_triple_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, false, false>,
			cryptonight_triple_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, false, true>,
			cryptonight_triple_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, true, false>,
			cryptonight_triple_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, true, true>,
		};
		const std::array<cn_hash_fun_multi, 4> func_table_4x = {
			cryptonight_quad_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, false, false>,
			cryptonight_quad_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, false, true>,
			cryptonight_quad_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, true, false>,
			cryptonight_quad_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, true, true>,
		};
		const std::array<cn_hash_fun_multi, 4> func_table_5x = {
			cryptonight_penta_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, false, false>,
			cryptonight_penta_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, false, true>,
			cryptonight_penta_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, true, false>,
			cryptonight_penta_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, true, true>
		};

		bool bResult = true;

		std::array<unsigned char, 32 * MAX_N> out;
		for (int i = 0; i < 4; i++) {
			// 1x
			{
				const auto hashf = func_table_1x[i];
				hashf("This is a test", 14, &out[0], ctx[0]);
				bResult &= memcmp(&out[0], "\xa0\x84\xf0\x1d\x14\x37\xa0\x9c\x69\x85\x40\x1b\x60\xd4\x35\x54\xae\x10\x58\x02\xc5\xf5\xd8\xa9\xb3\x25\x36\x49\xc0\xbe\x66\x05", 32) == 0;
			}
			// 2x
			{
				unsigned char out[32 * MAX_N];
				const auto hashf_multi = func_table_2x[i];
				hashf_multi("The quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy log", 43, out, &ctx[0]);
				bResult &= memcmp(out, "\x3e\xbb\x7f\x9f\x7d\x27\x3d\x7c\x31\x8d\x86\x94\x77\x55\x0c\xc8\x00\xcf\xb1\x1b\x0c\xad\xb7\xff\xbd\xf6\xf8\x9f\x3a\x47\x1c\x59\xb4\x77\xd5\x02\xe4\xd8\x48\x7f\x42\xdf\xe3\x8e\xed\x73\x81\x7a\xda\x91\xb7\xe2\x63\xd2\x91\x71\xb6\x5c\x44\x3a\x01\x2a\x41\x22", 64) == 0;

				hashf_multi("The quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy log", 43, out, &ctx[0]);
				bResult &= memcmp(out, "\x3e\xbb\x7f\x9f\x7d\x27\x3d\x7c\x31\x8d\x86\x94\x77\x55\x0c\xc8\x00\xcf\xb1\x1b\x0c\xad\xb7\xff\xbd\xf6\xf8\x9f\x3a\x47\x1c\x59\xb4\x77\xd5\x02\xe4\xd8\x48\x7f\x42\xdf\xe3\x8e\xed\x73\x81\x7a\xda\x91\xb7\xe2\x63\xd2\x91\x71\xb6\x5c\x44\x3a\x01\x2a\x41\x22", 64) == 0;
			}
			// 3x
			{
				unsigned char out[32 * MAX_N];
				const auto hashf_multi = func_table_3x[i];
				hashf_multi("This is a testThis is a testThis is a test", 14, out, &ctx[0]);
				bResult &= memcmp(out, "\xa0\x84\xf0\x1d\x14\x37\xa0\x9c\x69\x85\x40\x1b\x60\xd4\x35\x54\xae\x10\x58\x02\xc5\xf5\xd8\xa9\xb3\x25\x36\x49\xc0\xbe\x66\x05\xa0\x84\xf0\x1d\x14\x37\xa0\x9c\x69\x85\x40\x1b\x60\xd4\x35\x54\xae\x10\x58\x02\xc5\xf5\xd8\xa9\xb3\x25\x36\x49\xc0\xbe\x66\x05\xa0\x84\xf0\x1d\x14\x37\xa0\x9c\x69\x85\x40\x1b\x60\xd4\x35\x54\xae\x10\x58\x02\xc5\xf5\xd8\xa9\xb3\x25\x36\x49\xc0\xbe\x66\x05", 96) == 0;
			}
			// 4x
			{
				unsigned char out[32 * MAX_N];
				const auto hashf_multi = func_table_4x[i];
				hashf_multi("This is a testThis is a testThis is a testThis is a test", 14, out, &ctx[0]);
				bResult &= memcmp(out, "\xa0\x84\xf0\x1d\x14\x37\xa0\x9c\x69\x85\x40\x1b\x60\xd4\x35\x54\xae\x10\x58\x02\xc5\xf5\xd8\xa9\xb3\x25\x36\x49\xc0\xbe\x66\x05\xa0\x84\xf0\x1d\x14\x37\xa0\x9c\x69\x85\x40\x1b\x60\xd4\x35\x54\xae\x10\x58\x02\xc5\xf5\xd8\xa9\xb3\x25\x36\x49\xc0\xbe\x66\x05\xa0\x84\xf0\x1d\x14\x37\xa0\x9c\x69\x85\x40\x1b\x60\xd4\x35\x54\xae\x10\x58\x02\xc5\xf5\xd8\xa9\xb3\x25\x36\x49\xc0\xbe\x66\x05\xa0\x84\xf0\x1d\x14\x37\xa0\x9c\x69\x85\x40\x1b\x60\xd4\x35\x54\xae\x10\x58\x02\xc5\xf5\xd8\xa9\xb3\x25\x36\x49\xc0\xbe\x66\x05", 128) == 0;
			}
			// 5x
			{
				unsigned char out[32 * MAX_N];
				const auto hashf_multi = func_table_5x[i];
				hashf_multi("This is a testThis is a testThis is a testThis is a testThis is a test", 14, out, &ctx[0]);
				bResult &= memcmp(out, "\xa0\x84\xf0\x1d\x14\x37\xa0\x9c\x69\x85\x40\x1b\x60\xd4\x35\x54\xae\x10\x58\x02\xc5\xf5\xd8\xa9\xb3\x25\x36\x49\xc0\xbe\x66\x05\xa0\x84\xf0\x1d\x14\x37\xa0\x9c\x69\x85\x40\x1b\x60\xd4\x35\x54\xae\x10\x58\x02\xc5\xf5\xd8\xa9\xb3\x25\x36\x49\xc0\xbe\x66\x05\xa0\x84\xf0\x1d\x14\x37\xa0\x9c\x69\x85\x40\x1b\x60\xd4\x35\x54\xae\x10\x58\x02\xc5\xf5\xd8\xa9\xb3\x25\x36\x49\xc0\xbe\x66\x05\xa0\x84\xf0\x1d\x14\x37\xa0\x9c\x69\x85\x40\x1b\x60\xd4\x35\x54\xae\x10\x58\x02\xc5\xf5\xd8\xa9\xb3\x25\x36\x49\xc0\xbe\x66\x05\xa0\x84\xf0\x1d\x14\x37\xa0\x9c\x69\x85\x40\x1b\x60\xd4\x35\x54\xae\x10\x58\x02\xc5\xf5\xd8\xa9\xb3\x25\x36\x49\xc0\xbe\x66\x05", 160) == 0;
			}

			if (!bResult) {
				std::cout << __FILE__ << ":" << __LINE__ << ": Failed self test on i=" << i << std::endl;
			} else {
				std::cout << __FILE__ << ":" << __LINE__ << ": Passed self test on i=" << i << std::endl;
			}
		}

		for (int i = 0; i < ctx.size(); i++) {
			cryptonight_free_ctx(ctx[i]);
		}

		if(!bResult) {
			std::cout << __FILE__ << ":" << __LINE__ << ": Cryptonight hash self-test failed. This might be caused by bad compiler optimizations." << std::endl;
		} else {
			std::cout << __FILE__ << ":" << __LINE__ << ": Cryptonight hash self-test passed" << std::endl;
		}

		return bResult;

	}
}
