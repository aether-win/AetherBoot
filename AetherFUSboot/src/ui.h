/*
 * Rufus: The Reliable USB Formatting Utility
 * UI-related function calls
 * Copyright © 2018 Pete Batard <pete@akeo.ie>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <windows.h>
#include <stdint.h>
#include "resource.h"
#include "localization.h"

#pragma once

// Progress bar colors
#define PROGRESS_BAR_NORMAL_TEXT_COLOR				RGB(0x00, 0x00, 0x00)
#define PROGRESS_BAR_INVERTED_TEXT_COLOR			RGB(0xFF, 0xFF, 0xFF)
#define PROGRESS_BAR_BACKGROUND_COLOR				RGB(0xE6, 0xE6, 0xE6)
#define PROGRESS_BAR_BOX_COLOR						RGB(0xBC, 0xBC, 0xBC)
#define PROGRESS_BAR_NORMAL_COLOR					RGB(0x1E, 0x6B, 0xFF)	// AetherFUSboot: azul eléctrico
#define PROGRESS_BAR_PAUSED_COLOR					RGB(0xD9, 0x77, 0x06)
#define PROGRESS_BAR_ERROR_COLOR					RGB(0xDC, 0x26, 0x26)

// Toolbar icons main color
#define TOOLBAR_ICON_COLOR							RGB(0x1E, 0x6B, 0xFF)

// Dark Mode Colors
#define DARKMODE_TOOLBAR_ICON_COLOR					RGB(0x22, 0xD3, 0xEE)	// AetherFUSboot: cian
#define DARKMODE_NORMAL_TEXT_COLOR					RGB(0xEA, 0xF1, 0xFF)
#define DARKMODE_DISABLED_TEXT_COLOR				RGB(0x5F, 0x72, 0x99)
#define DARKMODE_NORMAL_DIALOG_BACKGROUND_COLOR		RGB(0x0A, 0x18, 0x38)	// AetherFUSboot: azul noche
#define DARKMODE_NORMAL_CONTROL_BACKGROUND_COLOR	RGB(0x14, 0x2C, 0x62)
#define DARKMODE_HOT_CONTROL_BACKGROUND_COLOR		RGB(0x1A, 0x38, 0x77)
#define DARKMODE_NORMAL_CONTROL_EDGE_COLOR			RGB(0x2A, 0x4F, 0x9E)
#define DARKMODE_HOT_CONTROL_EDGE_COLOR				RGB(0x22, 0xD3, 0xEE)

// AetherFUSboot: paleta de la interfaz rediseñada (tema oscuro)
#define AETHER_DEEP_COLOR							RGB(0x06, 0x0E, 0x24)	// cabecera, barra de estado, título
#define AETHER_HEADER_END_COLOR						RGB(0x0D, 0x22, 0x55)	// final del degradado de la cabecera
#define AETHER_CARD_COLOR							RGB(0x0F, 0x23, 0x50)	// tarjetas de sección
#define AETHER_CARD_EDGE_COLOR						RGB(0x1C, 0x3A, 0x7C)
#define AETHER_LABEL_COLOR							RGB(0xA3, 0xB8, 0xDF)	// etiquetas secundarias
#define AETHER_TITLE_COLOR							RGB(0xFF, 0xFF, 0xFF)
#define AETHER_ELECTRIC_COLOR						RGB(0x1E, 0x6B, 0xFF)
#define AETHER_ELECTRIC_HOT_COLOR					RGB(0x3B, 0x82, 0xFF)
#define AETHER_ELECTRIC_DARK_COLOR					RGB(0x14, 0x52, 0xD6)
#define AETHER_CYAN_COLOR							RGB(0x22, 0xD3, 0xEE)
#define AETHER_DISABLED_FILL_COLOR					RGB(0x11, 0x26, 0x55)
#define AETHER_DISABLED_EDGE_COLOR					RGB(0x1F, 0x3B, 0x78)
#define AETHER_TRACK_COLOR							RGB(0x0A, 0x18, 0x38)

// Toolbar default style
#define TOOLBAR_STYLE						( WS_CHILD | WS_TABSTOP | WS_VISIBLE | \
											  WS_CLIPSIBLINGS | WS_CLIPCHILDREN  | \
											  CCS_NOPARENTALIGN | CCS_NODIVIDER  | \
											  TBSTYLE_FLAT | TBSTYLE_BUTTON      | \
											  TBSTYLE_AUTOSIZE | TBSTYLE_LIST    | \
											  TBSTYLE_TOOLTIPS )

// Types of update progress we report
enum update_progress_type {
	UPT_PERCENT = 0,
	UPT_SPEED,
	UPT_ETA,
	UPT_MAX
};

// Size of the download speed history ring.
#define SPEED_HISTORY_SIZE 20

// The minimum time length of a history sample. By default, each sample is at least 150ms long,
// which means that, over the course of 20 samples, "current" download speed spans at least 3s
// into the past.
#define SPEED_SAMPLE_MIN 150

// The time after which the download starts to be considered "stalled", i.e. the current
// bandwidth is not printed and the recent download speeds are scratched.
#define STALL_START_TIME 5000

// Time between screen refreshes will not be shorter than this.
// NB: In Rufus' case, "screen" means the text overlaid on the progress bar.
#define SCREEN_REFRESH_INTERVAL 200

// Don't refresh the ETA too often to avoid jerkiness in predictions.
// This allows ETA to change approximately once per second.
#define ETA_REFRESH_INTERVAL 990

extern HWND hMultiToolbar, hSaveToolbar, hHashToolbar, hAdvancedDeviceToolbar, hAdvancedFormatToolbar;
extern HFONT hInfoFont;
extern UINT_PTR UM_LANGUAGE_MENU_MAX;
extern BOOL advanced_mode_device, advanced_mode_format, force_large_fat32, app_changed_size;
extern loc_cmd* selected_locale;
extern uint64_t persistence_size;
extern const char *sfd_name, *flash_type[BADLOCKS_PATTERN_TYPES];
extern char *short_image_path, image_option_txt[128];
extern int advanced_device_section_height, advanced_format_section_height, persistence_unit_selection;
extern int selection_default, cbw, ddw, ddbh, bh, update_progress_type;

extern void SetAccessibleName(HWND hCtrl, const char* name);
extern void SetComboEntry(HWND hDlg, int data);
extern void GetBasicControlsWidth(HWND hDlg);
extern void GetMainButtonsWidth(HWND hDlg);
extern void GetHalfDropwdownWidth(HWND hDlg);
extern void GetFullWidth(HWND hDlg);
extern void PositionMainControls(HWND hDlg);
extern void AdjustForLowDPI(HWND hDlg);
extern void SetSectionHeaders(HWND hDlg, HFONT* hFont);
extern void SetPersistencePos(uint64_t pos);
extern void SetPersistenceSize(void);
extern void TogglePersistenceControls(BOOL display);
extern void ToggleAdvancedDeviceOptions(BOOL enable);
extern void ToggleAdvancedFormatOptions(BOOL enable);
extern void ToggleImageOptions(void);
extern void CreateSmallButtons(HWND hDlg);
extern void CreateAdditionalControls(HWND hDlg);
extern void EnableControls(BOOL enable, BOOL remove_checkboxes);
extern void InitProgress(BOOL bOnlyFormat);
extern void ShowLanguageMenu(RECT rcExclude);
extern void SetPassesTooltip(void);
extern void SetBootTypeDropdownWidth(void);
extern void OnPaint(HDC hdc);
