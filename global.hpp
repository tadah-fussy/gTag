/**
  @file global.hpp
  @brief グローバル変数・関数の定義

  @author tadah_fussy
  @date 2022/07/17 新規作成
**/

#ifndef GLOBAL_HPP_20220717
#define GLOBAL_HPP_20220717

#include <boost/format.hpp>

#define FORMAT( expr, var ) ( boost::format( expr ) % var ).str()

#endif