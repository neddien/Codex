#pragma once

#include "bitwise_enum.h"
#include "math.h"

namespace codex::util {
    namespace io {
        [[nodiscard]] inline std::string read_to_string(const char* filePath)
        {
            std::ifstream     fs(filePath);
            std::stringstream buffer;
            if (fs.is_open()) {
                buffer << fs.rdbuf();
                fs.close();
            }
            return buffer.str();
        }

        [[nodiscard]] CODEX_API std::vector<u8> compress_lz4(const void* src, const usize src_size) noexcept;

        CODEX_API bool decompress_lz4(const void* src, const usize compressed_size, void* const dst,
                                      const usize original_size) noexcept;
    }; // namespace io
    namespace str {
        [[nodiscard]] inline std::vector<std::string> split(const std::string& str, const char delim = ' ') noexcept
        {
            std::vector<std::string> xsplit;
            std::istringstream       iss{ str };
            std::string              token;

            while (std::getline(iss, token, delim)) {
                if (!token.empty())
                    xsplit.push_back(std::move(token));
            }

            return xsplit;
        }

        template <typename It>
            requires std::input_iterator<It> && std::convertible_to<std::iter_reference_t<It>, std::string_view>
        [[nodiscard]] inline std::string join(const It begin, const It end, const char delim = ' ') noexcept
        {
            std::string result;

            for (auto it = begin; it != end; ++it) {
                if (it != begin)
                    result += delim;
                result += std::string_view{ *it };
            }

            return result;
        }
    } // namespace str

    namespace chron {
        [[nodiscard]] inline u64 unix_epoch_now() noexcept
        {
            return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();
        }
        [[nodiscard]] inline u64 nt_to_unix(const u64 nttime) noexcept
        {
            return (nttime / 10000000ull) - 11644473600ull;
        }
    } // namespace chron

    namespace crypto {
        [[nodiscard]] inline u32 djb2_hash(const std::string_view str) noexcept
        {
            u32 hash = 5381;
            for (usize i = 0; i < str.length(); i++)
                hash = ((hash << 5) + hash) + str[i];
            return hash;
        }

        [[nodiscard]] constexpr usize fnv1a(const std::string_view str) noexcept
        {
            usize hash = 14695981039346656037ull;
            for (const char c : str) {
                hash ^= static_cast<usize>(c);
                hash *= 1099511628211ull;
            }
            return hash;
        }

        namespace detail {
            consteval u32 crc32_entry(u32 n) noexcept
            {
                for (int i = 0; i < 8; ++i)
                    n = (n >> 1) ^ (0xEDB88320u & -(n & 1u));
                return n;
            }
            template <usize... I>
            consteval std::array<u32, 256> make_crc32_table(std::index_sequence<I...>) noexcept
            {
                return { crc32_entry(static_cast<u32>(I))... };
            }
            inline constexpr auto crc32_table = make_crc32_table(std::make_index_sequence<256>{});
        } // namespace detail

        // Call with the default crc to start a new checksum, or pass a running value to continue.
        // Finalize with crc32_finalize() to get the standard CRC32 output.
        [[nodiscard]] inline u32 crc32_update(const void* data, const usize size, const u32 crc = 0xFFFFFFFFu) noexcept
        {
            u32        running = crc;
            const auto bytes   = static_cast<const u8*>(data);
            for (usize i = 0; i < size; ++i)
                running = (running >> 8) ^ detail::crc32_table[(running ^ bytes[i]) & 0xFF];
            return running;
        }

        [[nodiscard]] inline u32 crc32_finalize(const u32 running) noexcept
        {
            return running ^ 0xFFFFFFFFu;
        }

        // Convenience: compute CRC32 of a single contiguous buffer.
        [[nodiscard]] inline u32 crc32(const void* data, const usize size) noexcept
        {
            return crc32_finalize(crc32_update(data, size));
        }
    }; // namespace crypto
} // namespace codex::util
