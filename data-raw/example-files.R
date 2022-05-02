
example_wkt <- geoarrow::geoarrow_example_wkt

# eventually we'll have to not use sf because it drops NULLs?
example_tbl <- lapply(
  example_wkt,
  function(ex) {
    sfc <- wk::wk_handle(ex, wk::sfc_writer())
    wk::wk_crs(sfc) <- wk::wk_crs(ex)
    sf::st_as_sf(tibble::tibble(row_num = seq_along(ex), geometry = sfc))
  }
)

for (name in names(example_tbl)) {
  sf::write_sf(example_tbl[[name]], glue::glue("inst/extdata/{name}.gpkg"))
}
