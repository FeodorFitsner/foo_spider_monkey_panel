#include <stdafx.h>

#include "fb_metadb_handle.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_file_info.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

#include <stats.h>

using namespace smp;

namespace
{

using namespace mozjs;

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    JsFbMetadbHandle::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbMetadbHandle",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( ClearStats, JsFbMetadbHandle::ClearStats )
MJS_DEFINE_JS_FN_FROM_NATIVE( Compare, JsFbMetadbHandle::Compare )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetFileInfo, JsFbMetadbHandle::GetFileInfo )
MJS_DEFINE_JS_FN_FROM_NATIVE( RefreshStats, JsFbMetadbHandle::RefreshStats )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetFirstPlayed, JsFbMetadbHandle::SetFirstPlayed )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetLastPlayed, JsFbMetadbHandle::SetLastPlayed )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetLoved, JsFbMetadbHandle::SetLoved )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetPlaycount, JsFbMetadbHandle::SetPlaycount )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetRating, JsFbMetadbHandle::SetRating )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "ClearStats", ClearStats, 0, DefaultPropsFlags() ),
    JS_FN( "Compare", Compare, 1, DefaultPropsFlags() ),
    JS_FN( "GetFileInfo", GetFileInfo, 0, DefaultPropsFlags() ),
    JS_FN( "RefreshStats", RefreshStats, 0, DefaultPropsFlags() ),
    JS_FN( "SetFirstPlayed", SetFirstPlayed, 1, DefaultPropsFlags() ),
    JS_FN( "SetLastPlayed", SetLastPlayed, 1, DefaultPropsFlags() ),
    JS_FN( "SetLoved", SetLoved, 1, DefaultPropsFlags() ),
    JS_FN( "SetPlaycount", SetPlaycount, 1, DefaultPropsFlags() ),
    JS_FN( "SetRating", SetRating, 1, DefaultPropsFlags() ),
    JS_FS_END
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_FileSize, JsFbMetadbHandle::get_FileSize )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Length, JsFbMetadbHandle::get_Length )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Path, JsFbMetadbHandle::get_Path )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_RawPath, JsFbMetadbHandle::get_RawPath )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_SubSong, JsFbMetadbHandle::get_SubSong )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "FileSize", get_FileSize, DefaultPropsFlags() ),
    JS_PSG( "Length", get_Length, DefaultPropsFlags() ),
    JS_PSG( "Path", get_Path, DefaultPropsFlags() ),
    JS_PSG( "RawPath", get_RawPath, DefaultPropsFlags() ),
    JS_PSG( "SubSong", get_SubSong, DefaultPropsFlags() ),
    JS_PS_END
};

} // namespace

namespace mozjs
{

const JSClass JsFbMetadbHandle::JsClass = jsClass;
const JSFunctionSpec* JsFbMetadbHandle::JsFunctions = jsFunctions;
const JSPropertySpec* JsFbMetadbHandle::JsProperties = jsProperties;
const JsPrototypeId JsFbMetadbHandle::PrototypeId = JsPrototypeId::FbMetadbHandle;

JsFbMetadbHandle::JsFbMetadbHandle( JSContext* cx, const metadb_handle_ptr& handle )
    : pJsCtx_( cx )
    , metadbHandle_( handle )
{
}

JsFbMetadbHandle::~JsFbMetadbHandle()
{
}

std::unique_ptr<mozjs::JsFbMetadbHandle>
JsFbMetadbHandle::CreateNative( JSContext* cx, const metadb_handle_ptr& handle )
{
    SmpException::ExpectTrue( handle.is_valid(), "Internal error: metadb_handle_ptr is null" );

    return std::unique_ptr<JsFbMetadbHandle>( new JsFbMetadbHandle( cx, handle ) );
}

size_t JsFbMetadbHandle::GetInternalSize( const metadb_handle_ptr& /*handle*/ )
{
    return sizeof( metadb_handle );
}

metadb_handle_ptr& JsFbMetadbHandle::GetHandle()
{
    return metadbHandle_;
}

void JsFbMetadbHandle::ClearStats()
{
    metadb_index_hash hash;
    if ( !stats::hashHandle( metadbHandle_, hash ) )
    {
        stats::set( hash, stats::fields() );
    }
}

bool JsFbMetadbHandle::Compare( JsFbMetadbHandle* handle )
{
    SmpException::ExpectTrue( handle, "handle argument is null" );

    return ( handle->GetHandle() == metadbHandle_ );
}

JSObject* JsFbMetadbHandle::GetFileInfo()
{
    metadb_info_container::ptr containerInfo;
    if ( !metadbHandle_->get_info_ref( containerInfo ) )
    { // Not an error: info not loaded yet
        return nullptr;
    }

    return JsFbFileInfo::CreateJs( pJsCtx_, containerInfo );
}

void JsFbMetadbHandle::RefreshStats()
{
    if ( metadb_index_hash hash;
         stats::hashHandle( metadbHandle_, hash ) )
    {
        stats::refresh( hash );
    }
}

void JsFbMetadbHandle::SetFirstPlayed( const std::u8string& first_played )
{
    if ( metadb_index_hash hash;
         stats::hashHandle( metadbHandle_, hash ) )
    {
        stats::fields tmp = stats::get( hash );
        if ( tmp.first_played != first_played )
        {
            tmp.first_played = first_played;
            stats::set( hash, tmp );
        }
    }
}

void JsFbMetadbHandle::SetLastPlayed( const std::u8string& last_played )
{
    if ( metadb_index_hash hash;
         stats::hashHandle( metadbHandle_, hash ) )
    {
        stats::fields tmp = stats::get( hash );
        if ( tmp.last_played != last_played )
        {
            tmp.last_played = last_played;
            stats::set( hash, tmp );
        }
    }
}

void JsFbMetadbHandle::SetLoved( uint32_t loved )
{
    if ( metadb_index_hash hash;
         stats::hashHandle( metadbHandle_, hash ) )
    {
        stats::fields tmp = stats::get( hash );
        if ( tmp.loved != loved )
        {
            tmp.loved = loved;
            stats::set( hash, tmp );
        }
    }
}

void JsFbMetadbHandle::SetPlaycount( uint32_t playcount )
{
    if ( metadb_index_hash hash;
         stats::hashHandle( metadbHandle_, hash ) )
    {
        stats::fields tmp = stats::get( hash );
        if ( tmp.playcount != playcount )
        {
            tmp.playcount = playcount;
            stats::set( hash, tmp );
        }
    }
}

void JsFbMetadbHandle::SetRating( uint32_t rating )
{
    if ( metadb_index_hash hash;
         stats::hashHandle( metadbHandle_, hash ) )
    {
        stats::fields tmp = stats::get( hash );
        if ( tmp.rating != rating )
        {
            tmp.rating = rating;
            stats::set( hash, tmp );
        }
    }
}

int64_t JsFbMetadbHandle::get_FileSize()
{
    return static_cast<int64_t>( metadbHandle_->get_filesize() );
}

double JsFbMetadbHandle::get_Length()
{
    return metadbHandle_->get_length();
}

std::u8string JsFbMetadbHandle::get_Path()
{
    return std::u8string( file_path_display( metadbHandle_->get_path() ) );
}

std::u8string JsFbMetadbHandle::get_RawPath()
{
    return metadbHandle_->get_path();
}

uint32_t JsFbMetadbHandle::get_SubSong()
{
    return metadbHandle_->get_subsong_index();
}

} // namespace mozjs
