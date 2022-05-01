// Generated by cpp11: do not edit by hand
// clang-format off


#include "cpp11/declarations.hpp"
#include <R_ext/Visibility.h>

// gpkg.cpp
cpp11::sexp gpkg_cpp_open(std::string filename);
extern "C" SEXP _gpkg_gpkg_cpp_open(SEXP filename) {
  BEGIN_CPP11
    return cpp11::as_sexp(gpkg_cpp_open(cpp11::as_cpp<cpp11::decay_t<std::string>>(filename)));
  END_CPP11
}
// gpkg.cpp
void gpkg_cpp_close(cpp11::sexp con_sexp);
extern "C" SEXP _gpkg_gpkg_cpp_close(SEXP con_sexp) {
  BEGIN_CPP11
    gpkg_cpp_close(cpp11::as_cpp<cpp11::decay_t<cpp11::sexp>>(con_sexp));
    return R_NilValue;
  END_CPP11
}
// gpkg.cpp
int gpkg_cpp_exec(cpp11::sexp con_sexp, std::string sql);
extern "C" SEXP _gpkg_gpkg_cpp_exec(SEXP con_sexp, SEXP sql) {
  BEGIN_CPP11
    return cpp11::as_sexp(gpkg_cpp_exec(cpp11::as_cpp<cpp11::decay_t<cpp11::sexp>>(con_sexp), cpp11::as_cpp<cpp11::decay_t<std::string>>(sql)));
  END_CPP11
}
// gpkg.cpp
void gpkg_cpp_query_all(sexp con_sexp, std::string sql, sexp array_data_xptr, sexp schema_xptr);
extern "C" SEXP _gpkg_gpkg_cpp_query_all(SEXP con_sexp, SEXP sql, SEXP array_data_xptr, SEXP schema_xptr) {
  BEGIN_CPP11
    gpkg_cpp_query_all(cpp11::as_cpp<cpp11::decay_t<sexp>>(con_sexp), cpp11::as_cpp<cpp11::decay_t<std::string>>(sql), cpp11::as_cpp<cpp11::decay_t<sexp>>(array_data_xptr), cpp11::as_cpp<cpp11::decay_t<sexp>>(schema_xptr));
    return R_NilValue;
  END_CPP11
}

extern "C" {
static const R_CallMethodDef CallEntries[] = {
    {"_gpkg_gpkg_cpp_close",     (DL_FUNC) &_gpkg_gpkg_cpp_close,     1},
    {"_gpkg_gpkg_cpp_exec",      (DL_FUNC) &_gpkg_gpkg_cpp_exec,      2},
    {"_gpkg_gpkg_cpp_open",      (DL_FUNC) &_gpkg_gpkg_cpp_open,      1},
    {"_gpkg_gpkg_cpp_query_all", (DL_FUNC) &_gpkg_gpkg_cpp_query_all, 4},
    {NULL, NULL, 0}
};
}

extern "C" attribute_visible void R_init_gpkg(DllInfo* dll){
  R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
  R_forceSymbols(dll, TRUE);
}
