// aboutdlg.cpp : implementation of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"

LRESULT CAboutDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow(GetParent());
	CString declare("����������\r\n"
		"1.�������Ʒ��Ϊ����ѧϰATL&WTL��̼���֮��������ַ��κθ��˻��������棬���뱾����ϵ��\r\n"
		"2.���˶�����ʹ�ñ����������ɵ��κ��𻵺���ʧ�����κ����Ρ�\r\n"
		"3.����ʹ�ñ����֮ʱ�𣬱�ʾ����Ը���ܴ�����֮������Ŀ��");
	SetDlgItemText(IDC_ABOUT_EDIT, declare);
	return TRUE;
}

LRESULT CAboutDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}


LRESULT CAboutDlg::OnNMClickSyslinkMail(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/)
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	::ShellExecute(NULL, NULL, ((PNMLINK)pNMHDR)->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
	return 0;
}
