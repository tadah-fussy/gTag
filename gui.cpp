/**
   gui.cpp : GUI用関数
**/
#include "gui.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;

namespace fs = boost::filesystem;

FileData FILE_DATA;
TagData TAG_DATA;
//GTK_DrawingArea_ScrolledWindow* DRAW_SW = 0;
//GTK_DrawingArea* DRAW = 0;

string CURRENT_TAG_FOLDER;
string ROOT_PATH;
string TAG_FILE;
bool TAG_EDITED = false;
bool AUTO_SCALE = true;

GdkPixbufAnimationIter* AnimationIterator = 0;

gulong FILE_LIST_ID;

/*
  MessageBox : メッセージダイアログの表示

  message : 出力するメッセージ
  messageType : メッセージのタイプ(GTK_MESSAGE_INFO/WARNING/QUESTION/ERROR/OTHER)
  buttonsType : ボタンのタイプ(GTK_BUTTONS_NONE/OK/CLOSE/CANCEL/YES_NO/OK_CANCEL)
  builder : GtkBuilderオブジェクトへのポインタ

  戻り値 : レスポンスID(GTK_RESPONSE_OK/CANCEL/YES/NO/DELETE_EVENT)
*/
gint MessageBox( const string& message, GtkMessageType messageType, GtkButtonsType buttonsType, GtkBuilder* builder )
{
  GtkWidget* dialog = gtk_message_dialog_new( GTK_WINDOW( gtk_builder_get_object( builder, "root" ) ),
                                              static_cast< GtkDialogFlags >( GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT ),
                                              messageType, buttonsType,
                                              "%s", message.c_str() );

  gint res = gtk_dialog_run( GTK_DIALOG( dialog ) );
  gtk_widget_destroy( dialog );

  return( res );
}

/*
  InitTagList : タグリストの初期化

  builder : GtkBuilder オブジェクトへのポインタ
  fileName : 対象ファイルの名前
  fileData : ファイルをキーとするタグリスト
*/
void InitTagList( GtkBuilder* builder, const string& fileName, const FileData& fileData )
{
  // ファイルリストのパーツ
  //GtkTreeView* view = GTK_TREE_VIEW( gtk_builder_get_object( builder, "taglist" ) );
  //GtkListStore* store = GTK_LIST_STORE( gtk_tree_view_get_model( view ) );
  GtkListStore* store = GTK_LIST_STORE( gtk_builder_get_object( builder, "tagliststore" ) );
  GtkTreeIter iter;

  // リストの更新
  gtk_list_store_clear( store );
  const auto& s = ( fileData.find( fileName ) )->second;
  for ( auto i = s.begin() ; i != s.end() ; ++i ) {
    gtk_list_store_append( store, &iter );
    gtk_list_store_set( store, &iter, 0, i->c_str(), -1 );
  }
}

/*
  InitCompletionList : 補完用リストの初期化

  builder : GtkBuilder オブジェクトへのポインタ
  tagData : タグをキーとするファイルリスト
*/
void InitCompletionList( GtkBuilder* builder, const TagData& tagData )
{
  // 補完リストのパーツ
  //GtkEntry* entry = GTK_ENTRY( gtk_builder_get_object( builder, "tagentry" ) );
  //GtkEntryCompletion* completion = gtk_entry_get_completion( entry );
  //GtkListStore* store = GTK_LIST_STORE( gtk_entry_completion_get_model( completion ) );
  GtkListStore* store = GTK_LIST_STORE( gtk_builder_get_object( builder, "completionstore" ) );
  GtkTreeIter iter;

  // リストの更新
  gtk_list_store_clear( store );
  for ( auto t = tagData.begin() ; t != tagData.end() ; ++t ) {
    gtk_list_store_append( store, &iter );
    gtk_list_store_set( store, &iter, 0, ( t->first ).c_str(), -1 );
  }
}

/*
  ChangeCompletionList : 補完用リストのタグ名を変更する

  builder : GtkBuilder オブジェクトへのポインタ
  oldTag : 変更対象のタグ名
  newTag : 変更後のタグ名
*/
void ChangeCompletionList( GtkBuilder* builder, const string& oldTag, const string& newTag )
{
  // 補完リストのパーツ
  //GtkEntry* entry = GTK_ENTRY( gtk_builder_get_object( builder, "tagentry" ) );
  //GtkEntryCompletion* completion = gtk_entry_get_completion( entry );
  //GtkTreeModel* model = gtk_entry_completion_get_model( completion );
  GtkTreeModel* model = GTK_TREE_MODEL( gtk_builder_get_object( builder, "completionstore" ) );
  GtkTreeIter iter;

  if ( ! gtk_tree_model_get_iter_first( model, &iter ) )
    return;

  do {
    GValue value = G_VALUE_INIT;
    gtk_tree_model_get_value( model, &iter, 0, &value );
    string tag = g_value_get_string( &value );
    g_value_unset( &value );
    if ( tag == oldTag ) {
      gtk_list_store_set( GTK_LIST_STORE( model ), &iter, 0, newTag.c_str(), -1 );
      return;
    }
  } while ( gtk_tree_model_iter_next( model, &iter ) );
}

/*
  GetFileName : リストで選択されているファイル名を取得する

  builder : GtkBuilder オブジェクトへのポインタ
  rootPath : ルートパス
  fileName : ファイル名を取得する変数へのポインタ

  戻り値 : リスト選択されていなければ false を返す
*/
bool GetFileName( GtkBuilder* builder, const string& rootPath, string* fileName )
{
  GtkTreeSelection* selection = GTK_TREE_SELECTION( gtk_builder_get_object( builder, "filelistselection" ) );

  // 選択された GtkTreeIter を取得する
  GtkTreeModel* model = 0;
  GtkTreeIter iter;
  if ( ! gtk_tree_selection_get_selected( selection, &model, &iter ) )
    return( false );

  // ファイル名の取得
  gchar* gc;
  gtk_tree_model_get( model, &iter, 0, &gc, -1 );
  *fileName = fs::path( rootPath + "/" + gc ).lexically_normal().native();
  g_free( gc );

  return( true );
}

gboolean Timer( gpointer data )
{
  GdkPixbufAnimationIter* iter = static_cast< GdkPixbufAnimationIter* >( data );
  gdk_pixbuf_animation_iter_advance( iter, 0 );
  g_timeout_add( gdk_pixbuf_animation_iter_get_delay_time(iter), Timer, iter );

  return( FALSE );
}

/*
  CB_ShowImage : 画像の表示(コールバック関数)

  selection : GtkTreeSelection オブジェクトへのポインタ
  data : GtkBuilder オブジェクトへのポインタ
*/
void CB_ShowImage( GtkTreeSelection* selection, gpointer data )
{
  // GtkBuilder オブジェクトへのポインタへ変換
  GtkBuilder* builder = static_cast< GtkBuilder* >( data );

  // リスト選択されているファイル名の取得
  string fileName;
  if ( ! GetFileName( builder, ROOT_PATH, &fileName ) )
    return;

  // 画像の出力
  //try {
  GtkImage* image = GTK_IMAGE( gtk_builder_get_object( builder, "imageview" ) );
  gtk_image_set_from_file( image, fileName.c_str() );
  GtkImageType imgType = gtk_image_get_storage_type( image );
  if ( imgType == GTK_IMAGE_ANIMATION ) {
    GdkPixbufAnimation* anim = gtk_image_get_animation( image );
    AnimationIterator = gdk_pixbuf_animation_get_iter( anim, 0 );
    Timer( AnimationIterator );
  }
  //( DRAW_SW->drawingArea()->pixbuf() ).loadFromFile( fileName );
  //DRAW_SW->drawingArea()->draw();
  //DRAW_SW->initAdjustment();
  //} catch( std::runtime_error& e ) {
  //MessageBox( e.what(), GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, builder );
  //}

  InitTagList( builder, fileName, FILE_DATA );
}

gboolean CB_DrawImage( GtkWidget* widget, cairo_t* cairo, gpointer data )
{
  if ( ! AUTO_SCALE ) return( FALSE );

  // GtkBuilder オブジェクトへのポインタへ変換
  GtkBuilder* builder = static_cast< GtkBuilder* >( data );
  GtkWidget* viewport = GTK_WIDGET( gtk_builder_get_object( builder, "imageviewport" ) );

  int width = gtk_widget_get_allocated_width( viewport );
  int height = gtk_widget_get_allocated_height( viewport );
  std::cout << "width = " << width << ", height = " << height << std::endl;

  GtkImageType imgType = gtk_image_get_storage_type( GTK_IMAGE( widget ) );
  if ( imgType == GTK_IMAGE_ANIMATION ) {
    GdkPixbufAnimation* anim = gtk_image_get_animation( GTK_IMAGE( widget ) );
    int imgw = gdk_pixbuf_animation_get_width( anim );
    int imgh = gdk_pixbuf_animation_get_height( anim );
    std::cout << "imgw = " << imgw << ", imgh = " << imgh << std::endl;
    double ratio = std::min( static_cast< double >( width ) / imgw, static_cast< double >( height ) / imgh );
    if ( ratio < 1.0 ) {
      std::cout << "ratio = " << ratio << std::endl;
      //GdkPixbufAnimationIter* iter = gdk_pixbuf_animation_get_iter( anim, 0 );
      GdkPixbuf* pixbuf = gdk_pixbuf_animation_iter_get_pixbuf( AnimationIterator );
      GdkPixbuf* buff = gdk_pixbuf_scale_simple( pixbuf, imgw * ratio, imgh * ratio, GDK_INTERP_HYPER );
      gtk_image_set_from_pixbuf( GTK_IMAGE( widget ), buff );
      //gdk_cairo_set_source_pixbuf( cairo, buff, 0, 0 );
      //cairo_paint( cairo );
      g_object_unref( buff );
    }
  } else if ( imgType == GTK_IMAGE_PIXBUF ) {
    GdkPixbuf* pixbuf = gtk_image_get_pixbuf( GTK_IMAGE( widget ) );
    int imgw = gdk_pixbuf_get_width( pixbuf );
    int imgh = gdk_pixbuf_get_height( pixbuf );
    std::cout << "imgw = " << imgw << ", imgh = " << imgh << std::endl;
    double ratio = std::min( static_cast< double >( width ) / imgw, static_cast< double >( height ) / imgh );
    if ( ratio < 1.0 ) {
      std::cout << "ratio = " << ratio << std::endl;
      GdkPixbuf* buff = gdk_pixbuf_scale_simple( pixbuf, imgw * ratio, imgh * ratio, GDK_INTERP_HYPER );
      gtk_image_set_from_pixbuf( GTK_IMAGE( widget ), buff );
      //gdk_cairo_set_source_pixbuf( cairo, buff, 0, 0 );
      //cairo_paint( cairo );
      g_object_unref( buff );
    }
  }

  return( FALSE );
}

/*
  InitFileList : ファイルリストの初期化

  builder : GtkBuilder オブジェクトへのポインタ
  fileData : ファイルをキーとするタグリスト
  rootPath : ルートパス
*/
void InitFileList( GtkBuilder* builder, const FileData& fileData, const string& rootPath )
{
  // ファイルリストのパーツ
  GtkListStore* store = GTK_LIST_STORE( gtk_builder_get_object( builder, "fileliststore" ) );
  GtkTreeSelection* selection = GTK_TREE_SELECTION( gtk_builder_get_object( builder, "filelistselection" ) );
  GtkTreeIter iter;

  g_signal_handler_block( selection, FILE_LIST_ID );

  // ファイルリストの更新
  gtk_list_store_clear( store );
  for ( auto i = fileData.begin() ; i != fileData.end() ; ++i ) {
    gtk_list_store_append( store, &iter );
    gtk_list_store_set( store, &iter, 0, ( i->first ).lexically_relative( rootPath ).native().c_str(), -1 );
  }

  g_signal_handler_unblock( selection, FILE_LIST_ID );

  // タグリストの消去
  store = GTK_LIST_STORE( gtk_builder_get_object( builder, "tagliststore" ) );
  gtk_list_store_clear( store );
}

/*
  GetFileNameFromDialog : ファイル・フォルダ名の取得

  builder : GtkBuilderオブジェクトへのポインタ
  title : ダイアログのタイトル
  action : ダイアログのタイプ(GTK_FILE_CHOOSER_ACTION_OPEN/SAVE/SELECT_FOLDER)
  cancelAction : キャンセルボタン用の名称
  acceptAction : 適用ボタン用の名称
  fileName : ファイル名を取得する変数へのポインタ
  currentFolder : 現在のパスを保持する変数へのポインタ

  戻り値 : レスポンスID(GTK_RESPONSE_ACCEPT/CANCEL)
*/
gint GetFileNameFromDialog( GtkBuilder* builder, const string& title,
                            GtkFileChooserAction action,
                            const string& cancelAction, const string& acceptAction,
                            string* fileName, string* currentFolder )
{
  // ディレクトリ選択用ダイアログ生成
  GtkWidget* dialog = gtk_file_chooser_dialog_new
    (
     title.c_str(),
     GTK_WINDOW( gtk_builder_get_object( builder, "root" ) ),
     action,
     cancelAction.c_str(), GTK_RESPONSE_CANCEL,
     acceptAction.c_str(), GTK_RESPONSE_ACCEPT,
     NULL
     );

  // カレント・ディレクトリとファイル名のセット
  if ( action == GTK_FILE_CHOOSER_ACTION_SAVE ) {
    GtkFileChooser* chooser = GTK_FILE_CHOOSER( dialog );
    if ( fileName->empty() ) {
      if ( ! currentFolder->empty() )
        gtk_file_chooser_set_current_folder( chooser, currentFolder->c_str() );
      gtk_file_chooser_set_current_name( chooser, "untitled.tag" );
    } else {
      GFile* file = g_file_new_for_path( fileName->c_str() );
      gtk_file_chooser_set_file( chooser, file, 0 );
      g_object_unref( file );
    }
  } else {
    if ( ! currentFolder->empty() )
      gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER( dialog ), currentFolder->c_str() );
  }

  gint res = gtk_dialog_run( GTK_DIALOG( dialog ) );

  // ディレクトリ選択時の処理
  if ( res == GTK_RESPONSE_ACCEPT ) {
    // ルートパスの決定
    GtkFileChooser* chooser = GTK_FILE_CHOOSER( dialog );
    gchar* gc = gtk_file_chooser_get_filename( chooser );
    *fileName = gc;
    g_free( gc );
    // カレント・ディレクトリの更新
    gc = gtk_file_chooser_get_current_folder( chooser );
    if ( gc != 0 ) {
      *currentFolder = gc;
      g_free( gc );
    }
  }

  gtk_widget_destroy( dialog );

  return( res );
}

/*
  CB_FileNew : タグリストの新規作成(コールバック関数)

  menuItem : GtkMenuItem オブジェクトへのポインタ
  data : GtkBuilder オブジェクトへのポインタ
*/
void CB_FileNew( GtkMenuItem* menuItem, gpointer data )
{
  static string CURRENT_IMAGE_FOLDER;

  // GtkBuilder オブジェクトへのポインタに変換
  GtkBuilder* builder = static_cast< GtkBuilder* >( data );

  if ( GetFileNameFromDialog( builder, "ルートパスの選択", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                              "Cancel", "Select", &ROOT_PATH, &CURRENT_IMAGE_FOLDER ) == GTK_RESPONSE_ACCEPT ) {
    // タグの初期化
    InitTagData( ROOT_PATH, &FILE_DATA, &TAG_DATA );
    // ファイルリストの初期化
    InitFileList( builder, FILE_DATA, ROOT_PATH );
    // 変数の初期化
    TAG_FILE.clear();
    TAG_EDITED = false;
  }
}

/*
  CB_FileOpen : タグリストを開く(コールバック関数)

  menuItem : GtkMenuItem オブジェクトへのポインタ
  data : GtkBuilder オブジェクトへのポインタ
*/
void CB_FileOpen( GtkMenuItem* menuItem, gpointer data )
{
  // GtkBuilder オブジェクトへのポインタに変換
  GtkBuilder* builder = static_cast< GtkBuilder* >( data );

  string tagFile;
  string rootPath;
  if ( GetFileNameFromDialog( builder, "タグリストを開く", GTK_FILE_CHOOSER_ACTION_OPEN,
                              "Cancel", "Open", &tagFile, &CURRENT_TAG_FOLDER ) == GTK_RESPONSE_ACCEPT ) {
    // タグファイルの読み込み
    try {
      ReadTagData( tagFile, &rootPath, &FILE_DATA, &TAG_DATA );
    } catch( std::runtime_error& e ) {
      MessageBox( e.what(), GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, builder );
      return;
    }
    // ファイルリストの初期化
    InitFileList( builder, FILE_DATA, rootPath );
    // 補完用リストの初期化
    InitCompletionList( builder, TAG_DATA );
    // 変数の初期化
    ROOT_PATH = rootPath;
    TAG_FILE = tagFile;
    TAG_EDITED = false;
  }
}

/*
  CB_FileSaveAs : タグリストの新規保存(コールバック関数)

  menuItem : GtkMenuItem オブジェクトへのポインタ
  data : GtkBuilder オブジェクトへのポインタ
*/
void CB_FileSaveAs( GtkMenuItem* menuItem, gpointer data )
{
  // GtkBuilder オブジェクトへのポインタに変換
  GtkBuilder* builder = static_cast< GtkBuilder* >( data );

  if ( GetFileNameFromDialog( builder, "タグリストの新規保存", GTK_FILE_CHOOSER_ACTION_SAVE,
                              "Cancel", "Save", &TAG_FILE, &CURRENT_TAG_FOLDER ) == GTK_RESPONSE_ACCEPT ) {
    WriteTagData( TAG_FILE, ROOT_PATH, FILE_DATA );
    // 変数の初期化
    TAG_EDITED = false;
  }
}

/*
  CB_FileSave : タグリストの上書き保存(コールバック関数)

  menuItem : GtkMenuItem オブジェクトへのポインタ
  data : GtkBuilder オブジェクトへのポインタ
*/
void CB_FileSave( GtkMenuItem* menuItem, gpointer data )
{
  if ( TAG_FILE.empty() )
    CB_FileSaveAs( menuItem, data );

  if ( ! TAG_EDITED )
    return;

  WriteTagData( TAG_FILE, ROOT_PATH, FILE_DATA );

  TAG_EDITED = false;
}

/*
  AddTag : タグの登録を行う

  tag : 登録するタグ
  fileName : 登録対象のファイル
  fileData : ファイルをキーとしたタグリスト
  tagData : タグをキーとしたファイルリスト

  戻り値 : タグがすでに登録されていた場合は false を返す
*/
bool AddTag( const string& tag, const string& fileName, FileData* fileData, TagData* tagData )
{
  auto& f = ( *fileData )[fileName];
  if ( ! f.insert( tag ).second )
    return( false );

  auto& t = ( *tagData )[tag];
  t.insert( fileName );

  return( true );
}

/*
  CheckTag : タグの両端の空白文字を除去し、チェックする

  tag : タグへのポインタ
  message : 不正があった場合のメッセージを保持する変数へのポインタ
  tagData : タグをキーとするファイルリスト

  戻り値 : 不正があった場合は false を返す
*/
bool CheckTag( string* tag, string* message )
{
  boost::algorithm::trim( *tag );

  // 空文字のチェック
  if ( tag->empty() ) {
    *message = "空文字はタグにできません。";
    return( false );
  }

  // 空白文字を含んでいるか？
  if ( tag->find( ' ' ) != string::npos ) {
    *message = "空白文字はタグに使えません。";
    return( false );
  }

  return( true );
}

bool CheckDuplicateTag( const string& tag, string* message, const TagData& tagData )
{
  // 同じタグが存在していないか？
  if ( tagData.find( tag ) != tagData.end() ) {
    *message = "すでに同名のタグがあります。";
    return( false );
  }

  return( true );
}

/*
  CB_AddTag : タグを登録し、タグリストに表示する(コールバック関数)

  entry : GtkEntry オブジェクトへのポインタ
  data : GtkBuilder オブジェクトへのポインタ
*/
void CB_AddTag( GtkEntry* entry, gpointer data )
{
  // GtkBuilder オブジェクトへのポインタに変換
  GtkBuilder* builder = static_cast< GtkBuilder* >( data );

  // タグの取得
  string tag = gtk_entry_get_text( entry );

  // タグのチェック
  string message;
  if ( ! CheckTag( &tag, &message ) ) {
    MessageBox( message, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, builder );
    return;
  }

  // タグを登録する対象ファイルの取得
  string fileName;
  if ( ! GetFileName( builder, ROOT_PATH, &fileName ) )
    return;

  // タグの登録
  if ( ! AddTag( tag, fileName, &FILE_DATA, &TAG_DATA ) )
    return;

  // 補完用リストへの登録
  GtkTreeIter iter;
  //GtkEntryCompletion* completion = gtk_entry_get_completion( entry );
  //GtkListStore* store = GTK_LIST_STORE( gtk_entry_completion_get_model( completion ) );
  GtkListStore* store = GTK_LIST_STORE( gtk_builder_get_object( builder, "completionstore" ) );
  gtk_list_store_append( store, &iter );
  gtk_list_store_set( store, &iter, 0, tag.c_str(), -1 );

  // タグリストへの登録
  //GtkTreeView* view = GTK_TREE_VIEW( gtk_builder_get_object( builder, "taglist" ) );
  //store = GTK_LIST_STORE( gtk_tree_view_get_model( view ) );
  store = GTK_LIST_STORE( gtk_builder_get_object( builder, "tagliststore" ) );
  gtk_list_store_append( store, &iter );
  gtk_list_store_set( store, &iter, 0, tag.c_str(), -1 );

  gtk_entry_set_text( entry, "" );

  TAG_EDITED = true;
}

/*
  CreateCompletion : 補完リストの生成

  builder : GtkBuilder オブジェクトへのポインタ
*/
void CreateCompletion( GtkBuilder* builder )
{
  GtkTreeModel* sorted = GTK_TREE_MODEL( gtk_builder_get_object( builder, "completionsort" ) );
  gtk_tree_sortable_set_sort_column_id( GTK_TREE_SORTABLE( sorted ), 0, GTK_SORT_ASCENDING );

  //GtkEntryCompletion* completion = gtk_entry_completion_new();

  //GtkListStore* store = gtk_list_store_new( 1, G_TYPE_STRING );
  //gtk_entry_completion_set_model( completion, GTK_TREE_MODEL( store ) );
  //g_object_unref( store );

  GtkEntryCompletion* completion = GTK_ENTRY_COMPLETION( gtk_builder_get_object( builder, "entrycompletion" ) );
  gtk_entry_completion_set_text_column( completion, 0 );

  //gtk_entry_set_completion( GTK_ENTRY( gtk_builder_get_object( builder, "tagentry" ) ), completion );
  //g_object_unref( completion );
}

/*
  CreateFileList : ファイルリストの生成

  builder : GtkBuilder オブジェクトへのポインタ
*/
void CreateFileList( GtkBuilder* builder )
{
  GtkTreeView* view = GTK_TREE_VIEW( gtk_builder_get_object( builder, "filelist" ) );
  //GtkListStore* store = gtk_list_store_new( 1, G_TYPE_STRING );
  //gtk_tree_view_set_model( view, GTK_TREE_MODEL( store ) );
  //g_object_unref( store );

  GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
  GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes
    ( "path name", renderer, "text", 0, NULL );
  gtk_tree_view_append_column( view, column );

  GtkTreeSelection* selection = gtk_tree_view_get_selection( view );
  gtk_tree_selection_set_mode( selection, GTK_SELECTION_SINGLE );
  FILE_LIST_ID = g_signal_connect( G_OBJECT( selection ), "changed", G_CALLBACK( CB_ShowImage ), builder );
}

/*
  CB_TagPopup : タグリスト上のメニュー表示(コールバック関数)

  widget : GtkWidget オブジェクトへのポインタ
  event : GdkEventButton オブジェクトへのポインタ
  data : GtkBuilder オブジェクトへのポインタ
*/
gboolean CB_TagPopup( GtkWidget* widget, GdkEventButton* event, gpointer data )
{
  GtkBuilder* builder = static_cast< GtkBuilder* >( data );

  if ( event->button == 3 ) {
    GtkMenu* menu = GTK_MENU( gtk_builder_get_object( builder, "tagmenu" ) );
    gtk_menu_popup_at_pointer( menu, reinterpret_cast< GdkEvent* >( event ) );
  }

  return( FALSE );
}

/*
  CreateTagList : タグリストの生成

  builder : GtkBuilder オブジェクトへのポインタ
*/
void CreateTagList( GtkBuilder* builder )
{
  GtkTreeView* view = GTK_TREE_VIEW( gtk_builder_get_object( builder, "taglist" ) );
  //GtkListStore* store = gtk_list_store_new( 1, G_TYPE_STRING );
  //gtk_tree_view_set_model( view, GTK_TREE_MODEL( store ) );
  //g_object_unref( store );

  GtkTreeModel* sorted = GTK_TREE_MODEL( gtk_builder_get_object( builder, "taglistsort" ) );
  gtk_tree_sortable_set_sort_column_id( GTK_TREE_SORTABLE( sorted ), 0, GTK_SORT_ASCENDING );
  GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
  GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes
    ( "tag name", renderer, "text", 0, NULL );
  gtk_tree_view_append_column( view, column );

  g_signal_connect( G_OBJECT( view ), "button-press-event", G_CALLBACK( CB_TagPopup ), builder );
}

/*
  CreateMenu : メニューの生成

  builder : GtkBuilder オブジェクトへのポインタ
*/
void CreateMenu( GtkBuilder* builder )
{
  GObject* obj = gtk_builder_get_object( builder, "filenew" );
  g_signal_connect( obj, "activate", G_CALLBACK( CB_FileNew ), builder );
  obj = gtk_builder_get_object( builder, "fileopen" );
  g_signal_connect( obj, "activate", G_CALLBACK( CB_FileOpen ), builder );
  obj = gtk_builder_get_object( builder, "filesave" );
  g_signal_connect( obj, "activate", G_CALLBACK( CB_FileSave ), builder );
  obj = gtk_builder_get_object( builder, "filesaveas" );
  g_signal_connect( obj, "activate", G_CALLBACK( CB_FileSaveAs ), builder );
}

/*
  ChangeTagName : タグ名の変更

  oldTag : 変更対象のタグ
  newTag : 新しいタグ
  fileData : ファイルをキーとするタグリストへのポインタ
  tagData : タグをキーとするファイルリストへのポインタ
*/
void ChangeTagName( const string& oldTag, const string& newTag, FileData* fileData, TagData* tagData )
{
  auto t = tagData->find( oldTag );
  if ( t == tagData->end() ) return;

  const auto& fileList = t->second;
  for ( auto f = fileList.begin() ; f != fileList.end() ; ++f ) {
    auto& tagList = ( *fileData )[*f];
    tagList.erase( oldTag );
    tagList.insert( newTag );
  }

  tagData->insert( std::make_pair( newTag, fileList ) );
  tagData->erase( oldTag );
}

/*
  GetSelectedRow : リスト内の選択行を取得する

  builder : GtkBuilderオブジェクトへのポインタ
  listName : リスト名(filelist/taglist)
  rowName : 選択行をセットする変数へのポインタ
  model : GtkTreeModelオブジェクトへのハンドル
  iter : GtkTreeIterオブジェクトへのポインタ

  戻り値 : 選択されていた場合は true を返す
*/
bool GetSelectedRow( GtkBuilder* builder, const string& listName, string* rowName, GtkTreeModel** model, GtkTreeIter* iter )
{
  GtkTreeView* view = GTK_TREE_VIEW( gtk_builder_get_object( builder, listName.c_str() ) );
  *model = gtk_tree_view_get_model( view );
  GtkTreeSelection* selection = gtk_tree_view_get_selection( view );
  if ( ! gtk_tree_selection_get_selected( selection, model, iter ) )
    return( false );
  gchar* gc;
  gtk_tree_model_get( *model, iter, 0, &gc, -1 );
  *rowName = gc;
  g_free( gc );

  return( true );
}

/*
  CB_DlgOk : レスポンスID(GTK_RESPONSE_ACCEPT)を投げる

  entry : GtkEntryオブジェクトへのポインタ
  data : GtkDialogオブジェクトへのポインタ
*/
void CB_DlgOk( GtkEntry* entry, gpointer data )
{
  GtkDialog* dialog = static_cast< GtkDialog* >( data );

  gtk_dialog_response( dialog, GTK_RESPONSE_ACCEPT );
}

/*
  TagEdit : タグの編集

  builder : GtkBuilderオブジェクトへのポインタ
  currentTag : 現在のタグ
  newTag : 新たなタグをセットする変数へのポインタ

  戻り値 : レスポンスID(GTK_RESPONSE_ACCEPT/REJECT)
*/
gint TagEdit( GtkBuilder* builder, const string& currentTag, string* newTag )
{
  GtkWidget* dialog = gtk_dialog_new_with_buttons( "タグの編集", GTK_WINDOW( gtk_builder_get_object( builder, "root" ) ),
                                                   static_cast< GtkDialogFlags >( GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT ),
                                                   "OK", GTK_RESPONSE_ACCEPT,
                                                   "Cancel", GTK_RESPONSE_REJECT,
                                                   NULL );

  // GtkEntryの追加
  GtkWidget* contentArea = gtk_dialog_get_content_area( GTK_DIALOG( dialog ) );
  GtkWidget* entry = gtk_entry_new();
  gtk_entry_set_text( GTK_ENTRY( entry ), currentTag.c_str() );
  g_signal_connect( entry, "activate", G_CALLBACK( CB_DlgOk ), dialog );
  gtk_container_add( GTK_CONTAINER( contentArea ), entry );
  gtk_widget_show_all( dialog );

  gint res = gtk_dialog_run( GTK_DIALOG( dialog ) );

  if ( res == GTK_RESPONSE_ACCEPT )
    *newTag = gtk_entry_get_text( GTK_ENTRY( entry ) );

  gtk_widget_destroy( dialog );

  return( res );
}

/*
  CB_TagEdit : タグの編集(コールバック関数)

  menuItem : GtkMenuItem オブジェクトへのポインタ
  data : GtkBuilder オブジェクトへのポインタ
*/
void CB_TagEdit( GtkMenuItem* menuItem, gpointer data )
{
  GtkBuilder* builder = static_cast< GtkBuilder* >( data );

  GtkTreeModel* model;
  GtkTreeIter iter;
  string currentTag;
  string newTag;

  if ( ! GetSelectedRow( builder, "taglist", &currentTag, &model, &iter ) )
    return;

  while ( TagEdit( builder, currentTag, &newTag ) == GTK_RESPONSE_ACCEPT ) {
    string message;
    if ( ! ( CheckTag( &newTag, &message ) && CheckDuplicateTag( newTag, &message, TAG_DATA ) ) ) {
      MessageBox( message, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, builder );
      continue;
    } else {
      ChangeTagName( currentTag, newTag, &FILE_DATA, &TAG_DATA );

      GtkTreeIter child_iter;
      gtk_tree_model_sort_convert_iter_to_child_iter( GTK_TREE_MODEL_SORT( model ), &child_iter, &iter );
      GtkTreeModel* child = gtk_tree_model_sort_get_model( GTK_TREE_MODEL_SORT( model ) );
      gtk_list_store_set( GTK_LIST_STORE( child ), &child_iter, 0, newTag.c_str(), -1 );

      ChangeCompletionList( builder, currentTag, newTag );
      TAG_EDITED = true;
      break;
    }
  }
}

/*
  CB_TagDelete : タグの削除(コールバック関数)

  menuItem : GtkMenuItem オブジェクトへのポインタ
  data : GtkBuilder オブジェクトへのポインタ
*/
void CB_TagDelete( GtkMenuItem* menuItem, gpointer data )
{
  GtkBuilder* builder = static_cast< GtkBuilder* >( data );

  GtkTreeModel* model;
  GtkTreeIter iter;
  string fileName;
  if ( ! GetFileName( builder, ROOT_PATH, &fileName ) )
    return;
  string tagName;
  if ( ! GetSelectedRow( builder, "taglist", &tagName, &model, &iter ) )
    return;

  FILE_DATA[fileName].erase( tagName );
  TAG_DATA[tagName].erase( fileName );

  GtkTreeIter child_iter;
  gtk_tree_model_sort_convert_iter_to_child_iter( GTK_TREE_MODEL_SORT( model ), &child_iter, &iter );
  GtkTreeModel* child = gtk_tree_model_sort_get_model( GTK_TREE_MODEL_SORT( model ) );
  gtk_list_store_remove( GTK_LIST_STORE( child ), &child_iter );

  TAG_EDITED = true;
}

/*
  CreateTagPopupMenu : タグリスト上のポップアップメニューの作成

  builder : GtkBuilderオブジェクトへのポインタ
*/
void CreateTagPopupMenu( GtkBuilder* builder )
{
  GObject* tagEdit = gtk_builder_get_object( builder, "tagedit" );
  g_signal_connect( tagEdit, "activate", G_CALLBACK( CB_TagEdit ), builder );
  GObject* tagDelete = gtk_builder_get_object( builder, "tagdelete" );
  g_signal_connect( tagDelete, "activate", G_CALLBACK( CB_TagDelete ), builder );
}

int main( int argc, char* argv[] )
{
  gtk_init( &argc, &argv );

  GtkBuilder* builder = gtk_builder_new();
  GError* error = 0;
  if ( gtk_builder_add_from_file( builder, "gTag.ui", &error ) == 0 ) {
    cerr << "Failed to load gTag.ui." << error->message << endl;
    g_clear_error( &error );
    return( 1 );
  }
  GObject* rootWin = gtk_builder_get_object( builder, "root" );
  //g_signal_connect( rootWin, "destroy", G_CALLBACK( gtk_main_quit ), 0 );
  //GObject* pane2 = gtk_builder_get_object( builder, "pane2" );

  //GTK_DrawingArea draw;
  //DRAW_SW = new GTK_DrawingArea_ScrolledWindow( draw );
  //GTK_DrawingArea_ImageShiftEvent drawEvent( *DRAW );
  //gtk_paned_add2( GTK_PANED( pane2 ), DRAW_SW->widget() );

  CreateMenu( builder );
  CreateFileList( builder );
  CreateTagList( builder );
  CreateCompletion( builder );
  CreateTagPopupMenu( builder );

  GObject* tagEntry = gtk_builder_get_object( builder, "tagentry" );
  g_signal_connect( tagEntry, "activate", G_CALLBACK( CB_AddTag ), builder );
  GObject* image = gtk_builder_get_object( builder, "imageview" );
  g_signal_connect( image, "draw", G_CALLBACK( CB_DrawImage ), builder );

  gtk_builder_connect_signals( builder, 0 );
  //map< fs::path, vector< string > > tagData;
  //InitTagData( argv[1], &tagData );
  //WriteTagData( "test.txt", argv[1], tagData );

  gtk_widget_show_all( GTK_WIDGET( rootWin ) );

  gtk_main();

  //delete DRAW_SW;

  return( 0 );
}
