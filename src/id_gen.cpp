#include "id_gen.hpp"

#include <random>

std::string generate_id() {
    static const char charset[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789";
    constexpr int kLen = 6;
    constexpr int kN = sizeof(charset) - 1;  // exclude null terminator

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, kN - 1);

    std::string id;
    id.reserve(kLen);
    for (int i = 0; i < kLen; ++i) id.push_back(charset[dist(gen)]);
    return id;
}
