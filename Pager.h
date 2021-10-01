//
// Created by Rahul Kushwaha on 10/11/21.
//

#ifndef RK_SQLLITE_PAGER_H
#define RK_SQLLITE_PAGER_H

#include <cstdint>
#include <string>

#define TABLE_MAX_PAGES 100
const uint32_t PAGE_SIZE = 4096;

class Pager {
 private:
  int file_descriptor;
  uint32_t file_length;
  uint32_t num_pages;
  std::byte *pages[TABLE_MAX_PAGES]{};

 public:
  explicit Pager(const std::string &filename);
  void flush(uint32_t page_num);
  [[nodiscard]] uint32_t get_unused_page_num() const;
  std::byte *get_page(uint32_t page_num);
  uint32_t get_num_pages() const;
  ~Pager();
};


#endif //RK_SQLLITE_PAGER_H
