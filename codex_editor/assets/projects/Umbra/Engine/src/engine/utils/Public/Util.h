#ifndef CODEX_UTIL_H
#define CODEX_UTIL_H

namespace codex::util {
    struct File
    {
        static std::string read_to_string(const char* filePath)
        {
            std::ifstream     fs(filePath);
            std::stringstream buffer;
            if (fs.is_open()) {
                buffer << fs.rdbuf();
                fs.close();
            }
            return buffer.str();
        }
    };

    struct Crypto
    {
        static u32 djb2_hash(const std::string_view str)
        {
            u32 hash = 5381;
            for (usize i = 0; i < str.length(); i++) {
                hash = ((hash << 5) + hash) + str[i];
            }
            return hash;
        }
    };

    struct U824Id
    {
    private:
        u32 id_ = 0;

    public:
        inline u32  get_id() { return id_ & 0x00ffffff; }
        inline u8   get_gen() { return id_ >> 24; }
        inline void set_id(const u32 id)
        {
            id_ &= 0xff000000;
            id_ |= id;
        }
        inline void set_gen(const u8 gen)
        {
            id_ &= 0x00ffffff;
            id_ |= (u32)(gen) << 24;
        }
    };
} // namespace codex::util

#endif // CODEX_UTIL_H
