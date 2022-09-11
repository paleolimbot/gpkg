
#include <cpp11.hpp>
using namespace cpp11;

#include <sstream>
#include <future>
#include <vector>

#include "sqlite3.h"

#include "nanoarrow.h"
#include "nanoarrow_sqlite3.h"


class GPKGConnection {
public:
  sqlite3* ptr;
  GPKGConnection(): ptr(nullptr) {}

  ~GPKGConnection() {
    if (ptr != nullptr) {
      sqlite3_close(ptr);
    }
  }
};

class GPKGStmt {
public:
  sqlite3_stmt* ptr;
  GPKGStmt(): ptr(nullptr) {}

  ~GPKGStmt() {
    if (ptr != nullptr) {
      sqlite3_finalize(ptr);
    }
  }
};

class GPKGError2: public std::runtime_error {
public:
    GPKGError2(const std::string& err): std::runtime_error(err) {}
};

class GPKGArrayStreamHolder2 {
public:
  GPKGArrayStreamHolder2(sqlite3* con, const std::string& sql):
    con_(con), sql_(sql), status_(SQLITE_OK) {
    ArrowSQLite3ResultInit(&result_);
  }

  ~GPKGArrayStreamHolder2() {
    ArrowSQLite3ResultReset(&result_);
  }

  int step() {
    int result = ArrowSQLite3ResultStep(&result_, stmt());
    if (result != NANOARROW_OK) {
      throw GPKGError2(ArrowSQLite3ResultError(&result_));
    }

    switch (result_.step_return_code) {
    case SQLITE_DONE:
    case SQLITE_ROW:
      status_ = result_.step_return_code;
      return result_.step_return_code;
    default:
      break;
    }

    std::stringstream stream;
    stream << sqlite3_errstr(result_.step_return_code) << sqlite3_errmsg(con_);
    throw GPKGError2(stream.str().c_str());
  }

  void release_batch(struct ArrowArray* array_data, struct ArrowSchema* schema) {
    int result = ArrowSchemaDeepCopy(&result_.schema, schema);
    if (result != NANOARROW_OK) {
      throw GPKGError2("Failed to copy result schema");
    }

    result = ArrowSQLite3ResultFinishArray(&result_, array_data);
    if (result != NANOARROW_OK) {
      throw GPKGError2(ArrowSQLite3ResultError(&result_));
    }
  }

private:
  sqlite3* con_;
  std::string sql_;
  GPKGStmt stmt_;
  int status_;

  struct ArrowSQLite3Result result_;

  sqlite3_stmt* stmt() {
    if (stmt_.ptr == nullptr) {
      const char* tail;
      int result = sqlite3_prepare_v2(con_, sql_.c_str(), sql_.size(), &stmt_.ptr, &tail);
      if (result != SQLITE_OK) {
        std::stringstream stream;
        stream << "<" << sqlite3_errstr(result) << "> " << sqlite3_errmsg(con_);
        throw GPKGError2(stream.str().c_str());
      }
    }

    return stmt_.ptr;
  }
};


std::string gpkg_query2(sqlite3* con,
                       const std::string& sql,
                       struct ArrowArray* array_data_out,
                       struct ArrowSchema* schema_out) {
  try {
    GPKGArrayStreamHolder2 holder(con, sql);
    int result;
    do {
      result = holder.step();
    } while (result != SQLITE_DONE);

    holder.release_batch(array_data_out, schema_out);

    return std::string("");
  } catch (std::exception& e) {
    return std::string(e.what());
  }
}

std::future<std::string> gpkg_query_async2(sqlite3* con,
                                          const std::string& sql,
                                          struct ArrowArray* array_data_out,
                                          struct ArrowSchema* schema_out) {
  return std::async(
    std::launch::async,
    gpkg_query2,
    con, sql, array_data_out, schema_out
  );
}

std::future<std::string> gpkg_query_async_sexp2(sexp con_sexp, std::string sql,
                                                sexp array_data_xptr, sexp schema_xptr) {
  external_pointer<GPKGConnection> con(con_sexp);

  struct ArrowArray* array_data_out = reinterpret_cast<struct ArrowArray*>(
    R_ExternalPtrAddr(array_data_xptr)
  );
  struct ArrowSchema* schema_out = reinterpret_cast<struct ArrowSchema*>(
    R_ExternalPtrAddr(schema_xptr)
  );

  return gpkg_query_async2(con->ptr, sql, array_data_out, schema_out);
}

[[cpp11::register]]
void gpkg_cpp_query2(list con_sexp, strings sql, list array_data_xptr, list schema_xptr) {
  std::vector<std::future<std::string>> futures;

  for (R_xlen_t i = 0; i < con_sexp.size(); i++) {
    futures.push_back(
      gpkg_query_async_sexp2(
        con_sexp[i],
        sql[i],
        array_data_xptr[i],
        schema_xptr[i]
      )
    );
  }

  std::string error_message("");
  SEXP interrupt_error = R_NilValue;

  while (!futures.empty() && error_message == "" && interrupt_error == R_NilValue) {
    const auto& fut = futures.back();
    auto status = fut.wait_for(std::chrono::milliseconds(500));

    if (status == std::future_status::ready) {
      auto& fut_read = futures.back();
      std::string result = fut_read.get();
      futures.pop_back();
      if (result != "") {
        error_message = result;
        break;
      }
    }

    try {
      check_user_interrupt();
    } catch (unwind_exception& e) {
      interrupt_error = e.token;
    }
  } ;

  // cancel everything left and wait for the futures to finish
  for (size_t i = 0; i < futures.size(); i++) {
    external_pointer<GPKGConnection> con(con_sexp[static_cast<R_xlen_t>(i)]);
    sqlite3_interrupt(con->ptr);
  }

  for (const auto& fut: futures) {
    fut.wait();
  }

  if (interrupt_error != R_NilValue) {
    throw unwind_exception(interrupt_error);
  }

  if (error_message != "") {
    throw GPKGError2(error_message);
  }
}
