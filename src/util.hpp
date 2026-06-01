#pragma once
#include <string>

namespace util {

// Read all of stdin into a string (for long text like task summaries).
std::string read_stdin();

// Today's date as ISO "YYYY-MM-DD" (local time).
std::string today_iso();

// Trim leading/trailing whitespace.
std::string trim(const std::string& s);

}  // namespace util
