#include <stdafx.h>
#include "ui_property.h"

#include <utils/file_helpers.h>
#include <js_panel_window.h>
#include <abort_callback.h>

// precision
#include <iomanip>
#include <map>
#include <filesystem>

namespace fs = std::filesystem;

namespace smp::ui
{

CDialogProperty::CDialogProperty( smp::panel::js_panel_window* p_parent )
    : m_parent( p_parent )
{
}

LRESULT CDialogProperty::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    DlgResize_Init();

    // Subclassing
    m_properties.SubclassWindow( GetDlgItem( IDC_LIST_PROPERTIES ) );
    m_properties.ModifyStyle( 0, LBS_SORT | LBS_HASSTRINGS );
    m_properties.SetExtendedListStyle( PLS_EX_SORTED | PLS_EX_XPLOOK );

    LoadProperties();

    return TRUE; // set focus to default control
}

LRESULT CDialogProperty::OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    switch ( wID )
    {
    case IDOK:
        Apply();
        break;

    case IDAPPLY:
        Apply();
        return 0;
    }

    EndDialog( wID );
    return 0;
}

LRESULT CDialogProperty::OnPinItemChanged( LPNMHDR pnmh )
{
    LPNMPROPERTYITEM pnpi = (LPNMPROPERTYITEM)pnmh;

    if ( auto it = m_dup_prop_map.find( pnpi->prop->GetName() );
         it != m_dup_prop_map.end() )
    {
        auto& val = *( it->second );
        _variant_t var;

        if ( pnpi->prop->GetValue( &var ) )
        {
            std::visit( [&var]( auto& arg ) {
                using T = std::decay_t<decltype( arg )>;
                if constexpr ( std::is_same_v<T, bool> )
                {
                    var.ChangeType( VT_BOOL );
                    arg = static_cast<bool>( var.boolVal );
                }
                else if constexpr ( std::is_same_v<T, int32_t> )
                {
                    var.ChangeType( VT_I4 );
                    arg = static_cast<int32_t>( var.lVal );
                }
                else if constexpr ( std::is_same_v<T, double> )
                {
                    if ( VT_BSTR == var.vt )
                    {
                        arg = std::stod( var.bstrVal );
                    }
                    else
                    {
                        var.ChangeType( VT_R8 );
                        arg = var.dblVal;
                    }
                }
                else if constexpr ( std::is_same_v<T, std::u8string> )
                {
                    var.ChangeType( VT_BSTR );
                    arg = smp::unicode::ToU8( var.bstrVal );
                }
                else
                {
                    static_assert( false, "non-exhaustive visitor!" );
                }
            }, val );
        }
    }

    return 0;
}

LRESULT CDialogProperty::OnClearallBnClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    m_dup_prop_map.clear();
    m_properties.ResetContent();

    return 0;
}

void CDialogProperty::Apply()
{
    // Copy back
    m_parent->get_config_prop().get_val() = m_dup_prop_map;
    m_parent->update_script();
    LoadProperties();
}

void CDialogProperty::LoadProperties( bool reload )
{
    m_properties.ResetContent();

    if ( reload )
    {
        m_dup_prop_map = m_parent->get_config_prop().get_val();
    }

    struct LowerLexCmp
    { // lexicographical comparison but with lower cased chars
        bool operator()( const std::wstring& a, const std::wstring& b ) const
        {
            return ( _wcsicmp( a.c_str(), b.c_str() ) < 0 );
        }
    };
    std::map<std::wstring, HPROPERTY, LowerLexCmp> propMap;
    for ( const auto& [name, pSerializedValue]: m_dup_prop_map )
    {
        HPROPERTY hProp = std::visit( [&name]( auto&& arg ) {
            using T = std::decay_t<decltype( arg )>;
            if constexpr ( std::is_same_v<T, bool> || std::is_same_v<T, int32_t> )
            {
                return PropCreateSimple( name.c_str(), arg );
            }
            else if constexpr ( std::is_same_v<T, double> )
            {
                const std::wstring strNumber = [arg] {
                    if ( std::trunc( arg ) == arg )
                    { // Most likely uint64_t
                        return std::to_wstring( static_cast<uint64_t>( arg ) );
                    }

                    // std::to_string(double) has precision of float
                    return fmt::format( L"{:.16g}", arg );
                }();
                return PropCreateSimple( name.c_str(), strNumber.c_str() );
            }
            else if constexpr ( std::is_same_v<T, std::u8string> )
            {
                return PropCreateSimple( name.c_str(), smp::unicode::ToWide( arg ).c_str() );
            }
            else
            {
                static_assert( false, "non-exhaustive visitor!" );
            }
        }, *pSerializedValue );

        propMap.emplace( name, hProp );
    }

    for ( auto& [name, hProp]: propMap )
    {
        m_properties.AddItem( hProp );
    }
}

LRESULT CDialogProperty::OnDelBnClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    int idx = m_properties.GetCurSel();

    if ( idx >= 0 )
    {
        HPROPERTY hproperty = m_properties.GetProperty( idx );
        std::wstring name = hproperty->GetName();

        m_properties.DeleteItem( hproperty );
        m_dup_prop_map.erase( name );
    }

    return 0;
}

LRESULT CDialogProperty::OnImportBnClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    constexpr COMDLG_FILTERSPEC k_DialogImportExtFilter[2] = {
        { L"Property files", L"*.json;*.smp;*.wsp" },
        { L"All files", L"*.*" },
    };

    fs::path path( smp::file::FileDialog( L"Import from", false, k_DialogImportExtFilter, L"json", L"props" ) );
    if ( path.empty() )
    {
        return 0;
    }
    path = path.lexically_normal();

    file_ptr io;
    auto& abort = smp::GlobalAbortCallback::GetInstance();

    try
    {
        filesystem::g_open_read( io, path.u8string().c_str(), abort );

        const auto extension = path.extension();
        if ( extension == ".json" )
        {
            smp::config::PanelProperties::g_load_json( m_dup_prop_map, *io, abort, true );
        }
        else if ( extension == ".smp" )
        {
            smp::config::PanelProperties::g_load( m_dup_prop_map, *io, abort );
        }
        else if ( extension == ".wsp" )
        {
            smp::config::PanelProperties::g_load_legacy( m_dup_prop_map, *io, abort );
        }
        else
        { // let's brute-force it!
            if ( !smp::config::PanelProperties::g_load_json( m_dup_prop_map, *io, abort, true )
                 && !smp::config::PanelProperties::g_load( m_dup_prop_map, *io, abort ) )
            {
                smp::config::PanelProperties::g_load_legacy( m_dup_prop_map, *io, abort );
            }
        }

        LoadProperties( false );
    }
    catch ( const pfc::exception& )
    {
    }

    return 0;
}

LRESULT CDialogProperty::OnExportBnClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    constexpr COMDLG_FILTERSPEC k_DialogExportExtFilter[2] = {
        { L"Property files", L"*.json" },
        { L"All files", L"*.*" },
    };

    fs::path path( smp::file::FileDialog( L"Save as", true, k_DialogExportExtFilter, L"json", L"props" ) );
    if ( path.empty() )
    {
        return 0;
    }
    path = path.lexically_normal();

    file_ptr io;
    auto& abort = smp::GlobalAbortCallback::GetInstance();

    try
    {
        filesystem::g_open_write_new( io, path.u8string().c_str(), abort );
        smp::config::PanelProperties::g_save_json( m_dup_prop_map, *io, abort, true );
    }
    catch ( const pfc::exception& )
    {
    }

    return 0;
}

} // namespace smp::ui
