
#' GeoPackage Example Files
#'
#' Current example files are 'nc', geometrycollection_m', 'geometrycollection_z',
#' 'geometrycollection_zm', 'geometrycollection', 'linestring_m',
#' 'linestring_z', 'linestring_zm', 'linestring', 'multilinestring_m',
#' 'multilinestring_z', 'multilinestring_zm', 'multilinestring', 'multipoint_m',
#' 'multipoint_z', 'multipoint_zm', 'multipoint', 'multipolygon_m',
#' 'multipolygon_z', 'multipolygon_zm', 'multipolygon', 'point_m',
#' 'point_z', 'point_zm', 'point', 'polygon_m', 'polygon_z', 'polygon_zm',
#' and 'polygon'.
#'
#' @param name The example name.
#'
#' @return A filename or an error if the example does not exist
#' @export
#'
#' @examples
#' gpkg_example("nc")
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
