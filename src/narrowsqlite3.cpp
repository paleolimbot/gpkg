
#include <cpp11.hpp>
using namespace cpp11;

#include <sstream>

#include <narrow.h>

#define ARROW_HPP_IMPL
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

class SQLite3Stmt {
public:
  sqlite3_stmt* ptr;
  SQLite3Stmt(): ptr(nullptr) {}

  ~SQLite3Stmt() {
    if (ptr != nullptr) {
      sqlite3_finalize(ptr);
    }
  }
};

class SQLite3Error: public std::runtime_error {
public:
    SQLite3Error(const std::string& err): std::runtime_error(err) {}
};

class SQLite3ColumnBuilder: public arrow::hpp::builder::ArrayBuilder {
public:
  SQLite3ColumnBuilder(): decl_type_("") {}

  const std::string& decl_type() { return decl_type_; }

  void set_decl_type(const char* decl_type) {
    if (decl_type != nullptr) {
      decl_type_ = std::string(decl_type);
    } else {
      decl_type_ = std::string("");
    }
  }

  virtual void append_null() = 0;
  virtual bool append_integer(int64_t value) { return false; }
  virtual bool append_float(double value) { return false; };
  virtual bool append_blob(const unsigned char* value, int64_t size) { return false; }
  virtual bool append_text(const unsigned char* value, int64_t size) { return false; }

  void append_stmt(sqlite3_stmt* stmt, int i) {
    int column_type_id = sqlite3_column_type(stmt, i);
    int64_t size = 0;

    switch (column_type_id) {
    case SQLITE_NULL:
      append_null();
      break;
    case SQLITE_INTEGER:
      if (append_integer(sqlite3_column_int64(stmt, i))) {
        break;
      }
    case SQLITE_FLOAT:
      if (append_float(sqlite3_column_double(stmt, i))) {
        break;
      }
    case SQLITE_BLOB:
      size = sqlite3_column_bytes(stmt, i);
      if (append_blob(sqlite3_column_text(stmt, i), size)) {
        break;
      }
    case SQLITE_TEXT:
      size = sqlite3_column_bytes(stmt, i);
      if (append_text(sqlite3_column_text(stmt, i), size)) {
        break;
      }

    default:
      throw SQLite3Error("All SQLITE3 type conversions failed");
    }
  }

private:
  std::string decl_type_;
};

template<typename BuilderT>
class SQLite3GenericStringBuilder: public SQLite3ColumnBuilder {
public:
  SQLite3GenericStringBuilder(): builder_(new BuilderT()) {}

  void append_null() {
    builder_->finish_element(false);
    size_++;
  }

  bool append_text(const unsigned char* value, int64_t size) {
    builder_->write_buffer(value, size);
    builder_->finish_element();
    size_++;
    return true;
  }

  void release(struct ArrowArray* array_data, struct ArrowSchema* schema) {
    builder_->set_name(name());
    builder_->set_metadata("R_NARROWSQLITE3:decltype", decl_type());
    builder_->shrink();
    builder_->release(array_data, schema);

    builder_ = std::unique_ptr<BuilderT>(
      new BuilderT()
    );
  }

private:
  std::unique_ptr<BuilderT> builder_;
};

template<typename BuilderT>
class SQLite3NumericBuilder: public SQLite3ColumnBuilder {
public:
  SQLite3NumericBuilder(): builder_(new BuilderT()) {}

  void append_null() {
    builder_->write_element(0, false);
    size_++;
  }

  bool append_integer(int64_t value) {
    builder_->write_element(value);
    return true;
  }

  bool append_float(double value) {
    builder_->write_element(value);
    return true;
  }

  bool append_blob(const unsigned char* value, int64_t size) {
    throw SQLite3Error("Can't write numeric from SQLITE_BLOB");
  }

  bool append_text(const unsigned char* value, int64_t size) {
    throw SQLite3Error("Can't write numeric from SQLITE_TEXT");
  }

  void release(struct ArrowArray* array_data, struct ArrowSchema* schema) {
    builder_->set_name(name());
    builder_->set_metadata("R_NARROWSQLITE3:decltype", decl_type());
    builder_->shrink();
    builder_->release(array_data, schema);

    builder_ = std::unique_ptr<BuilderT>(
      new BuilderT()
    );
  }

private:
  std::unique_ptr<BuilderT> builder_;
};

using SQLite3Int32Builder= SQLite3NumericBuilder<arrow::hpp::builder::Int32ArrayBuilder>;
using SQLite3Float64Builder= SQLite3NumericBuilder<arrow::hpp::builder::Float64ArrayBuilder>;
using SQLite3BinaryBuilder = SQLite3GenericStringBuilder<arrow::hpp::builder::BinaryArrayBuilder>;
using SQLite3StringBuilder = SQLite3GenericStringBuilder<arrow::hpp::builder::StringArrayBuilder>;

class SQLite3StructBuilder: public arrow::hpp::builder::StructArrayBuilder {
public:
  SQLite3ColumnBuilder* child(int64_t i) {
    return reinterpret_cast<SQLite3ColumnBuilder*>(children_[i].get());
  }
};

std::unique_ptr<SQLite3ColumnBuilder> MakeColumnBuilder(const char* decl_type,
                                                        int first_value_type) {
  std::unique_ptr<SQLite3ColumnBuilder> builder;
  switch (first_value_type) {
  case SQLITE_INTEGER:
    // int32 for prototype; narrow doesn't define a conversion from int64_t to R
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new SQLite3Int32Builder());
    break;
  case SQLITE_FLOAT:
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new SQLite3Float64Builder());
    break;
  case SQLITE_BLOB:
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new SQLite3BinaryBuilder());
    break;
  default:
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new SQLite3StringBuilder());
    break;
  }

  builder->set_decl_type(decl_type);
  return builder;
}

class SQLite3ArrayStreamHolder {
public:
  SQLite3ArrayStreamHolder(sexp con_sexp, const std::string& sql):
    con_sexp_(con_sexp), sql_(sql), status_(SQLITE_OK) {
    external_pointer<SQLite3Connection> con(con_sexp);
    con_= con->ptr;
    schema_.release = nullptr;
  }

  ~SQLite3ArrayStreamHolder() {
    if (schema_.release != nullptr) {
      schema_.release(&schema_);
    }
  }

  int step_first() {
    int result = step();

    this->builder_ = std::unique_ptr<SQLite3StructBuilder>(
      new SQLite3StructBuilder()
    );

    int n_columns = sqlite3_column_count(stmt());
    for (int i = 0; i < n_columns; i++) {
      const char* name = sqlite3_column_name(stmt(), i);
      if (name == nullptr) {
        throw SQLite3Error("failed to allocate: sqlite3_column_name()");
      }

      const char* decl_type = sqlite3_column_decltype(stmt(), i);
      int first_value_type = sqlite3_column_type(stmt(), i);

      builder_->add_child(MakeColumnBuilder(decl_type, first_value_type), name);
    }

    struct ArrowArray dummy_array_data;
    dummy_array_data.release = nullptr;
    builder_->release(&dummy_array_data, &schema_);

    return result;
  }

  int step_row() {
    if (status_ == SQLITE_DONE) {
      return status_;
    }

    int n_columns = builder_->num_children();
    for (int i = 0; i < n_columns; i++) {
      builder_->child(i)->append_stmt(stmt(), i);
    }
    builder_->set_size(builder_->size() + 1);

    return step();
  }

  void release_batch(struct ArrowArray* array_data, struct ArrowSchema* schema) {
    builder_->release(array_data, schema);
  }

private:
  sexp con_sexp_;
  sqlite3* con_;
  std::string sql_;
  SQLite3Stmt stmt_;
  int status_;

  std::unique_ptr<SQLite3StructBuilder> builder_;
  struct ArrowSchema schema_;

  int step() {
    int result = sqlite3_step(stmt());
    switch (result) {
    case SQLITE_DONE:
    case SQLITE_ROW:
      status_ = result;
      return result;
    default:
      break;
    }

    std::stringstream stream;
    stream << sqlite3_errstr(result) << sqlite3_errmsg(con_);
    throw SQLite3Error(stream.str().c_str());
  }

  sqlite3_stmt* stmt() {
    if (stmt_.ptr == nullptr) {
      const char* tail;
      int result = sqlite3_prepare_v2(con_, sql_.c_str(), sql_.size(), &stmt_.ptr, &tail);
      if (result != SQLITE_OK) {
        std::stringstream stream;
        stream << "<" << sqlite3_errstr(result) << "> " << sqlite3_errmsg(con_);
        throw SQLite3Error(stream.str().c_str());
      }
    }

    return stmt_.ptr;
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

[[cpp11::register]]
void sqlite_cpp_query_all(sexp con_sexp, std::string sql,
                          sexp array_data_xptr, sexp schema_xptr) {
  struct ArrowArray* array_data_out = reinterpret_cast<struct ArrowArray*>(
    R_ExternalPtrAddr(array_data_xptr)
  );
  struct ArrowSchema* schema_out = reinterpret_cast<struct ArrowSchema*>(
    R_ExternalPtrAddr(schema_xptr)
  );

  SQLite3ArrayStreamHolder holder(con_sexp, sql);
  holder.step_first();
  int result;
  int64_t i = 0;
  do {
    result = holder.step_row();
    i++;
    if ((i % 1000) == 0) {
      check_user_interrupt();
    }
  } while (result != SQLITE_DONE);

  holder.release_batch(array_data_out, schema_out);
}
