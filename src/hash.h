#include <cstdint>

typedef std::uint64_t hash_t;

constexpr hash_t prime = 0x100000001B3ull;
constexpr hash_t basis = 0xCBF29CE484222325ull;

constexpr hash_t constHash(char const* str, hash_t last_value = basis)
{
	return *str ? constHash(str+1, (*str ^ last_value) * prime) : last_value;
}