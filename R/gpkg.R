
#' SQLite3 Interface
#'
#' @param file A filename or :memory: for an in-memory database.
#' @param con A connection opened using [gpkg_open()]
#' @param sql An SQL statement
#'
#' @export
#'
gpkg_open <- function(file = ":memory:") {
  structure(gpkg_cpp_open(file), class = "gpkg_con")
}

#' @rdname gpkg_open
#' @export
gpkg_close <- function(con) {
  gpkg_cpp_close(con)
}

#' @rdname gpkg_open
#' @export
gpkg_exec <- function(con, sql) {
  stopifnot(inherits(con, "gpkg_con"))
  sql <- paste(sql, collapse = ";")
  gpkg_cpp_exec(con, sql)
}

#' @rdname gpkg_open
#' @export
gpkg_query <- function(con, sql) {
  narrow::from_narrow_array(
    gpkg_query_narrow(con, sql)
  )
}

#' @rdname gpkg_open
#' @export
gpkg_query_narrow <- function(con, sql) {
  stopifnot(inherits(con, "gpkg_con"))
  array_data <- narrow::narrow_allocate_array_data()
  schema <- narrow::narrow_allocate_schema()

  gpkg_cpp_query_all(con, sql, array_data, schema)
  narrow::narrow_array(schema, array_data)
}

#' @rdname gpkg_open
#' @export
gpkg_query_table <- function(con, sql) {
  batch <- narrow::from_narrow_array(
    gpkg_query_narrow(con, sql),
    arrow::RecordBatch
  )
  arrow::arrow_table(batch)
}

#' @rdname gpkg_open
#' @export
gpkg_open_test <- function() {
  con <- gpkg_open()
  gpkg_exec(
    con,
    c(
      "CREATE TABLE crossfit (exercise text,difficulty_level int)",
      "insert into crossfit values ('Push Ups', 3), ('Pull Ups', 5) , ('Push Jerk', 7), ('Bar Muscle Up', 10)"
    )
  )

  con
}

#' @rdname gpkg_open
#' @export
gpkg_list_tables <- function(con) {
  gpkg_query(
    con,
    "SELECT name FROM sqlite_schema WHERE type = 'table' AND name NOT LIKE 'sqlite_%';"
  )$name
}
