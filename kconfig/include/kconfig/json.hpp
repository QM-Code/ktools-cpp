#pragma once

#include <cstddef>
#include <initializer_list>
#include <iosfwd>
#include <iterator>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace kconfig::json {

class Value {
  public:
    using Object = std::map<std::string, Value>;
    using Array = std::vector<Value>;
    using object_iterator = Object::iterator;
    using const_object_iterator = Object::const_iterator;
    using array_iterator = Array::iterator;
    using const_array_iterator = Array::const_iterator;

    class Iterator {
      public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Value;
        using difference_type = std::ptrdiff_t;
        using pointer = Value *;
        using reference = Value &;

        Iterator();
        reference operator*() const;
        pointer operator->() const;
        Iterator &operator++();
        Iterator operator++(int);
        bool operator==(const Iterator &other) const;
        bool operator!=(const Iterator &other) const;
        const std::string &key() const;
        reference value() const;

      private:
        friend class Value;
        explicit Iterator(object_iterator iterator);
        explicit Iterator(array_iterator iterator);
        std::variant<std::monostate, object_iterator, array_iterator> iterator_;
    };

    class ConstIterator {
      public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const Value;
        using difference_type = std::ptrdiff_t;
        using pointer = const Value *;
        using reference = const Value &;

        ConstIterator();
        reference operator*() const;
        pointer operator->() const;
        ConstIterator &operator++();
        ConstIterator operator++(int);
        bool operator==(const ConstIterator &other) const;
        bool operator!=(const ConstIterator &other) const;
        const std::string &key() const;
        reference value() const;

      private:
        friend class Value;
        explicit ConstIterator(const_object_iterator iterator);
        explicit ConstIterator(const_array_iterator iterator);
        std::variant<std::monostate, const_object_iterator, const_array_iterator> iterator_;
    };

    class ItemsIterator {
      public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const std::string &, Value &>;
        using difference_type = std::ptrdiff_t;

        ItemsIterator();
        value_type operator*() const;
        ItemsIterator &operator++();
        ItemsIterator operator++(int);
        bool operator==(const ItemsIterator &other) const;
        bool operator!=(const ItemsIterator &other) const;

      private:
        friend class Value;
        friend class ItemsView;
        explicit ItemsIterator(object_iterator iterator);
        std::variant<std::monostate, object_iterator> iterator_;
    };

    class ConstItemsIterator {
      public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const std::string &, const Value &>;
        using difference_type = std::ptrdiff_t;

        ConstItemsIterator();
        value_type operator*() const;
        ConstItemsIterator &operator++();
        ConstItemsIterator operator++(int);
        bool operator==(const ConstItemsIterator &other) const;
        bool operator!=(const ConstItemsIterator &other) const;

      private:
        friend class Value;
        friend class ConstItemsView;
        explicit ConstItemsIterator(const_object_iterator iterator);
        std::variant<std::monostate, const_object_iterator> iterator_;
    };

    class ItemsView {
      public:
        ItemsView();
        ItemsIterator begin();
        ItemsIterator end();

      private:
        friend class Value;
        explicit ItemsView(Object *object);
        Object *object_;
    };

    class ConstItemsView {
      public:
        ConstItemsView();
        ConstItemsIterator begin() const;
        ConstItemsIterator end() const;

      private:
        friend class Value;
        explicit ConstItemsView(const Object *object);
        const Object *object_;
    };

    Value();
    Value(std::nullptr_t);
    Value(bool value);
    Value(int value);
    Value(long long value);
    Value(unsigned int value);
    Value(unsigned long long value);
    Value(double value);
    Value(const char *value);
    Value(std::string value);
    Value(const Object &value);
    Value(Object &&value);
    Value(const Array &value);
    Value(Array &&value);

    Value(const Value &other);
    Value(Value &&other) noexcept;
    Value &operator=(const Value &other);
    Value &operator=(Value &&other) noexcept;
    ~Value();

    Value &operator=(std::nullptr_t);
    Value &operator=(bool value);
    Value &operator=(int value);
    Value &operator=(long long value);
    Value &operator=(unsigned int value);
    Value &operator=(unsigned long long value);
    Value &operator=(double value);
    Value &operator=(const char *value);
    Value &operator=(std::string value);

    static Value parse(std::string_view text);
    static Value object();
    static Value array();

    bool is_null() const;
    bool is_object() const;
    bool is_array() const;
    bool is_string() const;
    bool is_boolean() const;
    bool is_number() const;
    bool is_number_integer() const;
    bool is_number_unsigned() const;
    bool is_number_float() const;

    std::size_t size() const;
    bool empty() const;
    bool contains(std::string_view key) const;

    Value &operator[](std::string_view key);
    const Value &operator[](std::string_view key) const;
    Value &operator[](std::size_t index);
    const Value &operator[](std::size_t index) const;

    Iterator find(std::string_view key);
    ConstIterator find(std::string_view key) const;

    void erase(std::string_view key);
    void push_back(const Value &value);
    void push_back(Value &&value);
    void push_back(std::nullptr_t);

    Iterator begin();
    Iterator end();
    ConstIterator begin() const;
    ConstIterator end() const;
    ConstIterator cbegin() const;
    ConstIterator cend() const;

    ItemsView items();
    ConstItemsView items() const;

    std::string dump(int indent = -1) const;

    template <typename T>
    T get() const;

  private:
    using Data = std::variant<std::nullptr_t, bool, long long, unsigned long long, double, std::string, Object, Array>;
    Data data_;
};

template <>
bool Value::get<bool>() const;

template <>
long long Value::get<long long>() const;

template <>
unsigned long long Value::get<unsigned long long>() const;

template <>
double Value::get<double>() const;

template <>
std::string Value::get<std::string>() const;

template <>
int Value::get<int>() const;

template <>
unsigned int Value::get<unsigned int>() const;

template <>
float Value::get<float>() const;

Value Parse(std::string_view text);
Value Object();
Value Array();

template <typename T>
Value Array(std::initializer_list<T> values) {
    Value result = Value::array();
    for (const auto &value : values) {
        result.push_back(Value(value));
    }
    return result;
}

std::string Dump(const Value &value, int indent = -1);

std::istream &operator>>(std::istream &input, Value &value);
std::ostream &operator<<(std::ostream &output, const Value &value);

} // namespace kconfig::json
