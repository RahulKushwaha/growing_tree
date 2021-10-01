//
// Created by Rahul Kushwaha on 10/11/21.
//
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include "Pager.h"

void Pager::flush(uint32_t page_num) {
  if (pages[page_num] == nullptr) {
    std::cout << "Tried to flush null page." << std::endl;
    exit(EXIT_FAILURE);
  }

  off_t offset = lseek(file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
  if (offset == -1) {
    std::cout << "Error seeking: " << errno << std::endl;
    exit(EXIT_FAILURE);
  }

  ssize_t bytes_written = write(file_descriptor, pages[page_num], PAGE_SIZE);

  if (bytes_written == -1) {
    std::cout << "Error writing: " << errno << std::endl;
    exit(EXIT_FAILURE);
  }

  std::cout << "Successfully flushed page: " << page_num << std::endl;
}

Pager::Pager(const std::string &filename) : file_descriptor(-1) {
  // O_RDWR => Read/Write mode.
  // O_CREAT => Create file if it does not exist.
  // S_IWUSR => User write permission.
  // S_IRUSR => User read permission.
  int fd = open(filename.c_str(), O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);

  if (fd == -1) {
    std::cout << "Unable to open file: " << errno << std::endl;
    exit(0);
  }

  off_t current_file_length = lseek(fd, 0, SEEK_END);

  this->file_descriptor = fd;
  this->file_length = current_file_length;
  this->num_pages = (current_file_length / PAGE_SIZE);

  if (file_length % PAGE_SIZE != 0) {
    std::cout << "Db file is not a whole number of pages. Corrupt file."
              << std::endl;
    exit(EXIT_FAILURE);
  }

//  for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
//    this->pages[i] = nullptr;
//  }
  for (auto &page: this->pages) {
    page = nullptr;
  }
}

Pager::~Pager() {
  // How to delete std byte array.
  for (uint32_t i = 0; i < num_pages; i++) {
    delete pages[i];
  }

  if (file_descriptor != -1) {
    int result = close(file_descriptor);
    if (result == -1) {
      std::cout << "Error closing db file." << std::endl;
      exit(EXIT_FAILURE);
    }
  }
}

uint32_t Pager::get_unused_page_num() const {
  return num_pages;
}

std::byte *Pager::get_page(uint32_t page_num) {
  if (page_num > TABLE_MAX_PAGES) {
    std::cout << "Tried to fetch page number out of bounds." << page_num
              << " > " << TABLE_MAX_PAGES << std::endl;
    exit(0);
  }

  if (pages[page_num] == nullptr) {
    // Cache miss. Allocate memory and load from file.
    //void *page = malloc(PAGE_SIZE);
    std::cout << "Returning a newly allocated page." << std::endl;
    std::cout << "Total number of pages: " << num_pages << std::endl;

    std::byte *page = new std::byte[PAGE_SIZE];
    uint32_t current_num_pages = file_length / PAGE_SIZE;

    // We might save a partial page at the end of file
    if (file_length % PAGE_SIZE) {
      current_num_pages += 1;
    }

    if (page_num <= current_num_pages) {
      lseek(file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
      ssize_t bytes_read = read(file_descriptor, page, PAGE_SIZE);
      if (bytes_read == -1) {
        std::cout << "Error reading file: " << errno;
        exit(EXIT_FAILURE);
      }
    }

    pages[page_num] = page;

    if (page_num >= num_pages) {
      num_pages = page_num + 1;
    }
  }

  return pages[page_num];
}

uint32_t Pager::get_num_pages() const {
  return num_pages;
}
