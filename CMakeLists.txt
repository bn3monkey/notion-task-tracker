cmake_minimum_required(VERSION 3.20)
project(ntt VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/utf-8 /W3)
else()
  add_compile_options(-Wall -Wextra)
endif()

include(FetchContent)

# Use URL (tarball) downloads instead of git clone so the build does not depend
# on a git binary being on PATH (the generated build.sh sanitizes PATH).
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)  # use download timestamp for extracted files
endif()

# ---- nlohmann/json (header-only) ----
FetchContent_Declare(nlohmann_json
  URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.tar.gz
)

# ---- CLI11 (header-only) ----
FetchContent_Declare(cli11
  URL https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.4.2.tar.gz
)

# ---- libcurl (built statically, no external deps) ----
set(BUILD_CURL_EXE     OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS  OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS  ON  CACHE BOOL "" FORCE)
set(CURL_DISABLE_TESTS ON  CACHE BOOL "" FORCE)
set(HTTP_ONLY          ON  CACHE BOOL "" FORCE)   # HTTP/HTTPS only
set(CURL_ZLIB          OFF CACHE STRING "" FORCE) # avoid zlib dependency
set(CURL_USE_LIBPSL    OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBSSH2   OFF CACHE BOOL "" FORCE)

# TLS backend: Schannel on Windows (no OpenSSL needed),
# Secure Transport on macOS, OpenSSL elsewhere.
if(WIN32)
  set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
elseif(APPLE)
  set(CURL_USE_SECTRANSP ON CACHE BOOL "" FORCE)
else()
  set(CURL_USE_OPENSSL ON CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(curl
  URL https://github.com/curl/curl/releases/download/curl-8_11_1/curl-8.11.1.tar.gz
)

FetchContent_MakeAvailable(nlohmann_json cli11 curl)

add_executable(ntt
  src/main.cpp
  src/config.cpp
  src/notion_client.cpp
  src/id_gen.cpp
  src/util.cpp
  src/commands/init.cpp
  src/commands/create.cpp
  src/commands/setup.cpp
  src/commands/check.cpp
  src/commands/start.cpp
  src/commands/resume.cpp
  src/commands/finish.cpp
  src/commands/search.cpp
  src/commands/guide.cpp
)

target_include_directories(ntt PRIVATE src)

target_link_libraries(ntt PRIVATE
  CURL::libcurl
  nlohmann_json::nlohmann_json
  CLI11::CLI11
)

# On Windows, libcurl + Schannel needs these system libs when linked statically.
if(WIN32)
  target_link_libraries(ntt PRIVATE ws2_32 crypt32 wldap32 normaliz bcrypt)
  target_compile_definitions(ntt PRIVATE CURL_STATICLIB)
endif()

install(TARGETS ntt RUNTIME DESTINATION bin)
