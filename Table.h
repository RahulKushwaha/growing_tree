//
// Created by oem on 9/30/21.
//
#include "Node.h"
#include "Pager.h"

#ifndef RK_SQLLITE_TABLE_H
#define RK_SQLLITE_TABLE_H

struct Table {
  Pager *pager;
  uint32_t root_page_num;
};

#endif //RK_SQLLITE_TABLE_H
