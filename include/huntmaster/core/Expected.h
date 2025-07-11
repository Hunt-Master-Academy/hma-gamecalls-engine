#pragma once

#include "Platform.h"
#include <optional>
#include <variant>
#include <type_traits>

// Check if huntmaster::expected is available
#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
    #include <expected>
    namespace huntmaster {
        template<typename T, typename E>
        using expected = huntmaster::expected<T, E>;
        
        template<typename E>
        using unexpected = std::unexpected<E>;
    }
#else
    // Provide a simple alternative for platforms without huntmaster::expected
    #include <variant>
    #include <type_traits>
    #include <functional> // For std::monostate
    
    namespace huntmaster {
        template<typename E>
        class unexpected {
            E error_;
        public:
            explicit unexpected(E e) : error_(std::move(e)) {}
            const E& error() const & { return error_; }
            E& error() & { return error_; }
            E&& error() && { return std::move(error_); }
        };
        
        template<typename T, typename E>
        class expected {
            std::variant<T, E> data_;
            
        public:
            expected(T value) : data_(std::move(value)) {}
            expected(unexpected<E> unex) : data_(std::move(unex.error())) {}
            
            bool has_value() const { return std::holds_alternative<T>(data_); }
            explicit operator bool() const { return has_value(); }
            
            T& value() & { return std::get<T>(data_); }
            const T& value() const & { return std::get<T>(data_); }
            T&& value() && { return std::get<T>(std::move(data_)); }
            
            E& error() & { return std::get<E>(data_); }
            const E& error() const & { return std::get<E>(data_); }
            
            T* operator->() { return &std::get<T>(data_); }
            const T* operator->() const { return &std::get<T>(data_); }
            
            T& operator*() & { return std::get<T>(data_); }
            const T& operator*() const & { return std::get<T>(data_); }
        };
        
        // Specialization for void
        template <typename E>
        class expected<void, E>
        {
            std::variant<std::monostate, E> data_;
            
        public:
            expected() = default;
            expected(unexpected<E> unex) : data_(std::move(unex.error())) {}
            
            bool has_value() const { return std::holds_alternative<std::monostate>(data_); }
            explicit operator bool() const { return has_value(); }
            
            void value() const {
                if (!has_value()) throw std::runtime_error("Bad expected access");
            }
            
            E& error() & { return std::get<E>(data_); }
            const E& error() const & { return std::get<E>(data_); }
        };
    }
#endif