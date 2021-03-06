//	CGItemListArea.cpp
//
//	CGItemListArea class

#include "PreComp.h"
#include "Transcendence.h"

const int DEFAULT_ROW_HEIGHT =				96;
const int ICON_WIDTH =						96;
const int ICON_HEIGHT =						96;

const int ITEM_TEXT_MARGIN_Y =				4;
const int ITEM_TEXT_MARGIN_X =				4;
const int ITEM_TEXT_MARGIN_BOTTOM =			10;
const int BORDER_RADIUS =					4;
const int SELECTION_WIDTH =					2;

const int MODIFIER_SPACING_X =				4;

const CG32bitPixel RGB_DISABLED_TEXT =					CG32bitPixel(128,128,128);

const CG32bitPixel RGB_MODIFIER_NORMAL_BACKGROUND =		CG32bitPixel(101,101,101);	//	H:0   S:0   B:40
const CG32bitPixel RGB_MODIFIER_NORMAL_TEXT =			CG32bitPixel(220,220,220);	//	H:0   S:0   B:86

const int DAMAGE_ADJ_ICON_WIDTH =			16;
const int DAMAGE_ADJ_ICON_HEIGHT =			16;
const int DAMAGE_ADJ_ICON_SPACING_X =		2;

const int DAMAGE_ADJ_SPACING_X =			6;

const int ATTRIB_PADDING_X =				4;
const int ATTRIB_PADDING_Y =				1;
const int ATTRIB_SPACING_X =				2;
const int ATTRIB_SPACING_Y =				2;

#define STR_NO_ITEMS						CONSTLIT("There are no items here")

CGItemListArea::CGItemListArea (const CVisualPalette &VI) :
		m_VI(VI),
		m_pListData(NULL),
		m_iType(listNone),
		m_pUIRes(NULL),
		m_pFonts(NULL),
        m_rgbTextColor(255, 255, 255),
		m_iOldCursor(-1),
		m_yOffset(0),
		m_yFirst(0),
		m_cyRow(DEFAULT_ROW_HEIGHT)

//	CGItemListArea constructor

	{
	}

CGItemListArea::~CGItemListArea (void)

//	CGItemListArea destructor

	{
	if (m_pListData)
		delete m_pListData;
	}

int CGItemListArea::CalcRowHeight (int iRow)

//	CalcRowHeight
//
//	Returns the height of the given row

	{
	//	Set the position

	int iOldPos = m_pListData->GetCursor();
	m_pListData->SetCursor(iRow);

	//	Compute the rect where we're painting (we only care about width)

	RECT rcRect = GetRect();

	//	Compute row height based on type of list

	int cyHeight;
	switch (m_iType)
		{
		case listItem:
			{
			//	Get the item

			if (!m_pListData->IsCursorValid())
				{
				cyHeight = DEFAULT_ROW_HEIGHT;
				break;
				}

			CUIHelper UIHelper(*g_pHI);
			cyHeight = UIHelper.CalcItemEntryHeight(m_pListData->GetSource(), m_pListData->GetItemAtCursor(), rcRect, 0);
			break;
			}

		case listCustom:
			cyHeight = m_cyRow;
			break;

		default:
			cyHeight = DEFAULT_ROW_HEIGHT;
			break;
		}

	//	Done

	m_pListData->SetCursor(iOldPos);
	return cyHeight;
	}

void CGItemListArea::CleanUp (void)

//	CleanUp
//
//	Free list data

	{
	if (m_pListData)
		{
		delete m_pListData;
		m_pListData = NULL;
		}
	}

int CGItemListArea::FindRow (int y)

//	FindRow
//
//	Returns the row that is at the given absolute coordinates (or -1 if not
//	found)

	{
	int i;

	for (i = 0; i < m_Rows.GetCount(); i++)
		if (y >= m_Rows[i].yPos && y < m_Rows[i].yPos + m_Rows[i].cyHeight)
			return i;

	return -1;
	}

ICCItem *CGItemListArea::GetEntryAtCursor (void)

//	GetEntryAtCursor
//
//	Returns the current entry

	{
	CCodeChain &CC = g_pUniverse->GetCC();

	if (m_pListData == NULL)
		return CC.CreateNil();

	return m_pListData->GetEntryAtCursor(CC);
	}

void CGItemListArea::InitRowDesc (void)

//	InitRowDesc
//
//	Initializes the row descriptors

	{
	int i;

	m_Rows.DeleteAll();
	if (m_pListData == NULL || m_pListData->GetCount() == 0)
		{
		m_cyTotalHeight = 0;
		return;
		}

	m_Rows.InsertEmpty(m_pListData->GetCount());
	int y = 0;
	for (i = 0; i < m_pListData->GetCount(); i++)
		{
		//	Compute the height of the row

		int cyRow = CalcRowHeight(i);

		//	Add it

		m_Rows[i].yPos = y;
		m_Rows[i].cyHeight = cyRow;

		y += cyRow;
		}

	m_cyTotalHeight = y;
	}

bool CGItemListArea::LButtonDown (int x, int y)

//	LButtonDown
//
//	Handle button down

	{
	if (m_iOldCursor != -1 && m_pListData->GetCount())
		{
		//	Figure out the cursor position that the user clicked on

		int iPos = FindRow((y - GetRect().top) - m_yFirst);
		if (iPos >= 0 && iPos < m_pListData->GetCount())
			SignalAction(iPos);

		return true;
		}

	return false;
	}

bool CGItemListArea::MoveCursorBack (void)

//	MoveCursorBack
//
//	Move cursor back

	{
	bool bOK = (m_pListData ? m_pListData->MoveCursorBack() : false);
	if (bOK)
		Invalidate();
	return bOK;
	}

bool CGItemListArea::MoveCursorForward (void)

//	MoveCursorForward
//
//	Move cursor forward

	{
	bool bOK = (m_pListData ? m_pListData->MoveCursorForward() : false);
	if (bOK)
		Invalidate();
	return bOK;
	}

void CGItemListArea::Paint (CG32bitImage &Dest, const RECT &rcRect)

//	Paint
//
//	Paint the area

	{
	//	Can't paint if we are not properly initialized

	if (m_pFonts == NULL)
		return;

	bool bPaintCursor = false;
	RECT rcCursor;

	//	Paint Background

    CG32bitPixel rgbFadeBackColor = CG32bitPixel(CG32bitPixel::Darken(m_rgbBackColor, 220), 220);
	CGDraw::RoundedRect(Dest,
			rcRect.left,
			rcRect.top,
			RectWidth(rcRect),
			RectHeight(rcRect),
			BORDER_RADIUS + 1,
			rgbFadeBackColor);

	//	If there are no items here, then say so

	if (m_pListData == NULL || !m_pListData->IsCursorValid())
		{
		int x = rcRect.left + (RectWidth(rcRect) - m_pFonts->LargeBold.MeasureText(STR_NO_ITEMS)) / 2;
		int y = rcRect.top + (RectHeight(rcRect) - m_pFonts->LargeBold.GetHeight()) / 2;

		Dest.DrawText(x, y,
				m_pFonts->LargeBold,
				RGB_DISABLED_TEXT,
				STR_NO_ITEMS);

		m_iOldCursor = -1;
		}

	//	Otherwise, paint the list of items

	else
		{
		int iCursor = m_pListData->GetCursor();
		int iCount = m_pListData->GetCount();

		ASSERT(iCursor >= 0 && iCursor < m_Rows.GetCount());

		//	If the cursor has changed, update the offset so that we
		//	have a smooth scroll.

		if (m_iOldCursor != -1 
				&& m_iOldCursor != iCursor
				&& m_iOldCursor < m_Rows.GetCount())
			{
			int cyDiff = (m_Rows[iCursor].cyHeight - m_Rows[m_iOldCursor].cyHeight) / 2;

			if (m_iOldCursor < iCursor)
				m_yOffset = m_Rows[m_iOldCursor].cyHeight + cyDiff;
			else
				m_yOffset = -(m_Rows[iCursor].cyHeight - cyDiff);
			}

		m_iOldCursor = iCursor;

		//	Figure out the ideal position of the cursor (relative to the
		//	rect).

		int yIdeal = m_yOffset + ((RectHeight(rcRect) - m_Rows[iCursor].cyHeight) / 2);

		//	Figure out the actual position of the cursor row (relative to the
		//	rect).

		int yCursor;

		//	If the cursor is in the top part of the list
		if (m_Rows[iCursor].yPos < yIdeal)
			yCursor = m_Rows[iCursor].yPos;

		//	If the total number of lines is less than the whole rect
		else if (m_cyTotalHeight < RectHeight(rcRect))
			yCursor = m_Rows[iCursor].yPos;

		//	If the cursor is in the bottom part of the list
		else if ((m_cyTotalHeight - m_Rows[iCursor].yPos) < (RectHeight(rcRect) - yIdeal))
			yCursor = (RectHeight(rcRect) - (m_cyTotalHeight - m_Rows[iCursor].yPos));

		//	The cursor is in the middle of the list
		else
			yCursor = yIdeal;

		//	Figure out the item position at which we start painting

		int iStart = FindRow(m_Rows[iCursor].yPos - yCursor);
		ASSERT(iStart != -1);
		if (iStart == -1)
			iStart = 0;

		int yStart = yCursor - (m_Rows[iCursor].yPos - m_Rows[iStart].yPos);

		//	Compute y offset of first row (so that we can handle clicks later)

		m_yFirst = yStart - m_Rows[iStart].yPos;

		//	Paint

		int y = rcRect.top + yStart;
		int iPos = iStart;
		bool bPaintSeparator = false;
		RECT rcItem;

		while (y < rcRect.bottom && iPos < m_Rows.GetCount())
			{
			//	Paint previous separator

			if (bPaintSeparator)
				{
				Dest.Fill(rcItem.left + BORDER_RADIUS,
						rcItem.bottom - 1,
						RectWidth(rcItem) - (BORDER_RADIUS * 2),
						1,
						CG32bitPixel(50,50,50));
				}
			else
				bPaintSeparator = true;

			//	Paint only if we have a valid entry. Sometimes we can
			//	start at an invalid entry because we're scrolling.

			if (iPos >= 0)
				{
				m_pListData->SetCursor(iPos);

				rcItem.top = y;
				rcItem.left = rcRect.left;
				rcItem.bottom = y + m_Rows[iPos].cyHeight;
				rcItem.right = rcRect.right;

				//	See if we need to paint the cursor

				bool bIsCursor = (iPos == iCursor);

				//	Paint selection background (if selected)

				if (bIsCursor)
					{
					bPaintCursor = true;
					bPaintSeparator = false;
					rcCursor = rcItem;

					CGDraw::RoundedRect(Dest, 
							rcCursor.left,
							rcCursor.top,
							RectWidth(rcCursor),
							RectHeight(rcCursor),
							BORDER_RADIUS,
							m_rgbBackColor);
					}

				//	Paint item

				switch (m_iType)
					{
					case listCustom:
						PaintCustom(Dest, rcItem, bIsCursor);
						break;

					case listItem:
						PaintItem(Dest, m_pListData->GetItemAtCursor(), rcItem, bIsCursor);
						break;
					}
				}

			//	Next

			y += m_Rows[iPos].cyHeight;
			iPos++;
			}

		//	Done

		m_pListData->SetCursor(iCursor);
		}

	//	Paint a frame

	CGDraw::RoundedRectOutline(Dest,
			rcRect.left,
			rcRect.top,
			RectWidth(rcRect) - 1,
			RectHeight(rcRect) - 1,
			BORDER_RADIUS + 1,
			1,
			CG32bitPixel(80,80,80));

	if (bPaintCursor)
		{
		CGDraw::RoundedRectOutline(Dest,
				rcCursor.left + 1,
				rcCursor.top + 1,
				RectWidth(rcCursor) - 2,
				RectHeight(rcCursor) - 2,
				BORDER_RADIUS,
				SELECTION_WIDTH,
				m_VI.GetColor(colorAreaDialogHighlight));
		}
	}

void CGItemListArea::PaintCustom (CG32bitImage &Dest, const RECT &rcRect, bool bSelected)

//	PaintCustom
//
//	Paints a custom element

	{
	//	Paint the image

	m_pListData->PaintImageAtCursor(Dest, rcRect.left, rcRect.top);

	RECT rcDrawRect = rcRect;
	rcDrawRect.left += ICON_WIDTH + ITEM_TEXT_MARGIN_X;
	rcDrawRect.right -= ITEM_TEXT_MARGIN_X;
	rcDrawRect.top += ITEM_TEXT_MARGIN_Y;

	//	Paint the title

	int cyHeight;
	RECT rcTitle = rcDrawRect;
	m_pFonts->LargeBold.DrawText(Dest,
			rcTitle,
			m_pFonts->rgbItemTitle,
			m_pListData->GetTitleAtCursor(),
			0,
			CG16bitFont::SmartQuotes | CG16bitFont::TruncateLine,
			&cyHeight);

	rcDrawRect.top += cyHeight;

	//	Paint the description

	m_pFonts->Medium.DrawText(Dest, 
			rcDrawRect,
			(bSelected ? m_pFonts->rgbItemDescSelected : m_pFonts->rgbItemDesc),
			m_pListData->GetDescAtCursor(),
			0,
			CG16bitFont::SmartQuotes,
			&cyHeight);

	rcDrawRect.top += cyHeight;
	}

void CGItemListArea::PaintItem (CG32bitImage &Dest, const CItem &Item, const RECT &rcRect, bool bSelected)

//	PaintItem
//
//	Paints the item

	{
	CUIHelper UIHelper(*g_pHI);

	DWORD dwOptions = 0;
	if (bSelected)
		dwOptions |= CUIHelper::OPTION_SELECTED;

	UIHelper.PaintItemEntry(Dest, m_pListData->GetSource(), Item, rcRect, m_rgbTextColor, dwOptions);
	}

void CGItemListArea::SetList (CSpaceObject *pSource)

//	SetList
//
//	Sets the item list that this control will display

	{
	CleanUp();

	//	Make sure the items in the source are sorted

	pSource->GetItemList().SortItems();

	//	Create a new data source

	m_pListData = new CItemListWrapper(pSource);
	m_iType = listItem;

	//	Done

	InitRowDesc();
	Invalidate();
	}

void CGItemListArea::SetList (CItemList &ItemList)

//	SetList
//
//	Set list of items

	{
	CleanUp();

	//	Create a new data source

	m_pListData = new CItemListWrapper(ItemList);
	m_iType = listItem;

	//	Done

	InitRowDesc();
	Invalidate();
	}

void CGItemListArea::SetList (CCodeChain &CC, ICCItem *pList)

//	SetList
//
//	Sets the list from a CC list

	{
	CleanUp();

	//	Create the new data source

	m_pListData = new CListWrapper(&CC, pList);
	m_iType = listCustom;

	//	Done

	InitRowDesc();
	Invalidate();
	}

void CGItemListArea::Update (void)

//	Update
//
//	Update state

	{
	//	Handle smooth scroll

	if (m_yOffset)
		{
		int iDelta;
		if (m_yOffset > 0)
			{
			iDelta = Max(12, m_yOffset / 4);
			m_yOffset = Max(0, m_yOffset - iDelta);
			}
		else
			{
			iDelta = Min(-12, m_yOffset / 4);
			m_yOffset = Min(0, m_yOffset - iDelta);
			}

		Invalidate();
		}
	}

