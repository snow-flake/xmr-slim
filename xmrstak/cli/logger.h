//
// Created by Snow Flake on 1/20/18.
//

#ifndef XMR_STAK_LOGGER_H
#define XMR_STAK_LOGGER_H

#include <string>

namespace logger {
	void log_new_job(
		const std::string job_id,
		const std::string blob,
		const std::string target
	);

	void log_job_result(
			const std::string job_id,
			const std::string blob,
			const std::string target,
			const std::string nonce,
			const std::string result
	);
}

#endif //XMR_STAK_LOGGER_H
