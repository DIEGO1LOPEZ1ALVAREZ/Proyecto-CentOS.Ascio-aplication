#pragma once

// Agregando las librerias que se van a utilizar en esta librer�a estatica
#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <optional>
#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <stdio.h>
#include <inttypes.h>
#include <string>
#include <functional>
#include <random>

//* Agregando y definiendo librer�as y par�metros para usar la librer�a asio. *//
#define ASIO_STANDALONE

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
//*****************************************************************************//