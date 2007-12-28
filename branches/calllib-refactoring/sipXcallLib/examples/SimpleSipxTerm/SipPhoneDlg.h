#if !defined(AFX_SIPPHONEDLG_H__4C7C13C8_03D2_462B_BD57_888D8A4FED7A__INCLUDED_)
#define AFX_SIPPHONEDLG_H__4C7C13C8_03D2_462B_BD57_888D8A4FED7A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SipPhoneDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSipPhoneDlg dialog
#include "tapi/sipXtapi.h"
#include "tapi/sipXtapiEvents.h"

class CSimpleSipxTermDlg;

class CSipPhoneDlg : public CDialog
{
// Construction
public:
	CSipPhoneDlg( CWnd* pParent                   = NULL,
                CSimpleSipxTermDlg* fi_pToCtrl  = NULL );   // standard constructor

  CSimpleSipxTermDlg* m_pCtrl;
  CString             m_OwnDnr;
  int                 m_PhoneIndex;
  bool                m_FirstTime;

  HWND  m_ParentHwnd;
  void  SetParentHwnd     ( HWND fi_Wnd) { m_ParentHwnd = fi_Wnd; } 
  void  SetConfig         ( int     fi_PhoneIndex,
                            int     fi_OwnDNR );
  void  SetConfig         ( int     fi_PhoneIndex,
                            CString fi_OwnDNR );
                            
// Dialog Data
	//{{AFX_DATA(CSipPhoneDlg)
	enum { IDD = IDD_SIPPHONE };
	CEdit	m_UpperDisplay;
	CEdit	m_LowerDisplay;
	CString	m_DestinationDnr;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSipPhoneDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSipPhoneDlg)
	afx_msg void OnButMakeCall();
	afx_msg void OnButAnswerCall();
	afx_msg void OnButClearCall();
	afx_msg void OnButHold_UnholdCall();
	afx_msg void OnButTransferCall();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SIPPHONEDLG_H__4C7C13C8_03D2_462B_BD57_888D8A4FED7A__INCLUDED_)
