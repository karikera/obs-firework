#include "stdafx.h"
//
//#ifdef _DEBUG
//#pragma comment(lib, "Live2DCubismCore_MDd.lib")
//#else
//#pragma comment(lib, "Live2DCubismCore_MT.lib")
//#endif
//
//
//#include <Live2DCubismCore.h>
//
//#include <KR3/initializer.h>
//
//class LAppModel : public Csm::CubismUserModel
//{
//public:
//	LAppModel();
//
//	/**
//	* @brief «Ç«¹«È«é«¯«¿
//	*
//	*/
//	virtual ~LAppModel();
//
//	/**
//	* @brief model3.jsonª¬öÇª«ªìª¿«Ç«£«ì«¯«È«êªÈ«Õ«¡«¤«ë«Ñ«¹ª«ªé«â«Ç«ëªòßæà÷ª¹ªë
//	*
//	*/
//	void LoadAssets(const Csm::csmChar dir[], const  Csm::csmChar fileName[]);
//
//	/**
//	* @brief «ì«ó«À«éªòî¢Ï°õéª¹ªë
//	*
//	*/
//	void ReloadRnederer();
//
//	/**
//	* @brief   «â«Ç«ëªÎÌÚãæ?×â¡£«â«Ç«ëªÎ«Ñ«é«á?«¿ª«ªéÙÚ??÷¾ªòÌ½ïÒª¹ªë¡£
//	*
//	*/
//	void Update();
//
//	/**
//	* @brief   «â«Ç«ëªòÙÚ?ª¹ªë?×â¡£«â«Ç«ëªòÙÚ?ª¹ªëÍöÊàªÎView-Projectionú¼ÖªªòÔ¤ª¹¡£
//	*
//	* @param[in]  matrix  View-Projectionú¼Öª
//	*/
//	void Draw(Csm::CubismMatrix44& matrix);
//
//	/**
//	* @brief   ìÚ?ªÇò¦ïÒª·ª¿«â?«·«ç«óªÎî¢ßæªòËÒã·ª¹ªë¡£
//	*
//	* @param[in]   group           «â?«·«ç«ó«°«ë?«×Ù£
//	* @param[in]   no              «°«ë?«×?ªÎÛã?
//	* @param[in]   priority        éĞà»Óø
//	* @return                      ËÒã·ª·ª¿«â?«·«ç«óªÎãÛÜ¬Ûã?ªòÚ÷ª¹¡£ËÁÜ¬ªÎ«â?«·«ç«óª¬ğûÖõª·ª¿ª«Üúª«ªò÷÷ïÒª¹ªëIsFinished()ªÎìÚ?ªÇŞÅéÄª¹ªë¡£ËÒã·ªÇª­ªÊª¤ãÁªÏ¡¸-1¡¹
//	*/
//	Csm::CubismMotionQueueEntryHandle StartMotion(const Csm::csmChar* group, Csm::csmInt32 no, Csm::csmInt32 priority);
//
//	/**
//	* @brief   «é«ó«À«àªËàÔªĞªìª¿«â?«·«ç«óªÎî¢ßæªòËÒã·ª¹ªë¡£
//	*
//	* @param[in]   group           «â?«·«ç«ó«°«ë?«×Ù£
//	* @param[in]   priority        éĞà»Óø
//	* @return                      ËÒã·ª·ª¿«â?«·«ç«óªÎãÛÜ¬Ûã?ªòÚ÷ª¹¡£ËÁÜ¬ªÎ«â?«·«ç«óª¬ğûÖõª·ª¿ª«Üúª«ªò÷÷ïÒª¹ªëIsFinished()ªÎìÚ?ªÇŞÅéÄª¹ªë¡£ËÒã·ªÇª­ªÊª¤ãÁªÏ¡¸-1¡¹
//	*/
//	Csm::CubismMotionQueueEntryHandle StartRandomMotion(const Csm::csmChar* group, Csm::csmInt32 priority);
//
//	/**
//	* @brief   ìÚ?ªÇò¦ïÒª·ª¿øúï×«â?«·«ç«óªò«»«Ã«Èª¹ªë
//	*
//	* @param   expressionID    øúï×«â?«·«ç«óªÎID
//	*/
//	void SetExpression(const Csm::csmChar* expressionID);
//
//	/**
//	* @brief   «é«ó«À«àªËàÔªĞªìª¿øúï×«â?«·«ç«óªò«»«Ã«Èª¹ªë
//	*
//	*/
//	void SetRandomExpression();
//
//	/**
//	* @brief   «¤«Ù«ó«ÈªÎ?ûıªòáôª±ö¢ªë
//	*
//	*/
//	virtual void MotionEventFired(const Live2D::Cubism::Framework::csmString& eventValue);
//
//	/**
//	* @brief    ?ª¿ªê÷÷ïÒ«Æ«¹«È¡£<br>
//	*            ò¦ïÒIDªÎğ¢ïÃ«ê«¹«Èª«ªéÏ»û¡ªòÍªß©ª·¡¢ñ¨øöª¬Ï»û¡Ûô??ª«÷÷ïÒª¹ªë¡£
//	*
//	* @param[in]   hitAreaName     ?ª¿ªê÷÷ïÒªò«Æ«¹«Èª¹ªë?ßÚªÎID
//	* @param[in]   x               ÷÷ïÒªòú¼ª¦Xñ¨øö
//	* @param[in]   y               ÷÷ïÒªòú¼ª¦Yñ¨øö
//	*/
//	virtual Csm::csmBool HitTest(const Csm::csmChar* hitAreaName, Csm::csmFloat32 x, Csm::csmFloat32 y);
//
//	Csm::csmRectF GetDrawableArea(Csm::csmInt32 drawableIndex, const Csm::CubismMatrix44& vpMatrix, const Csm::CubismVector2& windowSize) const;
//	const Csm::csmVector<Csm::csmRectF>& GetHitAreas(const Csm::CubismMatrix44& vpMatrix, const Csm::CubismVector2& windowSize);
//	const Csm::csmVector<Csm::csmRectF>& GetUserDataAreas(const Csm::CubismMatrix44& vpMatrix, const Csm::CubismVector2& windowSize);
//
//protected:
//	/**
//	*  @brief  «â«Ç«ëªòÙÚ?ª¹ªë?×â¡£«â«Ç«ëªòÙÚ?ª¹ªëÍöÊàªÎView-Projectionú¼ÖªªòÔ¤ª¹¡£
//	*
//	*/
//	void DoDraw();
//
//private:
//	/**
//	* @brief model3.jsonª«ªé«â«Ç«ëªòßæà÷ª¹ªë¡£<br>
//	*         model3.jsonªÎÑÀâûªË?ªÃªÆ«â«Ç«ëßæà÷¡¢«â?«·«ç«ó¡¢Úª×âæÑß©ªÊªÉªÎ«³«ó«İ?«Í«ó«Èßæà÷ªòú¼ª¦¡£
//	*
//	* @param[in]   setting     ICubismModelSettingªÎ«¤«ó«¹«¿«ó«¹
//	*
//	*/
//	void SetupModel(Csm::ICubismModelSetting* setting);
//
//	/**
//	* @brief OpenGLªÎ«Æ«¯«¹«Á«ã«æ«Ë«Ã«ÈªË«Æ«¯«¹«Á«ãªò«í?«Éª¹ªë
//	*
//	*/
//	void SetupTextures();
//
//	/**
//	* @brief   «â?«·«ç«ó«Ç?«¿ªò«°«ë?«×Ù£ª«ªéìéÎÀªÇ«í?«Éª¹ªë¡£<br>
//	*           «â?«·«ç«ó«Ç?«¿ªÎÙ£îñªÏ?İ»ªÇModelSettingª«ªéö¢Ôğª¹ªë¡£
//	*
//	* @param[in]   group  «â?«·«ç«ó«Ç?«¿ªÎ«°«ë?«×Ù£
//	*/
//	void PreloadMotionGroup(const Csm::csmChar* group);
//
//	/**
//	* @brief   «â?«·«ç«ó«Ç?«¿ªò«°«ë?«×Ù£ª«ªéìéÎÀªÇú°Û¯ª¹ªë¡£<br>
//	*           «â?«·«ç«ó«Ç?«¿ªÎÙ£îñªÏ?İ»ªÇModelSettingª«ªéö¢Ôğª¹ªë¡£
//	*
//	* @param[in]   group  «â?«·«ç«ó«Ç?«¿ªÎ«°«ë?«×Ù£
//	*/
//	void ReleaseMotionGroup(const Csm::csmChar* group) const;
//
//	/**
//	* @brief ª¹ªÙªÆªÎ«â?«·«ç«ó«Ç?«¿ªÎú°Û¯
//	*
//	* ª¹ªÙªÆªÎ«â?«·«ç«ó«Ç?«¿ªòú°Û¯ª¹ªë¡£
//	*/
//	void ReleaseMotions();
//
//	/**
//	* @brief ª¹ªÙªÆªÎøúï×«Ç?«¿ªÎú°Û¯
//	*
//	* ª¹ªÙªÆªÎøúï×«Ç?«¿ªòú°Û¯ª¹ªë¡£
//	*/
//	void ReleaseExpressions();
//
//	Csm::ICubismModelSetting* _modelSetting;        ///< «â«Ç«ë«»«Ã«Æ«£«ó«°ï×ÜÃ
//	Csm::csmString _modelHomeDir;                   ///< «â«Ç«ë«»«Ã«Æ«£«ó«°ª¬öÇª«ªìª¿«Ç«£«ì«¯«È«ê
//	Csm::csmFloat32 _userTimeSeconds;               ///< «Ç«ë«¿ãÁÊàªÎîİß©ö·[õ©]
//
//	Csm::csmVector<Csm::CubismIdHandle> _eyeBlinkIds; ///<¡¡«â«Ç«ëªËàâïÒªµªìª¿ªŞªĞª¿ª­Ñ¦ÒöéÄ«Ñ«é«á?«¿ID
//	Csm::csmVector<Csm::CubismIdHandle> _lipSyncIds;  ///< «â«Ç«ëªËàâïÒªµªìª¿«ê«Ã«×«·«ó«¯Ñ¦ÒöéÄ«Ñ«é«á?«¿ID
//
//	Csm::csmMap<Csm::csmString, Csm::ACubismMotion*>   _motions;       ///< ?ªß?ªŞªìªÆª¤ªë«â?«·«ç«óªÎ«ê«¹«È
//	Csm::csmMap<Csm::csmString, Csm::ACubismMotion*>   _expressions;   ///< ?ªß?ªŞªìªÆª¤ªëøúï×ªÎ«ê«¹«È
//
//	Csm::csmVector<Csm::csmRectF>_hitArea;
//	Csm::csmVector<Csm::csmRectF>_userArea;
//
//	const Csm::CubismId* _idParamAngleX;            ///< «Ñ«é«á?«¿ID: ParamAngleX
//	const Csm::CubismId* _idParamAngleY;            ///< «Ñ«é«á?«¿ID: ParamAngleX
//	const Csm::CubismId* _idParamAngleZ;            ///< «Ñ«é«á?«¿ID: ParamAngleX
//	const Csm::CubismId* _idParamBodyAngleX;        ///< «Ñ«é«á?«¿ID: ParamBodyAngleX
//	const Csm::CubismId* _idParamEyeBallX;          ///< «Ñ«é«á?«¿ID: ParamEyeBallX
//	const Csm::CubismId* _idParamEyeBallY;          ///< «Ñ«é«á?«¿ID: ParamEyeBallXY
//
//};
//
//
//
//
//
//int main()
//{
//
//}
