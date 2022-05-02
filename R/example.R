
gpkg_example <- function(name) {
  result <- file.path(
    system.file("extdata", package = "gpkg"),
    sprintf("%s.gpkg", name[1])
  )

  if (!file.exists(result)) {
    stop(sprintf("There is no file '%s'", result))
  }

  result
}
