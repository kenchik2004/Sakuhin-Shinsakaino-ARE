//---------------------------------------------------------------------------
//! @file   Shader.h
//! @brief  シェーダー
//---------------------------------------------------------------------------
#pragma once

#include <tuple>

//===========================================================================
//! シェーダー基底
//===========================================================================
class Shader
{
public:
    // デストラクタ
    virtual ~Shader();

    // 作成
    //! @param [in] path                ファイルパス
    //! @param [in] dxlib_shader_type   [DxLib] シェーダーの種類(DX_SHADERTYPE_VERTEXなど)
    //! @param [in] variant_count       シェーダーバリエーション数(default:0)
    Shader(std::string_view path, u32 dxlib_shader_type, u32 variant_count = 0);

    // [DxLib] シェーダーハンドルを取得
    operator int () const;

    // [DxLib] シェーダーハンドルを取得 (バリエーション指定)
    int variant(u32 variant_index) const;

    // [DxLib] シェーダーバイトコードを取得 (バリエーション指定)
    std::tuple<const void*, size_t> shader_bytecode(u32 variant_index = 0) const;

    //! ファイルパスを取得
    const std::wstring& path() const;

    // ファイル監視を更新
    static void updateFileWatcher();

private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};

//===========================================================================
//! 頂点シェーダー
//===========================================================================
class ShaderVs final : public Shader
{
public:
    ShaderVs(std::string_view path, u32 variant_count = 0)
        : Shader(path, DX_SHADERTYPE_VERTEX, variant_count)
    {
    }

private:
};

//===========================================================================
//! ピクセルシェーダー
//===========================================================================
class ShaderPs final : public Shader
{
public:
    ShaderPs(std::string_view path, u32 variant_count = 0)
        : Shader(path, DX_SHADERTYPE_PIXEL, variant_count)
    {
    }
};

//===========================================================================
//! ジオメトリシェーダー
//===========================================================================
class ShaderGs final : public Shader
{
public:
    ShaderGs(std::string_view path, u32 variant_count = 0)
        : Shader(path, DX_SHADERTYPE_GEOMETRY, variant_count)
    {
    }
};
