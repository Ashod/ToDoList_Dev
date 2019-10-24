// BurndownGraphColorListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "burndownext.h"
#include "BurndownGraphColorListCtrl.h"
#include "BurndownStatic.h"
#include "BurndownChart.h"

#include "..\Shared\GraphicsMisc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBurndownGraphColorListCtrl

CBurndownGraphColorListCtrl::CBurndownGraphColorListCtrl()
{
}

CBurndownGraphColorListCtrl::~CBurndownGraphColorListCtrl()
{
}


BEGIN_MESSAGE_MAP(CBurndownGraphColorListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CBurndownGraphColorListCtrl)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBurndownGraphColorListCtrl message handlers

BOOL CBurndownGraphColorListCtrl::Initialize(const CBurndownChart& chart)
{
	ASSERT(GetStyle() & LVS_OWNERDRAWFIXED);
	ASSERT((GetStyle() & (LVS_SORTASCENDING | LVS_SORTDESCENDING)) == 0);

	// Build columns
	AutoAdd(FALSE, FALSE);
	ShowGrid(TRUE, TRUE);
	SetFirstColumnStretchy(TRUE);

	AddCol("Graph");
	DisableColumnEditing(0, TRUE);

	int nColor = chart.GetMaxRequiredColorCount();

	while (nColor--)
		AddCol(_T("Colour"), GraphicsMisc::ScaleByDPIFactor(60), ILCT_BROWSE);

	// Populate items
	for (int nType = 0; nType < NUM_GRAPHTYPES; nType++)
	{
		const GRAPHTYPE& gt = GRAPHTYPES[nType];

		int nRow = AddRow(CEnString(gt.nLabelID));
		SetItemData(nRow, gt.nType);

		// For each type, sort all the related graphs by name
		// before adding to list
		CGraphArray aGraphs;
		VERIFY(chart.BuildSortedGraphList(gt.nType, aGraphs));

		for (int nItem = 0; nItem < aGraphs.GetSize(); nItem++)
		{
			BURNDOWN_GRAPH nGraph = aGraphs[nItem];

			nRow = AddRow(chart.GetGraphTitle(nGraph));
			SetItemData(nRow, nGraph);
		}
	}

	ResizeStretchyColumns();

	return (GetItemCount() > 0);
}

BOOL CBurndownGraphColorListCtrl::GetButtonRect(int nRow, int nCol, CRect& rButton) const
{
	int dwItemData = GetItemData(nRow);

	if ((dwItemData == BCT_FREQUENCY) || (dwItemData == BCT_TIMESERIES))
		return FALSE;

	//int nNumColors = 
	return CInputListCtrl::GetButtonRect(nRow, nCol, rButton);
}

COLORREF CBurndownGraphColorListCtrl::GetItemBackColor(int nItem, int nCol, BOOL bSelected, BOOL bDropHighlighted, BOOL bWndFocus) const
{
	int dwItemData = GetItemData(nItem);

	if ((dwItemData == BCT_FREQUENCY) || (dwItemData == BCT_TIMESERIES))
		return GetSysColor(COLOR_3DLIGHT);

	return CInputListCtrl::GetItemBackColor(nItem, nCol, bSelected, bDropHighlighted, bWndFocus);
}