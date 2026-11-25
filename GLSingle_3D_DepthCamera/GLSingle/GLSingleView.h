
// GLSingleView.h: CGLSingleView 클래스의 인터페이스
//

#pragma once
#include "opencv2/opencv.hpp"
#include "GL/freeglut.h"
#include "ObjectComparer.h"
using namespace cv;

class CGLSingleView : public CView
{
protected: // serialization에서만 만들어집니다.
	CGLSingleView() noexcept;
	DECLARE_DYNCREATE(CGLSingleView)

// 특성입니다.
public:
	CGLSingleDoc* GetDocument() const;
	HDC			m_hDC;
	HGLRC		m_hglRC;

	double		m_eye[3];
	double		m_center [3];
	float		m_zoomFactor;

	float m_xTheta;
	float m_yTheta;
	float m_zTheta;

	float m_xTranslate;
	float m_yTranslate;
	float m_zTranslate;

	bool m_xRotate;
	bool m_yRotate;
	bool m_zRotate;

	VideoCapture* capture;
	Mat mat_frame;

// 작업입니다.
public:

public:
	CString m_studentText;
	afx_msg void OnStudentSelect();
	afx_msg void OnCameraView();
	enum class ViewMode { Camera, Students, Compare };
	ViewMode m_viewMode = ViewMode::Camera;

	// 객체 비교
	cv::Mat m_compareReference;              // 실행 폴더의 compare\reference.png에 보관 (1개, 교체 가능)
	cv::Mat m_compareTarget;
	objcmp::CompareResult m_compareResult;   // status가 Ok일 때만 유효한 비교 결과
	afx_msg void OnCompareLoadReference();
	afx_msg void OnCompareLoadTarget();
	afx_msg void OnCompareRun();
	bool EnsureCompareReference();
	void ShowCompareView();
	void DrawCompareView(CDC* pDC);

// 재정의입니다.
public:
	virtual void OnDraw(CDC* pDC);  // 이 뷰를 그리기 위해 재정의되었습니다.
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// 구현입니다.
public:
	virtual ~CGLSingleView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 생성된 메시지 맵 함수
protected:
	DECLARE_MESSAGE_MAP()
public:
	BOOL SetPixelformat(HDC hdc);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	void ReSizeScene(GLsizei width, GLsizei height);
	
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};

#ifndef _DEBUG  // GLSingleView.cpp의 디버그 버전
inline CGLSingleDoc* CGLSingleView::GetDocument() const
   { return reinterpret_cast<CGLSingleDoc*>(m_pDocument); }
#endif

