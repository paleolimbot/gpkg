
#include <cpp11.hpp>
using namespace cpp11;

#include <narrow.h>

#include "arrow-hpp/builder.hpp"
#include "arrow-hpp/builder-string.hpp"
#include "arrow-hpp/builder-struct.hpp"


#include "sqlite3.h"


class SQLite3Connection {
public:
  sqlite3* ptr;
  SQLite3Connection(): ptr(nullptr) {}

  ~SQLite3Connection() {
    if (ptr != nullptr) {
      sqlite3_close(ptr);
    }
  }
};

[[cpp11::register]]
cpp11::sexp sqlite_cpp_open(std::string filename) {
  external_pointer<SQLite3Connection> con(new SQLite3Connection());
  int result = sqlite3_open(filename.c_str(), &con->ptr);
  if (result != SQLITE_OK) {
    stop("%s", sqlite3_errstr(result));
  }

  return as_sexp(con);
}

[[cpp11::register]]
void sqlite_cpp_close(cpp11::sexp con_sexp) {
  external_pointer<SQLite3Connection> con(con_sexp);
  int result = sqlite3_close(con->ptr);
  if (result != SQLITE_OK) {
    stop("%s", sqlite3_errstr(result));
  }

  con->ptr = nullptr;
}

[[cpp11::register]]
int sqlite_cpp_exec(cpp11::sexp con_sexp, std::string sql) {
  external_pointer<SQLite3Connection> con(con_sexp);
  char* error_message = nullptr;
  int result = sqlite3_exec(con->ptr, sql.c_str(), nullptr, nullptr, &error_message);
  if (error_message != nullptr) {
    stop("%s", error_message);
  }

  return result;
}
