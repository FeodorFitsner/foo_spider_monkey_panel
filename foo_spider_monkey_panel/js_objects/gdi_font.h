#pragma once

#include <js_objects/object_base.h>

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;
struct JSFunctionSpec;
struct JSPropertySpec;

namespace Gdiplus
{
class Font;
}

namespace mozjs
{

class JsGdiFont
    : public JsObjectBase<JsGdiFont>
{
public:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;

public:
    ~JsGdiFont();

    static std::unique_ptr<JsGdiFont> CreateNative( JSContext* cx, std::unique_ptr<Gdiplus::Font> gdiFont, HFONT hFont, bool isManaged );
    static size_t GetInternalSize( const std::unique_ptr<Gdiplus::Font>& gdiFont, HFONT hFont, bool isManaged );

public:
    Gdiplus::Font* GdiFont() const;
    HFONT GetHFont() const;

public: // ctor
    static JSObject* Constructor( JSContext* cx, const std::wstring& fontName, float pxSize, uint32_t style = 0 );
    static JSObject* ConstructorWithOpt( JSContext* cx, size_t optArgCount, const std::wstring& fontName, float pxSize, uint32_t style );

public: // props
    uint32_t get_Height() const;
    std::wstring get_Name() const;
    float get_Size() const;
    uint32_t get_Style() const;

private:
    JsGdiFont( JSContext* cx, std::unique_ptr<Gdiplus::Font> gdiFont, HFONT hFont, bool isManaged );

private:
    JSContext* pJsCtx_ = nullptr;
    bool isManaged_;
    std::unique_ptr<Gdiplus::Font> pGdi_;
    HFONT hFont_ = nullptr;
};

} // namespace mozjs
