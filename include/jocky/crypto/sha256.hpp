#pragma once
// Small dependency-free SHA-256 implementation used to bind registry metadata
// to the exact bytes embedded by jockyc.

#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace jocky {

namespace sha256_detail {

inline std::uint32_t rotate_right(std::uint32_t value, unsigned shift) {
    return (value >> shift) | (value << (32U - shift));
}

inline const std::array<std::uint32_t, 64>& round_constants() {
    static const std::array<std::uint32_t, 64> values = {
        0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
        0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
        0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
        0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
        0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
        0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
        0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
        0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
        0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
        0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
        0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
        0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
        0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
        0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
        0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
        0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U,
    };
    return values;
}

}  // namespace sha256_detail

inline std::string sha256_bytes(const std::string& input) {
    std::vector<std::uint8_t> data(input.begin(), input.end());
    const std::uint64_t bit_length =
        static_cast<std::uint64_t>(data.size()) * 8U;
    data.push_back(0x80U);
    while ((data.size() % 64U) != 56U) {
        data.push_back(0U);
    }
    for (int shift = 56; shift >= 0; shift -= 8) {
        data.push_back(static_cast<std::uint8_t>(bit_length >> shift));
    }

    std::array<std::uint32_t, 8> hash = {
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U,
    };
    const auto& constants = sha256_detail::round_constants();
    for (std::size_t offset = 0; offset < data.size(); offset += 64U) {
        std::array<std::uint32_t, 64> words{};
        for (std::size_t i = 0; i < 16; ++i) {
            const std::size_t at = offset + i * 4U;
            words[i] = (static_cast<std::uint32_t>(data[at]) << 24U) |
                       (static_cast<std::uint32_t>(data[at + 1]) << 16U) |
                       (static_cast<std::uint32_t>(data[at + 2]) << 8U) |
                       static_cast<std::uint32_t>(data[at + 3]);
        }
        for (std::size_t i = 16; i < words.size(); ++i) {
            const std::uint32_t s0 =
                sha256_detail::rotate_right(words[i - 15], 7) ^
                sha256_detail::rotate_right(words[i - 15], 18) ^
                (words[i - 15] >> 3U);
            const std::uint32_t s1 =
                sha256_detail::rotate_right(words[i - 2], 17) ^
                sha256_detail::rotate_right(words[i - 2], 19) ^
                (words[i - 2] >> 10U);
            words[i] = words[i - 16] + s0 + words[i - 7] + s1;
        }

        std::uint32_t a = hash[0], b = hash[1], c = hash[2], d = hash[3];
        std::uint32_t e = hash[4], f = hash[5], g = hash[6], h = hash[7];
        for (std::size_t i = 0; i < 64; ++i) {
            const std::uint32_t sum1 =
                sha256_detail::rotate_right(e, 6) ^
                sha256_detail::rotate_right(e, 11) ^
                sha256_detail::rotate_right(e, 25);
            const std::uint32_t choose = (e & f) ^ ((~e) & g);
            const std::uint32_t temp1 =
                h + sum1 + choose + constants[i] + words[i];
            const std::uint32_t sum0 =
                sha256_detail::rotate_right(a, 2) ^
                sha256_detail::rotate_right(a, 13) ^
                sha256_detail::rotate_right(a, 22);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = sum0 + majority;
            h = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }
        hash[0] += a; hash[1] += b; hash[2] += c; hash[3] += d;
        hash[4] += e; hash[5] += f; hash[6] += g; hash[7] += h;
    }

    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (std::uint32_t word : hash) {
        out << std::setw(8) << word;
    }
    return out.str();
}

inline std::string read_binary_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot open file '" + path + "'");
    }
    std::ostringstream out;
    out << in.rdbuf();
    if (!in.good() && !in.eof()) {
        throw std::runtime_error("cannot read file '" + path + "'");
    }
    return out.str();
}

inline std::string sha256_file(const std::string& path) {
    return sha256_bytes(read_binary_file(path));
}

}  // namespace jocky
