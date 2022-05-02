
#' Read a GeoPackage
#'
#' @param file A path to a .gpkg file
#' @param layer A name of a layer or NULL for the first layer
#'
#' @return
#'   - `read_gpkg_sf()` returns an [sf object][sf::st_sf]
#'   - `read_gpkg_table()` returns an [arrow::Table]
#' @export
#'
#' @examplesIf requireNamespace("arrow", quietly = TRUE) && requireNamespace("sf", quietly = TRUE)
#' read_gpkg_sf(gpkg_example("nc"))
#'
read_gpkg_sf <- function(file, layer = NULL) {
  table <- read_gpkg_table(file, layer)
  tbl <- geoarrow::geoarrow_collect(table, handler = wk::sfc_writer())
  sf::st_as_sf(tbl)
}

#' @rdname read_gpkg_sf
#' @export
read_gpkg_table <- function(file, layer = NULL) {
  batches <- read_gpkg_base(file, layer)
  batches <- lapply(batches, narrow::from_narrow_array, arrow::RecordBatch)
  arrow::arrow_table(!!! batches)
}

read_gpkg_base <- function(file, layer = NULL) {
  if (!inherits(file, "gpkg_con")) {
    con <- gpkg_open(file)
    on.exit(gpkg_close(con))
  } else {
    con <- file
  }

  contents <- gpkg_contents(con)
  if (is.null(layer)) {
    layer <- contents$identifier[1]
  } else {
    stopifnot(is.character(layer), length(layer) == 1)
  }

  srs_query <- sprintf(
    "SELECT table_name, column_name, gpkg_spatial_ref_sys.srs_id, definition
     FROM gpkg_geometry_columns
     INNER JOIN gpkg_spatial_ref_sys on gpkg_spatial_ref_sys.srs_id = gpkg_geometry_columns.srs_id
     WHERE table_name = '%s'",
    layer
  )

  result <- gpkg_query_narrow(
    con,
    c(
      srs_query,
      sprintf("SELECT * from %s", layer)
    )
  )

  col_srs <- narrow::from_narrow_array(result[[1]])
  lapply(result[-1], function(array) {
    for (i in seq_along(array$schema$children)) {
      child <- array$schema$children[[i]]
      if (identical(child$metadata[["ARROW:extension:name"]], "geoarrow.wkb")) {
        srs_i <- match(child$name, col_srs$column_name)
        srs <- col_srs$definition[srs_i]
        if (!is.na(srs) && !identical(srs, "undefined")) {
          wk::wk_crs(array) <- srs
        }
      }
    }

    array
  })
}

#' @rdname read_gpkg_sf
#' @export
gpkg_contents <- function(file) {
  if (!inherits(file, "gpkg_con")) {
    con <- gpkg_open(file)
    on.exit(gpkg_close(con))
  } else {
    con <- file
  }

  gpkg_query(con, "SELECT * from gpkg_contents")
}
