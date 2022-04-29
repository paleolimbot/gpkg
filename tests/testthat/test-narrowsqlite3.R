
test_that("sqlite3 connections can be opened and closed", {
  con <- sqlite3_open(":memory:")
  expect_s3_class(con, "narrowsqlite3_con")
  sqlite3_close(con)
})

test_that("sqlite_exec() executes SQL", {
  con <- sqlite3_open()
  on.exit(sqlite3_close(con))

  expect_identical(
    sqlite3_exec(con, "CREATE TABLE crossfit (exercise text,dificulty_level int)"),
    0L
  )
  expect_error(sqlite3_exec(con, "not sql"), "syntax error")
})

test_that("sqlite_open_test() returns a connection", {
  expect_s3_class(sqlite3_open_test(), "narrowsqlite3_con")

})

test_that("sqlite_query() errors for invalid SQL", {
  con <- sqlite3_open_test()
  on.exit(sqlite3_close(con))

  expect_error(sqlite3_query(con, "not sql"), "SQL logic error")
})

test_that("sqlite_query() can read to data.frame", {
  con <- sqlite3_open_test()
  on.exit(sqlite3_close(con))

  expect_identical(
    sqlite3_query(con, "SELECT * from crossfit"),
    data.frame(
      exercise = c("Push Ups", "Pull Ups", "Push Jerk", "Bar Muscle Up"),
      difficulty_level = c(3L, 5L, 7L, 10L),
      stringsAsFactors = FALSE
    )
  )
})

test_that("sqlite_query() can read to RecordBatch", {
  con <- sqlite3_open_test()
  on.exit(sqlite3_close(con))

  expect_identical(
    as.data.frame(as.data.frame(sqlite3_query_table(con, "SELECT * from crossfit"))),
    data.frame(
      exercise = c("Push Ups", "Pull Ups", "Push Jerk", "Bar Muscle Up"),
      difficulty_level = c(3L, 5L, 7L, 10L),
      stringsAsFactors = FALSE
    )
  )
})

