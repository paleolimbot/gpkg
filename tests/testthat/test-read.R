
test_that("gpkg_contents works", {
  expect_identical(gpkg_contents(gpkg_example("nc"))$identifier, "nc")
})

test_that("read_gpkg_sf() works", {
  skip_if_not_installed("arrow")
  skip_if_not_installed("sf")

  nc <- read_gpkg_sf(gpkg_example("nc"))
  expect_s3_class(nc, "sf")
  expect_s3_class(nc$geom, "sfc_MULTIPOLYGON")
})

test_that("read_gpkg_sf() works for all example files", {
  examples <- list.files(
    system.file("extdata", package = "gpkg"),
    full.names = TRUE
  )

  for (ex in examples) {
    expect_s3_class(read_gpkg_sf(!! ex), "sf")
  }
})
