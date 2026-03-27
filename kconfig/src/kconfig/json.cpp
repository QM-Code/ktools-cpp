#include <kconfig/json.hpp>

#include <array>
#include <charconv>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <istream>
#include <iterator>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {

using kconfig::json::Value;

[[noreturn]] void ThrowParseError(const std::size_t position, std::string_view message) {
    throw std::runtime_error("JSON parse error at offset "
                             + std::to_string(position)
                             + ": "
                             + std::string(message));
}

bool IsDigit(const char ch) {
    return std::isdigit(static_cast<unsigned char>(ch)) != 0;
}

void AppendCodepointAsUtf8(std::string& output, const std::uint32_t codepoint) {
    if (codepoint <= 0x7Fu) {
        output.push_back(static_cast<char>(codepoint));
        return;
    }
    if (codepoint <= 0x7FFu) {
        output.push_back(static_cast<char>(0xC0u | (codepoint >> 6)));
        output.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
        return;
    }
    if (codepoint <= 0xFFFFu) {
        output.push_back(static_cast<char>(0xE0u | (codepoint >> 12)));
        output.push_back(static_cast<char>(0x80u | ((codepoint >> 6) & 0x3Fu)));
        output.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
        return;
    }
    if (codepoint <= 0x10FFFFu) {
        output.push_back(static_cast<char>(0xF0u | (codepoint >> 18)));
        output.push_back(static_cast<char>(0x80u | ((codepoint >> 12) & 0x3Fu)));
        output.push_back(static_cast<char>(0x80u | ((codepoint >> 6) & 0x3Fu)));
        output.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
        return;
    }

    throw std::runtime_error("JSON parse error: invalid Unicode codepoint");
}

class JsonParser {
  public:
    explicit JsonParser(std::string_view text)
        : text_(text) {
    }

    Value parseDocument() {
        skipWhitespace();
        Value result = parseValue();
        skipWhitespace();
        if (!eof()) {
            error("unexpected trailing characters");
        }
        return result;
    }

  private:
    void skipWhitespace() {
        while (!eof() && std::isspace(static_cast<unsigned char>(text_[position_])) != 0) {
            ++position_;
        }
    }

    [[noreturn]] void error(std::string_view message) const {
        ThrowParseError(position_, message);
    }

    bool eof() const {
        return position_ >= text_.size();
    }

    char peek() const {
        if (eof()) {
            return '\0';
        }
        return text_[position_];
    }

    char consume() {
        if (eof()) {
            error("unexpected end of input");
        }
        return text_[position_++];
    }

    bool consumeIf(const char expected) {
        if (!eof() && text_[position_] == expected) {
            ++position_;
            return true;
        }
        return false;
    }

    void expect(const char expected, std::string_view message) {
        if (!consumeIf(expected)) {
            error(message);
        }
    }

    Value parseValue() {
        skipWhitespace();
        if (eof()) {
            error("unexpected end of input");
        }

        switch (peek()) {
        case 'n':
            parseLiteral("null");
            return Value(nullptr);
        case 't':
            parseLiteral("true");
            return Value(true);
        case 'f':
            parseLiteral("false");
            return Value(false);
        case '"':
            return Value(parseString());
        case '[':
            return parseArray();
        case '{':
            return parseObject();
        default:
            if (peek() == '-' || IsDigit(peek())) {
                return parseNumber();
            }
            error("unexpected token");
        }
    }

    void parseLiteral(std::string_view literal) {
        for (const char expected : literal) {
            if (consume() != expected) {
                error("invalid literal");
            }
        }
    }

    std::string parseString() {
        expect('"', "expected string");

        std::string result;
        while (true) {
            if (eof()) {
                error("unterminated string");
            }

            const char ch = consume();
            if (ch == '"') {
                return result;
            }
            if (static_cast<unsigned char>(ch) < 0x20u) {
                error("unescaped control character in string");
            }
            if (ch != '\\') {
                result.push_back(ch);
                continue;
            }

            if (eof()) {
                error("unterminated escape sequence");
            }

            switch (const char escaped = consume()) {
            case '"':
            case '\\':
            case '/':
                result.push_back(escaped);
                break;
            case 'b':
                result.push_back('\b');
                break;
            case 'f':
                result.push_back('\f');
                break;
            case 'n':
                result.push_back('\n');
                break;
            case 'r':
                result.push_back('\r');
                break;
            case 't':
                result.push_back('\t');
                break;
            case 'u':
                parseUnicodeEscape(result);
                break;
            default:
                error("invalid escape sequence");
            }
        }
    }

    std::uint32_t parseHexWord() {
        if (position_ + 4 > text_.size()) {
            error("incomplete unicode escape");
        }

        std::uint32_t value = 0;
        for (int i = 0; i < 4; ++i) {
            const char ch = text_[position_++];
            value <<= 4;
            if (ch >= '0' && ch <= '9') {
                value |= static_cast<std::uint32_t>(ch - '0');
            } else if (ch >= 'a' && ch <= 'f') {
                value |= static_cast<std::uint32_t>(10 + (ch - 'a'));
            } else if (ch >= 'A' && ch <= 'F') {
                value |= static_cast<std::uint32_t>(10 + (ch - 'A'));
            } else {
                error("invalid unicode escape");
            }
        }
        return value;
    }

    void parseUnicodeEscape(std::string& result) {
        const std::uint32_t first = parseHexWord();
        if (first >= 0xD800u && first <= 0xDBFFu) {
            if (position_ + 2 > text_.size() || text_[position_] != '\\' || text_[position_ + 1] != 'u') {
                error("expected low-surrogate unicode escape");
            }
            position_ += 2;
            const std::uint32_t second = parseHexWord();
            if (second < 0xDC00u || second > 0xDFFFu) {
                error("invalid low-surrogate unicode escape");
            }
            const std::uint32_t codepoint = 0x10000u + (((first - 0xD800u) << 10) | (second - 0xDC00u));
            AppendCodepointAsUtf8(result, codepoint);
            return;
        }

        if (first >= 0xDC00u && first <= 0xDFFFu) {
            error("unexpected low-surrogate unicode escape");
        }

        AppendCodepointAsUtf8(result, first);
    }

    Value parseArray() {
        expect('[', "expected array");
        skipWhitespace();

        Value result = Value::array();
        if (consumeIf(']')) {
            return result;
        }

        while (true) {
            result.push_back(parseValue());
            skipWhitespace();
            if (consumeIf(']')) {
                return result;
            }
            expect(',', "expected ',' or ']'");
        }
    }

    Value parseObject() {
        expect('{', "expected object");
        skipWhitespace();

        Value result = Value::object();
        if (consumeIf('}')) {
            return result;
        }

        while (true) {
            skipWhitespace();
            if (peek() != '"') {
                error("expected string key");
            }
            const std::string key = parseString();
            skipWhitespace();
            expect(':', "expected ':' after object key");
            result[key] = parseValue();
            skipWhitespace();
            if (consumeIf('}')) {
                return result;
            }
            expect(',', "expected ',' or '}'");
        }
    }

    Value parseNumber() {
        const std::size_t start = position_;
        const bool negative = consumeIf('-');

        if (eof()) {
            error("unexpected end of number");
        }

        if (consumeIf('0')) {
            if (!eof() && IsDigit(peek())) {
                error("leading zero in number");
            }
        } else {
            if (!IsDigit(peek())) {
                error("expected digit");
            }
            while (!eof() && IsDigit(peek())) {
                ++position_;
            }
        }

        bool isFloat = false;
        if (consumeIf('.')) {
            isFloat = true;
            if (eof() || !IsDigit(peek())) {
                error("expected digit after decimal point");
            }
            while (!eof() && IsDigit(peek())) {
                ++position_;
            }
        }

        if (!eof() && (peek() == 'e' || peek() == 'E')) {
            isFloat = true;
            ++position_;
            if (!eof() && (peek() == '+' || peek() == '-')) {
                ++position_;
            }
            if (eof() || !IsDigit(peek())) {
                error("expected exponent digits");
            }
            while (!eof() && IsDigit(peek())) {
                ++position_;
            }
        }

        const std::string_view token = text_.substr(start, position_ - start);
        if (isFloat) {
            double value = 0.0;
            const auto [ptr, ec] = std::from_chars(token.data(),
                                                   token.data() + token.size(),
                                                   value,
                                                   std::chars_format::general);
            if (ec != std::errc() || ptr != token.data() + token.size()) {
                error("invalid floating-point number");
            }
            return Value(value);
        }

        if (negative) {
            unsigned long long magnitude = 0;
            const std::string_view magnitudeToken = token.substr(1);
            const auto [ptr, ec] = std::from_chars(magnitudeToken.data(),
                                                   magnitudeToken.data() + magnitudeToken.size(),
                                                   magnitude);
            if (ec != std::errc() || ptr != magnitudeToken.data() + magnitudeToken.size()) {
                error("invalid integer");
            }

            constexpr unsigned long long kSignedMaxMagnitude =
                static_cast<unsigned long long>(std::numeric_limits<long long>::max());
            constexpr unsigned long long kSignedMinMagnitude = kSignedMaxMagnitude + 1ULL;
            if (magnitude == kSignedMinMagnitude) {
                return Value(std::numeric_limits<long long>::min());
            }
            if (magnitude > kSignedMaxMagnitude) {
                error("integer out of range");
            }
            return Value(-static_cast<long long>(magnitude));
        }

        unsigned long long value = 0;
        const auto [ptr, ec] = std::from_chars(token.data(),
                                               token.data() + token.size(),
                                               value);
        if (ec != std::errc() || ptr != token.data() + token.size()) {
            error("integer out of range");
        }
        return Value(value);
    }

    std::string_view text_;
    std::size_t position_ = 0;
};

void AppendEscapedString(std::string& output, std::string_view value) {
    output.push_back('"');
    static constexpr char kHexDigits[] = "0123456789ABCDEF";
    for (const unsigned char ch : value) {
        switch (ch) {
        case '"':
            output += "\\\"";
            break;
        case '\\':
            output += "\\\\";
            break;
        case '\b':
            output += "\\b";
            break;
        case '\f':
            output += "\\f";
            break;
        case '\n':
            output += "\\n";
            break;
        case '\r':
            output += "\\r";
            break;
        case '\t':
            output += "\\t";
            break;
        default:
            if (ch < 0x20u) {
                output += "\\u00";
                output.push_back(kHexDigits[(ch >> 4) & 0x0F]);
                output.push_back(kHexDigits[ch & 0x0F]);
            } else {
                output.push_back(static_cast<char>(ch));
            }
            break;
        }
    }
    output.push_back('"');
}

void AppendIndentation(std::string& output, const int depth, const int indentWidth) {
    output.push_back('\n');
    if (depth > 0 && indentWidth > 0) {
        output.append(static_cast<std::size_t>(depth * indentWidth), ' ');
    }
}

void AppendSerializedValue(std::string& output,
                           const Value& value,
                           const int indentWidth,
                           const int depth);

void AppendSerializedNumber(std::string& output, const Value& value) {
    std::array<char, 128> buffer{};
    if (value.is_number_unsigned()) {
        const auto number = value.get<unsigned long long>();
        const auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), number);
        if (ec != std::errc()) {
            throw std::runtime_error("failed to serialize JSON unsigned integer");
        }
        output.append(buffer.data(), ptr);
        return;
    }
    if (value.is_number_integer()) {
        const auto number = value.get<long long>();
        const auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), number);
        if (ec != std::errc()) {
            throw std::runtime_error("failed to serialize JSON integer");
        }
        output.append(buffer.data(), ptr);
        return;
    }

    const double number = value.get<double>();
    if (!std::isfinite(number)) {
        throw std::runtime_error("cannot serialize non-finite JSON float");
    }

    const auto [ptr, ec] = std::to_chars(buffer.data(),
                                         buffer.data() + buffer.size(),
                                         number,
                                         std::chars_format::general,
                                         std::numeric_limits<double>::max_digits10);
    if (ec != std::errc()) {
        throw std::runtime_error("failed to serialize JSON float");
    }

    output.append(buffer.data(), ptr);
    const auto writtenSize = static_cast<std::size_t>(ptr - buffer.data());
    if (output.find_first_of(".eE", output.size() - writtenSize) == std::string::npos) {
        output += ".0";
    }
}

void AppendSerializedArray(std::string& output,
                           const Value& value,
                           const int indentWidth,
                           const int depth) {
    output.push_back('[');
    if (value.empty()) {
        output.push_back(']');
        return;
    }

    const bool pretty = indentWidth >= 0;
    for (auto it = value.begin(); it != value.end();) {
        if (pretty) {
            AppendIndentation(output, depth + 1, indentWidth);
        }
        AppendSerializedValue(output, *it, indentWidth, depth + 1);
        ++it;
        if (it != value.end()) {
            output.push_back(',');
        }
    }

    if (pretty) {
        AppendIndentation(output, depth, indentWidth);
    }
    output.push_back(']');
}

void AppendSerializedObject(std::string& output,
                            const Value& value,
                            const int indentWidth,
                            const int depth) {
    output.push_back('{');
    if (value.empty()) {
        output.push_back('}');
        return;
    }

    const bool pretty = indentWidth >= 0;
    for (auto it = value.begin(); it != value.end();) {
        if (pretty) {
            AppendIndentation(output, depth + 1, indentWidth);
        }
        AppendEscapedString(output, it.key());
        output += pretty ? ": " : ":";
        AppendSerializedValue(output, *it, indentWidth, depth + 1);
        ++it;
        if (it != value.end()) {
            output.push_back(',');
        }
    }

    if (pretty) {
        AppendIndentation(output, depth, indentWidth);
    }
    output.push_back('}');
}

void AppendSerializedValue(std::string& output,
                           const Value& value,
                           const int indentWidth,
                           const int depth) {
    if (value.is_null()) {
        output += "null";
        return;
    }
    if (value.is_boolean()) {
        output += value.get<bool>() ? "true" : "false";
        return;
    }
    if (value.is_number()) {
        AppendSerializedNumber(output, value);
        return;
    }
    if (value.is_string()) {
        AppendEscapedString(output, value.get<std::string>());
        return;
    }
    if (value.is_array()) {
        AppendSerializedArray(output, value, indentWidth, depth);
        return;
    }
    if (value.is_object()) {
        AppendSerializedObject(output, value, indentWidth, depth);
        return;
    }

    throw std::runtime_error("unsupported JSON value type");
}

std::string SerializeJson(const Value& value, const int indentWidth) {
    std::string result;
    AppendSerializedValue(result, value, indentWidth, 0);
    return result;
}

} // namespace

namespace kconfig::json {

Value::Iterator::Iterator() = default;

Value::Iterator::Iterator(object_iterator iterator)
    : iterator_(iterator) {
}

Value::Iterator::Iterator(array_iterator iterator)
    : iterator_(iterator) {
}

Value::Iterator::reference Value::Iterator::operator*() const {
    if (const auto *objectIt = std::get_if<object_iterator>(&iterator_)) {
        return (*objectIt)->second;
    }
    if (const auto *arrayIt = std::get_if<array_iterator>(&iterator_)) {
        return **arrayIt;
    }
    throw std::logic_error("Invalid JSON iterator dereference");
}

Value::Iterator::pointer Value::Iterator::operator->() const {
    return &operator*();
}

Value::Iterator &Value::Iterator::operator++() {
    if (auto *objectIt = std::get_if<object_iterator>(&iterator_)) {
        ++(*objectIt);
    } else if (auto *arrayIt = std::get_if<array_iterator>(&iterator_)) {
        ++(*arrayIt);
    }
    return *this;
}

Value::Iterator Value::Iterator::operator++(int) {
    Iterator copy = *this;
    ++(*this);
    return copy;
}

bool Value::Iterator::operator==(const Iterator &other) const {
    return iterator_ == other.iterator_;
}

bool Value::Iterator::operator!=(const Iterator &other) const {
    return !(*this == other);
}

const std::string &Value::Iterator::key() const {
    if (const auto *objectIt = std::get_if<object_iterator>(&iterator_)) {
        return (*objectIt)->first;
    }
    throw std::logic_error("JSON key() is only valid for object iterators");
}

Value::Iterator::reference Value::Iterator::value() const {
    return operator*();
}

Value::ConstIterator::ConstIterator() = default;

Value::ConstIterator::ConstIterator(const_object_iterator iterator)
    : iterator_(iterator) {
}

Value::ConstIterator::ConstIterator(const_array_iterator iterator)
    : iterator_(iterator) {
}

Value::ConstIterator::reference Value::ConstIterator::operator*() const {
    if (const auto *objectIt = std::get_if<const_object_iterator>(&iterator_)) {
        return (*objectIt)->second;
    }
    if (const auto *arrayIt = std::get_if<const_array_iterator>(&iterator_)) {
        return **arrayIt;
    }
    throw std::logic_error("Invalid JSON iterator dereference");
}

Value::ConstIterator::pointer Value::ConstIterator::operator->() const {
    return &operator*();
}

Value::ConstIterator &Value::ConstIterator::operator++() {
    if (auto *objectIt = std::get_if<const_object_iterator>(&iterator_)) {
        ++(*objectIt);
    } else if (auto *arrayIt = std::get_if<const_array_iterator>(&iterator_)) {
        ++(*arrayIt);
    }
    return *this;
}

Value::ConstIterator Value::ConstIterator::operator++(int) {
    ConstIterator copy = *this;
    ++(*this);
    return copy;
}

bool Value::ConstIterator::operator==(const ConstIterator &other) const {
    return iterator_ == other.iterator_;
}

bool Value::ConstIterator::operator!=(const ConstIterator &other) const {
    return !(*this == other);
}

const std::string &Value::ConstIterator::key() const {
    if (const auto *objectIt = std::get_if<const_object_iterator>(&iterator_)) {
        return (*objectIt)->first;
    }
    throw std::logic_error("JSON key() is only valid for object iterators");
}

Value::ConstIterator::reference Value::ConstIterator::value() const {
    return operator*();
}

Value::ItemsIterator::ItemsIterator() = default;

Value::ItemsIterator::ItemsIterator(object_iterator iterator)
    : iterator_(iterator) {
}

Value::ItemsIterator::value_type Value::ItemsIterator::operator*() const {
    if (const auto *objectIt = std::get_if<object_iterator>(&iterator_)) {
        return {(*objectIt)->first, (*objectIt)->second};
    }
    throw std::logic_error("Invalid JSON items() iterator dereference");
}

Value::ItemsIterator &Value::ItemsIterator::operator++() {
    if (auto *objectIt = std::get_if<object_iterator>(&iterator_)) {
        ++(*objectIt);
    }
    return *this;
}

Value::ItemsIterator Value::ItemsIterator::operator++(int) {
    ItemsIterator copy = *this;
    ++(*this);
    return copy;
}

bool Value::ItemsIterator::operator==(const ItemsIterator &other) const {
    return iterator_ == other.iterator_;
}

bool Value::ItemsIterator::operator!=(const ItemsIterator &other) const {
    return !(*this == other);
}

Value::ConstItemsIterator::ConstItemsIterator() = default;

Value::ConstItemsIterator::ConstItemsIterator(const_object_iterator iterator)
    : iterator_(iterator) {
}

Value::ConstItemsIterator::value_type Value::ConstItemsIterator::operator*() const {
    if (const auto *objectIt = std::get_if<const_object_iterator>(&iterator_)) {
        return {(*objectIt)->first, (*objectIt)->second};
    }
    throw std::logic_error("Invalid JSON items() iterator dereference");
}

Value::ConstItemsIterator &Value::ConstItemsIterator::operator++() {
    if (auto *objectIt = std::get_if<const_object_iterator>(&iterator_)) {
        ++(*objectIt);
    }
    return *this;
}

Value::ConstItemsIterator Value::ConstItemsIterator::operator++(int) {
    ConstItemsIterator copy = *this;
    ++(*this);
    return copy;
}

bool Value::ConstItemsIterator::operator==(const ConstItemsIterator &other) const {
    return iterator_ == other.iterator_;
}

bool Value::ConstItemsIterator::operator!=(const ConstItemsIterator &other) const {
    return !(*this == other);
}

Value::ItemsView::ItemsView()
    : object_(nullptr) {
}

Value::ItemsView::ItemsView(Object *object)
    : object_(object) {
}

Value::ItemsIterator Value::ItemsView::begin() {
    if (!object_) {
        return ItemsIterator();
    }
    return ItemsIterator(object_->begin());
}

Value::ItemsIterator Value::ItemsView::end() {
    if (!object_) {
        return ItemsIterator();
    }
    return ItemsIterator(object_->end());
}

Value::ConstItemsView::ConstItemsView()
    : object_(nullptr) {
}

Value::ConstItemsView::ConstItemsView(const Object *object)
    : object_(object) {
}

Value::ConstItemsIterator Value::ConstItemsView::begin() const {
    if (!object_) {
        return ConstItemsIterator();
    }
    return ConstItemsIterator(object_->begin());
}

Value::ConstItemsIterator Value::ConstItemsView::end() const {
    if (!object_) {
        return ConstItemsIterator();
    }
    return ConstItemsIterator(object_->end());
}

Value::Value()
    : data_(nullptr) {
}

Value::Value(std::nullptr_t)
    : data_(nullptr) {
}

Value::Value(bool value)
    : data_(value) {
}

Value::Value(int value)
    : data_(static_cast<long long>(value)) {
}

Value::Value(long long value)
    : data_(value) {
}

Value::Value(unsigned int value)
    : data_(static_cast<unsigned long long>(value)) {
}

Value::Value(unsigned long long value)
    : data_(value) {
}

Value::Value(double value)
    : data_(value) {
}

Value::Value(const char *value)
    : data_(value ? std::string(value) : std::string()) {
}

Value::Value(std::string value)
    : data_(std::move(value)) {
}

Value::Value(const Object &value)
    : data_(value) {
}

Value::Value(Object &&value)
    : data_(std::move(value)) {
}

Value::Value(const Array &value)
    : data_(value) {
}

Value::Value(Array &&value)
    : data_(std::move(value)) {
}

Value::Value(const Value &other) = default;

Value::Value(Value &&other) noexcept = default;

Value &Value::operator=(const Value &other) = default;

Value &Value::operator=(Value &&other) noexcept = default;

Value::~Value() = default;

Value &Value::operator=(std::nullptr_t) {
    data_ = nullptr;
    return *this;
}

Value &Value::operator=(bool value) {
    data_ = value;
    return *this;
}

Value &Value::operator=(int value) {
    data_ = static_cast<long long>(value);
    return *this;
}

Value &Value::operator=(long long value) {
    data_ = value;
    return *this;
}

Value &Value::operator=(unsigned int value) {
    data_ = static_cast<unsigned long long>(value);
    return *this;
}

Value &Value::operator=(unsigned long long value) {
    data_ = value;
    return *this;
}

Value &Value::operator=(double value) {
    data_ = value;
    return *this;
}

Value &Value::operator=(const char *value) {
    data_ = value ? std::string(value) : std::string();
    return *this;
}

Value &Value::operator=(std::string value) {
    data_ = std::move(value);
    return *this;
}

Value Value::parse(std::string_view text) {
    JsonParser parser(text);
    return parser.parseDocument();
}

Value Value::object() {
    return Value(Object{});
}

Value Value::array() {
    return Value(Array{});
}

bool Value::is_null() const {
    return std::holds_alternative<std::nullptr_t>(data_);
}

bool Value::is_object() const {
    return std::holds_alternative<Object>(data_);
}

bool Value::is_array() const {
    return std::holds_alternative<Array>(data_);
}

bool Value::is_string() const {
    return std::holds_alternative<std::string>(data_);
}

bool Value::is_boolean() const {
    return std::holds_alternative<bool>(data_);
}

bool Value::is_number() const {
    return std::holds_alternative<long long>(data_)
        || std::holds_alternative<unsigned long long>(data_)
        || std::holds_alternative<double>(data_);
}

bool Value::is_number_integer() const {
    return std::holds_alternative<long long>(data_)
        || std::holds_alternative<unsigned long long>(data_);
}

bool Value::is_number_unsigned() const {
    return std::holds_alternative<unsigned long long>(data_);
}

bool Value::is_number_float() const {
    return std::holds_alternative<double>(data_);
}

std::size_t Value::size() const {
    if (const auto *object = std::get_if<Object>(&data_)) {
        return object->size();
    }
    if (const auto *array = std::get_if<Array>(&data_)) {
        return array->size();
    }
    if (std::holds_alternative<std::nullptr_t>(data_)) {
        return 0;
    }
    return 1;
}

bool Value::empty() const {
    if (const auto *object = std::get_if<Object>(&data_)) {
        return object->empty();
    }
    if (const auto *array = std::get_if<Array>(&data_)) {
        return array->empty();
    }
    if (const auto *string = std::get_if<std::string>(&data_)) {
        return string->empty();
    }
    return std::holds_alternative<std::nullptr_t>(data_);
}

bool Value::contains(std::string_view key) const {
    const auto *object = std::get_if<Object>(&data_);
    if (!object) {
        return false;
    }
    return object->find(std::string(key)) != object->end();
}

Value &Value::operator[](std::string_view key) {
    if (!is_object()) {
        data_ = Object{};
    }
    auto &object = std::get<Object>(data_);
    return object[std::string(key)];
}

const Value &Value::operator[](std::string_view key) const {
    const auto *object = std::get_if<Object>(&data_);
    if (!object) {
        throw std::out_of_range("JSON value is not an object");
    }
    const auto it = object->find(std::string(key));
    if (it == object->end()) {
        throw std::out_of_range("JSON object key not found");
    }
    return it->second;
}

Value &Value::operator[](std::size_t index) {
    if (!is_array()) {
        data_ = Array{};
    }
    auto &array = std::get<Array>(data_);
    if (index >= array.size()) {
        array.resize(index + 1);
    }
    return array[index];
}

const Value &Value::operator[](std::size_t index) const {
    const auto *array = std::get_if<Array>(&data_);
    if (!array) {
        throw std::out_of_range("JSON value is not an array");
    }
    if (index >= array->size()) {
        throw std::out_of_range("JSON array index out of range");
    }
    return (*array)[index];
}

Value::Iterator Value::find(std::string_view key) {
    if (!is_object()) {
        return end();
    }
    auto &object = std::get<Object>(data_);
    return Iterator(object.find(std::string(key)));
}

Value::ConstIterator Value::find(std::string_view key) const {
    if (!is_object()) {
        return end();
    }
    const auto &object = std::get<Object>(data_);
    return ConstIterator(object.find(std::string(key)));
}

void Value::erase(std::string_view key) {
    if (!is_object()) {
        return;
    }
    auto &object = std::get<Object>(data_);
    object.erase(std::string(key));
}

void Value::push_back(const Value &value) {
    if (!is_array()) {
        data_ = Array{};
    }
    auto &array = std::get<Array>(data_);
    array.push_back(value);
}

void Value::push_back(Value &&value) {
    if (!is_array()) {
        data_ = Array{};
    }
    auto &array = std::get<Array>(data_);
    array.push_back(std::move(value));
}

void Value::push_back(std::nullptr_t) {
    push_back(Value(nullptr));
}

Value::Iterator Value::begin() {
    if (is_object()) {
        return Iterator(std::get<Object>(data_).begin());
    }
    if (is_array()) {
        return Iterator(std::get<Array>(data_).begin());
    }
    return Iterator();
}

Value::Iterator Value::end() {
    if (is_object()) {
        return Iterator(std::get<Object>(data_).end());
    }
    if (is_array()) {
        return Iterator(std::get<Array>(data_).end());
    }
    return Iterator();
}

Value::ConstIterator Value::begin() const {
    if (is_object()) {
        return ConstIterator(std::get<Object>(data_).begin());
    }
    if (is_array()) {
        return ConstIterator(std::get<Array>(data_).begin());
    }
    return ConstIterator();
}

Value::ConstIterator Value::end() const {
    if (is_object()) {
        return ConstIterator(std::get<Object>(data_).end());
    }
    if (is_array()) {
        return ConstIterator(std::get<Array>(data_).end());
    }
    return ConstIterator();
}

Value::ConstIterator Value::cbegin() const {
    return begin();
}

Value::ConstIterator Value::cend() const {
    return end();
}

Value::ItemsView Value::items() {
    if (!is_object()) {
        return ItemsView(nullptr);
    }
    return ItemsView(&std::get<Object>(data_));
}

Value::ConstItemsView Value::items() const {
    if (!is_object()) {
        return ConstItemsView(nullptr);
    }
    return ConstItemsView(&std::get<Object>(data_));
}

std::string Value::dump(int indent) const {
    return SerializeJson(*this, indent);
}

template <>
bool Value::get<bool>() const {
    if (const auto *boolean = std::get_if<bool>(&data_)) {
        return *boolean;
    }
    throw std::runtime_error("JSON value is not a boolean");
}

template <>
long long Value::get<long long>() const {
    if (const auto *signedNumber = std::get_if<long long>(&data_)) {
        return *signedNumber;
    }
    if (const auto *unsignedNumber = std::get_if<unsigned long long>(&data_)) {
        if (*unsignedNumber <= static_cast<unsigned long long>(std::numeric_limits<long long>::max())) {
            return static_cast<long long>(*unsignedNumber);
        }
        throw std::runtime_error("JSON unsigned number cannot fit in long long");
    }
    if (const auto *floating = std::get_if<double>(&data_)) {
        if (std::isfinite(*floating)
            && std::trunc(*floating) == *floating
            && *floating >= static_cast<double>(std::numeric_limits<long long>::min())
            && *floating <= static_cast<double>(std::numeric_limits<long long>::max())) {
            return static_cast<long long>(*floating);
        }
        throw std::runtime_error("JSON floating number cannot fit in long long");
    }
    throw std::runtime_error("JSON value is not an integer number");
}

template <>
unsigned long long Value::get<unsigned long long>() const {
    if (const auto *unsignedNumber = std::get_if<unsigned long long>(&data_)) {
        return *unsignedNumber;
    }
    if (const auto *signedNumber = std::get_if<long long>(&data_)) {
        if (*signedNumber >= 0) {
            return static_cast<unsigned long long>(*signedNumber);
        }
        throw std::runtime_error("Negative JSON number cannot fit in unsigned long long");
    }
    if (const auto *floating = std::get_if<double>(&data_)) {
        if (std::isfinite(*floating)
            && std::trunc(*floating) == *floating
            && *floating >= 0.0
            && *floating <= static_cast<double>(std::numeric_limits<unsigned long long>::max())) {
            return static_cast<unsigned long long>(*floating);
        }
        throw std::runtime_error("JSON floating number cannot fit in unsigned long long");
    }
    throw std::runtime_error("JSON value is not an unsigned integer number");
}

template <>
double Value::get<double>() const {
    if (const auto *floating = std::get_if<double>(&data_)) {
        return *floating;
    }
    if (const auto *signedNumber = std::get_if<long long>(&data_)) {
        return static_cast<double>(*signedNumber);
    }
    if (const auto *unsignedNumber = std::get_if<unsigned long long>(&data_)) {
        return static_cast<double>(*unsignedNumber);
    }
    throw std::runtime_error("JSON value is not a numeric value");
}

template <>
std::string Value::get<std::string>() const {
    if (const auto *string = std::get_if<std::string>(&data_)) {
        return *string;
    }
    throw std::runtime_error("JSON value is not a string");
}

template <>
int Value::get<int>() const {
    const long long value = get<long long>();
    if (value < static_cast<long long>(std::numeric_limits<int>::min())
        || value > static_cast<long long>(std::numeric_limits<int>::max())) {
        throw std::runtime_error("JSON number cannot fit in int");
    }
    return static_cast<int>(value);
}

template <>
unsigned int Value::get<unsigned int>() const {
    const unsigned long long value = get<unsigned long long>();
    if (value > static_cast<unsigned long long>(std::numeric_limits<unsigned int>::max())) {
        throw std::runtime_error("JSON number cannot fit in unsigned int");
    }
    return static_cast<unsigned int>(value);
}

template <>
float Value::get<float>() const {
    const double value = get<double>();
    if (!std::isfinite(value)
        || value < static_cast<double>(-std::numeric_limits<float>::max())
        || value > static_cast<double>(std::numeric_limits<float>::max())) {
        throw std::runtime_error("JSON number cannot fit in float");
    }
    return static_cast<float>(value);
}

Value Parse(std::string_view text) {
    return Value::parse(text);
}

Value Object() {
    return Value::object();
}

Value Array() {
    return Value::array();
}

std::string Dump(const Value &value, int indent) {
    return value.dump(indent);
}

std::istream &operator>>(std::istream &input, Value &value) {
    std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    value = Value::parse(text);
    return input;
}

std::ostream &operator<<(std::ostream &output, const Value &value) {
    output << value.dump();
    return output;
}

} // namespace kconfig::json
