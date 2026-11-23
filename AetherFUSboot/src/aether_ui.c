/*
 * AetherFUSboot: interfaz rediseñada (tema oscuro AetherBoot)
 *
 * Dibujo propio de los controles de la ventana principal:
 *   - botones (INICIAR como botón principal azul, el resto secundarios),
 *     incluido el botón dividido SELECCIONAR;
 *   - listas desplegables con esquinas redondeadas y flecha cian;
 *   - campos de texto con borde propio;
 *   - textos estáticos transparentes sobre las tarjetas;
 *   - barra de progreso redondeada con degradado azul → cian.
 *
 * Basado en Rufus: The Reliable USB Formatting Utility
 * Copyright © 2011-2025 Pete Batard <pete@akeo.ie>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <stdlib.h>
#include <string.h>

#include "rufus.h"
#include "resource.h"
#include "ui.h"
#include "darkmode.h"

#define AETHER_SUBCLASS_ID      0xAE7F
#define AETHER_KIND_BUTTON      1
#define AETHER_KIND_COMBO       2
#define AETHER_KIND_EDIT        3
#define AETHER_KIND_STATIC      4

#define AETHER_ANIM_TIMER       0xAE01
#define AETHER_ANIM_STEP        0.16f	// ~6 pasos de 15 ms: transición de unos 100 ms

typedef struct {
	int kind;
	BOOL hot;
	BOOL tracking;
	BOOL timer_on;
	float hot_t;		// 0 = reposo, 1 = ratón encima (animado)
	float focus_t;		// 0 = sin foco, 1 = con foco (animado)
} AetherCtrlData;

int AetherScale(int v)
{
	return (int)((float)v * fScale + 0.5f);
}

/* -------------------------------------------------------------------------- */
/* Utilidades de dibujo                                                       */
/* -------------------------------------------------------------------------- */

static COLORREF AetherMix(COLORREF a, COLORREF b, int num, int den)
{
	if (den <= 0)
		return a;
	return RGB(GetRValue(a) + (GetRValue(b) - GetRValue(a)) * num / den,
		GetGValue(a) + (GetGValue(b) - GetGValue(a)) * num / den,
		GetBValue(a) + (GetBValue(b) - GetBValue(a)) * num / den);
}

void AetherFillGradient(HDC hdc, const RECT* rc, COLORREF c1, COLORREF c2, BOOL horizontal)
{
	RECT r;
	HBRUSH hb;
	int i, n, step = 2;

	n = horizontal ? (rc->right - rc->left) : (rc->bottom - rc->top);
	if (n <= 0)
		return;
	for (i = 0; i < n; i += step) {
		r = *rc;
		if (horizontal) {
			r.left = rc->left + i;
			r.right = min(rc->right, r.left + step);
		} else {
			r.top = rc->top + i;
			r.bottom = min(rc->bottom, r.top + step);
		}
		hb = CreateSolidBrush(AetherMix(c1, c2, i, n));
		FillRect(hdc, &r, hb);
		DeleteObject(hb);
	}
}

void AetherFillRound(HDC hdc, const RECT* rc, int radius, COLORREF fill, COLORREF edge, BOOL has_edge)
{
	HBRUSH hb = CreateSolidBrush(fill);
	HPEN hp = CreatePen(PS_SOLID, 1, has_edge ? edge : fill);
	HBRUSH hob = (HBRUSH)SelectObject(hdc, hb);
	HPEN hop = (HPEN)SelectObject(hdc, hp);
	RoundRect(hdc, rc->left, rc->top, rc->right, rc->bottom, radius, radius);
	SelectObject(hdc, hob);
	SelectObject(hdc, hop);
	DeleteObject(hb);
	DeleteObject(hp);
}

/* Degradado recortado a un rectángulo redondeado */
static void AetherFillRoundGradient(HDC hdc, const RECT* rc, int radius, COLORREF c1, COLORREF c2, BOOL horizontal)
{
	HRGN rgn = CreateRoundRectRgn(rc->left, rc->top, rc->right + 1, rc->bottom + 1, radius, radius);
	int saved = SaveDC(hdc);
	POINT org;

	/* La región se expresa en coordenadas de dispositivo */
	GetViewportOrgEx(hdc, &org);
	OffsetRgn(rgn, org.x, org.y);
	ExtSelectClipRgn(hdc, rgn, RGN_AND);
	AetherFillGradient(hdc, rc, c1, c2, horizontal);
	RestoreDC(hdc, saved);
	DeleteObject(rgn);
}

static void AetherDrawChevron(HDC hdc, int cx, int cy, int size, COLORREF color)
{
	HPEN hp = CreatePen(PS_SOLID, max(1, AetherScale(2) - 1) + 1, color);
	HPEN hop = (HPEN)SelectObject(hdc, hp);
	POINT pts[3];
	pts[0].x = cx - size;     pts[0].y = cy - size / 2;
	pts[1].x = cx;            pts[1].y = cy + size / 2;
	pts[2].x = cx + size + 1; pts[2].y = cy - size / 2 - 1;
	Polyline(hdc, pts, 3);
	SelectObject(hdc, hop);
	DeleteObject(hp);
}

static void AetherTrackMouse(HWND hWnd, AetherCtrlData* d)
{
	TRACKMOUSEEVENT tme;
	if (d->tracking)
		return;
	tme.cbSize = sizeof(tme);
	tme.dwFlags = TME_LEAVE;
	tme.hwndTrack = hWnd;
	tme.dwHoverTime = 0;
	if (TrackMouseEvent(&tme))
		d->tracking = TRUE;
}

/* -------------------------------------------------------------------------- */
/* Botones                                                                    */
/* -------------------------------------------------------------------------- */

static HFONT AetherBoldFont(HFONT base)
{
	static HFONT cache_base[4] = { 0 }, cache_bold[4] = { 0 };
	LOGFONTW lf;
	HFONT bold;
	int i;

	if (base == NULL)
		return NULL;
	for (i = 0; i < 4; i++) {
		if (cache_base[i] == base)
			return cache_bold[i];
	}
	if (GetObjectW(base, sizeof(lf), &lf) == 0)
		return base;
	lf.lfWeight = FW_SEMIBOLD;
	bold = CreateFontIndirectW(&lf);
	if (bold == NULL)
		return base;
	for (i = 0; i < 4; i++) {
		if (cache_base[i] == NULL) {
			cache_base[i] = base;
			cache_bold[i] = bold;
			break;
		}
	}
	return bold;
}

static void AetherPaintButton(HWND hWnd, HDC hdc, AetherCtrlData* d)
{
	RECT rc, rt;
	WCHAR text[128] = { 0 };
	LRESULT state = SendMessage(hWnd, BM_GETSTATE, 0, 0);
	LONG_PTR type = GetWindowLongPtr(hWnd, GWL_STYLE) & BS_TYPEMASK;
	BOOL enabled = IsWindowEnabled(hWnd);
	BOOL primary = (GetDlgCtrlID(hWnd) == IDC_START);
	BOOL split = (type == BS_SPLITBUTTON) || (type == BS_DEFSPLITBUTTON);
	BOOL pushed = (state & BST_PUSHED) != 0;
	int radius = AetherScale(8), split_w = 0;
	int ht = (int)(d->hot_t * 100.0f), ft = (int)(d->focus_t * 100.0f);
	COLORREF fill1, fill2 = 0, edge, txt;
	HFONT hFont, hOldFont;

	GetClientRect(hWnd, &rc);
	DrawThemeParentBackground(hWnd, hdc, &rc);

	if (primary) {
		if (!enabled) {
			AetherFillRound(hdc, &rc, radius, AETHER_DISABLED_FILL_COLOR, AETHER_DISABLED_EDGE_COLOR, TRUE);
			txt = DARKMODE_DISABLED_TEXT_COLOR;
		} else {
			if (pushed) {
				fill1 = fill2 = AETHER_ELECTRIC_DARK_COLOR;
			} else {
				fill1 = AetherMix(AETHER_ELECTRIC_COLOR, AETHER_ELECTRIC_HOT_COLOR, ht, 100);
				fill2 = AetherMix(AETHER_ELECTRIC_DARK_COLOR, AETHER_ELECTRIC_COLOR, ht, 100);
			}
			AetherFillRoundGradient(hdc, &rc, radius, fill1, fill2, FALSE);
			edge = AetherMix(fill1, AETHER_CYAN_COLOR, max(ft, ht / 2), 100);
			{
				HPEN hp = CreatePen(PS_SOLID, 1, edge);
				HPEN hop = (HPEN)SelectObject(hdc, hp);
				HBRUSH hob = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
				RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);
				SelectObject(hdc, hob);
				SelectObject(hdc, hop);
				DeleteObject(hp);
			}
			txt = RGB(255, 255, 255);
		}
	} else {
		if (!enabled) {
			fill1 = AETHER_DISABLED_FILL_COLOR;
			edge = AETHER_DISABLED_EDGE_COLOR;
			txt = DARKMODE_DISABLED_TEXT_COLOR;
		} else {
			fill1 = pushed ? AETHER_CARD_COLOR : AetherMix(DARKMODE_NORMAL_CONTROL_BACKGROUND_COLOR,
				DARKMODE_HOT_CONTROL_BACKGROUND_COLOR, ht, 100);
			edge = pushed ? AETHER_CYAN_COLOR : AetherMix(DARKMODE_NORMAL_CONTROL_EDGE_COLOR, AETHER_CYAN_COLOR, max(ht, ft), 100);
			txt = AetherMix(DARKMODE_NORMAL_TEXT_COLOR, RGB(255, 255, 255), ht, 100);
		}
		AetherFillRound(hdc, &rc, radius, fill1, edge, TRUE);
	}

	/* Zona de la flecha del botón dividido */
	if (split) {
		BUTTON_SPLITINFO bsi = { 0 };
		bsi.mask = BCSIF_SIZE;
		if (SendMessage(hWnd, BCM_GETSPLITINFO, 0, (LPARAM)&bsi) && (bsi.size.cx > 0))
			split_w = bsi.size.cx + AetherScale(4);
		else
			split_w = AetherScale(18);
		{
			HPEN hp = CreatePen(PS_SOLID, 1, enabled ? DARKMODE_NORMAL_CONTROL_EDGE_COLOR : AETHER_DISABLED_EDGE_COLOR);
			HPEN hop = (HPEN)SelectObject(hdc, hp);
			MoveToEx(hdc, rc.right - split_w, rc.top + AetherScale(5), NULL);
			LineTo(hdc, rc.right - split_w, rc.bottom - AetherScale(5));
			SelectObject(hdc, hop);
			DeleteObject(hp);
		}
		AetherDrawChevron(hdc, rc.right - split_w / 2 - 1, (rc.top + rc.bottom) / 2, AetherScale(3),
			enabled ? AETHER_CYAN_COLOR : DARKMODE_DISABLED_TEXT_COLOR);
	}

	GetWindowTextW(hWnd, text, ARRAYSIZE(text));
	hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
	if (primary)
		hFont = AetherBoldFont(hFont);
	hOldFont = (HFONT)SelectObject(hdc, hFont);
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, txt);
	rt = rc;
	rt.right -= split_w;
	if (pushed)
		OffsetRect(&rt, 0, 1);
	DrawTextW(hdc, text, -1, &rt, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
	SelectObject(hdc, hOldFont);
}

/* -------------------------------------------------------------------------- */
/* Listas desplegables                                                        */
/* -------------------------------------------------------------------------- */

static void AetherPaintCombo(HWND hWnd, HDC hdc, AetherCtrlData* d)
{
	RECT rc, rt;
	WCHAR* text = NULL;
	LRESULT sel, len;
	BOOL enabled = IsWindowEnabled(hWnd);
	BOOL dropped = (BOOL)SendMessage(hWnd, CB_GETDROPPEDSTATE, 0, 0);
	BOOL focus = (GetFocus() == hWnd);
	int radius = AetherScale(8), arrow_w = AetherScale(26);
	COLORREF fill, edge, txt;
	HFONT hOldFont;

	GetClientRect(hWnd, &rc);
	DrawThemeParentBackground(hWnd, hdc, &rc);

	if (!enabled) {
		fill = AETHER_DISABLED_FILL_COLOR;
		edge = AETHER_DISABLED_EDGE_COLOR;
		txt = DARKMODE_DISABLED_TEXT_COLOR;
	} else {
		int ht = dropped ? 100 : (int)(d->hot_t * 100.0f);
		int ft = (dropped || focus) ? max(80, (int)(d->focus_t * 100.0f)) : (int)(d->focus_t * 100.0f);
		fill = AetherMix(DARKMODE_NORMAL_CONTROL_BACKGROUND_COLOR, DARKMODE_HOT_CONTROL_BACKGROUND_COLOR, ht, 100);
		edge = AetherMix(AetherMix(DARKMODE_NORMAL_CONTROL_EDGE_COLOR, RGB(0x3B, 0x6F, 0xD8), ht, 100), AETHER_CYAN_COLOR, ft, 100);
		txt = DARKMODE_NORMAL_TEXT_COLOR;
	}
	AetherFillRound(hdc, &rc, radius, fill, edge, TRUE);

	sel = SendMessage(hWnd, CB_GETCURSEL, 0, 0);
	if (sel != CB_ERR) {
		len = SendMessageW(hWnd, CB_GETLBTEXTLEN, (WPARAM)sel, 0);
		if ((len != CB_ERR) && (len >= 0) && (len < 4096)) {
			text = (WCHAR*)calloc((size_t)len + 1, sizeof(WCHAR));
			if (text != NULL)
				SendMessageW(hWnd, CB_GETLBTEXT, (WPARAM)sel, (LPARAM)text);
		}
	}
	if (text != NULL) {
		hOldFont = (HFONT)SelectObject(hdc, (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0));
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, txt);
		rt = rc;
		rt.left += AetherScale(9);
		rt.right -= arrow_w;
		DrawTextW(hdc, text, -1, &rt, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
		SelectObject(hdc, hOldFont);
		free(text);
	}
	AetherDrawChevron(hdc, rc.right - arrow_w / 2 - 1, (rc.top + rc.bottom) / 2, AetherScale(3),
		enabled ? AETHER_CYAN_COLOR : DARKMODE_DISABLED_TEXT_COLOR);
}

/* -------------------------------------------------------------------------- */
/* Campos de texto (borde no cliente)                                         */
/* -------------------------------------------------------------------------- */

static void AetherPaintEditFrame(HWND hWnd, AetherCtrlData* d)
{
	RECT rc;
	HDC hdc;
	HBRUSH hb;
	LONG_PTR ex = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
	LONG_PTR st = GetWindowLongPtr(hWnd, GWL_STYLE);
	BOOL enabled = IsWindowEnabled(hWnd) && !(st & ES_READONLY);
	BOOL focus = (GetFocus() == hWnd);
	COLORREF edge, inner;

	if (!(st & WS_BORDER) && !(ex & WS_EX_CLIENTEDGE))
		return;
	(void)focus;
	edge = !enabled ? AETHER_DISABLED_EDGE_COLOR : AetherMix(AetherMix(DARKMODE_NORMAL_CONTROL_EDGE_COLOR,
		RGB(0x3B, 0x6F, 0xD8), (int)(d->hot_t * 100.0f), 100), AETHER_CYAN_COLOR, (int)(d->focus_t * 100.0f), 100);
	inner = enabled ? DARKMODE_NORMAL_CONTROL_BACKGROUND_COLOR : AETHER_DISABLED_FILL_COLOR;

	hdc = GetWindowDC(hWnd);
	if (hdc == NULL)
		return;
	GetWindowRect(hWnd, &rc);
	OffsetRect(&rc, -rc.left, -rc.top);
	hb = CreateSolidBrush(edge);
	FrameRect(hdc, &rc, hb);
	DeleteObject(hb);
	if (ex & WS_EX_CLIENTEDGE) {
		InflateRect(&rc, -1, -1);
		hb = CreateSolidBrush(inner);
		FrameRect(hdc, &rc, hb);
		DeleteObject(hb);
	}
	ReleaseDC(hWnd, hdc);
}

/* -------------------------------------------------------------------------- */
/* Subclase común                                                             */
/* -------------------------------------------------------------------------- */

static void AetherRefreshNow(HWND hWnd)
{
	InvalidateRect(hWnd, NULL, FALSE);
	UpdateWindow(hWnd);
}

static void AetherRedrawUnder(HWND hWnd)
{
	RECT rc;
	HWND hParent = GetParent(hWnd);
	GetWindowRect(hWnd, &rc);
	MapWindowPoints(NULL, hParent, (POINT*)&rc, 2);
	RedrawWindow(hParent, &rc, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

/* Animación de transiciones (ratón encima / foco): ~100 ms */
static void AetherAnimStart(HWND hWnd, AetherCtrlData* d)
{
	if (!d->timer_on) {
		if (SetTimer(hWnd, AETHER_ANIM_TIMER, 15, NULL))
			d->timer_on = TRUE;
	}
}

static float AetherApproach(float v, float target)
{
	if (v < target)
		return min(target, v + AETHER_ANIM_STEP);
	if (v > target)
		return max(target, v - AETHER_ANIM_STEP);
	return v;
}

/* Avanza un paso. Devuelve TRUE si la animación debe continuar. */
static BOOL AetherAnimStep(HWND hWnd, AetherCtrlData* d)
{
	float th = d->hot ? 1.0f : 0.0f;
	float tf = (GetFocus() == hWnd) ? 1.0f : 0.0f;
	d->hot_t = AetherApproach(d->hot_t, th);
	d->focus_t = AetherApproach(d->focus_t, tf);
	return (d->hot_t != th) || (d->focus_t != tf);
}

static LRESULT CALLBACK AetherCtrlSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	AetherCtrlData* d = (AetherCtrlData*)dwRefData;
	LRESULT lr;
	PAINTSTRUCT ps;

	switch (uMsg) {
	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, AetherCtrlSubclass, uIdSubclass);
		free(d);
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);

	case WM_MOUSEMOVE:
		if ((d->kind != AETHER_KIND_STATIC) && !d->hot) {
			d->hot = TRUE;
			AetherTrackMouse(hWnd, d);
			AetherAnimStart(hWnd, d);
		}
		break;

	case WM_MOUSELEAVE:
		d->hot = FALSE;
		d->tracking = FALSE;
		if (d->kind != AETHER_KIND_STATIC)
			AetherAnimStart(hWnd, d);
		break;

	case WM_SETFOCUS:
	case WM_KILLFOCUS:
		if (d->kind != AETHER_KIND_STATIC)
			AetherAnimStart(hWnd, d);
		break;

	case WM_TIMER:
		if (wParam == AETHER_ANIM_TIMER) {
			if (!AetherAnimStep(hWnd, d)) {
				KillTimer(hWnd, AETHER_ANIM_TIMER);
				d->timer_on = FALSE;
			}
			if (d->kind == AETHER_KIND_EDIT)
				AetherPaintEditFrame(hWnd, d);
			else
				InvalidateRect(hWnd, NULL, FALSE);
			return 0;
		}
		break;

	default:
		break;
	}

	switch (d->kind) {
	case AETHER_KIND_BUTTON:
	case AETHER_KIND_COMBO:
		switch (uMsg) {
		case WM_ERASEBKGND:
			return TRUE;
		case WM_PAINT:
		case WM_PRINTCLIENT: {
			RECT rc;
			HDC hmem, target;
			HBITMAP hbm, hob;
			target = (uMsg == WM_PRINTCLIENT) ? (HDC)wParam : BeginPaint(hWnd, &ps);
			GetClientRect(hWnd, &rc);
			/* Doble búfer para evitar parpadeo */
			hmem = CreateCompatibleDC(target);
			hbm = CreateCompatibleBitmap(target, max(1, rc.right), max(1, rc.bottom));
			hob = (HBITMAP)SelectObject(hmem, hbm);
			if (d->kind == AETHER_KIND_BUTTON)
				AetherPaintButton(hWnd, hmem, d);
			else
				AetherPaintCombo(hWnd, hmem, d);
			BitBlt(target, 0, 0, rc.right, rc.bottom, hmem, 0, 0, SRCCOPY);
			SelectObject(hmem, hob);
			DeleteObject(hbm);
			DeleteDC(hmem);
			if (uMsg == WM_PAINT)
				EndPaint(hWnd, &ps);
			return 0;
		}
		case WM_ENABLE:
			lr = DefWindowProc(hWnd, uMsg, wParam, lParam);
			InvalidateRect(hWnd, NULL, FALSE);
			return lr;
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_CHAR:
		case WM_MOUSEWHEEL:
		case WM_SETTEXT:
		case WM_CAPTURECHANGED:
		case BM_SETSTATE:
		case BM_SETSTYLE:
		case BM_SETCHECK:
		case CB_SETCURSEL:
		case CB_SHOWDROPDOWN:
		case CB_RESETCONTENT:
			/* Algunos de estos mensajes hacen que el control se dibuje a sí mismo
			 * fuera de WM_PAINT; se vuelve a dibujar con el estilo propio. */
			lr = DefSubclassProc(hWnd, uMsg, wParam, lParam);
			if (IsWindow(hWnd))
				AetherRefreshNow(hWnd);
			return lr;
		default:
			break;
		}
		break;

	case AETHER_KIND_EDIT:
		switch (uMsg) {
		case WM_NCPAINT:
			lr = DefSubclassProc(hWnd, uMsg, wParam, lParam);
			AetherPaintEditFrame(hWnd, d);
			return lr;
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
		case WM_ENABLE:
		case EM_SETREADONLY:
			lr = DefSubclassProc(hWnd, uMsg, wParam, lParam);
			RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE);
			return lr;
		default:
			break;
		}
		break;

	case AETHER_KIND_STATIC:
		switch (uMsg) {
		case WM_SETTEXT:
		case WM_ENABLE:
			/* El texto es transparente: hay que repintar el fondo de debajo */
			lr = DefSubclassProc(hWnd, uMsg, wParam, lParam);
			AetherRedrawUnder(hWnd);
			return lr;
		default:
			break;
		}
		break;
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static void AetherSubclass(HWND hWnd, int kind)
{
	AetherCtrlData* d;
	if (GetWindowSubclass(hWnd, AetherCtrlSubclass, AETHER_SUBCLASS_ID, NULL))
		return;
	d = (AetherCtrlData*)calloc(1, sizeof(AetherCtrlData));
	if (d == NULL)
		return;
	d->kind = kind;
	SetWindowSubclass(hWnd, AetherCtrlSubclass, AETHER_SUBCLASS_ID, (DWORD_PTR)d);
}

/*
 * Aplica el dibujo propio a los controles directos de la ventana principal.
 * Solo en el tema oscuro: en el tema claro se conserva el aspecto de Rufus.
 */
void AetherSubclassMainControls(HWND hDlg)
{
	HWND h;
	WCHAR cls[64];
	LONG_PTR st;

	if (!is_darkmode_enabled)
		return;
	for (h = GetWindow(hDlg, GW_CHILD); h != NULL; h = GetWindow(h, GW_HWNDNEXT)) {
		cls[0] = 0;
		GetClassNameW(h, cls, ARRAYSIZE(cls));
		st = GetWindowLongPtr(h, GWL_STYLE);
		if (_wcsicmp(cls, WC_BUTTONW) == 0) {
			switch (st & BS_TYPEMASK) {
			case BS_PUSHBUTTON:
			case BS_DEFPUSHBUTTON:
			case BS_SPLITBUTTON:
			case BS_DEFSPLITBUTTON:
				AetherSubclass(h, AETHER_KIND_BUTTON);
				break;
			default:
				break;
			}
		} else if (_wcsicmp(cls, WC_COMBOBOXW) == 0) {
			if ((st & 0x3) == CBS_DROPDOWNLIST)
				AetherSubclass(h, AETHER_KIND_COMBO);
		} else if (_wcsicmp(cls, WC_EDITW) == 0) {
			if (!(st & ES_MULTILINE)) {
				AetherSubclass(h, AETHER_KIND_EDIT);
				SetWindowPos(h, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			}
		} else if (_wcsicmp(cls, WC_STATICW) == 0) {
			AetherSubclass(h, AETHER_KIND_STATIC);
		}
	}
}

/* -------------------------------------------------------------------------- */
/* Barra de progreso                                                          */
/* -------------------------------------------------------------------------- */

static void AetherFillSegment(HDC hdc, const RECT* track, int x1, int x2, int radius, COLORREF state_color, BOOL normal)
{
	RECT seg;
	HRGN rgn;
	int saved;
	POINT org;

	if (x2 <= x1)
		return;
	seg = *track;
	seg.left = max(track->left, x1);
	seg.right = min(track->right, x2);
	if (seg.right <= seg.left)
		return;
	saved = SaveDC(hdc);
	GetViewportOrgEx(hdc, &org);
	rgn = CreateRoundRectRgn(track->left, track->top, track->right + 1, track->bottom + 1, radius, radius);
	OffsetRgn(rgn, org.x, org.y);
	ExtSelectClipRgn(hdc, rgn, RGN_AND);
	IntersectClipRect(hdc, seg.left, seg.top, seg.right, seg.bottom);
	if (normal)
		AetherFillGradient(hdc, track, AETHER_ELECTRIC_COLOR, AETHER_CYAN_COLOR, TRUE);
	else {
		HBRUSH hb = CreateSolidBrush(state_color);
		FillRect(hdc, track, hb);
		DeleteObject(hb);
	}
	RestoreDC(hdc, saved);
	DeleteObject(rgn);
}

/*
 * Dibuja la barra de progreso. 'segments' son pares [inicio, fin) en
 * proporción 0..1 (hasta dos segmentos, para el modo marquesina).
 */
void AetherPaintProgress(HWND hCtrl, HDC target, const float* segments, int nsegments,
	COLORREF state_color, const wchar_t* text, HFONT hFont, float sheen)
{
	RECT rc, track;
	HDC hdc;
	HBITMAP hbm, hob;
	HFONT hof = NULL;
	int i, w, radius;
	BOOL normal = (state_color == PROGRESS_BAR_NORMAL_COLOR);

	GetClientRect(hCtrl, &rc);
	hdc = CreateCompatibleDC(target);
	hbm = CreateCompatibleBitmap(target, max(1, rc.right), max(1, rc.bottom));
	hob = (HBITMAP)SelectObject(hdc, hbm);

	DrawThemeParentBackground(hCtrl, hdc, &rc);
	radius = min(rc.bottom - rc.top, AetherScale(14));
	track = rc;
	AetherFillRound(hdc, &track, radius, AETHER_TRACK_COLOR, AETHER_CARD_EDGE_COLOR, TRUE);
	w = track.right - track.left;
	for (i = 0; i < nsegments; i++)
		AetherFillSegment(hdc, &track, track.left + (int)(segments[2 * i] * w),
			track.left + (int)(segments[2 * i + 1] * w), radius, state_color, normal);

	/* Brillo que recorre la parte llena mientras se trabaja */
	if (normal && (sheen > -1.0f) && (nsegments == 1)) {
		int fill_l = track.left + (int)(segments[0] * w), fill_r = track.left + (int)(segments[1] * w);
		int bw = max(AetherScale(40), w / 6), cx = track.left + (int)(sheen * w);
		RECT band = track;
		HRGN rgn;
		int saved = SaveDC(hdc);
		POINT org;
		GetViewportOrgEx(hdc, &org);
		rgn = CreateRoundRectRgn(track.left, track.top, track.right + 1, track.bottom + 1, radius, radius);
		OffsetRgn(rgn, org.x, org.y);
		ExtSelectClipRgn(hdc, rgn, RGN_AND);
		IntersectClipRect(hdc, fill_l, track.top, fill_r, track.bottom);
		band.left = cx - bw / 2;
		band.right = cx;
		AetherFillGradient(hdc, &band, AetherMix(AETHER_ELECTRIC_COLOR, AETHER_CYAN_COLOR, max(0, band.left - track.left), max(1, w)),
			AetherMix(AETHER_CYAN_COLOR, RGB(255, 255, 255), 45, 100), TRUE);
		band.left = cx;
		band.right = cx + bw / 2;
		AetherFillGradient(hdc, &band, AetherMix(AETHER_CYAN_COLOR, RGB(255, 255, 255), 45, 100),
			AetherMix(AETHER_ELECTRIC_COLOR, AETHER_CYAN_COLOR, min(w, max(0, band.right - track.left)), max(1, w)), TRUE);
		RestoreDC(hdc, saved);
		DeleteObject(rgn);
	}

	if (hFont != NULL)
		hof = (HFONT)SelectObject(hdc, AetherBoldFont(hFont));
	SetBkMode(hdc, TRANSPARENT);
	/* Sombra suave para que el texto se lea sobre el degradado */
	SetTextColor(hdc, RGB(0x04, 0x0A, 0x1C));
	OffsetRect(&rc, 1, 1);
	DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
	OffsetRect(&rc, -1, -1);
	SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));
	DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
	if (hof != NULL)
		SelectObject(hdc, hof);

	BitBlt(target, 0, 0, rc.right, rc.bottom, hdc, 0, 0, SRCCOPY);
	SelectObject(hdc, hob);
	DeleteObject(hbm);
	DeleteDC(hdc);
}
