#include <iostream>
#include <utility>
#include "MetaCommandResult.h"
#include "Table.h"
#include "Cursor.h"
#include "Row.h"

enum StatementType {
  STATEMENT_INSERT,
  STATEMENT_SELECT
};

enum ExecuteResult {
  EXECUTE_SUCCESS,
  EXECUTE_DUPLICATE_KEY,
  EXECUTE_TABLE_FULL
};


struct Statement {
  StatementType type;
  Row row_to_insert; // only used by insert statement;
};

struct InputBuffer {
  std::string buffer;

  explicit InputBuffer(std::string buffer) : buffer(std::move(buffer)) {}

  ~InputBuffer() = default;
};

/* Function forward declarations*/
void db_close(Table *table);

void print_prompt() {
  std::cout << "db > ";
}

std::string readInput() {
  std::string user_text;
  std::getline(std::cin, user_text);
  return user_text;
}

MetaCommandResult do_meta_command(std::string &command, Table *table) {
  if (command == ".exit") {
    db_close(table);
    exit(EXIT_SUCCESS);
  } else if (command == ".btree") {
    std::cout << "Tree: " << std::endl;
    print_leaf_node(table->pager->get_page(0));
    return META_COMMAND_SUCCESS;
  } else if (command == ".constants") {
    std::cout << "Constants: " << std::endl;
    print_constants();
    return META_COMMAND_SUCCESS;
  } else {
    return META_COMMAND_UNRECOGNIZED_COMMAND;
  }
}

PrepareResult
prepare_statement(const std::string &user_input, Statement *statement) {
  if (user_input.compare(0, 6, "insert") == 0) {
    statement->type = STATEMENT_INSERT;
    int args_assigned = sscanf(user_input.c_str(),
                               "insert %d %s %s",
        // We need to provide addresses here.
                               &(statement->row_to_insert.id),
        // Name of the array is the address of the first byte.
                               statement->row_to_insert.username,
                               statement->row_to_insert.email);

    std::cout << "Printing Row: " << statement->row_to_insert.id << " "
              << statement->row_to_insert.username << " "
              << statement->row_to_insert.email << std::endl;

    if (args_assigned < 3) {
      return PREPARE_SYNTAX_ERROR;
    }

    return PREPARE_SUCCESS;
  }

  if (user_input.compare(0, 6, "select") == 0) {
    statement->type = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
  }

  return PREPARE_UNRECOGNIZED_STATEMENT;
}

ExecuteResult execute_select(Statement *statement, Table *table) {
  Cursor *cursor = table_start(table);
  Row row{};
  while (!(cursor->end_of_table)) {
    deserialize_row(static_cast<std::byte *>(cursor_value(cursor)), row);
    print_row(row);
    cursor_advance(cursor);
  }

  free(cursor);

  return EXECUTE_SUCCESS;
}

ExecuteResult execute_insert(Statement *statement, Table *table) {
  std::byte *node = table->pager->get_page(table->root_page_num);
  uint32_t num_cells = *leaf_node_num_cells(node);

  uint32_t key_to_insert = statement->row_to_insert.id;
  Cursor *cursor = table_find(table, key_to_insert);

  // This means that the key already exists.
  if (cursor->cell_num < num_cells) {
    uint32_t key_at_index = *(leaf_node_key(node, cursor->cell_num));
    if (key_to_insert == key_at_index) {
      return EXECUTE_DUPLICATE_KEY;
    }
  }

  Row *row_to_insert = &(statement->row_to_insert);

  leaf_node_insert(cursor, row_to_insert->id, row_to_insert);
  return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement *statement, Table *table) {
  switch (statement->type) {
    case STATEMENT_INSERT:
      std::cout << "This is where we would do an insert." << std::endl;
      return execute_insert(statement, table);

    case STATEMENT_SELECT:
      std::cout << "This is where do would do a select." << std::endl;
      return execute_select(statement, table);
  }
}

Table *db_open(const char *filename) {
  Pager *pager = new Pager(filename);
  Table *table = static_cast<Table *>(malloc(sizeof(Table)));
  table->pager = pager;
  table->root_page_num = 0;

  if (pager->get_num_pages() == 0) {
    // New database file. Initialize page 0 as leaf node.
    std::byte *root_node = pager->get_page(0);
    initialize_leaf_node(root_node);
  }

  return table;
}

void db_close(Table *table) {
  Pager *pager = table->pager;

  std::cout << "Closing DB " << pager->get_num_pages() << std::endl;
  for (uint32_t i = 0; i < pager->get_num_pages(); i++) {
    std::cout << "Flushing page: " << i << std::endl;
    pager->flush(i);
  }

  free(table);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Must supply a database filename." << std::endl;
    exit(0);
  }

  const char *filename = argv[1];
  Table *table = db_open(filename);

  while (true) {
    print_prompt();
    std::string user_text = readInput();

    if (user_text[0] == '.') {
      switch (do_meta_command(user_text, table)) {
        case (META_COMMAND_SUCCESS):
          // Continue statement cannot be used with switch. This actually is
          // being used for the parent level if.
          continue;

        case (META_COMMAND_UNRECOGNIZED_COMMAND):
          std::cout << "Unrecognized command " << user_text << std::endl;
          continue;
      }
    }

    Statement statement{};
    switch (prepare_statement(user_text, &statement)) {

      case PREPARE_SUCCESS:
        break;
      case PREPARE_SYNTAX_ERROR:
        std::cout << "Syntax error. Could not parse statement." << std::endl;
        continue;
      case PREPARE_UNRECOGNIZED_STATEMENT:
        std::cout << "Unrecognized keyword at the start of " << user_text
                  << std::endl;
        continue;
    }

    switch (execute_statement(&statement, table)) {
      case EXECUTE_SUCCESS:
        std::cout << "Executed " << std::endl;
        break;
      case EXECUTE_DUPLICATE_KEY:
        std::cout << "Error : Duplicate key." << std::endl;
        break;
      case EXECUTE_TABLE_FULL:
        std::cout << "Error: Table full." << std::endl;
        break;
    }
  }
}
