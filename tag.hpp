/**
  @file tag.hpp
  @brief タグの定義

  @author tadah_fussy
  @date 2022/06/21 新規作成
**/

#ifndef TAG_HPP_20220621
#define TAG_HPP_20220621

#include <vector>
#include <map>
#include <unordered_set>
#include <string>

#include <boost/filesystem.hpp>

#include <gtk/gtk.h>

#include <cassert>

#include "global.hpp"
#include "constant.hpp"

/**
 * @brief 画像タグ
 * 
 * @tparam TagId タグIDの型
 * @tparam ImageId 画像IDの型
 */
template< typename TagId, typename ImageId >
class Tag
{
  public:

  using tag_id = TagId;
  using image_id = ImageId;
  using container = typename std::unordered_set< image_id >;
  using const_iterator = container::const_iterator;

  /// @brief デフォルト・コンストラクタ
  Tag( tag_id tagId )
  : id_( tagId ), parent_{}
  { assert( id_ > 0 ); }

  /// @brief タグ ID を返す
  ///
  /// @return タグ ID
  teg_id id() const
  { return( id_ ); }

  /// @brief 親タグのタグ ID を設定する
  ///
  /// @return 親タグのタグ ID
  tag_id parent() const
  { return( parent_ ); }

  /// @brief 親タグを設定する
  ///
  /// @param tagId 親タグのタグ ID
  void setParent( tag_id tagId )
  { parent_ = tagId; }

  /// @brief タグに画像IDを登録する
  ///
  /// @param imageId 登録する画像ID
  void addImage( image_id imageId )
  {
    if ( images_.find( imageId ) == images_.end() )
      images_.insert( imageId );
  }

  /// @brief タグから画像を消去する
  ///
  /// @param imageId 消去する画像ID
  void eraseImage( image_id imageId )
  {
    auto i = images_.find( imageId );
    if ( i != end() )
      images_.erase( i );
  }

  /// @brief 画像リストの開始位置を返す
  ///
  /// @return 画像リストの開始位置
  const_iterator begin() const
  { return( images_.begin() ); }

  /// @brief 画像リストの末尾の次の位置を返す
  ///
  /// @return 画像リストの末尾の次の位置
  const_iterator end() const
  { return( images_.end() ); }

  private:

  tag_id id_;        // タグの ID
  tag_id parent_;    // 親のタグの ID
  container images_; // タグに属する画像
};

template< typename TagId, typename ImageId >
class Image
{
  public:

  using tag_id = TagId;
  using image_id = ImageId;
  using container = typename std::unordered_set< tag_id >;
  using const_iterator = container::const_iterator;

  /// @brief ID を指定して構築
  ///
  /// @param id ID
  Image( image_id id )
  : id_( id ), post_( id )
  { assert( id_ > 0 ); }

  /// @brief 画像の ID を返す
  ///
  /// @return 画像の ID
  image_id id() const
  { return( id_ ); }

  /// @brief グループ連結されている次の画像の ID を返す
  ///
  /// @return グループ連結されている次の画像の ID
  image_id post() const
  { return( post_ ); }

  /// @brief 二つの Image のグループ連結
  ///
  /// グループを持たない Image 同士を連結する。
  /// 片側がグループを持つ場合は assert を実行する。
  ///
  /// @param image1, image2 二つの画像
  static void connect( Image& image1, Image& image2 )
  {
    assert( image1.post() == image1.id() && image2.post() == image2.id() );

    image1.post_ = image2.id();
    image2.post_ = image1.id();
  }

  /// @brief 二つのグループ連結した Image の間に挿入する
  ///
  /// image1 と image2 がグループ連結していないか、自分自身がグループを持つ場合は assert を実行する。
  ///
  /// @param image1, image2 二つのグループ連結した画像
  void insert( Image& image1, Image& image2 )
  {
    assert( image1.post() == image2.id() && post() == id() );

    image1.post_ = id();
    post_ = image2.id();
  }

  /// @brief グループから除外する
  ///
  /// image1 が前、image2 が後にグループ連結していなければ assert を実行する。
  ///
  /// @param image1, image2 自分自身の前後にグループ連結された Image
  void remove( Image& image1, Image& image2 )
  {
    assert( image1.post() == id() && post() == image2.id() );

    image1.post_ = image2.id();
    post_ = id();
  }

  /// @brief 画像にタグを登録する
  ///
  /// @param tag 登録するタグ
  void addTag( tag_id tagId )
  {
    if ( tags_.find( tagId ) == tags_.end() )
      tags_.insert( tagId );
  }

  /// @brief 画像からタグを消去する
  ///
  /// @param tag 消去するタグ
  void eraseTag( tag_id tagId )
  {
    auto i = tags_.find( tagId );
    if ( i != end() )
      tags_.erase( i );
  }

  /// @brief タグリストの開始位置を返す
  ///
  /// @return タグリストの開始位置
  const_iterator begin() const
  { return( tags_.begin() ); }

  /// @brief タグリストの末尾の次の位置を返す
  ///
  /// @return タグリストの末尾の次の位置
  const_iterator end() const
  { return( tags_.end() ); }

  private:

  image_id id_;    // ID
  image_id post_;  // グループの後の画像ID ( グループに属さない場合自分自身のID )
  container tags_; // 画像にリンクされたタグのリスト
};

namespace std
{
  /**
   * @brief boost::filesystem::path 用ハッシュ関数オブジェクト
   * 
   */
  template<>
  struct hash< ::boost::filesystem::path >
  {
    using result_type = size_t;

    /// @brief boost::filesystem::path 用ハッシュ計算
    ///
    /// @param path ハッシュ値計算対象
    /// @return ハッシュ値
    result_type operator()( const ::boost::filesystem::path& path )
    { return( ::boost::filesystem::hash_value( path ) ); }

  };

} // namespace std

/**
   @brief 文字列の比較

   大文字と小文字・全角と半角を区別しない比較を行う。
   内部では、gtk+の関数 g_utf8_collate を利用している。
**/
template< class String >
struct StrLess
{
  /// @brief 文字列の比較
  ///
  /// @param s1, s2 対象の文字列
  /// @return s1 の方が小さければ true を返す
  bool operator()( const String& s1, const String& s2 ) const
  {
    using value_type = typename String::value_type;

    value_type* gc1 = g_utf8_casefold( s1.c_str(), -1 );
    value_type* gc2 = g_utf8_casefold( s2.c_str(), -1 );

    gint res = g_utf8_collate( gc1, gc2 );

    g_free( gc1 );
    g_free( gc2 );

    return( res < 0 );
  }
};

template< typename TagId, typename ImageId >
class TagList
{
  public:

  using tag_type = Tag< TagId, ImageId >;
  using image_type = Image< TagId, ImageId >;
  using tag_contents = std::map< std::string, TagId, StrLess< std::string > >;
  using image_path = std::map< boost::filesystem::path, ImageId, StrLess< boost::filesystem::path > >;
  using contents_iterator = typename tag_contents::iterator;
  using const_contents_iterator = typename tag_contents::const_iterator;
  using path_iterator = typename image_path::iterator;
  using const_path_iterator = typename image_path::const_iterator;

  /// @brief デフォルト・コンストラクタ
  ///
  /// 空のリストを作成する
  TagList() : nextTagId_{}, nextImageId_{} {}

  /// @brief 新たなタグを作成する
  ///
  /// @param content タグの内容
  /// @return 新たなタグへのポインタ
  tag_type* createTag( const std::string& content );

  /// @brief 新たな画像を登録する
  ///
  /// @param path 画像ファイルのパス
  /// @return 対象の画像へのポインタ
  image_type* addImage( const boost::filesystem::path& path );

  /// @brief 画像にタグを登録する
  ///
  /// 新規のタグの場合、タグ作成を行う。
  ///
  /// @param path 画像ファイルのパス
  /// @param content タグの内容
  void addTag( const boost::filesystem::path& path, const std::string& content );

  /// @brief 画像からタグを消去する
  ///
  /// @param path 画像ファイルのパス
  /// @param content 消去対象のタグの内容
  void eraseTag( const boost::filesystem::path& path, const std::string& content );

  /// @brief タグの内容を書き換える
  ///
  /// @param oldContent 現在のタグの内容
  /// @param newContent 新たなタグの内容
  void renewTag( const std::string& oldContent, const std::string& newContent );

  /// @brief 画像のパスを書き換える
  ///
  /// @param oldPath 現在のパスの内容
  /// @param newPath 新たなパスの内容
  void renewPath( const boost::filesystem::path& oldPath, const boost::filesystem::path& newPath );

  private:

  TagId nextTagId_;     // 次のタグにつける ID
  ImageId nextImageId_; // 次の画像につける ID
  tag_contents tagContents_; // タグ内容
  std::unordered_map< TagId, tag_type > tagList_;        // タグのリスト
  image_path imagePath_; // 画像ファイルのパス
  std::unordered_map< ImageId, image_type > imageList_;             // 画像のリスト

  // 指定した path をキーとする ID と Image へのポインタを返す
  std::pair< ImageId, image_type* > getImage( const boost::filesystem::path& path ) const;

  // 指定した content をキーとする ID と Tag へのポインタを返す
  std::pair< TagId, tag_type* > getTag( const std::string& content ) const;
};

#endif