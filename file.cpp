/**
   file.cpp : ファイル操作用関数
**/
#include "file.hpp"

using std::string;
using std::map;
using std::set;
using std::vector;

using std::ifstream;
using std::ofstream;
using std::endl;

namespace fs = boost::filesystem;

/*
  InitTagData : rootPath 内の全ファイルに対してリスト fileData と tagData を作成する
*/
void InitTagData( const string& rootPath, FileData* fileData, TagData* tagData )
{
  fs::path p( rootPath );

  if ( ! fs::exists( p ) )
    throw std::runtime_error( "指定したパスは存在しません。" );

  fileData->clear();
  tagData->clear();
  for ( auto rdi = fs::recursive_directory_iterator( p ) ;
        rdi != fs::recursive_directory_iterator() ; ++rdi ) {
    if ( fs::is_directory( *rdi ) ) continue;
    fileData->insert( std::make_pair( *rdi, set< string, StrLess >() ) );
  }
}

/*
  GetValueFromKey : data のキーが key であるかチェックし、そうなら value に値を登録する

  戻り値 : data のキーが key であるなら true を返す
*/
bool GetValueFromKey( const string& data, const string& key, string* value )
{
  string::size_type keyLen = key.length();

  if ( data.compare( 0, keyLen, key ) != 0 )
    return( false );

  *value = data.substr( keyLen );

  return( true );
}

const string PATH_KEY = "path="; // パス名に対するキー
const string FILE_KEY = "file="; // ファイル名に対するキー
const string TAG_KEY = "tag=";   // タグに対するキー

/*
  ReadTagData : fileName からタグを読み取り、fileData と tagData に登録する

  フォーマットは次のようにする

  path=[root path]
  file=[name of file1]
  tag=[name of tag1]
  :
  file=[name of file2]
  :
*/
void ReadTagData( const string& fileName, string* rootPath, FileData* fileData, TagData* tagData )
{
  if ( ! fs::exists( fs::path( fileName ) ) )
    throw std::runtime_error( "指定したタグファイルは存在しません。" );

  ifstream ifs( fileName );
  if ( ifs.fail() )
    throw std::runtime_error( "タグファイルのオープンに失敗しました。" );

  string data; // 1行読み込み
  rootPath->clear();
  while ( std::getline( ifs, data ) ) {
    if ( GetValueFromKey( data, PATH_KEY, rootPath ) ) {
      InitTagData( *rootPath, fileData, tagData );
      break;
    }
  }
  if ( rootPath->empty() ) {
    throw std::runtime_error( "ルートパスの取得に失敗しました。" );
  }

  string file; // 対象ファイル名
  string tag;  // タグ名
  auto fit = fileData->end();
  while ( std::getline( ifs, data ) ) {
    if ( GetValueFromKey( data, FILE_KEY, &file ) ) {
      fit = fileData->find( fs::path( *rootPath + "/" + file ).lexically_normal() );
      continue;
    }
    if ( GetValueFromKey( data, TAG_KEY, &tag ) ) {
      if ( fit == fileData->end() ) continue; // パスが見つからない場合は無視される
      ( fit->second ).insert( tag );
      auto tit = tagData->find( tag );
      if ( tit == tagData->end() )
        tit = ( tagData->insert( std::make_pair( tag, set< fs::path >() ) ) ).first;
      ( tit->second ).insert( fit->first );
    }
  }
}

/*
  WriteTagData : ルートパス rootPath とタグ fileData を fileName で指定したファイルに書き込む
*/
void WriteTagData( const string& fileName, const string& rootPath, const FileData& fileData )
{
  fs::path writeFile( fileName );
  fs::path tempFile( fileName + ".tmp" );

  ofstream ofs( tempFile.native() );
  ofs << PATH_KEY << rootPath << endl;
  for ( auto f = fileData.begin() ; f != fileData.end() ; ++f ) {
    ofs << FILE_KEY << fs::path( f->first ).lexically_relative( rootPath ).native() << endl;
    const auto& s = f->second;
    for ( auto t = s.begin() ; t != s.end() ; ++t )
      ofs << TAG_KEY << *t << endl;
  }
  ofs.close();

  fs::rename( tempFile, writeFile );
}
