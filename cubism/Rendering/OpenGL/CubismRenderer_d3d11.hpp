/*
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at http://live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include "../CubismRenderer.hpp"
#include "CubismFramework.hpp"
#include "Type/csmVector.hpp"
#include "Type/csmRectF.hpp"
#include "Type/csmMap.hpp"

#include <KRApp/dx/d3d11.h>

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {


//  前方宣言
class CubismRenderer_D3D11;
class CubismClippingContext;

/**
 * @brief  クリッピングマスクの処理を実行するクラス
 *
 */
class CubismClippingManager_D3D11
{
    friend class CubismShader_D3D11;
    friend class CubismRenderer_D3D11;

private:
    /**
     * @brief   レンダーテクスチャのリソースを定義する構造体<br>
     *           クリッピングマスクで使用する
     */
    struct CubismRenderTextureResource
    {
        friend class CubismClippingManager_D3D11;

    private:
        /**
         * @brief   引数付きコンストラクタ
         *
         * @param[in]   frameNo ->  レンダラのフレーム番号
         * @param[in]   texture ->  テクスチャのアドレス
         */
        CubismRenderTextureResource(csmInt32 frameNo, kr::d3d11::Texture2D texture) 
			: FrameNo(frameNo), Texture(texture) {};

        /**
         * @brief   デストラクタ
         */
        virtual ~CubismRenderTextureResource() {};

		void removeTexture() noexcept;

        csmInt32    FrameNo;    ///< レンダラのフレーム番号
        kr::d3d11::Texture2D Texture;    ///< テクスチャのアドレス
		kr::d3d11::RenderTargetSet RenderTarget;
    };

    /**
     * @brief カラーチャンネル(RGBA)のフラグを取得する
     *
     * @param[in]   channelNo   ->   カラーチャンネル(RGBA)の番号(0:R , 1:G , 2:B, 3:A)
     */
    CubismRenderer::CubismTextureColor* GetChannelFlagAsColor(csmInt32 channelNo);

    /**
     * @brief   テンポラリのレンダーテクスチャのアドレスを取得する。<br>
     *           FrameBufferObjectが存在しない場合、新しく生成する。
     *
     * @return  レンダーテクスチャのアドレス
     */
    void LoadMaskRenderTexture();

    /**
     * @brief   マスクされる描画オブジェクト群全体を囲む矩形(モデル座標系)を計算する
     *
     * @param[in]   model            ->  モデルのインスタンス
     * @param[in]   clippingContext  ->  クリッピングマスクのコンテキスト
     */
    void CalcClippedDrawTotalBounds(CubismModel& model, CubismClippingContext* clippingContext);

    /**
     * @brief    コンストラクタ
     */
    CubismClippingManager_D3D11();

    /**
     * @brief    デストラクタ
     */
    virtual ~CubismClippingManager_D3D11();

    /**
     * @brief    マネージャの初期化処理<br>
     *           クリッピングマスクを使う描画オブジェクトの登録を行う
     *
     * @param[in]   model           ->  モデルのインスタンス
     * @param[in]   drawableCount   ->  描画オブジェクトの数
     * @param[in]   drawableMasks   ->  描画オブジェクトをマスクする描画オブジェクトのインデックスのリスト
     * @param[in]   drawableMaskCounts   ->  描画オブジェクトをマスクする描画オブジェクトの数
     */
    void Initialize(CubismModel& model, csmInt32 drawableCount, const csmInt32** drawableMasks, const csmInt32* drawableMaskCounts);

    /**
     * @brief   クリッピングコンテキストを作成する。モデル描画時に実行する。
     *
     * @param[in]   model       ->  モデルのインスタンス
     * @param[in]   renderer    ->  レンダラのインスタンス
     */
    void SetupClippingContext(CubismModel& model, CubismRenderer_D3D11* renderer);

    /**
     * @brief   既にマスクを作っているかを確認。<br>
     *          作っているようであれば該当するクリッピングマスクのインスタンスを返す。<br>
     *          作っていなければNULLを返す
     *
     * @param[in]   drawableMasks    ->  描画オブジェクトをマスクする描画オブジェクトのリスト
     * @param[in]   drawableMaskCounts ->  描画オブジェクトをマスクする描画オブジェクトの数
     * @return          該当するクリッピングマスクが存在すればインスタンスを返し、なければNULLを返す。
     */
    CubismClippingContext* FindSameClip(const csmInt32* drawableMasks, csmInt32 drawableMaskCounts) const;

    /**
     * @brief   クリッピングコンテキストを配置するレイアウト。<br>
     *           ひとつのレンダーテクスチャを極力いっぱいに使ってマスクをレイアウトする。<br>
     *           マスクグループの数が4以下ならRGBA各チャンネルに１つずつマスクを配置し、5以上6以下ならRGBAを2,2,1,1と配置する。
     *
     * @param[in]   usingClipCount  ->  配置するクリッピングコンテキストの数
     */
    void SetupLayoutBounds(csmInt32 usingClipCount) const;

    /**
     * @breif   カラーバッファのアドレスを取得する
     *
     * @return  カラーバッファのアドレス
     */
    kr::d3d11::ShaderResourceView GetColorBuffer() const;

    /**
     * @brief   画面描画に使用するクリッピングマスクのリストを取得する
     *
     * @return  画面描画に使用するクリッピングマスクのリスト
     */
    csmVector<CubismClippingContext*>* GetClippingContextListForDraw();

    /**
     *@brief  クリッピングマスクバッファのサイズを設定する
     *
     *@param  size -> クリッピングマスクバッファのサイズ
     *
     */
    void SetClippingMaskBufferSize(csmInt32 size);

    /**
     *@brief  クリッピングマスクバッファのサイズを取得する
     *
     *@return クリッピングマスクバッファのサイズ
     *
     */
    csmInt32 GetClippingMaskBufferSize() const;

	kr::d3d11::Direct3D11 *_device;
    CubismRenderTextureResource      *_maskRenderTexture;      ///< マスク用レンダーテクスチャーのアドレス
	kr::d3d11::ShaderResourceView      _colorBuffer;            ///< マスク用カラーバッファーのアドレス
    csmInt32    _currentFrameNo;         ///< マスクテクスチャに与えるフレーム番号

    csmVector<CubismRenderer::CubismTextureColor*>  _channelColors;
    csmVector<CubismRenderTextureResource*>         _maskTextures;                 ///< マスク用のテクスチャリソースのリスト
    csmVector<CubismClippingContext*>               _clippingContextListForMask;   ///< マスク用クリッピングコンテキストのリスト
    csmVector<CubismClippingContext*>               _clippingContextListForDraw;   ///< 描画用クリッピングコンテキストのリスト
    csmInt32                                        _clippingMaskBufferSize; ///< クリッピングマスクのバッファサイズ（初期値:256）

    CubismMatrix44  _tmpMatrix;              ///< マスク計算用の行列
    CubismMatrix44  _tmpMatrixForMask;       ///< マスク計算用の行列
    CubismMatrix44  _tmpMatrixForDraw;       ///< マスク計算用の行列
    csmRectF        _tmpBoundsOnModel;       ///< マスク配置計算用の矩形

};

/**
 * @brief   クリッピングマスクのコンテキスト
 */
class CubismClippingContext
{
    friend class CubismClippingManager_D3D11;
    friend class CubismShader_D3D11;
    friend class CubismRenderer_D3D11;

private:
    /**
     * @brief   引数付きコンストラクタ
     *
     */
    CubismClippingContext(CubismClippingManager_D3D11* manager, const csmInt32* clippingDrawableIndices, csmInt32 clipCount);

    /**
     * @brief   デストラクタ
     */
    virtual ~CubismClippingContext();

    /**
     * @brief   このマスクにクリップされる描画オブジェクトを追加する
     *
     * @param[in]   drawableIndex   ->  クリッピング対象に追加する描画オブジェクトのインデックス
     */
    void AddClippedDrawable(csmInt32 drawableIndex);

    /**
     * @brief   このマスクを管理するマネージャのインスタンスを取得する。
     *
     * @return  クリッピングマネージャのインスタンス
     */
    CubismClippingManager_D3D11* GetClippingManager();

    csmBool _isUsing;                                ///< 現在の描画状態でマスクの準備が必要ならtrue
    const csmInt32* _clippingIdList;                 ///< クリッピングマスクのIDリスト
    csmInt32 _clippingIdCount;                       ///< クリッピングマスクの数
    csmInt32 _layoutChannelNo;                       ///< RGBAのいずれのチャンネルにこのクリップを配置するか(0:R , 1:G , 2:B , 3:A)
    csmRectF* _layoutBounds;                         ///< マスク用チャンネルのどの領域にマスクを入れるか(View座標-1..1, UVは0..1に直す)
    csmRectF* _allClippedDrawRect;                   ///< このクリッピングで、クリッピングされる全ての描画オブジェクトの囲み矩形（毎回更新）
    CubismMatrix44 _matrixForMask;                   ///< マスクの位置計算結果を保持する行列
    CubismMatrix44 _matrixForDraw;                   ///< 描画オブジェクトの位置計算結果を保持する行列
    csmVector<csmInt32>* _clippedDrawableIndexList;  ///< このマスクにクリップされる描画オブジェクトのリスト

    CubismClippingManager_D3D11* _owner;        ///< このマスクを管理しているマネージャのインスタンス
};

/**
 * @brief   OpenGLES2用のシェーダプログラムを生成・破棄するクラス<br>
 *           シングルトンなクラスであり、CubismShader_D3D11::GetInstance()からアクセスする。
 *
 */
class CubismShader_D3D11
{
    friend class CubismRenderer_D3D11;

private:

    /**
     * @brief   インスタンスを解放する（シングルトン）。
     */
    static void DeleteInstance();

    /**
    * @bref    シェーダープログラムとシェーダ変数のアドレスを保持する構造体
    *
    */
    struct CubismShaderSet
    {
        kr::d3d11::PixelShader ps;
		kr::d3d11::VertexShader vs;
		kr::d3d11::InputLayout layout;

		void load(kr::d3d11::Direct3D11 *_device, kr::Buffer vsdata, kr::Buffer psdata) noexcept;
    };

    /**
     * @brief   privateなコンストラクタ
     */
    CubismShader_D3D11();

    /**
     * @brief   privateなデストラクタ
     */
    virtual ~CubismShader_D3D11();

    /**
     * @brief   シェーダプログラムの一連のセットアップを実行する
     *
     * @param[in]   renderer              ->  レンダラのインスタンス
     * @param[in]   textureId             ->  GPUのテクスチャID
     * @param[in]   vertexCount           ->  ポリゴンメッシュの頂点数
     * @param[in]   vertexArray           ->  ポリゴンメッシュの頂点配列
     * @param[in]   uvArray               ->  uv配列
     * @param[in]   opacity               ->  不透明度
     * @param[in]   colorBlendMode        ->  カラーブレンディングのタイプ
     * @param[in]   baseColor             ->  ベースカラー
     * @param[in]   isPremultipliedAlpha  ->  乗算済みアルファかどうか
     * @param[in]   matrix4x4             ->  Model-View-Projection行列
     */
    void SetupShaderProgram(CubismRenderer_D3D11* renderer
							, ID3D11ShaderResourceView *texture
                            , csmInt32 vertexCount, csmFloat32 * vertexArray
                            , csmFloat32 * uvArray, csmFloat32 opacity
                            , CubismRenderer::CubismBlendMode colorBlendMode
                            , CubismRenderer::CubismTextureColor baseColor
                            , csmBool isPremultipliedAlpha, CubismMatrix44 matrix4x4);

    /**
     * @brief   シェーダプログラムを解放する
     */
    void ReleaseShaderProgram();

    /**
     * @brief   シェーダプログラムを初期化する
     */
    void GenerateShaders();

	kr::d3d11::Direct3D11 * _device;
    csmVector<CubismShaderSet*> _shaderSets;   ///< ロードしたシェーダプログラムを保持する変数

};

/**
 * @brief   OpenGLES2用の描画命令を実装したクラス
 *
 */
class CubismRenderer_D3D11 : public CubismRenderer
{
    friend class CubismRenderer;
    friend class CubismClippingManager_D3D11;
    friend class CubismShader_D3D11;

public:
    /**
     * @brief    レンダラの初期化処理を実行する<br>
     *           引数に渡したモデルからレンダラの初期化処理に必要な情報を取り出すことができる
     *
     * @param[in]  model -> モデルのインスタンス
     */
    void Initialize(Framework::CubismModel* model);

	void SetDevice(kr::d3d11::Direct3D11 * device) noexcept;

    /**
     * @brief  クリッピングマスクバッファのサイズを設定する<br>
     *         マスク用のFrameBufferを破棄・再作成するため処理コストは高い。
     *
     * @param[in]  size -> クリッピングマスクバッファのサイズ
     *
     */
    void SetClippingMaskBufferSize(csmInt32 size);

    /**
     * @brief  クリッピングマスクバッファのサイズを取得する
     *
     * @return クリッピングマスクバッファのサイズ
     *
     */
    csmInt32 GetClippingMaskBufferSize() const;

	void BindTexture(csmInt32 index, kr::d3d11::ShaderResourceView texture) noexcept;

protected:
    /**
     * @brief   コンストラクタ
     */
    CubismRenderer_D3D11();

    /**
     * @brief   デストラクタ
     */
    virtual ~CubismRenderer_D3D11();

    /**
     * @brief   モデルを描画する実際の処理
     *
     */
    void DoDrawModel();

    /**
     * @brief   [オーバーライド]<br>
     *           描画オブジェクト（アートメッシュ）を描画する。<br>
     *           ポリゴンメッシュとテクスチャ番号をセットで渡す。
     *
     * @param[in]   textureNo       ->  描画するテクスチャ番号
     * @param[in]   indexCount      ->  描画オブジェクトのインデックス値
     * @param[in]   vertexCount     ->  ポリゴンメッシュの頂点数
     * @param[in]   indexArray      ->  ポリゴンメッシュのインデックス配列
     * @param[in]   vertexArray     ->  ポリゴンメッシュの頂点配列
     * @param[in]   uvArray         ->  uv配列
     * @param[in]   opacity         ->  不透明度
     * @param[in]   colorBlendMode  ->  カラー合成タイプ
     *
     */
    void DrawMesh(csmInt32 textureNo, csmInt32 indexCount, csmInt32 vertexCount
                  , csmUint16* indexArray, csmFloat32* vertexArray, csmFloat32* uvArray
                  , csmFloat32 opacity, CubismBlendMode colorBlendMode);


private:
    // Prevention of copy Constructor
    CubismRenderer_D3D11(const CubismRenderer_D3D11&);
    CubismRenderer_D3D11& operator=(const CubismRenderer_D3D11&);

    /**
     * @brief   レンダラが保持する静的なリソースを解放する<br>
     *           OpenGLES2の静的なシェーダプログラムを解放する
     */
    static void DoStaticRelease();

    /**
     * @brief   描画開始時の追加処理。<br>
     *           モデルを描画する前にクリッピングマスクに必要な処理を実装している。
     */
    void PreDraw();

    /**
     * @brief   描画完了後の追加処理。
     *
     */
    void PostDraw(){};

    /**
     * @brief   モデル描画直前のOpenGLES2のステートを保持する
     */
    virtual void SaveProfile();

    /**
     * @brief   モデル描画直前のOpenGLES2のステートを保持する
     */
    virtual void RestoreProfile();

    /**
     * @brief   マスクテクスチャに描画するクリッピングコンテキストをセットする。
     */
    void SetClippingContextBufferForMask(CubismClippingContext* clip);

    /**
     * @brief   マスクテクスチャに描画するクリッピングコンテキストを取得する。
     *
     * @return  マスクテクスチャに描画するクリッピングコンテキスト
     */
    CubismClippingContext* GetClippingContextBufferForMask() const;

    /**
     * @brief   画面上に描画するクリッピングコンテキストをセットする。
     */
    void SetClippingContextBufferForDraw(CubismClippingContext* clip);

    /**
     * @brief   画面上に描画するクリッピングコンテキストを取得する。
     *
     * @return  画面上に描画するクリッピングコンテキスト
     */
    CubismClippingContext* GetClippingContextBufferForDraw() const;

#ifdef CSM_TARGET_WIN_GL
    /**
     * @brief   Windows対応。OpenGL命令のバインドを行う。
     */
    void  InitializeGlFunctions();

    /**
     * @brief   Windows対応。OpenGL命令のバインドする際に関数ポインタを取得する。
     */
    void* WinGlGetProcAddress(const csmChar* name);

    /**
     * @brief   Windows対応。OpenGLのエラーチェック。
     */
    void  CheckGlError(const csmChar* message);
#endif

	kr::d3d11::Direct3D11 * _device;
	kr::d3d11::DepthStencilState m_disableDepth;
	kr::d3d11::SamplerState _sampler;
    csmMap<csmInt32, kr::d3d11::ShaderResourceView>            _textures;                      ///< モデルが参照するテクスチャとレンダラでバインドしているテクスチャとのマップ
    csmVector<csmInt32>                 _sortedDrawableIndexList;       ///< 描画オブジェクトのインデックスを描画順に並べたリスト
   // CubismRendererProfile_D3D11     _rendererProfile;               ///< OpenGLのステートを保持するオブジェクト
    CubismClippingManager_D3D11*    _clippingManager;               ///< クリッピングマスク管理オブジェクト
    CubismClippingContext*              _clippingContextBufferForMask;  ///< マスクテクスチャに描画するためのクリッピングコンテキスト
    CubismClippingContext*              _clippingContextBufferForDraw;  ///< 画面上描画するためのクリッピングコンテキスト
	CubismShader_D3D11 _shader;

};

}}}}
//------------ LIVE2D NAMESPACE ------------
