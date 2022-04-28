
#' SQLite3 Interface
#'
#' @param file A filename or :memory: for an in-memory database.
#' @param con A connection opened using [sqlite3_open()]
#' @param sql An SQL statement
#'
#' @export
#'
sqlite3_open <- function(file = ":memory:") {
  structure(sqlite_cpp_open(file), class = "narrowsqlite3_con")
}

#' @rdname sqlite3_open
#' @export
sqlite3_close <- function(con) {
  sqlite_cpp_close(con)
}

#' @rdname sqlite3_open
#' @export
sqlite3_exec <- function(con, sql) {
  stopifnot(inherits(con, "narrowsqlite3_con"))
  sql <- paste(sql, collapse = ";")
  sqlite_cpp_exec(con, sql)
}

#' @rdname sqlite3_open
#' @export
sqlite3_open_test <- function() {
  con <- sqlite3_open()
  sqlite3_exec(
    con,
    c(
      "CREATE TABLE crossfit (exercise text,dificulty_level int)",
      "insert into crossfit values ('Push Ups', 3), ('Pull Ups', 5) , (' Push Jerk', 7), ('Bar Muscle Up', 10)"
    )
  )

  con
}
