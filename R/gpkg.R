
#' SQLite3 Interface
#'
#' @param file A filename or :memory: for an in-memory database.
#' @param con A connection opened using [gpkg_open()]
#' @param sql An SQL statement
#'
#' @export
#'
gpkg_open <- function(file = ":memory:") {
  if (!identical(file, ":memory:")) {
    file <- path.expand(file)
  }

  structure(
    gpkg_cpp_open(file),
    file = file,
    class = "gpkg_con"
  )
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
  dfs <- lapply(gpkg_query_narrow(con, sql), narrow::from_narrow_array)
  do.call(rbind, dfs)
}

#' @rdname gpkg_open
#' @export
gpkg_query_narrow <- function(con, sql) {
  stopifnot(inherits(con, "gpkg_con"))

  sql <- as.character(sql)
  if (length(sql) == 0) {
    return(list())
  }

  # we need copies of the connection to execute using multiple threads
  con_list <- rep_len(list(con), length(sql))
  con_list[-1] <- list(NULL)

  # clean up the extras no matter what
  on.exit({
    lapply(con_list[-1], function(c) if (!is.null(c)) gpkg_close(c))
  })

  con_list[-1] <- lapply(con_list[-1], function(c) {
    file <- attr(con, "file")
    stopifnot(file != ":memory:")
    gpkg_open(file)
  })

  # allocate space for all the results
  array_data <- lapply(seq_along(sql), function(i) {
    narrow::narrow_allocate_array_data()
  })

  schema <- lapply(seq_along(sql), function(i) {
    narrow::narrow_allocate_schema()
  })

  gpkg_cpp_query(
    con_list,
    sql,
    array_data,
    schema
  )

  lapply(seq_along(sql), function(i) {
    narrow::narrow_array(schema[[i]], array_data[[i]])
  })
}

#' @rdname gpkg_open
#' @export
gpkg_query_table <- function(con, sql) {
  batches <- lapply(
    gpkg_query_narrow(con, sql),
    narrow::from_narrow_array,
    arrow::RecordBatch
  )

  arrow::arrow_table(!!! batches)
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
