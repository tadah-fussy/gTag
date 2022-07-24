#include "tag.hpp"

using std::string;
using std::unordered_map;
using std::pair;

namespace fs = boost::filesystem;

namespace
{
  /*
    getPointer : map1[key] で ID を取得し、ID と map2[ID] へのポインタを返す
  */
  template< class Res, class Key, class Id,
            template< class, class > class Map >
  pair< Id, Res* > getValue( const Map< Key, Id >& map1, const Map< Id, Res >& map2, const Key& key )
  {
    auto i = map1.find( key );
    if ( i == map1.end() )
      throw std::runtime_error( FORMAT( error_message::KEY_0_NOT_FOUND, key ) );

    auto j = map2.find( i->second );
    assert( j != map2.end() );

    return( std::make_pair( j->first, &( j->second ) ) );
  }
} // namespace

/*
  TagList< TagId, ImageId >::getImageP : path にリンクした Image へのポインタを返す
*/
template< typename TagId, typename ImageId >
pair< ImageId, typename TagList< TagId, ImageId >::image_type* >
TagList< TagId, ImageId >::getImage( const fs::path& path ) const
{ return( getValue( imagePath_, imageList_, path ) ); }

/*
  TagList< TagId, ImageId >::getTagP : content にリンクした Tag へのポインタを返す
*/
template< typename TagId, typename ImageId >
pair< TagId, typename TagList< TagId, ImageId >::tag_type* >
TagList< TagId, ImageId >::getTag( const string& content ) const
{ return( getValue( tagContents_, tagList_, content ) ); }

namespace
{
  template< class Res, class Key, class Id,
            template< class, class > class Map, class Op >
  Res* createValue( const Key& key, Map< Key, Id >* map1, Map< Id, Res >* map2, Id* nextId, Op op )
  {
    auto i = map1->find( key );
    if ( i != map1->end() )
      throw std::runtime_error( FORMAT( error_message::KEY_0_EXIST, key ) );

    ( *map1 )[key] = ++( *nextId );
    auto j = map2->insert( std::make_pair( *nextId, op( *nextId ) ) ).first;

    return( &( j->second ) );
  }

} // namespace

/*
  TagList< TagId, ImageId >::getTag : タグを返す(なければ新規作成)
*/
template< typename TagId, typename ImageId >
typename TagList< TagId, ImageId >::tag_type*
TagList< TagId, ImageId >::createTag( const string& content )
{ return( createValue( content, &tagContents_, &tagList_, &nextTagId_, []( TagId id )->Tag{ retern( Tag{} ); } ) ); }

/*
  TagList< TagId, ImageId >::getImage : 画像を返す(なければ新規作成)
*/
template< typename TagId, typename ImageId >
typename TagList< TagId, ImageId >::image_type*
TagList< TagId, ImageId >::addImage( const boost::filesystem::path& path )
{ return( createValue( path, &imagePath_, &imageList_, &nextImageId_, []( ImageId id )->Image{ return( Image( id ) ); } ) ); }

/*
  TagList< TagId, ImageId >::addTag : path にリンクした Image に content にリンクした Tag を追加する
*/
template< typename TagId, typename ImageId >
void TagList< TagId, ImageId >::addTag( const fs::path& path, const string& content )
{
  pair< ImageId, image_type* > image = getImage( path ); // path にリンクした Image の ID とポインタ
  pair< TagId, tag_type* > tag = createTag( content ); // content にリンクした Tag の ID とポインタ
  
  ( image.second )->addTag( tag.first );
  ( tag.second )->addImage( image.first );
}

/*
  TagList< TagId, ImageId >::eraseTag : path にリンクした Image から content にリンクした Tag を削除する
*/
template< typename TagId, typename ImageId >
void TagList< TagId, ImageId >::eraseTag( const fs::path& path, const string& content )
{
  pair< ImageId, image_type* > image = getImage( path ); // path にリンクした Image の ID とポインタ
  if ( image.first == ImageId{} )
    throw std::runtime_error( FORMAT( error_message::KEY_0_NOT_FOUND, path ) );

  pair< TagId, tag_type* > tag = getTag( content );
  if ( tag.first == TagId{} )
    throw std::runtime_error( FORMAT( error_message::KEY_0_NOT_FOUND, content ) );

  ( image.second )->eraseTag( tag.first );
  ( tag.second )->eraseImage( image.first );
}

namespace
{
  /*
    RenewMapKey : map の oldKey を newKey に入れ替える
  */
  template< class Key, class Value, template< class, class > class Map  >
  void RenewMapKey( Map< Key, Value >* map, const Key& oldKey, const Key& newKey )
  {
    auto it = map->find( oldKey );
    if ( it == map->end() )
      throw std::runtime_error( FORMAT( error_message::KEY_0_NOT_FOUND, oldKey ) );

    if ( map->find( newKey ) != map->end() )
      throw std::runtime_error( FORMAT( error_message::KEY_0_EXIST, newKey ) );

    Value value = it->second;
    map->erase( it );
    (*map)[newKey] = value;
  }
} // namespace

/*
  TagList< TagId, ImageId >::renewTag : タグの内容を変更する
*/
template< typename TagId, typename ImageId >
void TagList< TagId, ImageId >::renewTag( const string& oldContent, const string& newContent )
{ RenewMapKey( &tagContents_, oldContent, newContent ); }

/*
  TagList< TagId, ImageId >::renewPath : 画像のパスの内容を変更する
*/
template< typename TagId, typename ImageId >
void TagList< TagId, ImageId >::renewPath( const boost::filesystem::path& oldPath, const boost::filesystem::path& newPath )
{ RenewMapKey( &imagePath_, oldPath, newPath ); }


