
<!-- README.md is generated from README.Rmd. Please edit that file -->

# gpkg

<!-- badges: start -->

[![R-CMD-check](https://github.com/paleolimbot/gpkg/workflows/R-CMD-check/badge.svg)](https://github.com/paleolimbot/gpkg/actions)
<!-- badges: end -->

The goal of gpkg is to provide a proof-of-concept reader for SQLite
queries into [Arrow C Data
interface](https://arrow.apache.org/docs/format/CDataInterface.html)
structures.

## Installation

You can install the development version of gpkg from
[GitHub](https://github.com/) with:

``` r
# install.packages("remotes")
remotes::install_github("paleolimbot/gpkg")
```

## Example

This is a basic example which shows you how to solve a common problem:

``` r
library(gpkg)
read_gpkg_sf(gpkg_example("nc"))
#> Simple feature collection with 100 features and 2 fields
#> Geometry type: MULTIPOLYGON
#> Dimension:     XY
#> Bounding box:  xmin: -84.32385 ymin: 33.88199 xmax: -75.45698 ymax: 36.58965
#> Geodetic CRS:  NAD27
#> # A tibble: 100 × 3
#>      fid row_num                                                            geom
#>    <int>   <int>                                              <MULTIPOLYGON [°]>
#>  1     1       1 (((-81.47276 36.23436, -81.54084 36.27251, -81.56198 36.27359,…
#>  2     2       2 (((-81.23989 36.36536, -81.24069 36.37942, -81.26284 36.40504,…
#>  3     3       3 (((-80.45634 36.24256, -80.47639 36.25473, -80.53688 36.25674,…
#>  4     4       4 (((-76.00897 36.3196, -76.01735 36.33773, -76.03288 36.33598, …
#>  5     5       5 (((-77.21767 36.24098, -77.23461 36.2146, -77.29861 36.21153, …
#>  6     6       6 (((-76.74506 36.23392, -76.98069 36.23024, -76.99475 36.23558,…
#>  7     7       7 (((-76.00897 36.3196, -75.95718 36.19377, -75.98134 36.16973, …
#>  8     8       8 (((-76.56251 36.34057, -76.60424 36.31498, -76.64822 36.31532,…
#>  9     9       9 (((-78.30876 36.26004, -78.28293 36.29188, -78.32125 36.54553,…
#> 10    10      10 (((-80.02567 36.25023, -80.45301 36.25709, -80.43531 36.55104,…
#> # … with 90 more rows
```
