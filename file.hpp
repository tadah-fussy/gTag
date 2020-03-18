/**
   @file file.hpp
   @brief ファイル操作

   @author tadah_fussy
   @date 2020/01/06 新規作成
**/
#ifndef FILE_20200106_H
#define FILE_20200106_H

#include <iostream>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <stdexcept>

#include <gtk/gtk.h>

#include <boost/filesystem.hpp>

/**
   @brief 文字列の比較

   大文字と小文字・全角と半角を区別しない比較を行う。
   内部では、gtk+の関数 g_utf8_collate を利用している。
**/
struct StrLess
{
  /// @brief 文字列の比較
  ///
  /// @param s1, s2 対象の文字列
  /// @return s1 の方が小さければ true を返す
  bool operator()( const std::string& s1, const std::string& s2 ) const
  {
    gchar* gc1 = g_utf8_casefold( s1.c_str(), -1 );
    gchar* gc2 = g_utf8_casefold( s2.c_str(), -1 );

    gint res = g_utf8_collate( gc1, gc2 );

    g_free( gc1 );
    g_free( gc2 );

    return( res < 0 );
  }
};

using FileData = std::map< boost::filesystem::path, std::set< std::string, StrLess > >;
using TagData = std::map< std::string, std::set< boost::filesystem::path >, StrLess >;

/// @brief パス内の全ファイルを探索し、タグ登録する
///
/// パス内にサブディレクトリがある場合、その中も探索する。
/// パスが存在しない場合、例外 runtime_error を投げる。
///
/// @param rootPath パス名
/// @param fileData ファイルをキーとするタグリストへのポインタ
/// @param tagData タグをキーとするファイルリストへのポインタ
/// @return なし
void InitTagData( const std::string& rootPath, FileData* fileData, TagData* tagData );

/// @brief ファイルからタグを読み込む
///
/// ファイルが存在しない場合、オープンに失敗した場合、ルートパスの取得に失敗した場合、
/// ルートパスが存在しない場合は例外 runtime_error を投げる。
///
/// @param fileName 読み込むファイルのファイル名
/// @param fileData ファイルをキーとするタグリストへのポインタ
/// @param tagData タグをキーとするファイルリストへのポインタ
/// @return なし
void ReadTagData( const std::string& fileName, std::string* rootPath, FileData* fileData, TagData* tagData );

/// @brief ファイルにタグを書き込む
///
/// @param fileNamw 書き込むファイルのファイル名
/// @param rootPath データがある対象のパス名
/// @param fileData 書き込むタグ
/// @return なし
void WriteTagData( const std::string& fileName, const std::string& rootPath, const FileData& fileData );

#endif
