
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

This is a basic example which shows you how to solve a common problem
(test SQL courtesy of the [duckdb](https://github.com/duckdb/duckdb/)
tests):

``` r
library(gpkg)

con <- gpkg_open()
gpkg_exec(con, "CREATE TABLE crossfit (exercise text, difficulty_level int)")
#> [1] 0
gpkg_exec(con, "INSERT INTO crossfit VALUES ('Push Ups', 3), ('Pull Ups', 5) , ('Push Jerk', 7), ('Bar Muscle Up', 10)")
#> [1] 0

# Query to Table
gpkg_query_table(con, "SELECT * from crossfit where difficulty_level >= 5")
#> Table
#> 3 rows x 2 columns
#> $exercise <string>
#> $difficulty_level <int32>

# ...or directly to data.frame:
gpkg_query(con, "SELECT * from crossfit where difficulty_level >= 5")
#>        exercise difficulty_level
#> 1      Pull Ups                5
#> 2     Push Jerk                7
#> 3 Bar Muscle Up               10
```
