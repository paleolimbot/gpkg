
sqlite3_open <- function(file = ":memory:") {
  structure(sqlite_cpp_open(file), class = "narrowsqlite3_con")
}

sqlite3_close <- function(con) {
  sqlite_cpp_close(con)
}
