#ifndef NETLIB_NETCOMMON_H
#define NETLIB_NETCOMMON_H

#include <iostream>
#include <cstdint>
#include <cstring>
#include <vector>
#include <memory>
#include <deque>
#include <thread>
#include <future>
#include <random>

#ifdef _WIN32
#	define _WIN32_WINNT 0x0601
#endif
#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

#endif // NETLIB_NETCOMMON_H