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
//	* @brief �ǫ��ȫ髯��
//	*
//	*/
//	virtual ~LAppModel();
//
//	/**
//	* @brief model3.json���Ǫ��쪿�ǫ��쫯�ȫ�ȫի�����ѫ������ǫ����������
//	*
//	*/
//	void LoadAssets(const Csm::csmChar dir[], const  Csm::csmChar fileName[]);
//
//	/**
//	* @brief ��������ϰ�骹��
//	*
//	*/
//	void ReloadRnederer();
//
//	/**
//	* @brief   ��ǫ������?�⡣��ǫ�Ϋѫ��?��������??����̽�Ҫ��롣
//	*
//	*/
//	void Update();
//
//	/**
//	* @brief   ��ǫ����?����?�⡣��ǫ����?���������View-Projection��֪��Ԥ����
//	*
//	* @param[in]  matrix  View-Projection��֪
//	*/
//	void Draw(Csm::CubismMatrix44& matrix);
//
//	/**
//	* @brief   ��?����Ҫ�����?������������㷪��롣
//	*
//	* @param[in]   group           ��?����󫰫�?��٣
//	* @param[in]   no              ����?��?����?
//	* @param[in]   priority        �����
//	* @return                      ��㷪�����?��������ܬ��?����������ܬ�Ϋ�?������������������������Ҫ���IsFinished()����?�����Ī��롣��㷪Ǫ��ʪ����ϡ�-1��
//	*/
//	Csm::CubismMotionQueueEntryHandle StartMotion(const Csm::csmChar* group, Csm::csmInt32 no, Csm::csmInt32 priority);
//
//	/**
//	* @brief   ��������ԪЪ쪿��?������������㷪��롣
//	*
//	* @param[in]   group           ��?����󫰫�?��٣
//	* @param[in]   priority        �����
//	* @return                      ��㷪�����?��������ܬ��?����������ܬ�Ϋ�?������������������������Ҫ���IsFinished()����?�����Ī��롣��㷪Ǫ��ʪ����ϡ�-1��
//	*/
//	Csm::CubismMotionQueueEntryHandle StartRandomMotion(const Csm::csmChar* group, Csm::csmInt32 priority);
//
//	/**
//	* @brief   ��?����Ҫ������׫�?�����򫻫ëȪ���
//	*
//	* @param   expressionID    ���׫�?������ID
//	*/
//	void SetExpression(const Csm::csmChar* expressionID);
//
//	/**
//	* @brief   ��������ԪЪ쪿���׫�?�����򫻫ëȪ���
//	*
//	*/
//	void SetRandomExpression();
//
//	/**
//	* @brief   ���٫�Ȫ�?������������
//	*
//	*/
//	virtual void MotionEventFired(const Live2D::Cubism::Framework::csmString& eventValue);
//
//	/**
//	* @brief    ?�������ҫƫ��ȡ�<br>
//	*            ���ID����ë꫹�Ȫ���ϻ����ͪߩ���������ϻ����??�����Ҫ��롣
//	*
//	* @param[in]   hitAreaName     ?�������Ҫ�ƫ��Ȫ���?�ڪ�ID
//	* @param[in]   x               ���Ҫ�����X���
//	* @param[in]   y               ���Ҫ�����Y���
//	*/
//	virtual Csm::csmBool HitTest(const Csm::csmChar* hitAreaName, Csm::csmFloat32 x, Csm::csmFloat32 y);
//
//	Csm::csmRectF GetDrawableArea(Csm::csmInt32 drawableIndex, const Csm::CubismMatrix44& vpMatrix, const Csm::CubismVector2& windowSize) const;
//	const Csm::csmVector<Csm::csmRectF>& GetHitAreas(const Csm::CubismMatrix44& vpMatrix, const Csm::CubismVector2& windowSize);
//	const Csm::csmVector<Csm::csmRectF>& GetUserDataAreas(const Csm::CubismMatrix44& vpMatrix, const Csm::CubismVector2& windowSize);
//
//protected:
//	/**
//	*  @brief  ��ǫ����?����?�⡣��ǫ����?���������View-Projection��֪��Ԥ����
//	*
//	*/
//	void DoDraw();
//
//private:
//	/**
//	* @brief model3.json�����ǫ���������롣<br>
//	*         model3.json��������?�êƫ�ǫ���������?�����ڪ����ߩ�ʪɪΫ����?�ͫ��������������
//	*
//	* @param[in]   setting     ICubismModelSetting�Ϋ��󫹫���
//	*
//	*/
//	void SetupModel(Csm::ICubismModelSetting* setting);
//
//	/**
//	* @brief OpenGL�Ϋƫ��������˫ëȪ˫ƫ���������?�ɪ���
//	*
//	*/
//	void SetupTextures();
//
//	/**
//	* @brief   ��?������?���򫰫�?��٣���������ǫ�?�ɪ��롣<br>
//	*           ��?������?����٣���?ݻ��ModelSetting�������𪹪롣
//	*
//	* @param[in]   group  ��?������?���Ϋ���?��٣
//	*/
//	void PreloadMotionGroup(const Csm::csmChar* group);
//
//	/**
//	* @brief   ��?������?���򫰫�?��٣������������ۯ���롣<br>
//	*           ��?������?����٣���?ݻ��ModelSetting�������𪹪롣
//	*
//	* @param[in]   group  ��?������?���Ϋ���?��٣
//	*/
//	void ReleaseMotionGroup(const Csm::csmChar* group) const;
//
//	/**
//	* @brief ���٪ƪΫ�?������?������ۯ
//	*
//	* ���٪ƪΫ�?������?������ۯ���롣
//	*/
//	void ReleaseMotions();
//
//	/**
//	* @brief ���٪ƪ����׫�?������ۯ
//	*
//	* ���٪ƪ����׫�?������ۯ���롣
//	*/
//	void ReleaseExpressions();
//
//	Csm::ICubismModelSetting* _modelSetting;        ///< ��ǫ뫻�ëƫ�������
//	Csm::csmString _modelHomeDir;                   ///< ��ǫ뫻�ëƫ��󫰪��Ǫ��쪿�ǫ��쫯�ȫ�
//	Csm::csmFloat32 _userTimeSeconds;               ///< �ǫ뫿�������ߩ��[��]
//
//	Csm::csmVector<Csm::CubismIdHandle> _eyeBlinkIds; ///<����ǫ�����Ҫ��쪿�ުЪ���Ѧ���īѫ��?��ID
//	Csm::csmVector<Csm::CubismIdHandle> _lipSyncIds;  ///< ��ǫ�����Ҫ��쪿��ë׫���Ѧ���īѫ��?��ID
//
//	Csm::csmMap<Csm::csmString, Csm::ACubismMotion*>   _motions;       ///< ?��?�ު�ƪ����?�����Ϋ꫹��
//	Csm::csmMap<Csm::csmString, Csm::ACubismMotion*>   _expressions;   ///< ?��?�ު�ƪ������תΫ꫹��
//
//	Csm::csmVector<Csm::csmRectF>_hitArea;
//	Csm::csmVector<Csm::csmRectF>_userArea;
//
//	const Csm::CubismId* _idParamAngleX;            ///< �ѫ��?��ID: ParamAngleX
//	const Csm::CubismId* _idParamAngleY;            ///< �ѫ��?��ID: ParamAngleX
//	const Csm::CubismId* _idParamAngleZ;            ///< �ѫ��?��ID: ParamAngleX
//	const Csm::CubismId* _idParamBodyAngleX;        ///< �ѫ��?��ID: ParamBodyAngleX
//	const Csm::CubismId* _idParamEyeBallX;          ///< �ѫ��?��ID: ParamEyeBallX
//	const Csm::CubismId* _idParamEyeBallY;          ///< �ѫ��?��ID: ParamEyeBallXY
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
