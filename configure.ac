AC_INIT(pstat, 0.5, mazen.abdulaziz@gmail.com)
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([src/config.h])

CXXFLAGS="$CXXFLAGS -std=c++0x"
LDFLAGS="-pthread"
CXX=$CXX

# Checks for programs.
AC_PROG_CXX
AC_LANG([C++])

# Checks for header files.
AC_CHECK_HEADER([tbb/tbb.h],[have_tbb=1],[have_tbb=0])

if test $have_tbb == 0
then
	AC_MSG_WARN([cannot find Intel Thread Building Blocks (TBB) library. Performance of pstat will 
be massively degraded without TBB. Please consider installing it.])
else
	AC_DEFINE([HAVE_TBB_HEADERS_], 1, [Define 1 if TBB was found])
	LDFLAGS="-Wl,--no-as-needed -pthread -ltbb"
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_STRUCT_ST_BLOCKS

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MKTIME
AC_CHECK_FUNCS([localtime_r realpath strchr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
