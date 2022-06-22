/**
   @file tag.hpp
   @brief タグの定義

   @author tadah_fussy
   @date 2022/06/21 新規作成
**/

#include <vector>
#include <map>
#include <unordered_set>
#include <string>

#include <boost/filesystem.hpp>

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

   /// @brief 等号演算子の多重定義
   ///
   /// @param tag 比較対象
   /// @return 等しければ true
   bool operator==( const Tag& tag )
   { return( id_ == tag.id_ ); }

   private:

   std::string name_;   // タグ名
   size_t id_;          // 一意のキー ( >0 )
   size_t parent_;      // 親タグの id ( 親がない場合は 0 )
};

namespace std
{
   /**
    * @brief Tag 用ハッシュ関数オブジェクト
    * 
    */
   template<>
   struct hash< Tag >
   {
      using result_type = hash< size_t >::result_type;
      using argument_type = Tag;

      /// @brief Tag 用ハッシュ計算
      ///
      /// @param tag ハッシュ値計算対象
      /// @return ハッシュ値
      result_type operator()( const argument_type& tag )
      { return( hash< size_t >().operator()( tag.id() ) ); }
   };

   /**
    * @brief boost::filesystem::path 用ハッシュ関数オブジェクト
    * 
    */
   template<>
   struct hash< ::boost::filesystem::path >
   {
      using result_type = size_t;
      using argument_type = ::boost::filesystem::path;

      /// @brief Tag 用ハッシュ計算
      ///
      /// @param tag ハッシュ値計算対象
      /// @return ハッシュ値
      result_type operator()( const argument_type& path )
      { return( ::boost::filesystem::hash_value( path ) ); }
   };

} // namespace std

class TagList
{
   public:

   /// @brief デフォルト・コンストラクタ
   ///
   /// 空のリストを作成する
   TagList() {}

   /// @brief ルート・ディレクトリからファイル情報だけを読み込む
   ///
   /// @param root ルート・ディレクトリ
   TagList( const boost::filesystem::path& root );

   /// @brief 入力ストリームからファイル情報とタグ情報を読み込む
   ///
   /// ルート・ディレクトリ内の情報と整合を取る
   TagList( const std::istream& is )

   private:

   std::map< size_t, std::unordered_set< size_t > > tag2File_;  // タグに対するファイル一覧
   std::map< size_t, std::unordered_set< size_t > > file2Tag_;  // ファイルに対するタグ一覧
   std::unordered_set< Tag > tagList_;  // タグのリスト
   std::unordered_set< boost::filesystem::path > fileList_;  // ファイルのリスト
};