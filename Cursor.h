//
// Created by oem on 9/30/21.
//
#ifndef RK_SQLLITE_CURSOR_H
#define RK_SQLLITE_CURSOR_H
#include "Table.h"
#include "Row.h"
#include "Node.h"

struct Cursor {
  Table *table;
  uint32_t page_num;
  uint32_t cell_num;
  bool end_of_table;
};

void create_new_root(Table &table, uint32_t right_child_page_num);


Cursor *table_start(Table *table) {
  Cursor *cursor = static_cast<Cursor *> (malloc(sizeof(Cursor)));
  cursor->table = table;
  cursor->page_num = table->root_page_num;
  cursor->cell_num = 0;

//  void *root_node = get_page(table->pager, table->root_page_num);
  std::byte *root_node = table->pager->get_page(table->root_page_num);
  uint32_t num_cells = *leaf_node_num_cells(root_node);
  cursor->end_of_table = (num_cells == 0);

  return cursor;
}

Cursor *table_end(Table *table) {
  Cursor *cursor = static_cast<Cursor *> (malloc(sizeof(Cursor)));
  cursor->table = table;
  cursor->page_num = table->root_page_num;
  std::byte *root_node = table->pager->get_page(table->root_page_num);
  uint32_t num_cells = *leaf_node_num_cells(root_node);
  cursor->cell_num = num_cells;
  cursor->end_of_table = true;

  return cursor;
}

void cursor_advance(Cursor *cursor) {
  uint32_t page_num = cursor->page_num;
  std::byte *node = cursor->table->pager->get_page(page_num);
  cursor->cell_num += 1;
  if (cursor->cell_num >= (*leaf_node_num_cells(node))) {
    cursor->end_of_table = true;
  }
}

void *cursor_value(Cursor *cursor) {
  uint32_t page_num = cursor->page_num;
  std::byte *page = cursor->table->pager->get_page(page_num);
  return leaf_node_value(page, cursor->cell_num);
}

bool is_node_root(std::byte *node) {
  uint8_t value = *((uint8_t *) (node + IS_ROOT_OFFSET));
  return (bool) value;
}

void set_node_root(std::byte *node, bool is_root) {
  uint8_t value = is_root;
  *((uint8_t *) (node + IS_ROOT_OFFSET)) = value;
}

void leaf_node_split_and_insert(Cursor *cursor, uint32_t key, Row *value) {
  std::cout << "leaf_node_split_and_insert called" << std::endl;
  /*
    Create a new node and move half of the cells over.
    Insert the new value in one of the two cells.
    Update parent or create new parent.
   * */
  std::byte *old_node = cursor->table->pager->get_page(cursor->page_num);
  uint32_t new_page_num = cursor->table->pager->get_unused_page_num();
  std::byte *new_node = cursor->table->pager->get_page(new_page_num);
  initialize_leaf_node(new_node);

  std::cout << "New leaf node has been initialized" << std::endl;

  /*
   All existing keys plus the new one should be divided evenly between the
   old(left) and (right) nodes. Starting from right, move each key to correct
   position.
   */

  for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--) {
    std::cout << "Moving cell: " << i << std::endl;
    void *destination_node;
    if (i >= LEAF_NODE_LEFT_SPLIT_COUNT) {
      destination_node = new_node;
    } else {
      destination_node = old_node;
    }

    uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
    void *destination = leaf_node_cell(destination_node, index_within_node);

    if (i == cursor->cell_num) {
      serialize_row(*value, static_cast<std::byte *> (destination));
    } else if (i > cursor->cell_num) {
      memcpy(destination, leaf_node_cell(old_node, i - 1), LEAF_NODE_CELL_SIZE);
    } else {
      memcpy(destination, leaf_node_cell(old_node, i), LEAF_NODE_CELL_SIZE);
    }
  }

  std::cout << "Data copy complete between the new and old node." << std::endl;

  /*Update cell count on both leaf nodes*/
  *(leaf_node_num_cells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
  *(leaf_node_num_cells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;

  if (is_node_root(old_node)) {
    return create_new_root(*cursor->table, new_page_num);
  } else {
    std::cout << "Need to implement updating parent after split" << std::endl;
  }
}

void leaf_node_insert(Cursor *cursor, uint32_t key, Row *value) {
  std::byte *node = cursor->table->pager->get_page(cursor->page_num);
  uint32_t num_cells = *leaf_node_num_cells(node);

  std::cout << "Total number of cells in node: " << num_cells
            << "\n Max Cells in a node: " << LEAF_NODE_MAX_CELLS << std::endl;

  if (num_cells >= LEAF_NODE_MAX_CELLS) {
    leaf_node_split_and_insert(cursor, key, value);
    return;
  }

  if (cursor->cell_num < num_cells) {
    // Make room for the new cell.
    for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
      memcpy(leaf_node_cell(node, i),
             leaf_node_cell(node, i - 1),
             LEAF_NODE_CELL_SIZE);
    }
  }

  *(leaf_node_num_cells(node)) += 1;
  *(leaf_node_key(node, cursor->cell_num)) = key;
  serialize_row(*value,
                static_cast<std::byte *>(leaf_node_value(node,
                                                         cursor->cell_num)));
}

Cursor *leaf_node_find(Table *table, uint32_t page_num, uint32_t key) {
  std::byte *node = table->pager->get_page(page_num);
  uint32_t num_cells = *leaf_node_num_cells(node);

  Cursor *cursor = static_cast<Cursor *>(malloc(sizeof(Cursor)));
  cursor->table = table;
  cursor->page_num = page_num;

  uint32_t min_index = 0;
  uint32_t one_past_max_index = num_cells;

  while (one_past_max_index != min_index) {
    uint32_t index = (min_index + one_past_max_index) / 2;
    uint32_t key_at_index = *leaf_node_key(node, index);

    if (key == key_at_index) {
      cursor->cell_num = index;
      return cursor;
    }

    if (key < key_at_index) {
      one_past_max_index = index;
    } else {
      min_index = index + 1;
    }
  }

  cursor->cell_num = min_index;
  return cursor;
}

/*
 * Return the position of the given key.
 * If the key is not present, return the position of where it should be found.
 * */
Cursor *table_find(Table *table, uint32_t key) {
  uint32_t root_page_num = table->root_page_num;
  std::byte *root_node = table->pager->get_page(root_page_num);

  if (get_node_type(root_node) == NODE_LEAF) {
    return leaf_node_find(table, root_page_num, key);
  } else {
    std::cout << "Need to implement searching an internal node." << std::endl;
    exit(EXIT_FAILURE);
  }
}

void create_new_root(Table &table, uint32_t right_child_page_num) {
  /*
   Handle splitting the root.
   Old root copied to new page, becomes left child.
   Address of the right child passed in.
   Re-initialize root page to contain the new root.
   New root node points to two children.
   **/

  std::byte *root = table.pager->get_page(table.root_page_num);
  std::byte *right_child = table.pager->get_page(right_child_page_num);
  uint32_t left_child_page_num = table.pager->get_unused_page_num();
  std::byte *left_child = table.pager->get_page(left_child_page_num);

  // Left child has the data copied from the root node.
  memcpy(left_child, root, PAGE_SIZE);
  set_node_root(left_child, false);

  // Initialize internal node
  // initialize_internal_node(root);
  set_node_root(root, true);
}

#endif //RK_SQLLITE_CURSOR_H
