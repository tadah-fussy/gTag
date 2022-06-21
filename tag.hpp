/**
   @file tag.hpp
   @brief タグの定義

   @author tadah_fussy
   @date 2022/06/21 新規作成
**/

#include <vector>
#include <map>
#include <string>

#include <cassert>

/**
 * @brief 属性タグ
 * 
 */
class Tag
{
   public:

   /// @brief タグ名とキーを指定して構築
   ///
   /// キーは 0 より大きくなければならない。
   /// 0 の場合、assert を実行する。
   ///
   /// @param name タグ名
   /// @param id 一意のキー ( >0 )
   Tag( const std::string& name, size_t id )
   : name_( name ), id_( id )
   { assert( id_ > 0 ); }

   /// @brief タグ名への参照を返す
   ///
   /// @return タグ名への参照
   std::string& name() { return( name_ ); }

   /// @brief タグ名への定数参照を返す
   ///
   /// @return タグ名への定数参照
   const std::string& name() const { return( name_ ); }

   /// @brief id を返す
   ///
   /// @return id
   size_t id() const { return( id_ ); }

   /// @brief 親タグのセット
   ///
   /// @param parent 親タグへの参照
   void setParent( const Tag& parent )
   { if ( &parent != this ) parent_ = parent.id(); }

   /// @brief 親タグのリセット
   ///
   void resetParent() { parent_ = 0; }

   private:

   std::string name_;   // タグ名
   size_t id_;          // 一意のキー ( >0 )
   size_t parent_;      // 親タグの id ( 親がない場合は 0 )
};

/**
 * @brief 画像ファイル情報
 * 
 */
class ImageFile
{
   public:

   private:

   std::string name_;   // ファイル名(フルパス)
   
};

class TagList
{
   public:

   private:

   std::map< size_t, std::vector< size_t > > tag2File_;
   std::map< size_t, std::vector< size_t > > file2Tag_;
   std::vector< std::string > tagList_;
   std::vector< std::string > fileList_;
};