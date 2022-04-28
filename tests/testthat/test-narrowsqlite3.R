
test_that("sqlite3 connections can be opened and closed", {
  con <- sqlite3_open(":memory:")
  expect_s3_class(con, "narrowsqlite3_con")
  sqlite3_close(con)
})
