//
// Created by oem on 9/30/21.
//

#ifndef RK_SQLLITE_NODE_H
#define RK_SQLLITE_NODE_H
#include <iostream>
#include <cstdint>
#include "Row.h"

enum NodeType {
  NODE_INTERNAL,
  NODE_LEAF
};

/*
 * Common Node Header Layout.
 * */
constexpr uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
constexpr uint32_t NODE_TYPE_OFFSET = 0;
constexpr uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
constexpr uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
constexpr uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
constexpr uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
constexpr uint8_t COMMON_NODE_HEADER_SIZE =
    NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

/*
 * Leaf Node Header format.
 * */
constexpr uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
constexpr uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
constexpr uint32_t
    LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;

/*
 * Leaf Node Body Layout.
 * */
constexpr uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
constexpr uint32_t LEAF_NODE_KEY_OFFSET = 0;
constexpr uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
constexpr uint32_t
    LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
constexpr uint32_t
    LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
constexpr uint32_t
    LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
constexpr uint32_t
    LEAF_NODE_MAX_CELLS = LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;

constexpr uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2;
constexpr uint32_t LEAF_NODE_LEFT_SPLIT_COUNT =
    (LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT;

/*
 * Internal Node layout.
 * */
constexpr uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
constexpr uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
constexpr uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t);
constexpr uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET =
    INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE;
constexpr uint32_t INTERNAL_NODE_HEADER_SIZE =
    COMMON_NODE_HEADER_SIZE + INTERNAL_NODE_NUM_KEYS_SIZE
        + INTERNAL_NODE_RIGHT_CHILD_SIZE;

/*
 * Internal node body layout.
 * */

constexpr uint32_t internal_node_key_size = sizeof(uint32_t);
constexpr uint32_t internal_node_child_size = sizeof(uint32_t);
constexpr uint32_t
    internal_node_cell_size = internal_node_key_size + internal_node_child_size;


constexpr uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
constexpr uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t);
constexpr uint32_t
    INTERNAL_NODE_CELL_SIZE = INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE;

uint32_t *internal_node_num_keys(std::byte *node) {
  return reinterpret_cast<uint32_t *>(node + INTERNAL_NODE_NUM_KEYS_OFFSET);
}

uint32_t *internal_node_right_child(std::byte *node) {
  return reinterpret_cast<uint32_t *>(node + INTERNAL_NODE_RIGHT_CHILD_OFFSET);
}

uint32_t *internal_node_cell(std::byte *node, uint32_t cell_num) {
  return reinterpret_cast<uint32_t *>(node + INTERNAL_NODE_HEADER_SIZE
      + cell_num * INTERNAL_NODE_CELL_SIZE);
}

uint32_t *internal_node_child(std::byte *node, uint32_t child_num) {
  uint32_t num_keys = *internal_node_num_keys(node);
  if (child_num > num_keys) {
    std::cout << "Tried to access child_num " << child_num << " > " << num_keys
              << std::endl;
    exit(EXIT_FAILURE);
  } else if (child_num == num_keys) {
    return internal_node_right_child(node);
  } else {
    return internal_node_cell(node, child_num);
  }
}

uint32_t *internal_node_key(std::byte *node, uint32_t key_num) {
  return internal_node_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE;
}

uint32_t *leaf_node_num_cells(void *node) {
  return reinterpret_cast<uint32_t *>(static_cast<char *>(node)
      + LEAF_NODE_NUM_CELLS_OFFSET);
}

void *leaf_node_cell(void *node, uint32_t cell_num) {
  return static_cast<char *> (node) + LEAF_NODE_HEADER_SIZE
      + cell_num * LEAF_NODE_CELL_SIZE;
}

uint32_t *leaf_node_key(void *node, uint32_t cell_num) {
  return static_cast<uint32_t *>(leaf_node_cell(node, cell_num));
}

void *leaf_node_value(void *node, uint32_t cell_num) {
  return static_cast<char *>(leaf_node_cell(node, cell_num))
      + LEAF_NODE_KEY_SIZE;
}

NodeType get_node_type(void *node) {
  uint8_t
      value = *((uint8_t *) (static_cast<char * >(node) + NODE_TYPE_OFFSET));
  return (NodeType) value;
}

void set_node_type(void *node, NodeType type) {
  uint8_t value = type;
  *((uint8_t *) (static_cast<char *>(node) + NODE_TYPE_OFFSET)) = value;
}

void initialize_leaf_node(void *node) {
  set_node_type(node, NODE_LEAF);
  *leaf_node_num_cells(node) = 0;
}

void set_node_root(void *node, bool is_root) {
  uint8_t value = is_root;
  *((uint8_t *) (static_cast<char *>(node) + IS_ROOT_OFFSET)) = value;
}

void initialize_internal_node(std::byte *node) {
  set_node_type(node, NODE_INTERNAL);
  set_node_root(node, false);
  *internal_node_num_keys(node) = 0;
}

void print_constants() {
  std::cout << "ROW_SIZE: " << ROW_SIZE << std::endl;
  std::cout << "COMMON_NODE_HEADER_SIZE: " << COMMON_NODE_HEADER_SIZE
            << std::endl;
  std::cout << "LEAF_NODE_HEADER_SIZE: " << LEAF_NODE_HEADER_SIZE << std::endl;
  std::cout << "LEAF_NODE_CELL_SIZE: " << LEAF_NODE_CELL_SIZE << std::endl;
  std::cout << "LEAF_NODE_SPACE_FOR_CELLS: " << LEAF_NODE_SPACE_FOR_CELLS
            << std::endl;
  std::cout << "LEAF_NODE_MAX_CELLS: " << LEAF_NODE_MAX_CELLS << std::endl;
}

void print_leaf_node(void *node) {
  uint32_t num_cells = *leaf_node_num_cells(node);
  std::cout << "Leaf (size " << num_cells << ")" << std::endl;
  for (uint32_t i = 0; i < num_cells; i++) {
    uint32_t key = *leaf_node_key(node, i);
    std::cout << "  - " << i << " : " << key << std::endl;
  }
}

#endif //RK_SQLLITE_NODE_H
