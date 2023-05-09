#pragma once

#include "spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <iostream>
#include <time.h>
#include <chrono>

static inline int NowDateToInt()
{
	time_t now;
	time(&now);

	tm p;
#ifdef _WIN32
	localtime_s(&p, &now);
#else
	localtime_r(&now, &p);
#endif // _WIN32
	int now_date = (1900 + p.tm_year) * 10000 + (p.tm_mon + 1) * 100 + p.tm_mday;
	return now_date;
}

static inline int NowTimeToInt()
{
	time_t now;
	time(&now);
	// choose thread save version in each platform
	tm p;
#ifdef _WIN32
	localtime_s(&p, &now);
#else
	localtime_r(&now, &p);
#endif // _WIN32

	int now_int = p.tm_hour * 10000 + p.tm_min * 100 + p.tm_sec;
	return now_int;
}

class Log {
private:
	// 同步日志器
	std::shared_ptr<spdlog::logger> logger;
	// 异步日志器，使用线程池来异步写入日志
	std::shared_ptr<spdlog::logger> async_logger;

public:
	static Log* getInstance()
	{
		static Log mLogger;
		return &mLogger;
	}

	std::shared_ptr<spdlog::logger> getLogger()
	{
		return logger;
	}

	std::shared_ptr<spdlog::logger> getAsyncLogger()
	{
		return async_logger;
	}

	Log() {
		try
		{
			logger = spdlog::stdout_color_st("HBLogger");
			logger->set_level(spdlog::level::debug);
			
			int date = NowDateToInt();
			int time = NowTimeToInt();
			std::string logFilePath = "../../../log/" + std::to_string(date) + '-' + std::to_string(time) + ".txt";
			async_logger = spdlog::basic_logger_mt<spdlog::async_factory>("HBAsyncLogger", logFilePath);
			async_logger->set_level(spdlog::level::warn);
			async_logger->flush_on(spdlog::level::warn);
		}
		catch (const spdlog::spdlog_ex& ex)
		{
			std::cout << "Log initialization failed: " << ex.what() << std::endl;
		}
	}

	~Log()
	{
		spdlog::drop_all();
	}

	Log(const Log&) = delete;
	Log& operator=(const Log&) = delete;
};

#define LOG_DEBUG(...) SPDLOG_LOGGER_CALL(Log::getInstance()->getLogger().get(), spdlog::level::debug, __VA_ARGS__)
#define LOG_INFO(...) SPDLOG_LOGGER_CALL(Log::getInstance()->getLogger().get(), spdlog::level::info, __VA_ARGS__)
#define LOG_WARN(...) SPDLOG_LOGGER_CALL(Log::getInstance()->getLogger().get(), spdlog::level::warn, __VA_ARGS__);SPDLOG_LOGGER_CALL(Log::getInstance()->getAsyncLogger().get(), spdlog::level::warn, __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_CALL(Log::getInstance()->getLogger().get(), spdlog::level::err, __VA_ARGS__);SPDLOG_LOGGER_CALL(Log::getInstance()->getAsyncLogger().get(), spdlog::level::err, __VA_ARGS__)