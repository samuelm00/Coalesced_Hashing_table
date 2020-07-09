#ifndef COALESCEDHASHTABLE_H
#define COALESCEDHASHTABLE_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

//Coalesced Hasing implementation

template <typename Key, size_t N = 21>
class HashTable {
public:
  class Iterator;
  using value_type = Key;
  using key_type = Key;
  using reference = key_type &;
  using const_reference = const key_type &;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using iterator = Iterator;
  using const_iterator = Iterator;
  using key_equal = std::equal_to<key_type>;
  using hasher = std::hash<key_type>;

private:
  enum class Mode {free, used, end};
  struct element {
    key_type key;
    Mode mode {Mode::free};
    size_type next_ {0};
  };

  element *table {nullptr};
  size_type table_size {0}, keller{0}, last_free_element {0}, curr_size {0};
  float max_lf {0.91};

  size_type h(const key_type &key) const;
  element *insert_(const key_type &key);
  element *find_(const key_type &key) const;
  void rehash(const size_type& n);
  void reserve(const size_type& n);
  size_type find_last_free_element();

public:
  HashTable();
  HashTable(std::initializer_list<key_type> ilist);
  template<typename InputIt> HashTable(InputIt first, InputIt last);
  ~HashTable();
  HashTable(const HashTable &other);
  HashTable &operator=(const HashTable &other);
  HashTable &operator=(std::initializer_list<key_type> ilist);
  size_type size() const;
  bool empty() const;
  size_type count(const key_type &key) const;
  iterator find(const key_type &key) const;
  void clear();
  void swap(HashTable &other);
  void insert(std::initializer_list<key_type> ilist);
  std::pair<iterator,bool> insert(const key_type &key);
  template<typename InputIt> void insert(InputIt first, InputIt last);
  size_type erase(const key_type &key);
  const_iterator begin() const;
  const_iterator end() const;
  void dump(std::ostream &o = std::cerr) const;

  friend bool operator==(const HashTable<Key,N> &lhs, const HashTable<Key,N> &rhs) {
    if (lhs.curr_size != rhs.curr_size) return false;
    for (const auto &it : rhs) {
      if (!(lhs.count(it))) return false;
    }
    return true;
  }
  friend bool operator!=(const HashTable<Key,N> &lhs, const HashTable<Key,N> &rhs) { return !(lhs == rhs); }
};



template <typename Key, size_t N>
typename HashTable<Key,N>::size_type HashTable<Key,N>::h(const key_type &key) const { return hasher{}(key) % table_size; }

template<typename Key, size_t N>
template<typename InputIt> 
HashTable<Key,N>::HashTable(InputIt first, InputIt last) : HashTable{} { insert(first, last); }

template <typename Key, size_t N>
HashTable<Key,N>::HashTable() { rehash(N); }

template <typename Key, size_t N>
HashTable<Key,N>::HashTable(std::initializer_list<key_type> ilist) : HashTable{} { insert(ilist); }

template<typename Key, size_t N>
HashTable<Key,N>::~HashTable() { delete[] table; } 

template<typename Key, size_t N>
HashTable<Key,N>::HashTable(const HashTable &other) : HashTable{} {
  reserve(other.curr_size);
  last_free_element = table_size+keller-1;
  size_type help {other.table_size + other.keller};
  for (size_type idx{0}; idx < help; ++idx) {
    if (other.table[idx].mode == Mode::used) insert_(other.table[idx].key);
  }
  find_last_free_element();
}

template<typename Key, size_t N>
HashTable<Key,N> &HashTable<Key,N>::operator=(const HashTable &other) {
  if (this == &other) return *this;
  HashTable tmp {other};
  swap(tmp);
  return *this;
}

template<typename Key, size_t N>
HashTable<Key,N> &HashTable<Key,N>::operator=(std::initializer_list<key_type> ilist) {
  HashTable tmp{ilist};
  swap(tmp);
  return *this;
}

template<typename Key, size_t N>
typename HashTable<Key,N>::size_type HashTable<Key,N>::size() const { return curr_size; }

template<typename Key, size_t N>
bool HashTable<Key,N>::empty() const { return curr_size == 0; }

template<typename Key, size_t N>
typename HashTable<Key,N>::size_type HashTable<Key,N>::count(const key_type &key) const { return !!find_(key); }

template<typename Key, size_t N>
void HashTable<Key,N>::insert(std::initializer_list<key_type> ilist) { insert(std::begin(ilist), std::end(ilist)); }

template<typename Key, size_t N>
typename HashTable<Key,N>::const_iterator HashTable<Key,N>::begin() const { return const_iterator{table}; }

template<typename Key, size_t N>
typename HashTable<Key,N>::const_iterator HashTable<Key,N>::end() const { return const_iterator{table+table_size+keller}; }

template <typename Key, size_t N>
std::pair<typename HashTable<Key,N>::iterator ,bool> HashTable<Key,N>::insert(const key_type &key) {
  if (auto p {find_(key)}) return {iterator{p},false};
  reserve(curr_size+1);
  return {iterator{insert_(key)}, true};
}

template <typename Key, size_t N>
typename HashTable<Key,N>::iterator HashTable<Key,N>::find(const key_type &key) const {
  if (auto p{find_(key)}) return iterator{p};
  return end();
}

template <typename Key, size_t N>
typename HashTable<Key,N>::size_type HashTable<Key,N>::erase(const key_type &key) {
  element *p {find_(key)};
  if (p) {
    --curr_size;
    last_free_element = table_size + keller - 1;
    if (p->next_ != 0 && table[p->next_].mode == Mode::used) {
      size_type kette {p->next_};
      p->mode = Mode::free;
      p->next_ = 0;
      std::vector<size_type> positionen {kette};
      while (table[kette].next_ != 0) {
        if (table[table[kette].next_].mode == Mode::free) break;
        kette = table[kette].next_;
        positionen.push_back(kette);
      }
      for (size_t i{0}; i < positionen.size(); ++i) {
        kette = h(table[positionen.at(i)].key);
        if (table[kette].mode == Mode::free) {
          table[kette].key = table[positionen.at(i)].key;
          table[kette].mode = Mode::used;
          table[positionen.at(i)].mode = Mode::free;
          table[positionen.at(i)].next_ = 0;
        }
        else {
          while (table[kette].next_ != 0 && table[table[kette].next_].mode == Mode::used) {
            kette = table[kette].next_;
          }
          if (kette != positionen.at(i)) {
            table[kette].next_ = positionen.at(i); 
            table[positionen.at(i)].next_ = 0;
          }
        }
      }
      return 1;
    }
    else {
      p->mode = Mode::free;
      p->next_ = 0;
      return 1;
    }
  } else return 0;
}

template <typename Key, size_t N>
void HashTable<Key,N>::clear() {
  HashTable tmp {};
  swap(tmp);
}

template <typename Key, size_t N>
void HashTable<Key,N>::swap(HashTable &other) {
  std::swap(table, other.table);
  std::swap(table_size, other.table_size);
  std::swap(keller, other.keller);
  std::swap(curr_size, other.curr_size);
  std::swap(max_lf, other.max_lf);
  std::swap(last_free_element, other.last_free_element); 
}

template <typename Key, size_t N>
typename HashTable<Key,N>::element *HashTable<Key,N>::insert_(const key_type &key) {
  size_type idx{h(key)};
  if (table[idx].mode != Mode::used) {
    table[idx].key = key;
    table[idx].mode = Mode::used;
    ++curr_size;
    return table+idx;
  }
  else {
    while (table[idx].next_ != 0) {
      if (table[table[idx].next_].mode == Mode::free) break;
      idx = table[idx].next_;
    }
    while (table[last_free_element].mode == Mode::used) {
      --last_free_element;
    }

    table[last_free_element].key = key;
    table[last_free_element].mode = Mode::used;
    table[idx].next_ = last_free_element;
    ++curr_size;
    return table+last_free_element;
  }
}

template <typename Key, size_t N>
template<typename InputIt> void HashTable<Key,N>::insert(InputIt first, InputIt last) {
  for (auto it {first}; it != last; ++it) {
    if (!count(*it)) {
      reserve(curr_size+1);
      insert_(*it);
    }
  }
}

template <typename Key, size_t N>
typename HashTable<Key,N>::element *HashTable<Key,N>::find_(const key_type &key) const {
  size_type idx{h(key)};
  if (table[idx].mode == Mode::used) {
    if (key_equal{}(table[idx].key,key)) {
      return table+idx;
    }
    while (table[idx].next_ != 0) {
      idx = table[idx].next_;
      if (table[idx].mode == Mode::end || table[idx].mode == Mode::free) return nullptr; 
      if (key_equal{}(table[idx].key,key) && table[idx].mode == Mode::used) return table+idx;
    }
  }
  
  return nullptr;
}

template <typename Key, size_t N>
void HashTable<Key,N>::reserve(const size_type& n) {
  if (n > (table_size+keller)*max_lf) {
    size_type new_table_size {table_size};
    do {
      new_table_size = new_table_size * 2 + 1;
    } while (n > new_table_size * max_lf);
    rehash(new_table_size);
  }
}

template <typename Key, size_t N>
typename HashTable<Key,N>::size_type HashTable<Key,N>::find_last_free_element() {
  while (table[last_free_element].mode == Mode::used) {
    --last_free_element;
  }
  return last_free_element;
}

template <typename Key, size_t N>
void HashTable<Key,N>::rehash(const size_type& n) {
  size_type new_table_size {std::max(N, std::max(n,size_type(curr_size / max_lf)))};
  size_type old_keller {keller};
  keller = static_cast<size_type>(new_table_size*0.628);
  element *new_table {new element[new_table_size+keller+1]};
  size_type old_table_size {table_size};
  element *old_table = table;
  curr_size = 0;
  table = new_table;
  table_size = new_table_size;
  last_free_element = table_size+keller-1;
  table[table_size+keller].mode = Mode::end;
  for (size_type idx{0}; idx < old_keller+old_table_size; ++idx) {
    if (old_table[idx].mode == Mode::used) insert_(old_table[idx].key);
  }
  delete[] old_table;
}

template <typename Key, size_t N>
void HashTable<Key,N>::dump(std::ostream &o) const {
  o << "curr_size = " << curr_size << " table_size = " << table_size << "\n";
  for (size_type idx{0}; idx < table_size+keller+1; ++idx) {
    o << idx << ": ";
    switch (table[idx].mode) {
    case Mode::free:
      o << "--free--" << table[idx].next_;
      break;
    case Mode::used:
      o << table[idx].key;
      if (table[idx].next_ != 0) o << ", next: " << table[idx].next_;
      break;
    case Mode::end:
      o << "--END--";
      break;
    }
    o << "\n";
  }
}


//------------------------------------------------Iterator--------------------------------------------------------
template <typename Key, size_t N>
class HashTable<Key,N>::Iterator {
public:
  using value_type = Key;
  using difference_type = std::ptrdiff_t;
  using reference = const value_type &;
  using pointer = const value_type *;
  using iterator_category = std::forward_iterator_tag;

private:
  element *pos;
  void skip();

public:
  explicit Iterator(element *pos = nullptr);
  reference operator*() const;
  pointer operator->() const;
  Iterator &operator++();
  Iterator operator++(int);
  friend bool operator==(const Iterator &lhs, const Iterator &rhs) { return lhs.pos == rhs.pos; }
  friend bool operator!=(const Iterator &lhs, const Iterator &rhs) { return !(lhs == rhs); }
};

template <typename Key, size_t N>
HashTable<Key,N>::Iterator::Iterator(element *pos) : pos{pos} { if(pos) skip();}

template <typename Key, size_t N>
typename HashTable<Key,N>::Iterator::reference HashTable<Key,N>::Iterator::operator*() const {
  return pos->key;
}

template <typename Key, size_t N>
typename HashTable<Key,N>::Iterator::pointer HashTable<Key,N>::Iterator::operator->() const {
  return &pos->key;
}

template <typename Key, size_t N>
typename HashTable<Key,N>::Iterator &HashTable<Key,N>::Iterator::operator++() {
  ++pos; 
  skip(); 
  return *this;
}

template <typename Key, size_t N>
typename HashTable<Key,N>::Iterator HashTable<Key,N>::Iterator::operator++(int) {
  auto help {*this}; 
  ++pos; 
  skip(); 
  return help;
}

template <typename Key, size_t N>
void HashTable<Key,N>::Iterator::skip() {
  while(pos->mode != Mode::used && pos->mode != Mode::end) {
    ++pos;
  }
}

template <typename Key, size_t N> void swap(HashTable<Key,N> &lhs, HashTable<Key,N> &rhs) { lhs.swap(rhs); }


#endif
