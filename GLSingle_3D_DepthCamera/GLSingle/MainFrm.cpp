
// MainFrm.cpp: CMainFrame 클래스의 구현
//

#include "pch.h"
#include "framework.h"
#include "GLSingle.h"

#include "MainFrm.h"

#include "GLSingleDoc.h"
#include "GLSingleView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
using namespace std;

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_COMMAND(ID_32773, &CMainFrame::OnSerialConnect)
	ON_COMMAND(ID_32774, &CMainFrame::OnSerialRead)
	ON_COMMAND(ID_32775, &CMainFrame::OnSerialDisconnect)
	ON_COMMAND(ID_32777, &CMainFrame::OnFileOpenText)
END_MESSAGE_MAP()

// CMainFrame 생성/소멸

namespace // UTF-8, UTF-16, CP949 자동 처리 코드 입니다. CStdioFile로 읽은 버퍼를 CString으로 변환합니다. 
{ // 이거 없으면 한글 깨집니다. 
	CString Utf8ToCString(const BYTE* data, size_t size)
	{
		int wlen = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)data, (int)size, NULL, 0);
		CString result;
		if (wlen > 0)
		{
			MultiByteToWideChar(CP_UTF8, 0, (LPCCH)data, (int)size, result.GetBufferSetLength(wlen), wlen);
			result.ReleaseBuffer();
		}
		return result;
	}

	CString BytesToCString(const std::vector<BYTE>& buf)
	{
		if (buf.empty())
			return CString();

		const BYTE* data = buf.data();
		size_t size = buf.size();

		// UTF-8 BOM (EF BB BF)
		if (size >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
		{
			return Utf8ToCString(data + 3, size - 3);
		}
		// UTF-16 LE BOM (FF FE)
		else if (size >= 2 && data[0] == 0xFF && data[1] == 0xFE)
		{
			int wlen = (int)(size - 2) / 2;
			CString result;
			memcpy(result.GetBufferSetLength(wlen), data + 2, wlen * 2);
			result.ReleaseBuffer();
			return result;
		}
		else
		{
			// BOM이 없으면 UTF-8인지 검사 (MB_ERR_INVALID_CHARS로 엄격 검증)
			int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCCH)data, (int)size, NULL, 0);
			if (wlen > 0)
			{
				return Utf8ToCString(data, size);
			}

			// UTF-8이 아니면 시스템 기본 코드페이지(한글 Windows는 CP949)로 처리
			int alen = MultiByteToWideChar(CP_ACP, 0, (LPCCH)data, (int)size, NULL, 0);
			CString result;
			if (alen > 0)
			{
				MultiByteToWideChar(CP_ACP, 0, (LPCCH)data, (int)size, result.GetBufferSetLength(alen), alen);
				result.ReleaseBuffer();
			}
			return result;
		}
	}
}

CMainFrame::CMainFrame() noexcept
{
	// TODO: 여기에 멤버 초기화 코드를 추가합니다.
}

CMainFrame::~CMainFrame()
{
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: CREATESTRUCT cs를 수정하여 여기에서
	//  Window 클래스 또는 스타일을 수정합니다.

	return TRUE;
}

// CMainFrame 진단

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG


// CMainFrame 메시지 처리기


void CMainFrame::OnSerialConnect()
{
	// TODO: 여기에 명령 처리기 코드를 추가합니다.
	CGLSingleView* pView = (CGLSingleView*)GetActiveView();
	CGLSingleDoc* pDoc = (CGLSingleDoc*)pView->GetDocument();

	if (pDoc->bConnected) {
		AfxMessageBox(_T("이미 연결되어 있습니다."));
		return;
	}

	pDoc->hSerial = CreateFile(
		_T("\\\\.\\COM3"),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (pDoc->hSerial == INVALID_HANDLE_VALUE) {
		AfxMessageBox(_T("포트 열기 실패"));
		return;
	}

	DCB dcb = { 0 };
	dcb.DCBlength = sizeof(dcb);
	GetCommState(pDoc->hSerial, &dcb);
	dcb.BaudRate = CBR_115200;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	if (!SetCommState(pDoc->hSerial, &dcb)) {
		AfxMessageBox(_T("통신 규격 설정 실패"));
		CloseHandle(pDoc->hSerial);
		pDoc->hSerial = INVALID_HANDLE_VALUE;
		return;
	}

	COMMTIMEOUTS to = { 0 };
	to.ReadIntervalTimeout = 50;
	to.ReadTotalTimeoutConstant = 500;
	to.ReadTotalTimeoutMultiplier = 10;
	SetCommTimeouts(pDoc->hSerial, &to);

	pDoc->bConnected = TRUE;
	AfxMessageBox(_T("포트 연결에 성공했습니다."));
	pView->Invalidate(TRUE);
}

void CMainFrame::OnSerialRead()
{
	// TODO: 여기에 명령 처리기 코드를 추가합니다.
	CGLSingleView* pView = (CGLSingleView*)GetActiveView();
	CGLSingleDoc* pDoc = (CGLSingleDoc*)pView->GetDocument();

	if (!pDoc->bConnected) {
		AfxMessageBox(_T("포트가 연결되어 있지 않습니다. 데이터 통신 - 포트 연결 메뉴를 사용하여 포트를 연결하세요."));
		return;
	}

	PurgeComm(pDoc->hSerial, PURGE_RXCLEAR);

	Sleep(1200);

	char buf[256] = { 0 };
	DWORD nRead = 0;

	if (!ReadFile(pDoc->hSerial, buf, sizeof(buf) - 1, &nRead, NULL)) {
		AfxMessageBox(_T("읽기 실패"));
		return;
	}

	if (nRead == 0) {
		AfxMessageBox(_T("데이터가 오지 않습니다."));
		return;
	}

	buf[nRead] = '\0';

	pDoc->strReceivedData = buf;
	pView->Invalidate(TRUE);
}

void CMainFrame::OnSerialDisconnect()
{
	// TODO: 여기에 명령 처리기 코드를 추가합니다.
	CGLSingleView* pView = (CGLSingleView*)GetActiveView();
	CGLSingleDoc* pDoc = (CGLSingleDoc*)pView->GetDocument();

	if (!pDoc->bConnected) {
		AfxMessageBox(_T("이미 포트가 연결되어 있지 않은 상태입니다."));
		return;
	}

	CloseHandle(pDoc->hSerial);
	pDoc->hSerial = INVALID_HANDLE_VALUE;
	pDoc->bConnected = FALSE;
	pDoc->strReceivedData = "";
	AfxMessageBox(_T("포트 연결이 해제되었습니다."));

	pView->Invalidate(TRUE);
}

void CMainFrame::OnFileOpenText()
{
	// TODO: 여기에 명령 처리기 코드를 추가합니다.
	CGLSingleView* pView = (CGLSingleView*)GetActiveView();
	if (!pView)
		return;
	CGLSingleDoc* pDoc = pView->GetDocument();
	if (!pDoc)
		return;

	CFileDialog dlg(TRUE, L"txt", NULL,
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		L"텍스트 파일 (*.txt)|*.txt|모든 파일 (*.*)|*.*||");

	if (dlg.DoModal() != IDOK)
		return;

	//-------------------------------------- 위에 코드는 이미지 여는것과 같이 파일 열기 위한 코드입니다. 읽니느 밑에서 합니다.

	CStdioFile file; // CStdioFile를 사용하여 텍스트 파일을 읽습니다.
	CFileException fe; // 파일 예외 처리를 위한 객체 생성

	if (!file.Open(dlg.GetPathName(), CFile::modeRead | CFile::typeText, &fe)) // 파일 열기 시도 , 실패시 실행
	{
		AfxMessageBox(L"텍스트 파일을 열지 못했습니다.");
		return;
	}

	ULONGLONG len = file.GetLength(); // 파일 길이 가져오기 len이 길이 입니다. 비었으면 0 입니다. 아시겠지만, CStdioFile는 텍스트 모드로 열기 때문에 GetLength()는 실제 바이트 수를 반환합니다.
	vector<BYTE> buf(static_cast<size_t>(len)); // 버퍼를 파일 길이만큼 할당합니다.
	if (len > 0) // 얘가 비었으면 read를 호출하면 안되기에 조건문을 넣었습니다. 
		file.Read(buf.data(), (UINT)len);

	file.Close(); // 닫는거져

	CString content = BytesToCString(buf); // 버퍼를 Cstring으로 변환하고 이떄 BOM을 확인하고 UTF-8, UTF-16, CP949를 자동으로 처리합니다.

	pDoc->m_text[0] = content; // 문서에 내용을 저장합니다.

	AfxMessageBox(content.IsEmpty() ? L"(빈 파일입니다)" : content); // 내용을 메시지 박스로 표시합니다.

	//MFC 창에 출력하고 싶으시면 pDC에 있는 TextOutW를 사용하시면 됩니다. ^ ^
}
