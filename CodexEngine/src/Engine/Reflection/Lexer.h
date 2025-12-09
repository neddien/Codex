#ifndef CODEX_REFLECTION_LEXER_H
#define CODEX_REFLECTION_LEXER_H

#include <sdafx.h>

namespace codex::rf {
    enum class TokenType : u32
    {
        None,

        // Identifier
        Identifier,

        // Literals
        NumberLiteral,
        StringLiteral,
        CharacterLiteral,

        // Operators
        Colon,
        DoubleColon,
        SemiColon,
        Equals,
        LeftBrace,
        RightBrace,
        LeftCurlyBrace,
        RightCurlyBrace,
        Plus,
        Minus,
        Asterisk,
        ForwardSlash,
        LeftAngleBracket,
        RightAngleBracket,
        LeftSquareBracket,
        RightSquareBracket,
        DoubleQuote,
        Quote,
        Comma,
        Exclamation,
        Bar,
        Pound,
        Dot,
        Tilda,
        // Conditionals
        EqualsEquals,
        GreaterThanEquals,
        LesserThanEquals,
        ExclamationEquals,
        AmpersasndAmpersand,
        BarBar,
        // Unary
        Increment,
        Decrement,
        // Shorthand
        PlusEquals,
        MinusEquals,
        AsteriskEquals,
        ForwardSlashEquals,

        // Keywords
        KeywordChar,
        KeywordBool,
        KeywordShort,
        KeywordInt,
        KeywordFloat,
        KeywordLong,
        KeywordDouble,
        KeywordConst,
        KeywordConstexpr,
        KeywordVolatile,
        KeywordUnsigned,
        KeywordSigned,
        KeywordImport,
        KeywordExport,
        KeywordIf,
        KeywordElse,
        KeywordElseIf,
        KeywordWhile,
        KeywordReturn,
        KeywordTrue,
        KeywordFalse,
        KeywordPublic,
        KeywordPrivate,
        KeywordFinal,
        KeywordClass,
        KeywordStruct,
        KeywordNamespace,
        KeywordUsing,
        KeywordInline,
        KeywordStatic,

        // Comments
        LineComment,
        BlockComment,

        // Eof
        Eof
    };

    std::string_view TokenTypeToString(const TokenType type) noexcept;

    struct TextSpan
    {
        usize       line{};
        usize       cur{};
        std::string text{};
    };

    struct Token
    {
    public:
        TokenType type = TokenType::None;
        TextSpan  span{};
        i64       num{};

    public:
        // Few handy methods to make parsing easier.
        bool IsValid() const noexcept { return type != TokenType::None && type != TokenType::Eof; }
        bool IsOperator() const noexcept
        {
            return ((u32)type >= (u32)TokenType::Colon && (u32)type <= (u32)TokenType::ForwardSlashEquals);
        }
        bool IsKeyword() const noexcept
        {
            return ((u32)type >= (u32)TokenType::KeywordChar && (u32)type <= (u32)TokenType::KeywordStatic);
        }
        std::string_view ToString() const noexcept { return TokenTypeToString(type); }
    };

    using TokenList = std::vector<Token>;

    class CODEX_API Lexer
    {
    private:
        std::string_view m_Source{};
        usize            m_CurrentPos{};
        usize            m_TokenCount{};
        usize            m_LineCount = 1;

    public:
        Lexer() = default;
        explicit Lexer(const std::string_view source);

    public:
        std::optional<Token> NextToken();
        std::optional<Token> PeekToken();

    private:
        std::optional<char>        CurrentChar() const noexcept;
        std::optional<char>        Consume() noexcept;
        std::optional<i64>         ConsumeNumber() noexcept;
        std::string                ConsumeIdentifier() noexcept;
        TokenType                  ConsumeOperator() noexcept;
        std::optional<std::string> ConsumeString() noexcept;
        std::optional<std::string> ConsumeLineComment() noexcept;
        std::optional<std::string> ConsumeBlockComment() noexcept;
        bool                       IsIdentifierStart(const char c) const noexcept;
    };

} // namespace codex::rf

/*
namespace nlohmann {
    template <>
    struct adl_serializer<codex::rf::TokenType>
    {
        static void to_json(ordered_json& j, const codex::rf::TokenType& e)
        {
            switch (e)
            {
                using enum codex::rf::TokenType;

                case None: j = "None"; break;
                case Identifier: j = "Identifier"; break;
                case NumberLiteral: j = "NumberLiteral"; break;
                case StringLiteral: j = "StringLiteral"; break;
                case CharacterLiteral: j = "CharacterLiteral"; break;
                case Colon: j = "Colon"; break;
                case DoubleColon: j = "DoubleColon"; break;
                case SemiColon: j = "SemiColon"; break;
                case Equals: j = "Equals"; break;
                case LeftBrace: j = "LeftBrace"; break;
                case RightBrace: j = "RightBrace"; break;
                case LeftCurlyBrace: j = "LeftCurlyBrace"; break;
                case RightCurlyBrace: j = "RightCurlyBrace"; break;
                case Plus: j = "Plus"; break;
                case Minus: j = "Minus"; break;
                case Asterisk: j = "Asterisk"; break;
                case ForwardSlash: j = "ForwardSlash"; break;
                case LeftAngleBracket: j = "LeftAngleBracket"; break;
                case RightAngleBracket: j = "RightAngleBracket"; break;
                case LeftSquareBracket: j = "LeftSquareBracket"; break;
                case RightSquareBracket: j = "RightSquareBracket"; break;
                case DoubleQuote: j = "DoubleQuote"; break;
                case Quote: j = "Quote"; break;
                case Comma: j = "Comma"; break;
                case Exclamation: j = "Exclamation"; break;
                case Bar: j = "Bar"; break;
                case Pound: j = "Pound"; break;
                case Dot: j = "Dot"; break;
                case EqualsEquals: j = "EqualsEquals"; break;
                case GreaterThanEquals: j = "GreaterThanEquals"; break;
                case LesserThanEquals: j = "LesserThanEquals"; break;
                case ExclamationEquals: j = "ExclamationEquals"; break;
                case AmpersasndAmpersand: j = "AmpersandAmpersand"; break;
                case BarBar: j = "BarBar"; break;
                case Increment: j = "Increment"; break;
                case Decrement: j = "Decrement"; break;
                case PlusEquals: j = "PlusEquals"; break;
                case MinusEquals: j = "MinusEquals"; break;
                case AsteriskEquals: j = "AsteriskEquals"; break;
                case ForwardSlashEquals: j = "ForwardSlashEquals"; break;
                case KeywordChar: j = "KeywordChar"; break;
                case KeywordBool: j = "KeywordBool"; break;
                case KeywordShort: j = "KeywordShort"; break;
                case KeywordInt: j = "KeywordInt"; break;
                case KeywordFloat: j = "KeywordFloat"; break;
                case KeywordLong: j = "KeywordLong"; break;
                case KeywordDouble: j = "KeywordDouble"; break;
                case KeywordConst: j = "KeywordConst"; break;
                case KeywordConstexpr: j = "KeywordConstexpr"; break;
                case KeywordVolatile: j = "KeywordVolatile"; break;
                case KeywordUnsigned: j = "KeywordUnsigned"; break;
                case KeywordSigned: j = "KeywordSigned"; break;
                case KeywordImport: j = "KeywordImport"; break;
                case KeywordExport: j = "KeywordExport"; break;
                case KeywordIf: j = "KeywordIf"; break;
                case KeywordElse: j = "KeywordElse"; break;
                case KeywordElseIf: j = "KeywordElseIf"; break;
                case KeywordWhile: j = "KeywordWhile"; break;
                case KeywordReturn: j = "KeywordReturn"; break;
                case KeywordTrue: j = "KeywordTrue"; break;
                case KeywordFalse: j = "KeywordFalse"; break;
                case KeywordPublic: j = "KeywordPublic"; break;
                case KeywordPrivate: j = "KeywordPrivate"; break;
                case KeywordFinal: j = "KeywordFinal"; break;
                case KeywordClass: j = "KeywordClass"; break;
                case KeywordStruct: j = "KeywordStruct"; break;
                case KeywordNamespace: j = "KeywordNamespace"; break;
                case KeywordUsing: j = "KeywordUsing"; break;
                case KeywordInline: j = "KeywordInline"; break;
                case KeywordStatic: j = "KeywordStatic"; break;
                case LineComment: j = "LineComment"; break;
                case BlockComment: j = "BlockComment"; break;
                case Eof: j = "Eof"; break;
                default: j = "Unknown"; break;
            }
        }

        // Do not need a from_json() for now.
    };

    template <>
    struct adl_serializer<codex::rf::TextSpan>
    {
        static void to_json(ordered_json& j, const codex::rf::TextSpan& t)
        {
            j["line"] = t.line;
            j["cur"]  = t.cur;
            j["text"] = t.text;
        }
    };

    template <>
    struct adl_serializer<codex::rf::Token>
    {
        static void to_json(ordered_json& j, const codex::rf::Token& t)
        {
            j["type"] = t.type;
            j["span"] = t.span;
            j["num"]  = t.num;
        }
    };

} // namespace nlohmann
*/

std::ostream& operator<<(std::ostream& stream, const codex::rf::TextSpan& span) noexcept;
std::ostream& operator<<(std::ostream& stream, const codex::rf::Token& token) noexcept;

#endif // CODEX_REFLECTION_LEXER_H
