/*
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at http://live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "Math/CubismMatrix44.hpp"
#include "Type/csmVector.hpp"
#include "Model/CubismModel.hpp"
#include "CubismRenderer_d3d11.hpp"
#include <float.h>

#include "shader/FragShaderSrc.h"
#include "shader/FragShaderSrcAddMask.h"
#include "shader/FragShaderSrcAddMaskPremultipliedAlpha.h"
#include "shader/FragShaderSrcDebug.h"
#include "shader/FragShaderSrcMultMask.h"
#include "shader/FragShaderSrcMultMaskPremultipliedAlpha.h"
#include "shader/FragShaderSrcNormalMask.h"
#include "shader/FragShaderSrcNormalMaskPremultipliedAlpha.h"
#include "shader/FragShaderSrcPremultipliedAlpha.h"
#include "shader/FragShaderSrcSetupMask.h"
#include "shader/VertShaderSrc.h"
#include "shader/VertShaderSrcDebug.h"
#include "shader/VertShaderSrcMasked.h"
#include "shader/VertShaderSrcSetupMask.h"

using kr::d3d11::Direct3D11;
using kr::d3d11::TextureAddressMode;
using kr::d3d11::Filter;
using kr::d3d11::RenderTargetSet;
using kr::d3d11::DeviceContext;
using kr::d3d11::PixelShader;
using kr::d3d11::VertexShader;
using kr::d3d11::BlendMode;
using kr::d3d11::CullMode;
using kr::d3d11::FillMode;
using kr::d3d11::PrimitiveTopology;
using kr::d3d11::InputLayout;
using kr::d3d11::VertexLayout;

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {

/*********************************************************************************************************************
*                                      CubismClippingManager_D3D11
********************************************************************************************************************/
///< ファイルスコープの変数宣言
namespace {
const csmInt32 ColorChannelCount = 4;   ///< 実験時に1チャンネルの場合は1、RGBだけの場合は3、アルファも含める場合は4
}

CubismClippingManager_D3D11::CubismClippingManager_D3D11() :
                                                                   _maskRenderTexture()
                                                                   , _colorBuffer(0)
                                                                   , _currentFrameNo(0)
                                                                   , _clippingMaskBufferSize(256)
{
    CubismRenderer::CubismTextureColor* tmp;
    tmp = CSM_NEW CubismRenderer::CubismTextureColor();
    tmp->R = 1.0f;
    tmp->G = 0.0f;
    tmp->B = 0.0f;
    tmp->A = 0.0f;
    _channelColors.PushBack(tmp);
    tmp = CSM_NEW CubismRenderer::CubismTextureColor();
    tmp->R = 0.0f;
    tmp->G = 1.0f;
    tmp->B = 0.0f;
    tmp->A = 0.0f;
    _channelColors.PushBack(tmp);
    tmp = CSM_NEW CubismRenderer::CubismTextureColor();
    tmp->R = 0.0f;
    tmp->G = 0.0f;
    tmp->B = 1.0f;
    tmp->A = 0.0f;
    _channelColors.PushBack(tmp);
    tmp = CSM_NEW CubismRenderer::CubismTextureColor();
    tmp->R = 0.0f;
    tmp->G = 0.0f;
    tmp->B = 0.0f;
    tmp->A = 1.0f;
    _channelColors.PushBack(tmp);

}

CubismClippingManager_D3D11::~CubismClippingManager_D3D11()
{
    for (csmUint32 i = 0; i < _clippingContextListForMask.GetSize(); i++)
    {
        if (_clippingContextListForMask[i]) CSM_DELETE_SELF(CubismClippingContext, _clippingContextListForMask[i]);
        _clippingContextListForMask[i] = NULL;
    }

    // _clippingContextListForDrawは_clippingContextListForMaskにあるインスタンスを指している。上記の処理により要素ごとのDELETEは不要。
    for (csmUint32 i = 0; i < _clippingContextListForDraw.GetSize(); i++)
    {
        _clippingContextListForDraw[i] = NULL;
    }
    for (CubismRenderTextureResource *& texture : _maskTextures)
    {
		if (texture)
		{
			texture->removeTexture();
			CSM_DELETE_SELF(CubismRenderTextureResource, texture);
			texture = NULL;
		}
    }

    for (csmUint32 i = 0; i < _channelColors.GetSize(); i++)
    {
        if (_channelColors[i]) CSM_DELETE(_channelColors[i]);
        _channelColors[i] = NULL;
    }
}

void CubismClippingManager_D3D11::CubismRenderTextureResource::removeTexture() noexcept
{
	Texture = nullptr;
}

void CubismClippingManager_D3D11::Initialize(CubismModel& model, csmInt32 drawableCount, const csmInt32** drawableMasks, const csmInt32* drawableMaskCounts)
{
    //クリッピングマスクを使う描画オブジェクトを全て登録する
    //クリッピングマスクは、通常数個程度に限定して使うものとする
    for (csmInt32 i = 0; i < drawableCount; i++)
    {
        if (drawableMaskCounts[i] <= 0)
        {
            //クリッピングマスクが使用されていないアートメッシュ（多くの場合使用しない）
            _clippingContextListForDraw.PushBack(NULL);
            continue;
        }

        // 既にあるClipContextと同じかチェックする
        CubismClippingContext* cc = FindSameClip(drawableMasks[i], drawableMaskCounts[i]);
        if (cc == NULL)
        {
            // 同一のマスクが存在していない場合は生成する
            cc = CSM_NEW CubismClippingContext(this, drawableMasks[i], drawableMaskCounts[i]);
            _clippingContextListForMask.PushBack(cc);
        }

        cc->AddClippedDrawable(i);

        _clippingContextListForDraw.PushBack(cc);
    }
}

CubismClippingContext* CubismClippingManager_D3D11::FindSameClip(const csmInt32* drawableMasks, csmInt32 drawableMaskCounts) const
{
    // 作成済みClippingContextと一致するか確認
    for (csmUint32 i = 0; i < _clippingContextListForMask.GetSize(); i++)
    {
        CubismClippingContext* cc = _clippingContextListForMask[i];
        const csmInt32 count = cc->_clippingIdCount;
        if (count != drawableMaskCounts) continue; //個数が違う場合は別物
        csmInt32 samecount = 0;

        // 同じIDを持つか確認。配列の数が同じなので、一致した個数が同じなら同じ物を持つとする。
        for (csmInt32 j = 0; j < count; j++)
        {
            const csmInt32 clipId = cc->_clippingIdList[j];
            for (csmInt32 k = 0; k < count; k++)
            {
                if (drawableMasks[k] == clipId)
                {
                    samecount++;
                    break;
                }
            }
        }
        if (samecount == count)
        {
            return cc;
        }
    }
    return NULL; //見つからなかった
}

void CubismClippingManager_D3D11::LoadMaskRenderTexture()
{
    // テンポラリのRenderTexutreを取得する
    for (CubismRenderTextureResource* texture : _maskTextures)
    {
        if (texture->Texture != nullptr) //前回使ったものを返す
        {
			texture->FrameNo = _currentFrameNo;
			_maskRenderTexture = texture;
            return;
        }
    }

    // FramebufferObjectが存在しない場合、新しく生成する
    const csmInt32 size = _clippingMaskBufferSize;

	kr::d3d11::Texture2D texture;
	texture.create(
		kr::d3d11::BindRenderTarget|kr::d3d11::BindShaderResource, 
		kr::d3d11::Usage::Staging, DXGI_FORMAT_B8G8R8A8_UNORM, size, size, 0, 0, 0);
	
	CubismRenderTextureResource * res = CSM_NEW CubismRenderTextureResource(_currentFrameNo, texture);
    _maskTextures.PushBack(res);
	_maskRenderTexture = res;
}

void CubismClippingManager_D3D11::SetupClippingContext(CubismModel& model, CubismRenderer_D3D11* renderer)
{
    _currentFrameNo++;

    // 全てのクリッピングを用意する
    // 同じクリップ（複数の場合はまとめて１つのクリップ）を使う場合は１度だけ設定する
    csmInt32 usingClipCount = 0;
    for (csmUint32 clipIndex = 0; clipIndex < _clippingContextListForMask.GetSize(); clipIndex++)
    {
        // １つのクリッピングマスクに関して
        CubismClippingContext* cc = _clippingContextListForMask[clipIndex];

        // このクリップを利用する描画オブジェクト群全体を囲む矩形を計算
        CalcClippedDrawTotalBounds(model, cc);

        if (cc->_isUsing)
        {
            usingClipCount++; //使用中としてカウント
        }
    }

    // マスク作成処理
    if (usingClipCount > 0)
    {
        // 現在のビューポートの値を退避
		DeviceContext * ctx = _device->getContext();
		kr::d3d11::Viewport vp = ctx->getViewport();

        // 生成したFrameBufferと同じサイズでビューポートを設定
		ctx->setViewport((float)_clippingMaskBufferSize, (float)_clippingMaskBufferSize);

        // マスクactive切り替え前のFBOを退避
		RenderTargetSet rts = ctx->getRenderTarget();

        // マスクをactiveにする
        LoadMaskRenderTexture();

        // モデル描画時にDrawMeshNowに渡される変換（モデルtoワールド座標変換）
        CubismMatrix44 modelToWorldF = renderer->GetMvpMatrix();

        renderer->PreDraw(); // バッファをクリアする

        // 各マスクのレイアウトを決定していく
        SetupLayoutBounds(usingClipCount);

        // ---------- マスク描画処理 -----------
        // マスク用RenderTextureをactiveにセット
		ctx->setRenderTarget(_maskRenderTexture->RenderTarget);

        // マスクをクリアする
        //（仮仕様） 1が無効（描かれない）領域、0が有効（描かれる）領域。（シェーダで Cd*Csで0に近い値をかけてマスクを作る。1をかけると何も起こらない）
		ctx->clear(_maskRenderTexture->RenderTarget.renderTargetView, {1, 1, 1, 1});
		ctx->clear(_maskRenderTexture->RenderTarget.depthStencilView);

        // 実際にマスクを生成する
        // 全てのマスクをどの様にレイアウトして描くかを決定し、ClipContext , ClippedDrawContext に記憶する
        for (csmUint32 clipIndex = 0; clipIndex < _clippingContextListForMask.GetSize(); clipIndex++)
        {
            // --- 実際に１つのマスクを描く ---
            CubismClippingContext* clipContext = _clippingContextListForMask[clipIndex];
            csmRectF* allClippedDrawRect = clipContext->_allClippedDrawRect; //このマスクを使う、全ての描画オブジェクトの論理座標上の囲み矩形
            csmRectF* layoutBoundsOnTex01 = clipContext->_layoutBounds; //この中にマスクを収める

            // モデル座標上の矩形を、適宜マージンを付けて使う
            const csmFloat32 MARGIN = 0.05f;
            _tmpBoundsOnModel.SetRect(allClippedDrawRect);
            _tmpBoundsOnModel.Expand(allClippedDrawRect->Width * MARGIN, allClippedDrawRect->Height * MARGIN);
            //########## 本来は割り当てられた領域の全体を使わず必要最低限のサイズがよい

            // シェーダ用の計算式を求める。回転を考慮しない場合は以下のとおり
            // movePeriod' = movePeriod * scaleX + offX		  [[ movePeriod' = (movePeriod - tmpBoundsOnModel.movePeriod)*scale + layoutBoundsOnTex01.movePeriod ]]
            const csmFloat32 scaleX = layoutBoundsOnTex01->Width / _tmpBoundsOnModel.Width;
            const csmFloat32 scaleY = layoutBoundsOnTex01->Height / _tmpBoundsOnModel.Height;

            // マスク生成時に使う行列を求める
            {
                // シェーダに渡す行列を求める <<<<<<<<<<<<<<<<<<<<<<<< 要最適化（逆順に計算すればシンプルにできる）
                _tmpMatrix.LoadIdentity();
                {
                    // Layout0..1 を -1..1に変換
                    _tmpMatrix.TranslateRelative(-1.0f, -1.0f);
                    _tmpMatrix.ScaleRelative(2.0f, 2.0f);
                }
                {
                    // view to Layout0..1
                    _tmpMatrix.TranslateRelative(layoutBoundsOnTex01->X, layoutBoundsOnTex01->Y); //new = [translate]
                    _tmpMatrix.ScaleRelative(scaleX, scaleY); //new = [translate][scale]
                    _tmpMatrix.TranslateRelative(-_tmpBoundsOnModel.X, -_tmpBoundsOnModel.Y);
                    //new = [translate][scale][translate]
                }
                // tmpMatrixForMask が計算結果
                _tmpMatrixForMask.SetMatrix(_tmpMatrix.GetArray());
            }

            //--------- draw時の mask 参照用行列を計算
            {
                // シェーダに渡す行列を求める <<<<<<<<<<<<<<<<<<<<<<<< 要最適化（逆順に計算すればシンプルにできる）
                _tmpMatrix.LoadIdentity();
                {
                    _tmpMatrix.TranslateRelative(layoutBoundsOnTex01->X, layoutBoundsOnTex01->Y); //new = [translate]
                    _tmpMatrix.ScaleRelative(scaleX, scaleY); //new = [translate][scale]
                    _tmpMatrix.TranslateRelative(-_tmpBoundsOnModel.X, -_tmpBoundsOnModel.Y);
                    //new = [translate][scale][translate]
                }

                _tmpMatrixForDraw.SetMatrix(_tmpMatrix.GetArray());
            }

            clipContext->_matrixForMask.SetMatrix(_tmpMatrixForMask.GetArray());

            clipContext->_matrixForDraw.SetMatrix(_tmpMatrixForDraw.GetArray());

            const csmInt32 clipDrawCount = clipContext->_clippingIdCount;
            for (csmInt32 i = 0; i < clipDrawCount; i++)
            {
                const csmInt32 clipDrawIndex = clipContext->_clippingIdList[i];

                renderer->IsCulling(model.GetDrawableCulling(clipDrawIndex) != 0);

                // 今回専用の変換を適用して描く
                // チャンネルも切り替える必要がある(A,R,G,B)
                renderer->SetClippingContextBufferForMask(clipContext);
                renderer->DrawMesh(
                    model.GetDrawableTextureIndices(clipDrawIndex),
                    model.GetDrawableVertexIndexCount(clipDrawIndex),
                    model.GetDrawableVertexCount(clipDrawIndex),
                    const_cast<csmUint16*>(model.GetDrawableVertexIndices(clipDrawIndex)),
                    const_cast<csmFloat32*>(model.GetDrawableVertices(clipDrawIndex)),
                    reinterpret_cast<csmFloat32*>(const_cast<Core::csmVector2*>(model.GetDrawableVertexUvs(clipDrawIndex))),
                    model.GetDrawableOpacity(clipDrawIndex),
                    CubismRenderer::CubismBlendMode::CubismBlendMode_Normal   //クリッピングは通常描画を強制
                );
            }
        }

        // --- 後処理 ---
		ctx->setRenderTarget(rts); // 描画対象を戻す
        renderer->SetClippingContextBufferForMask(NULL);

		ctx->setViewport(vp);
    }
}

void CubismClippingManager_D3D11::CalcClippedDrawTotalBounds(CubismModel& model, CubismClippingContext* clippingContext)
{
    // 被クリッピングマスク（マスクされる描画オブジェクト）の全体の矩形
    csmFloat32 clippedDrawTotalMinX = FLT_MAX, clippedDrawTotalMinY = FLT_MAX;
    csmFloat32 clippedDrawTotalMaxX = FLT_MIN, clippedDrawTotalMaxY = FLT_MIN;

    // このマスクが実際に必要か判定する
    // このクリッピングを利用する「描画オブジェクト」がひとつでも使用可能であればマスクを生成する必要がある

    const csmInt32 clippedDrawCount = clippingContext->_clippedDrawableIndexList->GetSize();
    for (csmInt32 clippedDrawableIndex = 0; clippedDrawableIndex < clippedDrawCount; clippedDrawableIndex++)
    {
        // マスクを使用する描画オブジェクトの描画される矩形を求める
        const csmInt32 drawableIndex = (*clippingContext->_clippedDrawableIndexList)[clippedDrawableIndex];

        const csmInt32 drawableVertexCount = model.GetDrawableVertexCount(drawableIndex);
        csmFloat32* drawableVertexes = const_cast<csmFloat32*>(model.GetDrawableVertices(drawableIndex));

        csmFloat32 minX = FLT_MAX, minY = FLT_MAX;
        csmFloat32 maxX = FLT_MIN, maxY = FLT_MIN;

        csmInt32 loop = drawableVertexCount * Constant::VertexStep;
        for (csmInt32 pi = Constant::VertexOffset; pi < loop; pi += Constant::VertexStep)
        {
            csmFloat32 x = drawableVertexes[pi];
            csmFloat32 y = drawableVertexes[pi + 1];
            if (x < minX) minX = x;
            if (x > maxX) maxX = x;
            if (y < minY) minY = y;
            if (y > maxY) maxY = y;
        }

        //
        if (minX == FLT_MAX) continue; //有効な点がひとつも取れなかったのでスキップする

        // 全体の矩形に反映
        if (minX < clippedDrawTotalMinX) clippedDrawTotalMinX = minX;
        if (minY < clippedDrawTotalMinY) clippedDrawTotalMinY = minY;
        if (maxX > clippedDrawTotalMaxX) clippedDrawTotalMaxX = maxX;
        if (maxY > clippedDrawTotalMaxY) clippedDrawTotalMaxY = maxY;
    }
    if (clippedDrawTotalMinX == FLT_MAX)
    {
        clippingContext->_allClippedDrawRect->X = 0.0f;
        clippingContext->_allClippedDrawRect->Y = 0.0f;
        clippingContext->_allClippedDrawRect->Width = 0.0f;
        clippingContext->_allClippedDrawRect->Height = 0.0f;
        clippingContext->_isUsing = false;
    }
    else
    {
        clippingContext->_isUsing = true;
        csmFloat32 w = clippedDrawTotalMaxX - clippedDrawTotalMinX;
        csmFloat32 h = clippedDrawTotalMaxY - clippedDrawTotalMinY;
        clippingContext->_allClippedDrawRect->X = clippedDrawTotalMinX;
        clippingContext->_allClippedDrawRect->Y = clippedDrawTotalMinY;
        clippingContext->_allClippedDrawRect->Width = w;
        clippingContext->_allClippedDrawRect->Height = h;
    }
}

void CubismClippingManager_D3D11::SetupLayoutBounds(csmInt32 usingClipCount) const
{
    // ひとつのRenderTextureを極力いっぱいに使ってマスクをレイアウトする
    // マスクグループの数が4以下ならRGBA各チャンネルに１つずつマスクを配置し、5以上6以下ならRGBAを2,2,1,1と配置する

    // RGBAを順番に使っていく。
    const csmInt32 div = usingClipCount / ColorChannelCount; //１チャンネルに配置する基本のマスク個数
    const csmInt32 mod = usingClipCount % ColorChannelCount; //余り、この番号のチャンネルまでに１つずつ配分する

    // RGBAそれぞれのチャンネルを用意していく(0:R , 1:G , 2:B, 3:A, )
    csmInt32 curClipIndex = 0; //順番に設定していく

    for (csmInt32 channelNo = 0; channelNo < ColorChannelCount; channelNo++)
    {
        // このチャンネルにレイアウトする数
        const csmInt32 layoutCount = div + (channelNo < mod ? 1 : 0);

        // 分割方法を決定する
        if (layoutCount == 0)
        {
            // 何もしない
        }
        else if (layoutCount == 1)
        {
            //全てをそのまま使う
            CubismClippingContext* cc = _clippingContextListForMask[curClipIndex++];
            cc->_layoutChannelNo = channelNo;
            cc->_layoutBounds->X = 0.0f;
            cc->_layoutBounds->Y = 0.0f;
            cc->_layoutBounds->Width = 1.0f;
            cc->_layoutBounds->Height = 1.0f;
        }
        else if (layoutCount == 2)
        {
            for (csmInt32 i = 0; i < layoutCount; i++)
            {
                const csmInt32 xpos = i % 2;

                CubismClippingContext* cc = _clippingContextListForMask[curClipIndex++];
                cc->_layoutChannelNo = channelNo;

                cc->_layoutBounds->X = xpos * 0.5f;
                cc->_layoutBounds->Y = 0.0f;
                cc->_layoutBounds->Width = 0.5f;
                cc->_layoutBounds->Height = 1.0f;
                //UVを2つに分解して使う
            }
        }
        else if (layoutCount <= 4)
        {
            //4分割して使う
            for (csmInt32 i = 0; i < layoutCount; i++)
            {
                const csmInt32 xpos = i % 2;
                const csmInt32 ypos = i / 2;

                CubismClippingContext* cc = _clippingContextListForMask[curClipIndex++];
                cc->_layoutChannelNo = channelNo;

                cc->_layoutBounds->X = xpos * 0.5f;
                cc->_layoutBounds->Y = ypos * 0.5f;
                cc->_layoutBounds->Width = 0.5f;
                cc->_layoutBounds->Height = 0.5f;
            }
        }
        else if (layoutCount <= 9)
        {
            //9分割して使う
            for (csmInt32 i = 0; i < layoutCount; i++)
            {
                const csmInt32 xpos = i % 3;
                const csmInt32 ypos = i / 3;

                CubismClippingContext* cc = _clippingContextListForMask[curClipIndex++];
                cc->_layoutChannelNo = channelNo;

                cc->_layoutBounds->X = xpos / 3.0f;
                cc->_layoutBounds->Y = ypos / 3.0f;
                cc->_layoutBounds->Width = 1.0f / 3.0f;
                cc->_layoutBounds->Height = 1.0f / 3.0f;
            }
        }
        else
        {
            CubismLogError("not supported mask count : %d", layoutCount);
        }
    }
}

CubismRenderer::CubismTextureColor* CubismClippingManager_D3D11::GetChannelFlagAsColor(csmInt32 channelNo)
{
    return _channelColors[channelNo];
}

kr::d3d11::ShaderResourceView CubismClippingManager_D3D11::GetColorBuffer() const
{
    return _colorBuffer;
}

csmVector<CubismClippingContext*>* CubismClippingManager_D3D11::GetClippingContextListForDraw()
{
    return &_clippingContextListForDraw;
}

void CubismClippingManager_D3D11::SetClippingMaskBufferSize(csmInt32 size)
{
    _clippingMaskBufferSize = size;
}

csmInt32 CubismClippingManager_D3D11::GetClippingMaskBufferSize() const
{
    return _clippingMaskBufferSize;
}

/*********************************************************************************************************************
*                                      CubismClippingContext
********************************************************************************************************************/
CubismClippingContext::CubismClippingContext(CubismClippingManager_D3D11* manager, const csmInt32* clippingDrawableIndices, csmInt32 clipCount)
{
    _owner = manager;

    // クリップしている（＝マスク用の）Drawableのインデックスリスト
    _clippingIdList = clippingDrawableIndices;

    // マスクの数
    _clippingIdCount = clipCount;

    _allClippedDrawRect = CSM_NEW csmRectF();
    _layoutBounds = CSM_NEW csmRectF();

    _clippedDrawableIndexList = CSM_NEW csmVector<csmInt32>();
}

CubismClippingContext::~CubismClippingContext()
{
    if (_layoutBounds != NULL)
    {
        CSM_DELETE(_layoutBounds);
        _layoutBounds = NULL;
    }

    if (_allClippedDrawRect != NULL)
    {
        CSM_DELETE(_allClippedDrawRect);
        _allClippedDrawRect = NULL;
    }

    if (_clippedDrawableIndexList != NULL)
    {
        CSM_DELETE(_clippedDrawableIndexList);
        _clippedDrawableIndexList = NULL;
    }
}

void CubismClippingContext::AddClippedDrawable(csmInt32 drawableIndex)
{
    _clippedDrawableIndexList->PushBack(drawableIndex);
}

CubismClippingManager_D3D11* CubismClippingContext::GetClippingManager()
{
    return _owner;
}

/*********************************************************************************************************************
*                                       CubismShader_D3D11
********************************************************************************************************************/
namespace {
    const csmInt32 ShaderCount = 13; ///< シェーダの数 = マスク生成用 + (通常 + 加算 + 乗算) * (マスク無 + マスク有 + マスク無の乗算済アルファ対応版 + マスク有の乗算済アルファ対応版)
    CubismShader_D3D11* s_instance;
}

enum ShaderNames
{
    // SetupMask
    ShaderNames_SetupMask,

    //Normal
    ShaderNames_Normal,
    ShaderNames_NormalMasked,
    ShaderNames_NormalPremultipliedAlpha,
    ShaderNames_NormalMaskedPremultipliedAlpha,

    //Add
    ShaderNames_Add,
    ShaderNames_AddMasked,
    ShaderNames_AddPremultipliedAlpha,
    ShaderNames_AddMaskedPremultipliedAlpha,

    //Mult
    ShaderNames_Mult,
    ShaderNames_MultMasked,
    ShaderNames_MultPremultipliedAlpha,
    ShaderNames_MultMaskedPremultipliedAlpha,
};

void CubismShader_D3D11::ReleaseShaderProgram()
{
    for (CubismShaderSet *& shader : _shaderSets)
    {
        if (shader)
        {
			shader->ps = nullptr;
			shader->vs = nullptr;
            CSM_DELETE(shader);
        }
    }
}


CubismShader_D3D11::CubismShader_D3D11()
{ }

CubismShader_D3D11::~CubismShader_D3D11()
{
    ReleaseShaderProgram();
}

void CubismShader_D3D11::DeleteInstance()
{
    if (s_instance)
    {
        CSM_DELETE_SELF(CubismShader_D3D11, s_instance);
        s_instance = NULL;
    }
}

void CubismShader_D3D11::CubismShaderSet::load(Direct3D11 *_device, kr::Buffer vsdata, kr::Buffer psdata) noexcept
{
	// Create shader program.

	vs.create(vsdata);
	ps.create(psdata);

	layout.create(vsdata, {
		VertexLayout(VertexLayout::Position, DXGI_FORMAT_R32G32_FLOAT),
		VertexLayout(VertexLayout::TexCoord, DXGI_FORMAT_R32G32_FLOAT),
	});
}


void CubismShader_D3D11::GenerateShaders()
{
    for (csmInt32 i = 0; i < ShaderCount; i++)
    {
        _shaderSets.PushBack(CSM_NEW CubismShaderSet());
    }

    _shaderSets[0]->load(_device, shader_VertShaderSrcSetupMask, shader_FragShaderSrcSetupMask);

    _shaderSets[1]->load(_device, shader_VertShaderSrc, shader_FragShaderSrc);
    _shaderSets[2]->load(_device, shader_VertShaderSrcMasked, shader_FragShaderSrcNormalMask);
    _shaderSets[3]->load(_device, shader_VertShaderSrc, shader_FragShaderSrcPremultipliedAlpha);
    _shaderSets[4]->load(_device, shader_VertShaderSrcMasked, shader_FragShaderSrcNormalMaskPremultipliedAlpha);

    _shaderSets[5]->load(_device, shader_VertShaderSrc, shader_FragShaderSrc);
    _shaderSets[6]->load(_device, shader_VertShaderSrcMasked, shader_FragShaderSrcAddMask);
    _shaderSets[7]->load(_device, shader_VertShaderSrc, shader_FragShaderSrcPremultipliedAlpha);
    _shaderSets[8]->load(_device, shader_VertShaderSrcMasked, shader_FragShaderSrcAddMaskPremultipliedAlpha);

    _shaderSets[9]->load(_device, shader_VertShaderSrc, shader_FragShaderSrc);
    _shaderSets[10]->load(_device, shader_VertShaderSrcMasked, shader_FragShaderSrcMultMask);
    _shaderSets[11]->load(_device, shader_VertShaderSrc, shader_FragShaderSrcPremultipliedAlpha);
    _shaderSets[12]->load(_device, shader_VertShaderSrcMasked, shader_FragShaderSrcMultMaskPremultipliedAlpha);
}

void CubismShader_D3D11::SetupShaderProgram(CubismRenderer_D3D11* renderer
	, ID3D11ShaderResourceView *texture
	, csmInt32 vertexCount, csmFloat32* vertexArray
	, csmFloat32* uvArray, csmFloat32 opacity
	, CubismRenderer::CubismBlendMode colorBlendMode
	, CubismRenderer::CubismTextureColor baseColor
, csmBool isPremultipliedAlpha, CubismMatrix44 matrix4x4)
{
	DeviceContext * ctx = _device->getContext();
    if (_shaderSets.GetSize() == 0)
    {
        GenerateShaders();
    }

    // Blending
	ID3D11BlendState * blend;

    if (renderer->GetClippingContextBufferForMask() != NULL) // マスク生成時
    {
        CubismShaderSet* shaderSet = _shaderSets[ShaderNames_SetupMask];
		ctx->setVertexShader(shaderSet->vs);
		ctx->setPixelShader(shaderSet->ps);

        //テクスチャ設定
		ctx->setPixelResource(0, texture);
		
        // 頂点配列の設定
        glVertexAttribPointer(shaderSet->AttributePositionLocation, 2, GL_FLOAT, GL_FALSE, sizeof(csmFloat32) * 2, vertexArray);
        // テクスチャ頂点の設定
        glEnableVertexAttribArray(shaderSet->AttributeTexCoordLocation);
        glVertexAttribPointer(shaderSet->AttributeTexCoordLocation, 2, GL_FLOAT, GL_FALSE, sizeof(csmFloat32) * 2, uvArray);

        // チャンネル
        const csmInt32 channelNo = renderer->GetClippingContextBufferForMask()->_layoutChannelNo;
        CubismRenderer::CubismTextureColor* colorChannel = renderer->GetClippingContextBufferForMask()->GetClippingManager()->GetChannelFlagAsColor(channelNo);
        glUniform4f(shaderSet->UnifromChannelFlagLocation, colorChannel->R, colorChannel->G, colorChannel->B, colorChannel->A);

        glUniformMatrix4fv(shaderSet->UniformClipMatrixLocation, 1, GL_FALSE, renderer->GetClippingContextBufferForMask()->_matrixForMask.GetArray());

        csmRectF* rect = renderer->GetClippingContextBufferForMask()->_layoutBounds;

        glUniform4f(shaderSet->UniformBaseColorLocation,
                    rect->X * 2.0f - 1.0f,
                    rect->Y * 2.0f - 1.0f,
                    rect->GetRight() * 2.0f - 1.0f,
                    rect->GetBottom() * 2.0f - 1.0f);

		blend = _device->blend(BlendMode::Subtract);
    }
    else // マスク生成以外の場合
    {
        const csmBool masked = renderer->GetClippingContextBufferForDraw() != NULL;  // この描画オブジェクトはマスク対象か
        const csmInt32 offset = (masked ? 1 : 0) + (isPremultipliedAlpha ? 2 : 0);

        CubismShaderSet* shaderSet;

        switch (colorBlendMode)
        {
        case CubismRenderer::CubismBlendMode::CubismBlendMode_Normal:
        default:
            shaderSet = _shaderSets[ShaderNames_Normal + offset];
			blend = _device->blend(BlendMode::Normal);
            break;

        case CubismRenderer::CubismBlendMode::CubismBlendMode_Additive:
            shaderSet = _shaderSets[ShaderNames_Add + offset];
			blend = _device->blend(BlendMode::Add);
            break;

        case CubismRenderer::CubismBlendMode::CubismBlendMode_Multiplicative:
            shaderSet = _shaderSets[ShaderNames_Mult + offset];
			blend = _device->blend(BlendMode::Multiply);
            break;
        }

		
		ctx->setVertexShader(shaderSet->vs);
		ctx->setPixelShader(shaderSet->ps);

        // 頂点配列の設定
		
        glVertexAttribPointer(shaderSet->AttributePositionLocation, 2, GL_FLOAT, GL_FALSE, sizeof(csmFloat32) * 2, vertexArray);
        // テクスチャ頂点の設定
        glEnableVertexAttribArray(shaderSet->AttributeTexCoordLocation);
        glVertexAttribPointer(shaderSet->AttributeTexCoordLocation, 2, GL_FLOAT, GL_FALSE, sizeof(csmFloat32) * 2, uvArray);

        if (masked)
        {
            glActiveTexture(GL_TEXTURE1);
            GLuint tex = renderer->GetClippingContextBufferForDraw()->GetClippingManager()->GetColorBuffer();
            glBindTexture(GL_TEXTURE_2D, tex);
            glUniform1i(shaderSet->SamplerTexture1Location, 1);

            // View座標をClippingContextの座標に変換するための行列を設定
            glUniformMatrix4fv(shaderSet->UniformClipMatrixLocation, 1, 0, renderer->GetClippingContextBufferForDraw()->_matrixForDraw.GetArray());

            // 使用するカラーチャンネルを設定
            const csmInt32 channelNo = renderer->GetClippingContextBufferForDraw()->_layoutChannelNo;
            CubismRenderer::CubismTextureColor* colorChannel = renderer->GetClippingContextBufferForDraw()->GetClippingManager()->GetChannelFlagAsColor(channelNo);
            glUniform4f(shaderSet->UnifromChannelFlagLocation, colorChannel->R, colorChannel->G, colorChannel->B, colorChannel->A);
        }

        //テクスチャ設定
		ctx->setPixelResource(0, texture);

        //座標変換
        glUniformMatrix4fv(shaderSet->UniformMatrixLocation, 1, 0, matrix4x4.GetArray()); //

        glUniform4f(shaderSet->UniformBaseColorLocation, baseColor.R, baseColor.G, baseColor.B, baseColor.A);
    }

	ctx->setBlendState(blend);
}

CubismRenderer* CubismRenderer::Create()
{
    return CSM_NEW CubismRenderer_D3D11();
}

void CubismRenderer::StaticRelease()
{
    CubismRenderer_D3D11::DoStaticRelease();
}

CubismRenderer_D3D11::CubismRenderer_D3D11() : _clippingContextBufferForMask(NULL)
                                                     , _clippingContextBufferForDraw(NULL)
                                                     , _clippingManager(NULL)
{
    // テクスチャ対応マップの容量を確保しておく.
    _textures.PrepareCapacity(32, true);


	m_disableDepth = _device->depthStencil(false);
}

CubismRenderer_D3D11::~CubismRenderer_D3D11()
{
    CSM_DELETE_SELF(CubismClippingManager_D3D11, _clippingManager);
}

void CubismRenderer_D3D11::DoStaticRelease()
{
#ifdef CSM_TARGET_WINGL
    s_isInitializeGlFunctionsSuccess = false;     ///< 初期化が完了したかどうか。trueなら初期化完了
    s_isFirstInitializeGlFunctions = true;        ///< 最初の初期化実行かどうか。trueなら最初の初期化実行
#endif
    CubismShader_D3D11::DeleteInstance();
}

void CubismRenderer_D3D11::Initialize(CubismModel* model)
{
    if (model->IsUsingMasking())
    {
        _clippingManager = CSM_NEW CubismClippingManager_D3D11();  //クリッピングマスク・バッファ前処理方式を初期化
        _clippingManager->Initialize(
            *model,
            model->GetDrawableCount(),
            model->GetDrawableMasks(),
            model->GetDrawableMaskCounts()
        );
    }

	_sampler = _device->sampler(TextureAddressMode::CLAMP, Filter::MIN_MAG_MIP_LINEAR);

    _sortedDrawableIndexList.Resize(model->GetDrawableCount(), 0);

    CubismRenderer::Initialize(model);  //親クラスの処理を呼ぶ
}

void CubismRenderer_D3D11::SetDevice(kr::d3d11::Direct3D11 * device) noexcept
{
	_device = device;
	_shader._device = device;
}

void CubismRenderer_D3D11::PreDraw()
{
#ifdef CSM_TARGET_WIN_GL
    if (s_isFirstInitializeGlFunctions) InitializeGlFunctions();
    if (!s_isInitializeGlFunctionsSuccess) return;
#endif

	DeviceContext * ctx = _device->getContext();
	ctx->setDepthStencilState(_device->depthStencil(false), 0);
	
    //異方性フィルタリング。プラットフォームのOpenGLによっては未対応の場合があるので、未設定のときは設定しない
	float anisotropy = GetAnisotropy();
    if (anisotropy > 0.0f)
    {
        //for (ID3D11ShaderResourceView * res : _textures)
        //{
        //    glBindTexture(GL_TEXTURE_2D, _textures[i]);
        //    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, GetAnisotropy());
        //}
    }
}

void CubismRenderer_D3D11::DoDrawModel()
{
    //------------ クリッピングマスク・バッファ前処理方式の場合 ------------
    if (_clippingManager != NULL)
    {
        PreDraw();
        _clippingManager->SetupClippingContext(*GetModel(), this);
    }

    // 上記クリッピング処理内でも一度PreDrawを呼ぶので注意!!
    PreDraw();

	DeviceContext * ctx = _device->getContext();
	ctx->setPrimitiveTopology(PrimitiveTopology::TRIANGLELIST);
	ctx->setPixelSampler(0, _sampler);

    const csmInt32 drawableCount = GetModel()->GetDrawableCount();
    const csmInt32* renderOrder = GetModel()->GetDrawableRenderOrders();

    // インデックスを描画順でソート
    for (csmInt32 i = 0; i < drawableCount; ++i)
    {
        const csmInt32 order = renderOrder[i];
        _sortedDrawableIndexList[order] = i;
    }

    // 描画
    for (csmInt32 i = 0; i < drawableCount; ++i)
    {
        const csmInt32 drawableIndex = _sortedDrawableIndexList[i];

        // クリッピングマスクをセットする
        SetClippingContextBufferForDraw((_clippingManager != NULL)
            ? (*_clippingManager->GetClippingContextListForDraw())[drawableIndex]
            : NULL);

        IsCulling(GetModel()->GetDrawableCulling(drawableIndex) != 0);

        DrawMesh(
            GetModel()->GetDrawableTextureIndices(drawableIndex),
            GetModel()->GetDrawableVertexIndexCount(drawableIndex),
            GetModel()->GetDrawableVertexCount(drawableIndex),
            const_cast<csmUint16*>(GetModel()->GetDrawableVertexIndices(drawableIndex)),
            const_cast<csmFloat32*>(GetModel()->GetDrawableVertices(drawableIndex)),
            reinterpret_cast<csmFloat32*>(const_cast<Core::csmVector2*>(GetModel()->GetDrawableVertexUvs(drawableIndex))),
            GetModel()->GetDrawableOpacity(drawableIndex),
            GetModel()->GetDrawableBlendMode(drawableIndex)
        );
    }

    //
    PostDraw();

}

void CubismRenderer_D3D11::DrawMesh(csmInt32 textureNo, csmInt32 indexCount, csmInt32 vertexCount
                                        , csmUint16* indexArray, csmFloat32* vertexArray, csmFloat32* uvArray
                                        , csmFloat32 opacity, CubismBlendMode colorBlendMode)
{

#ifdef CSM_TARGET_WIN_GL
    if (s_isFirstInitializeGlFunctions) return;  // WindowsプラットフォームではGL命令のバインドを済ませておく必要がある
#endif

#ifndef CSM_DEBUG
    if (_textures[textureNo] == nullptr) return;    // モデルが参照するテクスチャがバインドされていない場合は描画をスキップする
#endif

    // 描画不要なら描画処理をスキップする
    if (opacity <= 0.0f && GetClippingContextBufferForMask() == NULL) return;

	DeviceContext * ctx = _device->getContext();

    // 裏面描画の有効・無効
    if (IsCulling())
    {
		ctx->setRasterizerState(_device->rasterizer(CullMode::BACK, FillMode::SOLID, false));
    }
    else
    {
		ctx->setRasterizerState(_device->rasterizer(CullMode::NONE, FillMode::SOLID, false));
    }

    CubismTextureColor modelColorRGBA = GetModelColor();

    if (GetClippingContextBufferForMask() == NULL) // マスク生成時以外
    {
        modelColorRGBA.A *= opacity;
        if (IsPremultipliedAlpha())
        {
            modelColorRGBA.R *= modelColorRGBA.A;
            modelColorRGBA.G *= modelColorRGBA.A;
            modelColorRGBA.B *= modelColorRGBA.A;
        }
    }

    ID3D11ShaderResourceView * drawTexture = _textures[textureNo];   // シェーダに渡すテクスチャID

	_shader.SetupShaderProgram(
        this, drawTexture, vertexCount, vertexArray, uvArray
        , opacity, colorBlendMode, modelColorRGBA, IsPremultipliedAlpha()
        , GetMvpMatrix()
    );

    // ポリゴンメッシュを描画する
	ctx->drawIndexed(indexCount, 0, indexArray, 0);

    // 後処理
	ctx->setPixelShader(nullptr);
	ctx->setVertexShader(nullptr);
    SetClippingContextBufferForDraw(NULL);
    SetClippingContextBufferForMask(NULL);
}

void CubismRenderer_D3D11::SaveProfile()
{
    // _rendererProfile.Save();
}

void CubismRenderer_D3D11::RestoreProfile()
{
    // _rendererProfile.Restore();
}

void CubismRenderer_D3D11::SetClippingMaskBufferSize(csmInt32 size)
{
    //FrameBufferのサイズを変更するためにインスタンスを破棄・再作成する
    CSM_DELETE_SELF(CubismClippingManager_D3D11, _clippingManager);

    _clippingManager = CSM_NEW CubismClippingManager_D3D11();

    _clippingManager->SetClippingMaskBufferSize(size);

    _clippingManager->Initialize(
        *GetModel(),
        GetModel()->GetDrawableCount(),
        GetModel()->GetDrawableMasks(),
        GetModel()->GetDrawableMaskCounts()
    );
}

csmInt32 CubismRenderer_D3D11::GetClippingMaskBufferSize() const
{
    return _clippingManager->GetClippingMaskBufferSize();
}

void CubismRenderer_D3D11::BindTexture(csmInt32 index, kr::d3d11::ShaderResourceView texture) noexcept
{
	_textures[index] = texture;
}

void CubismRenderer_D3D11::SetClippingContextBufferForMask(CubismClippingContext* clip)
{
    _clippingContextBufferForMask = clip;
}

CubismClippingContext* CubismRenderer_D3D11::GetClippingContextBufferForMask() const
{
    return _clippingContextBufferForMask;
}

void CubismRenderer_D3D11::SetClippingContextBufferForDraw(CubismClippingContext* clip)
{
    _clippingContextBufferForDraw = clip;
}

CubismClippingContext* CubismRenderer_D3D11::GetClippingContextBufferForDraw() const
{
    return _clippingContextBufferForDraw;
}

}}}}

//------------ LIVE2D NAMESPACE ------------
