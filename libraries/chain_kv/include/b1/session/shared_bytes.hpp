#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <memory>
#include <string_view>
#include <vector>

#include <fc/crypto/base64.hpp>

#include <eosio/chain/exceptions.hpp>

namespace eosio::session {

// \brief An immutable type to represent a pointer and a length.
//
class shared_bytes {
 public:
   using iterator       = const char*;
   using const_iterator = const iterator;

 public:
   template <typename T>
   shared_bytes(const T* data, size_t size);
   shared_bytes(size_t size);
   shared_bytes(const shared_bytes& b) = default;
   shared_bytes(shared_bytes&& b)      = default;
   ~shared_bytes()                     = default;

   shared_bytes& operator=(const shared_bytes& b) = default;
   shared_bytes& operator=(shared_bytes&& b) = default;

   char*             data();
   const char* const data() const;
   size_t            size() const;

   char operator[](size_t index) const;

   bool operator==(const shared_bytes& other) const;
   bool operator!=(const shared_bytes& other) const;

   bool operator!() const;
        operator bool() const;

   shared_bytes operator++(int);

   bool operator<(const shared_bytes& other) const;
   bool operator<=(const shared_bytes& other) const;
   bool operator>(const shared_bytes& other) const;
   bool operator>=(const shared_bytes& other) const;

   iterator begin() const;
   iterator end() const;

   static const shared_bytes& invalid();

 private:
   shared_bytes() = default;

 private:
   std::shared_ptr<char> m_data;
   size_t                m_size{ 0 };
};

template <typename T>
shared_bytes::shared_bytes(const T* data, size_t size)
    : m_data{ [&]() {
         if (!data || size == 0) {
            return std::shared_ptr<char>{};
         }

         auto actual_size  = size * sizeof(T);
         auto aligned_size = ((actual_size / 8) + 1) * 8;
         auto result       = std::shared_ptr<char>{ new char[aligned_size], std::default_delete<char[]>() };
         std::memcpy(result.get(), reinterpret_cast<const char*>(data), actual_size);
         std::memset(result.get() + actual_size, 0, aligned_size - actual_size);
         return result;
      }() },
      m_size{ size * sizeof(T) } {}

inline shared_bytes::shared_bytes(size_t size)
    : m_data{ [&]() {
         if (size == 0) {
            return std::shared_ptr<char>{};
         }

         auto actual_size = ((size / 8) + 1) * 8;
         auto result      = std::shared_ptr<char>{ new char[actual_size], std::default_delete<char[]>() };
         std::memset(result.get(), 0, actual_size);
         return result;
      }() },
      m_size{ size } {}

inline const shared_bytes& shared_bytes::invalid() {
   static shared_bytes bad;
   return bad;
}

inline shared_bytes shared_bytes::operator++(int) {
   auto buffer = std::vector<char>{ std::begin(*this), std::end(*this) };

   while (!buffer.empty()) {
      if (++buffer.back()) {
         break;
      }
      buffer.pop_back();
   }

   return eosio::session::shared_bytes(buffer.data(), buffer.size());
}

inline char*             shared_bytes::data() { return m_data.get(); }
inline const char* const shared_bytes::data() const { return m_data.get(); }

inline size_t shared_bytes::size() const { return m_size; }

inline char shared_bytes::operator[](size_t index) const {
   EOS_ASSERT(m_data && index < size(), eosio::chain::chain_exception, "shared_bytes is invalid");
   return m_data.get()[index];
}

inline bool shared_bytes::operator==(const shared_bytes& other) const {
   if (m_data.get() == other.m_data.get()) {
      return true;
   }

   return size() == other.size() && std::memcmp(data(), other.data(), size()) == 0;
}

inline bool shared_bytes::operator!=(const shared_bytes& other) const { return !(*this == other); }

inline bool shared_bytes::operator<(const shared_bytes& other) const {
   if (m_data.get() == other.m_data.get()) {
      return false;
   }

   int32_t cmp = std::memcmp(data(), other.data(), std::min(size(), other.size()));
   return (cmp == 0 && size() < other.size()) || cmp < 0;
}

inline bool shared_bytes::operator<=(const shared_bytes& other) const {
   if (m_data.get() == other.m_data.get()) {
      return true;
   }

   int32_t cmp = std::memcmp(data(), other.data(), std::min(size(), other.size()));
   return cmp <= 0 && size() <= other.size();
}

inline bool shared_bytes::operator>(const shared_bytes& other) const {
   if (m_data.get() == other.m_data.get()) {
      return false;
   }

   int32_t cmp = std::memcmp(data(), other.data(), std::min(size(), other.size()));
   return (cmp == 0 && size() > other.size()) || cmp > 0;
}

inline bool shared_bytes::operator>=(const shared_bytes& other) const {
   if (m_data.get() == other.m_data.get()) {
      return true;
   }

   int32_t cmp = std::memcmp(data(), other.data(), std::min(size(), other.size()));
   return cmp >= 0 && size() >= other.size();
}

inline bool shared_bytes::operator!() const { return *this == shared_bytes::invalid(); }

inline shared_bytes::operator bool() const { return *this != shared_bytes::invalid(); }

inline shared_bytes::iterator shared_bytes::begin() const { return m_data.get(); }

inline shared_bytes::iterator shared_bytes::end() const { return m_data.get() + size(); }

inline std::ostream& operator<<(std::ostream& os, const shared_bytes& bytes) {
   os << fc::base64_encode({ bytes.data(), bytes.size() });
   return os;
}

} // namespace eosio::session

namespace std {

template <>
struct less<eosio::session::shared_bytes> {
   bool operator()(const eosio::session::shared_bytes& lhs, const eosio::session::shared_bytes& rhs) const {
      return lhs < rhs;
   };
};

template <>
struct greater<eosio::session::shared_bytes> {
   bool operator()(const eosio::session::shared_bytes& lhs, const eosio::session::shared_bytes& rhs) const {
      return lhs > rhs;
   };
};

template <>
struct hash<eosio::session::shared_bytes> {
   size_t operator()(const eosio::session::shared_bytes& b) const {
      return std::hash<std::string_view>{}({ reinterpret_cast<const char*>(b.data()), b.size() });
   }
};

template <>
struct equal_to<eosio::session::shared_bytes> {
   bool operator()(const eosio::session::shared_bytes& lhs, const eosio::session::shared_bytes& rhs) const {
      return lhs == rhs;
   }
};

} // namespace std
