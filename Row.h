//
// Created by oem on 9/30/21.
//

#ifndef RK_SQLLITE_ROW_H
#define RK_SQLLITE_ROW_H

#include <cstring>
#include "Pager.h"

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

struct Row {
  uint32_t id;
  char username[COLUMN_USERNAME_SIZE];
  char email[COLUMN_EMAIL_SIZE];
};

#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0) -> Attribute)
const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);

const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

void serialize_row(Row &source, std::byte *destination) {
  memcpy(destination + ID_OFFSET, &(source.id), ID_SIZE);
  memcpy(destination + USERNAME_OFFSET, &(source.username), USERNAME_SIZE);
  memcpy(destination + EMAIL_OFFSET, &(source.email), EMAIL_SIZE);
}

void deserialize_row(std::byte *source, Row &destination) {
  memcpy(&(destination.id), source + ID_OFFSET, ID_SIZE);
  memcpy(&(destination.username), source + USERNAME_OFFSET, USERNAME_SIZE);
  memcpy(&(destination.email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

void print_row(const Row &row) {
  std::cout << "(" << row.id << ", " << row.username << ", " << row.email << ")"
            << std::endl;
}

#endif //RK_SQLLITE_ROW_H
