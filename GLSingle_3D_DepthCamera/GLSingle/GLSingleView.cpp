
// GLSingleView.cpp: CGLSingleView 클래스의 구현
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS는 미리 보기, 축소판 그림 및 검색 필터 처리기를 구현하는 ATL 프로젝트에서 정의할 수 있으며
// 해당 프로젝트와 문서 코드를 공유하도록 해 줍니다.
#ifndef SHARED_HANDLERS
#include "GLSingle.h"
#endif

#include "GLSingleDoc.h"
#include "GLSingleView.h"

#include "sqlite3.h"
#include "ImageFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CGLSingleView

IMPLEMENT_DYNCREATE(CGLSingleView, CView)

BEGIN_MESSAGE_MAP(CGLSingleView, CView)
	ON_COMMAND(ID_STUDENT_SELECT, &CGLSingleView::OnStudentSelect)
	ON_COMMAND(ID_CAMERA_VIEW, &CGLSingleView::OnCameraView)
	ON_COMMAND(ID_COMPARE_LOAD_REFERENCE, &CGLSingleView::OnCompareLoadReference)
	ON_COMMAND(ID_COMPARE_LOAD_TARGET, &CGLSingleView::OnCompareLoadTarget)
	ON_COMMAND(ID_COMPARE_RUN, &CGLSingleView::OnCompareRun)
	// 표준 인쇄 명령입니다.
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
	ON_WM_CREATE()
	ON_WM_DESTROY()
//	ON_WM_ERASEBKGND()
ON_WM_SIZE()
ON_WM_KEYDOWN()
ON_WM_TIMER()
END_MESSAGE_MAP()

// CGLSingleView 생성/소멸

CGLSingleView::CGLSingleView() noexcept
{
	// TODO: 여기에 생성 코드를 추가합니다.
	m_eye[0] = 0.;
	m_eye[1] = 0.;
	m_eye[2] = 10.;
	m_center[0] = 0.;
	m_center[1] = 0.;
	m_center[2] = 0.;

	m_xTheta = 0.0;
	m_yTheta = 0.0;
	m_zTheta = 0.0;

	m_xTranslate = 0.0;
	m_yTranslate = 0.0;
	m_zTranslate = 0.0;

	m_xRotate = FALSE;
	m_yRotate = FALSE;
	m_zRotate = FALSE;

	capture = new VideoCapture(0);

	if (!capture->isOpened())
	{
		MessageBox(_T("Can't Connect Camera!\n"));
	}

	capture->set(CAP_PROP_FRAME_WIDTH, 640);
	capture->set(CAP_PROP_FRAME_HEIGHT, 480);

	
}

CGLSingleView::~CGLSingleView()
{
	delete capture;
}

BOOL CGLSingleView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: CREATESTRUCT cs를 수정하여 여기에서
	//  Window 클래스 또는 스타일을 수정합니다.

	return CView::PreCreateWindow(cs);
}

// CGLSingleView 그리기

void CGLSingleView::OnDraw(CDC* pDC)
{
	CGLSingleDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	if (m_viewMode == ViewMode::Compare)
	{
		DrawCompareView(pDC);
		return;
	}

	if (m_viewMode == ViewMode::Students)
	{
		CRect rect;
		GetClientRect(&rect);
		pDC->FillSolidRect(&rect, RGB(255, 255, 255));
		rect.DeflateRect(100, 100);
		if (rect.IsRectEmpty())
			return;

		CFont font;
		font.CreatePointFont(240, L"맑은 고딕");
		CFont* oldFont = pDC->SelectObject(&font);
		const int oldBkMode = pDC->SetBkMode(TRANSPARENT);
		const COLORREF oldTextColor = pDC->SetTextColor(RGB(0, 0, 0));
		pDC->DrawTextW(m_studentText, &rect, DT_LEFT | DT_TOP | DT_EXPANDTABS | DT_NOPREFIX);
		pDC->SetTextColor(oldTextColor);
		pDC->SetBkMode(oldBkMode);
		pDC->SelectObject(oldFont);
		return;
	}

	// TODO: 여기에 원시 데이터에 대한 그리기 코드를 추가합니다.
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	gluLookAt(m_eye[0], m_eye[1], m_eye[2], m_center[0], m_center[1], m_center[2], 0, 1, 0);
	
	glTranslatef(m_xTranslate, m_yTranslate, m_zTranslate);

	glRotatef(m_xTheta, 1.0, 0.0, 0.0);
	glRotatef(m_yTheta, 0.0, 1.0, 0.0);
	glRotatef(m_zTheta, 0.0, 0.0, 1.0);

	float temp_x=0;
	float temp_y = 0;
	float temp_color_R = 0;
	float temp_color_G = 0;
	float temp_color_B = 0;
	float color_avg = 0;
	glBegin(GL_LINES);
	for (int row = 0; row < mat_frame.rows; row++)
	{
		for (int col = 0; col < mat_frame.cols; col++)
		{
			uchar b = mat_frame.data[row * mat_frame.cols * 3 + col * 3];
			uchar g = mat_frame.data[row * mat_frame.cols * 3 + col * 3 + 1];
			uchar r = mat_frame.data[row * mat_frame.cols * 3 + col * 3 + 2];
			
			temp_color_R = (float)r / 255;
			temp_color_G = (float)g / 255;
			temp_color_B = (float)b / 255;

			color_avg=(temp_color_R + temp_color_G + temp_color_B) / 3;

			temp_x = (float)(2 * col - 320 * 2) / 320 * 2;
			temp_y = (float)-(2 * row - 240 * 2) / 240 * 2;
			glColor3f(temp_color_R, temp_color_G, temp_color_B);
			glVertex3f(temp_x, temp_y, 0.0);
			glColor3f(color_avg, color_avg, color_avg);
			glVertex3f(temp_x, temp_y, (temp_color_R+ temp_color_G+ temp_color_B)/3);
		}
	}
	glEnd();

	glFlush();
	SwapBuffers(m_hDC);
}


// CGLSingleView 인쇄

BOOL CGLSingleView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// 기본적인 준비
	return DoPreparePrinting(pInfo);
}

void CGLSingleView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 인쇄하기 전에 추가 초기화 작업을 추가합니다.
}

void CGLSingleView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 인쇄 후 정리 작업을 추가합니다.
}


// CGLSingleView 진단

#ifdef _DEBUG
void CGLSingleView::AssertValid() const
{
	CView::AssertValid();
}

void CGLSingleView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CGLSingleDoc* CGLSingleView::GetDocument() const // 디버그되지 않은 버전은 인라인으로 지정됩니다.
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CGLSingleDoc)));
	return (CGLSingleDoc*)m_pDocument;
}
#endif //_DEBUG

BOOL CGLSingleView::SetPixelformat(HDC hdc)
{
	// TODO: 여기에 구현 코드 추가.
	int pixelformat;

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW |
		PFD_SUPPORT_OPENGL |
		PFD_GENERIC_FORMAT |
		PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0,
		8,
		0,
		8,
		0, 0, 0, 0,
		16,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	if ((pixelformat = ChoosePixelFormat(hdc, &pfd)) == FALSE)
	{
		return FALSE;
	}
	if (SetPixelFormat(hdc, pixelformat, &pfd) == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

int CGLSingleView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  여기에 특수화된 작성 코드를 추가합니다.
	m_hDC = GetDC()->m_hDC;

	if (!SetPixelformat(m_hDC))
	{
		return -1;
	}

	m_hglRC = wglCreateContext(m_hDC);
	wglMakeCurrent(m_hDC, m_hglRC);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glClearColor(0.75f, 0.75f, 0.75f, 1.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	
	return 0;
}

void CGLSingleView::OnDestroy()
{
	CView::OnDestroy();

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	wglMakeCurrent(m_hDC, NULL);
	wglDeleteContext(m_hglRC);
	KillTimer(1);
	KillTimer(2);
	KillTimer(3);
	KillTimer(4);
}

void CGLSingleView::ReSizeScene(GLsizei width, GLsizei height)
{
	// TODO: 여기에 구현 코드 추가.
	if (height == 0)
	{
		height = 1;
	}
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.f);
	glMatrixMode(GL_MODELVIEW);
}

void CGLSingleView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	if (m_viewMode != ViewMode::Camera)
		Invalidate();

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	SetTimer(4, 100, NULL);
	ReSizeScene(cx, cy);
}

void CGLSingleView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	switch (nChar)
	{
		case 65:
			m_xTranslate -= 0.1;
			break;
		case 68:
			m_xTranslate += 0.1;
			break;
		case 87:
			m_yTranslate += 0.1;
			break;
		case 83:
			m_yTranslate -= 0.1;
			break;

			case 88:
			if (m_xRotate == FALSE)
			{
				SetTimer(1, 50, NULL);
				m_xRotate = TRUE;
			}
			else if (m_xRotate == TRUE)
			{
				KillTimer(1);
				m_xRotate = FALSE;
			}
			break;
		case 89:
			if (m_yRotate == FALSE)
			{
				SetTimer(2, 50, NULL);
				m_yRotate = TRUE;
			}
			else if (m_yRotate == TRUE)
			{
				KillTimer(2);
				m_yRotate = FALSE;
			}
			break;
		case 90:
			if (m_zRotate == FALSE)
			{
				SetTimer(3, 50, NULL);
				m_zRotate = TRUE;
			}
			else if (m_zRotate == TRUE)
			{
				KillTimer(3);
				m_zRotate = FALSE;
			}
			break;
				
		default:break;
	}
	Invalidate(FALSE);
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CGLSingleView::OnTimer(UINT_PTR nIDEvent)
{
	if (m_viewMode != ViewMode::Camera)
	{
		CView::OnTimer(nIDEvent);
		return;
	}
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (nIDEvent == 1)
	{
		m_xTheta -= 1;
	}
	else if (nIDEvent == 2)
	{
		m_yTheta -= 1;
	}
	else if (nIDEvent == 3)
	{
		m_zTheta -= 1;
	}
	else
	{

	}

	if (nIDEvent == 4 && capture && capture->isOpened())
	{
		capture->read(mat_frame);
	}

	Invalidate(FALSE);
	CView::OnTimer(nIDEvent);
}

void CGLSingleView::OnStudentSelect()
{
	// Resolve the database beside the executable, independent of the VS working directory.
	wchar_t executablePath[32768] = {};
	const DWORD length = GetModuleFileNameW(nullptr, executablePath, _countof(executablePath));
	if (length == 0 || length >= _countof(executablePath))
	{
		AfxMessageBox(L"실행 파일 경로를 확인할 수 없습니다.");
		return;
	}
	CString databasePath(executablePath);
	databasePath = databasePath.Left(databasePath.ReverseFind(L'\\') + 1) + L"test.db";
	CStringA databasePathUtf8(CW2A(databasePath, CP_UTF8));
	sqlite3* database = nullptr;
	int result = sqlite3_open_v2(databasePathUtf8, &database, SQLITE_OPEN_READONLY, nullptr);
	if (result != SQLITE_OK)
	{
		CString message = L"데이터베이스를 열 수 없습니다.\r\n" + databasePath;
		if (database)
			message += L"\r\n" + CString(CA2W(sqlite3_errmsg(database), CP_UTF8));
		sqlite3_close(database);
		AfxMessageBox(message, MB_ICONERROR);
		return;
	}

	sqlite3_stmt* statement = nullptr;
	result = sqlite3_prepare_v2(database, "SELECT id, name, age FROM Student ORDER BY id;", -1, &statement, nullptr);
	CString output;
	output = L"ID\t이름\t나이\r\n--------------------------------\r\n";
	int count = 0;
	if (result == SQLITE_OK)
	{
		while ((result = sqlite3_step(statement)) == SQLITE_ROW)
		{
			const unsigned char* name = sqlite3_column_text(statement, 1);
			CString wideName = name ? CString(CA2W(reinterpret_cast<const char*>(name), CP_UTF8)) : L"(NULL)";
			CString row;
			row.Format(L"%d\t%s\t%d\r\n", sqlite3_column_int(statement, 0), wideName.GetString(), sqlite3_column_int(statement, 2));
			output += row;
			++count;
		}
	}
	CString error;
	if (result != SQLITE_DONE)
		error = CString(CA2W(sqlite3_errmsg(database), CP_UTF8));
	sqlite3_finalize(statement);
	sqlite3_close(database);
	if (!error.IsEmpty())
	{
		AfxMessageBox(L"학생 조회 실패: " + error, MB_ICONERROR);
		return;
	}
	CString summary;
	summary.Format(L"\r\n총 %d명", count);
	m_studentText = output + summary;
	m_viewMode = ViewMode::Students;
	Invalidate();
}

void CGLSingleView::OnCameraView()
{
	m_viewMode = ViewMode::Camera;
	SetFocus();
	Invalidate(FALSE);
}

// 객체 비교

namespace
{
	// 기준 이미지는 1개만 두고, 업로드할 때마다 이 파일을 교체합니다.
	CString GetCompareReferencePath()
	{
		wchar_t executablePath[32768] = {};
		const DWORD length = GetModuleFileNameW(nullptr, executablePath, _countof(executablePath));
		if (length == 0 || length >= _countof(executablePath))
			return CString();
		CString path(executablePath);
		return path.Left(path.ReverseFind(L'\\') + 1) + L"compare\\reference.png";
	}

	bool SelectImageFile(CWnd* parent, CString& path)
	{
		CFileDialog dialog(TRUE, nullptr, nullptr, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
			L"이미지 파일 (*.png;*.jpg;*.jpeg;*.bmp;*.tif;*.tiff)|*.png;*.jpg;*.jpeg;*.bmp;*.tif;*.tiff|모든 파일 (*.*)|*.*||", parent);
		if (dialog.DoModal() != IDOK)
			return false;
		path = dialog.GetPathName();
		return true;
	}

	// 가로세로 비율을 유지하며 area 가운데에 BGR 이미지를 그립니다.
	void DrawMat(CDC* pDC, const cv::Mat& image, const CRect& area)
	{
		if (image.empty() || area.Width() <= 0 || area.Height() <= 0)
			return;
		const double scale = (std::min)(static_cast<double>(area.Width()) / image.cols, static_cast<double>(area.Height()) / image.rows);
		const int width = (std::max)(1, static_cast<int>(image.cols * scale));
		const int height = (std::max)(1, static_cast<int>(image.rows * scale));
		const int left = area.left + (area.Width() - width) / 2;
		const int top = area.top + (area.Height() - height) / 2;

		cv::Mat bgra;  // 32비트로 바꾸면 DIB 행의 4바이트 정렬 조건이 항상 만족됩니다.
		cv::cvtColor(image, bgra, cv::COLOR_BGR2BGRA);
		BITMAPINFO info = {};
		info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		info.bmiHeader.biWidth = bgra.cols;
		info.bmiHeader.biHeight = -bgra.rows;  // 음수: 위에서 아래로 저장된 이미지
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB;

		const int oldMode = pDC->SetStretchBltMode(HALFTONE);
		SetBrushOrgEx(pDC->GetSafeHdc(), 0, 0, nullptr);
		StretchDIBits(pDC->GetSafeHdc(), left, top, width, height, 0, 0, bgra.cols, bgra.rows,
			bgra.data, &info, DIB_RGB_COLORS, SRCCOPY);
		pDC->SetStretchBltMode(oldMode);
	}
}

bool CGLSingleView::EnsureCompareReference()
{
	if (m_compareReference.empty())
	{
		const CString path = GetCompareReferencePath();
		if (!path.IsEmpty())
			m_compareReference = objcmp::ReadImageFile(path.GetString());
	}
	return !m_compareReference.empty();
}

void CGLSingleView::ShowCompareView()
{
	EnsureCompareReference();
	m_viewMode = ViewMode::Compare;
	Invalidate();
}

void CGLSingleView::OnCompareLoadReference()
{
	CString path;
	if (!SelectImageFile(this, path))
		return;
	const cv::Mat image = objcmp::ReadImageFile(path.GetString());
	if (image.empty())
	{
		AfxMessageBox(L"이미지를 읽을 수 없습니다.\r\n" + path, MB_ICONERROR);
		return;
	}

	m_compareReference = image;
	m_compareResult = objcmp::CompareResult();

	const CString referencePath = GetCompareReferencePath();
	CreateDirectoryW(referencePath.Left(referencePath.ReverseFind(L'\\')), nullptr);
	if (referencePath.IsEmpty() || !objcmp::WriteImageFile(referencePath.GetString(), image))
		AfxMessageBox(L"기준 이미지를 저장하지 못했습니다. 프로그램을 다시 실행하면 기준 이미지를 다시 업로드해야 합니다.\r\n" + referencePath, MB_ICONWARNING);

	ShowCompareView();
}

void CGLSingleView::OnCompareLoadTarget()
{
	CString path;
	if (!SelectImageFile(this, path))
		return;
	const cv::Mat image = objcmp::ReadImageFile(path.GetString());
	if (image.empty())
	{
		AfxMessageBox(L"이미지를 읽을 수 없습니다.\r\n" + path, MB_ICONERROR);
		return;
	}

	m_compareTarget = image;
	m_compareResult = objcmp::CompareResult();
	ShowCompareView();
}

void CGLSingleView::OnCompareRun()
{
	if (!EnsureCompareReference())
	{
		AfxMessageBox(L"기준 이미지를 먼저 업로드하세요.", MB_ICONINFORMATION);
		return;
	}
	if (m_compareTarget.empty())
	{
		AfxMessageBox(L"비교대상 이미지를 먼저 업로드하세요.", MB_ICONINFORMATION);
		return;
	}

	try
	{
		CWaitCursor wait;
		m_compareResult = objcmp::CompareObjects(m_compareReference, m_compareTarget);
	}
	catch (const cv::Exception& e)
	{
		m_compareResult = objcmp::CompareResult();
		AfxMessageBox(L"이미지 비교 중 오류가 발생했습니다.\r\n" + CString(e.what()), MB_ICONERROR);
		return;
	}

	ShowCompareView();
	if (m_compareResult.status == objcmp::CompareStatus::AlignmentFailed)
		AfxMessageBox(L"두 이미지를 정렬하지 못했습니다.\r\n같은 물체를 비슷한 위치와 각도에서 촬영한 이미지인지 확인하세요.", MB_ICONWARNING);
}

void CGLSingleView::DrawCompareView(CDC* pDC)
{
	CRect client;
	GetClientRect(&client);
	pDC->FillSolidRect(&client, RGB(255, 255, 255));

	const bool hasResult = m_compareResult.status == objcmp::CompareStatus::Ok;
	const cv::Mat panes[3] = {
		hasResult ? m_compareResult.reference : m_compareReference,
		hasResult ? m_compareResult.alignedTarget : m_compareTarget,
		hasResult ? m_compareResult.overlay : cv::Mat() };
	const wchar_t* titles[3] = { L"기준 이미지", hasResult ? L"비교 대상 (기준에 맞춰 정렬)" : L"비교 대상 이미지", L"차이 검출 결과" };
	const wchar_t* placeholders[3] = { L"기준 이미지를 업로드하세요", L"비교대상 이미지를 업로드하세요", L"이미지 비교를 실행하세요" };

	CFont font;
	font.CreatePointFont(110, L"맑은 고딕");
	CFont* oldFont = pDC->SelectObject(&font);
	const int oldBkMode = pDC->SetBkMode(TRANSPARENT);
	const COLORREF oldTextColor = pDC->SetTextColor(RGB(0, 0, 0));

	const int margin = 20, titleHeight = 30, summaryHeight = 60;
	const int paneWidth = (client.Width() - margin * 4) / 3;
	const int imageBottom = client.bottom - summaryHeight - margin;
	CBrush frameBrush(RGB(200, 200, 200));
	for (int i = 0; i < 3; ++i)
	{
		const int left = margin + i * (paneWidth + margin);
		CRect titleRect(left, margin, left + paneWidth, margin + titleHeight);
		CRect imageRect(left, titleRect.bottom, left + paneWidth, imageBottom);
		if (imageRect.Width() <= 0 || imageRect.Height() <= 0)
			continue;
		pDC->DrawTextW(titles[i], &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		pDC->FrameRect(&imageRect, &frameBrush);
		if (panes[i].empty())
			pDC->DrawTextW(placeholders[i], &imageRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		else
			DrawMat(pDC, panes[i], imageRect);
	}

	CString summary;
	if (hasResult)
	{
		summary.Format(L"판정: %s    |    차이 영역: %d개    |    변화 면적 비율: %.2f%%    |    정렬 특징점(inlier): %d개",
			m_compareResult.defects.empty() ? L"변화 없음" : L"변화 감지",
			static_cast<int>(m_compareResult.defects.size()), m_compareResult.damagedRatio * 100.0, m_compareResult.inlierCount);
	}
	else if (m_compareResult.status == objcmp::CompareStatus::AlignmentFailed)
		summary = L"정렬 실패: 같은 물체를 비슷한 위치와 각도에서 촬영한 이미지인지 확인하세요.";
	else
		summary = L"객체 비교 메뉴에서 기준 이미지와 비교대상 이미지를 업로드한 뒤 '이미지 비교'를 실행하세요.";
	CRect summaryRect(margin, imageBottom, client.right - margin, client.bottom);
	pDC->DrawTextW(summary, &summaryRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	pDC->SetTextColor(oldTextColor);
	pDC->SetBkMode(oldBkMode);
	pDC->SelectObject(oldFont);
}
