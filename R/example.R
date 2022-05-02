
#' GeoPackage Example Files
#'
#' @param name The example name.
#'
#' @return A filename or an error if the example does not exist
#' @export
#'
#' @examples
#' gpkg_example("point")
#'
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
