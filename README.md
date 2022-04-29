
<!-- README.md is generated from README.Rmd. Please edit that file -->

# narrowsqlite3

<!-- badges: start -->

[![R-CMD-check](https://github.com/paleolimbot/narrowsqlite3/workflows/R-CMD-check/badge.svg)](https://github.com/paleolimbot/narrowsqlite3/actions)
<!-- badges: end -->

The goal of narrowsqlite3 is to provide a proof-of-concept reader for
SQLite queries into [Arrow C Data
interface](https://arrow.apache.org/docs/format/CDataInterface.html)
structures.

## Installation

You can install the development version of narrowsqlite3 from
[GitHub](https://github.com/) with:

``` r
# install.packages("remotes")
remotes::install_github("paleolimbot/narrowsqlite3")
```

## Example

This is a basic example which shows you how to solve a common problem
(test SQL courtesy of the [duckdb](https://github.com/duckdb/duckdb/)
tests):

``` r
library(narrowsqlite3)

con <- sqlite3_open()
sqlite3_exec(con, "CREATE TABLE crossfit (exercise text, difficulty_level int)")
#> [1] 0
sqlite3_exec(con, "INSERT INTO crossfit VALUES ('Push Ups', 3), ('Pull Ups', 5) , ('Push Jerk', 7), ('Bar Muscle Up', 10)")
#> [1] 0

# Query to Table
sqlite3_query_table(con, "SELECT * from crossfit where difficulty_level >= 5")
#> Table
#> 3 rows x 2 columns
#> $exercise <string>
#> $difficulty_level <int32>

# ...or directly to data.frame:
sqlite3_query(con, "SELECT * from crossfit where difficulty_level >= 5")
#>        exercise difficulty_level
#> 1      Pull Ups                5
#> 2     Push Jerk                7
#> 3 Bar Muscle Up               10
```
