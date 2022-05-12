
test_that("sqlite3 connections can be opened and closed", {
  con <- gpkg_open(":memory:")
  expect_s3_class(con, "gpkg_con")
  gpkg_close(con)
})

test_that("gpkg_exec() executes SQL", {
  con <- gpkg_open()
  on.exit(gpkg_close(con))

  expect_identical(
    gpkg_exec(con, "CREATE TABLE crossfit (exercise text, difficulty_level int)"),
    0L
  )
  expect_error(gpkg_exec(con, "not sql"), "syntax error")
})

test_that("gpkg_open_test() returns a connection", {
  expect_s3_class(gpkg_open_test(), "gpkg_con")

})

test_that("gpkg_query() errors for invalid SQL", {
  con <- gpkg_open_test()
  on.exit(gpkg_close(con))

  expect_error(gpkg_query(con, "not sql"), "SQL logic error")
})

test_that("gpkg_query_narrow() can read in parallel", {
  con <- gpkg_open(gpkg_example("nc"))
  query <- "SELECT row_num from nc"

  expect_identical(
    gpkg_query_narrow(con, character()),
    list()
  )

  result1 <- gpkg_query_narrow(con, query)[[1]]
  expect_identical(result1$array_data$length, 100L)

  result10 <- gpkg_query_narrow(con, rep(query, 10))
  expect_identical(
    lapply(result10, narrow::from_narrow_array),
    rep(list(narrow::from_narrow_array(result1)), 10)
  )
})

test_that("gpkg_query_narrow() can error in parallel", {
  con <- gpkg_open(gpkg_example("nc"))
  on.exit(gpkg_close(con))

  query <- "SELECT row_num from nc"
  expect_error(
    gpkg_query_narrow(con, c("NOT SQL", rep(query, 10))),
    "SQL logic error"
  )
})


test_that("gpkg_query() can read to data.frame", {
  con <- gpkg_open_test()
  on.exit(gpkg_close(con))

  expect_identical(
    gpkg_query(con, "SELECT * from crossfit"),
    data.frame(
      exercise = c("Push Ups", "Pull Ups", "Push Jerk", "Bar Muscle Up"),
      difficulty_level = c(3L, 5L, 7L, 10L),
      stringsAsFactors = FALSE
    )
  )
})

test_that("gpkg_query() can read to data.frame with zero rows", {
  con <- gpkg_open_test()
  on.exit(gpkg_close(con))

  expect_identical(
    gpkg_query(con, "SELECT * from crossfit WHERE 0"),
    data.frame(
      exercise = character(),
      # probs should be integer
      difficulty_level = character(),
      stringsAsFactors = FALSE
    )
  )
})

test_that("gpkg_query_table() can read to Table", {
  con <- gpkg_open_test()
  on.exit(gpkg_close(con))

  expect_identical(
    as.data.frame(as.data.frame(gpkg_query_table(con, "SELECT * from crossfit"))),
    data.frame(
      exercise = c("Push Ups", "Pull Ups", "Push Jerk", "Bar Muscle Up"),
      difficulty_level = c(3L, 5L, 7L, 10L),
      stringsAsFactors = FALSE
    )
  )
})

test_that("gpkg_list_tables() lists tables", {
  con <- gpkg_open()
  expect_identical(gpkg_list_tables(con), character())
  gpkg_close(con)

  con <- gpkg_open_test()
  on.exit(gpkg_close(con))
  expect_identical(gpkg_list_tables(con), "crossfit")
})

test_that("gpkg_query_table() can decode the geometry column", {
  skip_if_not_installed("arrow")

  for (ex in c("point", "linestring", "linestring_z")) {
    con <- gpkg_open(gpkg_example(ex))
    table <- gpkg_query_table(con, sprintf("SELECT * FROM %s", ex))
    wkb <- wk::wkb(unclass(table$geom$chunk(0)$storage()$as_vector()))
    expect_identical(wk::validate_wk_wkb(wkb), wkb)

    array <- gpkg_query_narrow(con, sprintf("SELECT * FROM %s", ex))[[1]]
    expect_identical(
      array$schema$children[[2]]$metadata$`ARROW:extension:name`,
      "geoarrow.wkb"
    )
    gpkg_close(con)
  }
})
