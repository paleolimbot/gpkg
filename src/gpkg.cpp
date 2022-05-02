
#include <cpp11.hpp>
using namespace cpp11;

#include <sstream>
#include <future>

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

  virtual void append_stmt(sqlite3_stmt* stmt, int i) {
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

  virtual bool append_blob(const unsigned char* value, int64_t size) {
    return append_text(value, size);
  }

  virtual bool append_text(const unsigned char* value, int64_t size) {
    builder_->write_buffer(value, size);
    builder_->finish_element();
    size_++;
    return true;
  }

  virtual void release(struct ArrowArray* array_data, struct ArrowSchema* schema) {
    builder_->set_name(name());
    builder_->set_metadata("R_gpkg:decltype", decl_type());
    builder_->shrink();
    builder_->release(array_data, schema);

    builder_ = std::unique_ptr<BuilderT>(
      new BuilderT()
    );
  }

protected:
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
    builder_->set_metadata("R_gpkg:decltype", decl_type());
    builder_->shrink();
    builder_->release(array_data, schema);

    builder_ = std::unique_ptr<BuilderT>(
      new BuilderT()
    );
  }

private:
  std::unique_ptr<BuilderT> builder_;
};

using BooleanBuilder = SQLite3NumericBuilder<arrow::hpp::builder::Int32ArrayBuilder>;
using TinyIntBuilder = SQLite3NumericBuilder<arrow::hpp::builder::Int32ArrayBuilder>;
using SmallIntBuilder = SQLite3NumericBuilder<arrow::hpp::builder::Int32ArrayBuilder>;
using MediumIntBuilder = SQLite3NumericBuilder<arrow::hpp::builder::Int32ArrayBuilder>;
using IntBuilder = SQLite3NumericBuilder<arrow::hpp::builder::Int32ArrayBuilder>;
using FloatBuilder = SQLite3NumericBuilder<arrow::hpp::builder::Float64ArrayBuilder>;
using DoubleBuilder = SQLite3NumericBuilder<arrow::hpp::builder::Float64ArrayBuilder>;
using TextBuilder = SQLite3GenericStringBuilder<arrow::hpp::builder::StringArrayBuilder>;
using BlobBuilder = SQLite3GenericStringBuilder<arrow::hpp::builder::BinaryArrayBuilder>;

using DateBuilder = SQLite3GenericStringBuilder<arrow::hpp::builder::StringArrayBuilder>;
using DateTimeBuilder = SQLite3GenericStringBuilder<arrow::hpp::builder::StringArrayBuilder>;


class GeometryBuilder: public BlobBuilder {
public:
  bool append_blob(const unsigned char* value, int64_t size) {
    // We need to skip the gpkg header and just append the WKB
    // https://www.geopackage.org/spec130/index.html#gpb_format
    if (size < 4 ) {
      throw SQLite3Error("Unexpected geometry BLOB (size < 4)");
    }

    unsigned char flag_byte = value[3];
    unsigned char flag_byte_masked = flag_byte & static_cast<unsigned char>(0x0e);
    unsigned char envelope_flag = flag_byte_masked >> 1;
    int64_t envelope_size = 0;
    switch (envelope_flag) {
    case 0:
      envelope_size = 0;
      break;
    case 1:
      envelope_size = 32;
      break;
    case 2:
    case 3:
      envelope_size = 48;
      break;
    case 4:
      envelope_size = 64;
      break;
    default:
      throw SQLite3Error("Unexpected envelope flag value");
    }

    if ((8 + envelope_size) > size) {
      throw SQLite3Error("Geometry BLOB size smaller than header + envelope");
    }

    builder_->write_buffer(value + (8 + envelope_size), size);
    builder_->finish_element();
    return true;
  }

  bool append_text(const unsigned char* value, int64_t size) {
    return false;
  }

  void release(struct ArrowArray* array_data, struct ArrowSchema* schema) {
    builder_->set_metadata("ARROW:extension:name", "geoarrow.wkb");
    builder_->set_metadata("ARROW:extension:metadata", {'\0', '\0', '\0', '\0'});
    SQLite3GenericStringBuilder::release(array_data, schema);
  }
};



class SQLite3StructBuilder: public arrow::hpp::builder::StructArrayBuilder {
public:
  SQLite3ColumnBuilder* child(int64_t i) {
    return reinterpret_cast<SQLite3ColumnBuilder*>(children_[i].get());
  }
};

std::unique_ptr<SQLite3ColumnBuilder> MakeColumnBuilder(const char* decl_type,
                                                        int first_value_type) {
  std::unique_ptr<SQLite3ColumnBuilder> builder;

  // Try using the declared type first (without parentheses)
  std::string decl_type_str(decl_type);
  const char* first_paren = strchr(decl_type, '(');
  if (first_paren != nullptr) {
    decl_type_str = std::string(decl_type, first_paren);
  }

  if (decl_type_str == "BOOLEAN") {
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new BooleanBuilder());
  } else if (decl_type_str == "TINYINT") {
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new TinyIntBuilder());
  } else if (decl_type_str == "SMALLINT") {
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new SmallIntBuilder());
  } else if (decl_type_str == "MEDIUMINT") {
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new MediumIntBuilder());
  } else if (decl_type_str == "INT" || decl_type_str == "INTEGER") {
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new IntBuilder());
  } else if (decl_type_str == "FLOAT") {
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new FloatBuilder());
  } else if (decl_type_str == "DOUBLE" || decl_type_str == "REAL") {
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new DoubleBuilder());
  } else if (decl_type_str == "TEXT") {
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new TextBuilder());
  } else if (decl_type_str == "BLOB") {
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new BlobBuilder());
  } else if (decl_type_str == "GEOMETRY") {
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new GeometryBuilder());
  } else if (decl_type_str == "POINT") {
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new GeometryBuilder());
  } else if (decl_type_str == "LINESTRING") {
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new GeometryBuilder());
  } else if (decl_type_str == "POLYGON") {
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new GeometryBuilder());
  } else if (decl_type_str == "MULTIPOINT") {
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new GeometryBuilder());
  } else if (decl_type_str == "MULTILINESTRING") {
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new GeometryBuilder());
  } else if (decl_type_str == "MULTIPOLYGON") {
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new GeometryBuilder());
  } else if (decl_type_str == "GEOMETRYCOLLECTION") {
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new GeometryBuilder());
  } else if (decl_type_str == "DATE") {
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new DateBuilder());
  } else if (decl_type_str == "DATETIME") {
    builder = std::unique_ptr<SQLite3ColumnBuilder>(new DateTimeBuilder());
  } else {
    // Fall back on the type of the first value
    switch (first_value_type) {
    case SQLITE_INTEGER:
      // int32 for prototype; narrow doesn't define a conversion from int64_t to R
      builder = std::unique_ptr<SQLite3ColumnBuilder>(new MediumIntBuilder());
      break;
    case SQLITE_FLOAT:
      builder = std::unique_ptr<SQLite3ColumnBuilder>(new DoubleBuilder());
      break;
    case SQLITE_BLOB:
      builder = std::unique_ptr<SQLite3ColumnBuilder>(new BlobBuilder());
      break;
    default:
      builder = std::unique_ptr<SQLite3ColumnBuilder>(new TextBuilder());
      break;
    }
  }

  builder->set_decl_type(decl_type);
  return builder;
}

class SQLite3ArrayStreamHolder {
public:
  SQLite3ArrayStreamHolder(sqlite3* con, const std::string& sql):
    con_(con), sql_(sql), status_(SQLITE_OK) {
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
cpp11::sexp gpkg_cpp_open(std::string filename) {
  external_pointer<SQLite3Connection> con(new SQLite3Connection());
  int result = sqlite3_open(filename.c_str(), &con->ptr);
  if (result != SQLITE_OK) {
    stop("%s", sqlite3_errstr(result));
  }

  return as_sexp(con);
}

[[cpp11::register]]
void gpkg_cpp_close(cpp11::sexp con_sexp) {
  external_pointer<SQLite3Connection> con(con_sexp);
  int result = sqlite3_close(con->ptr);
  if (result != SQLITE_OK) {
    stop("%s", sqlite3_errstr(result));
  }

  con->ptr = nullptr;
}

[[cpp11::register]]
int gpkg_cpp_exec(cpp11::sexp con_sexp, std::string sql) {
  external_pointer<SQLite3Connection> con(con_sexp);
  char* error_message = nullptr;
  int result = sqlite3_exec(con->ptr, sql.c_str(), nullptr, nullptr, &error_message);
  if (error_message != nullptr) {
    stop("%s", error_message);
  }

  return result;
}

std::string gpkg_query(sqlite3* con,
                       const std::string& sql,
                       struct ArrowArray* array_data_out,
                       struct ArrowSchema* schema_out) {
  try {
    SQLite3ArrayStreamHolder holder(con, sql);
    holder.step_first();
    int result;
    do {
      result = holder.step_row();
    } while (result != SQLITE_DONE);

    holder.release_batch(array_data_out, schema_out);

    return std::string("");
  } catch (std::exception& e) {
    return std::string(e.what());
  }
}

std::future<std::string> gpkg_query_async(sqlite3* con,
                                          const std::string& sql,
                                          struct ArrowArray* array_data_out,
                                          struct ArrowSchema* schema_out) {
  return std::async(
    std::launch::async,
    gpkg_query,
    con, sql, array_data_out, schema_out
  );
}

std::future<std::string> gpkg_query_async_sexp(sexp con_sexp, std::string sql,
                                               sexp array_data_xptr, sexp schema_xptr) {
  external_pointer<SQLite3Connection> con(con_sexp);

  struct ArrowArray* array_data_out = reinterpret_cast<struct ArrowArray*>(
    R_ExternalPtrAddr(array_data_xptr)
  );
  struct ArrowSchema* schema_out = reinterpret_cast<struct ArrowSchema*>(
    R_ExternalPtrAddr(schema_xptr)
  );

  return gpkg_query_async(con->ptr, sql, array_data_out, schema_out);
}

[[cpp11::register]]
void gpkg_cpp_query(list con_sexp, strings sql, list array_data_xptr, list schema_xptr) {
  std::vector<std::future<std::string>> futures;

  for (R_xlen_t i = 0; i < con_sexp.size(); i++) {
    futures.push_back(
      gpkg_query_async_sexp(
        con_sexp[i],
        sql[i],
        array_data_xptr[i],
        schema_xptr[i]
      )
    );
  }

  std::string error_message("");
  SEXP interrupt_error = R_NilValue;

  do {
    int64_t last_future = static_cast<int64_t>(futures.size()) - 1;
    for (int64_t i = last_future; i >= 0; i--) {
      auto& fut = futures[i];
      auto status = fut.wait_for(std::chrono::milliseconds(500));


      if (status == std::future_status::ready) {
        std::string result = fut.get();
        if (result != "") {
          error_message = result;
          futures.pop_back();
          break;
        } else {
          futures.pop_back();
        }
      }
    }

    try {
      check_user_interrupt();
    } catch (unwind_exception& e) {
      interrupt_error = e.token;
    }
  } while (futures.size() > 0 && error_message == "" && interrupt_error == R_NilValue);

  // cancel everything left and wait for the futures to finish
  for (size_t i = 0; i < futures.size(); i++) {
    external_pointer<SQLite3Connection> con(con_sexp[static_cast<R_xlen_t>(i)]);
    sqlite3_interrupt(con->ptr);
    futures[i].wait();
  }

  if (interrupt_error != R_NilValue) {
    throw unwind_exception(interrupt_error);
  }

  if (error_message != "") {
    throw SQLite3Error(error_message);
  }
}
