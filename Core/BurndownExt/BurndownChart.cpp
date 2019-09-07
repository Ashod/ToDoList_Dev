// BurndownChart.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "BurndownChart.h"
#include "BurndownStatic.h"
#include "BurndownGraphs.h"

#include "..\shared\datehelper.h"
#include "..\shared\holdredraw.h"
#include "..\shared\enstring.h"
#include "..\shared\graphicsmisc.h"
//#include "..\shared\filemisc.h"

/////////////////////////////////////////////////////////////////////////////

const int MIN_XSCALE_SPACING = 50; // pixels

static BURNDOWN_CHARTSCALE SCALES[] = 
{
	BCS_DAY,		
	BCS_WEEK,	
	BCS_MONTH,	
	BCS_2MONTH,	
	BCS_QUARTER,	
	BCS_HALFYEAR,
	BCS_YEAR,	
};
static int NUM_SCALES = sizeof(SCALES) / sizeof(int);

////////////////////////////////////////////////////////////////////////////////
// CBurndownChart

CBurndownChart::CBurndownChart(const CStatsItemArray& data)
	: 
	m_data(data),
	m_nScale(BCS_DAY),
	m_nChartType(BCT_INCOMPLETETASKS),
	m_calculator(data)
{
	VERIFY(m_graphs.Add(new CIncompleteTasksGraph()) == BCT_INCOMPLETETASKS);
	VERIFY(m_graphs.Add(new CRemainingDaysGraph()) == BCT_REMAININGDAYS);
	VERIFY(m_graphs.Add(new CStartedEndedTasksGraph()) == BCT_STARTEDENDEDTASKS);
	VERIFY(m_graphs.Add(new CEstimatedSpentDaysGraph()) == BCT_ESTIMATEDSPENTDAYS);
	//m_graphs.Add(new ());

	//FileMisc::EnableLogging(TRUE);
}

CBurndownChart::~CBurndownChart()
{
	int nGraph = m_graphs.GetSize();

	while (nGraph--)
	{
		delete m_graphs[nGraph];
		m_graphs.RemoveAt(nGraph);
	}
}

////////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CBurndownChart, CHMXChartEx)
	ON_WM_SIZE()
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////////
// CBurndownChart message handlers

BOOL CBurndownChart::IsValidType(BURNDOWN_CHARTTYPE nType) const
{
	return ((nType >= 0) && (nType < BCT_NUMGRAPHS) && (nType < m_graphs.GetSize()));
}

CString CBurndownChart::GetGraphTitle(BURNDOWN_CHARTTYPE nType) const
{
	if (!IsValidType(nType))
	{
		ASSERT(0);
		return _T("");
	}

	// else
	return m_graphs[nType]->GetTitle();
}

BOOL CBurndownChart::SetActiveGraph(BURNDOWN_CHARTTYPE nType)
{
	if (!IsValidType(nType))
	{
		ASSERT(0);
		return FALSE;
	}

	if (nType != m_nChartType)
	{
		m_nChartType = nType;
		RebuildGraph(m_dtExtents);

		return TRUE;
	}

	return FALSE;
}

BOOL CBurndownChart::SaveToImage(CBitmap& bmImage)
{
	CClientDC dc(this);
	CDC dcImage;

	if (dcImage.CreateCompatibleDC(&dc))
	{
		CRect rClient;
		GetClientRect(rClient);

		if (bmImage.CreateCompatibleBitmap(&dc, rClient.Width(), rClient.Height()))
		{
			CBitmap* pOldImage = dcImage.SelectObject(&bmImage);

			SendMessage(WM_PRINTCLIENT, (WPARAM)dcImage.GetSafeHdc(), PRF_ERASEBKGND);
			Invalidate(TRUE);

			dcImage.SelectObject(pOldImage);
		}
	}

	return (bmImage.GetSafeHandle() != NULL);
}

BURNDOWN_CHARTSCALE CBurndownChart::CalculateRequiredXScale() const
{
	// calculate new x scale
	int nDataWidth = GetDataArea().cx;
	int nNumDays = m_dtExtents.GetDayCount();

	// work thru the available scales until we find a suitable one
	for (int nScale = 0; nScale < NUM_SCALES; nScale++)
	{
		int nSpacing = MulDiv(SCALES[nScale], nDataWidth, nNumDays);

		if (nSpacing > MIN_XSCALE_SPACING)
			return SCALES[nScale];
	}

	return BCS_YEAR;
}

void CBurndownChart::RebuildXScale()
{
	ClearXScaleLabels();
	SetXLabelStep(1); // Because we often have an uneven label spacing

	// calc new scale
	m_nScale = CalculateRequiredXScale();

	// build ticks
	int nNumDays = m_calculator.GetTotalDays();
	COleDateTime dtTick = m_calculator.GetStartDate();

	CDateHelper dh;
	CString sTick;
	
	for (int nDay = 0; nDay <= nNumDays; )
	{
		sTick = dh.FormatDate(dtTick);
		SetXScaleLabel(nDay, sTick);

		// next Tick date
		COleDateTime dtNextTick(dtTick);

		switch (m_nScale)
		{
		case BCS_DAY:
			dtNextTick.m_dt += 1.0;
			break;
			
		case BCS_WEEK:
			dh.OffsetDate(dtNextTick, 1, DHU_WEEKS);
			break;
			
		case BCS_MONTH:
			dh.OffsetDate(dtNextTick, 1, DHU_MONTHS);
			break;
			
		case BCS_2MONTH:
			dh.OffsetDate(dtNextTick, 2, DHU_MONTHS);
			break;
			
		case BCS_QUARTER:
			dh.OffsetDate(dtNextTick, 3, DHU_MONTHS);
			break;
			
		case BCS_HALFYEAR:
			dh.OffsetDate(dtNextTick, 6, DHU_MONTHS);
			break;
			
		case BCS_YEAR:
			dh.OffsetDate(dtNextTick, 1, DHU_YEARS);
			break;
			
		default:
			ASSERT(0);
		}

		nDay += (int)(dtNextTick.m_dt - dtTick.m_dt);

		dtTick = dtNextTick;
	}
}

void CBurndownChart::OnSize(UINT nType, int cx, int cy) 
{
	CHMXChartEx::OnSize(nType, cx, cy);
	
	int nOldScale = m_nScale;
	RebuildXScale();
		
	// handle scale change
	if (m_nScale != nOldScale)
		RebuildGraph(m_dtExtents);
}

void CBurndownChart::RebuildGraph(const COleDateTimeRange& dtExtents)
{
	if (!m_dtExtents.Set(dtExtents))
	{
		ASSERT(0);
		return;
	}

	m_calculator.SetDateRange(m_dtExtents);

	CWaitCursor cursor;
	CHoldRedraw hr(*this);
	
	ClearData();
	SetYText(m_graphs[m_nChartType]->GetTitle());
	
	if (!m_data.IsEmpty())
		RebuildXScale();
	
	m_graphs[m_nChartType]->BuildGraph(m_calculator, m_datasets);

	CalcDatas();
}

void CBurndownChart::PreSubclassWindow()
{
	SetBkGnd(GetSysColor(COLOR_WINDOW));
	SetXLabelsAreTicks(true);
	SetXLabelAngle(45);
	SetYTicks(10);

	VERIFY(InitTooltip(TRUE));
}

CString CBurndownChart::GetTooltip(int nHit) const
{
	ASSERT(nHit != -1);

	return m_graphs[m_nChartType]->GetTooltip(m_calculator, m_datasets, nHit);
}

int CBurndownChart::HitTest(const CPoint& ptClient) const
{
	if (!m_dtExtents.IsValid())
		return -1;

	return CHMXChartEx::HitTest(ptClient);
}
