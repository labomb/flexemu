/*
    fsetpdlg.cpp


    flexemu, an MC6809 emulator running FLEX
    Copyright (C) 1997-2019  W. Schwotzer

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    // Include your minimal set of headers here, or wx.h
    #include <wx/wx.h>
#endif
#include <wx/combobox.h>
#include <wx/radiobox.h>
#include <wx/treebook.h>
#include <wx/valgen.h>
#include <wx/valtext.h>

#include "misc1.h"
#include <string>
#include <memory>

#ifdef _MSC_VER
    #include <direct.h>
#endif

#include "fsetpdlg.h"
#include "fsetup.h"


static const wxChar *color_table[] =
{
    _("default"),
    _("white"),
    _("red"),
    _("green"),
    _("blue"),
    _("yellow"),
    _("magenta"),
    _("cyan"),
    _("orange"),
    _("pink"),
    _("purple"),
    _("violet"),
    _("brown"),
};

int possible_nColors[4] = { 2, 8, 64, 0 };

BEGIN_EVENT_TABLE(FlexemuOptionsDialog, wxDialog)
    EVT_BUTTON(IDC_Drive0Button, FlexemuOptionsDialog::OnSelectDrive0)
    EVT_BUTTON(IDC_Drive1Button, FlexemuOptionsDialog::OnSelectDrive1)
    EVT_BUTTON(IDC_Drive2Button, FlexemuOptionsDialog::OnSelectDrive2)
    EVT_BUTTON(IDC_Drive3Button, FlexemuOptionsDialog::OnSelectDrive3)
    EVT_BUTTON(IDC_MdcrDrive0Button, FlexemuOptionsDialog::OnSelectMdcrDrive0)
    EVT_BUTTON(IDC_MdcrDrive1Button, FlexemuOptionsDialog::OnSelectMdcrDrive1)
    EVT_BUTTON(IDC_DiskDirButton, FlexemuOptionsDialog::OnSelectDiskDir)
    EVT_BUTTON(IDC_MonitorButton, FlexemuOptionsDialog::OnSelectMonitor)
    EVT_INIT_DIALOG(FlexemuOptionsDialog::OnInitDialog)
    EVT_RADIOBOX(IDC_RamExtension, FlexemuOptionsDialog::OnRamExtensionChanged)
    EVT_RADIOBOX(IDC_EmulatedHardware,
            FlexemuOptionsDialog::OnEmulatedHardwareChanged)
    EVT_RADIOBOX(IDC_FrequencyChoices,
            FlexemuOptionsDialog::OnFrequencyChoicesChanged)
    //  EVT_CLOSE(FlexemuOptionsDialog::OnCloseWindow)
END_EVENT_TABLE()

const int FlexemuOptionsDialog::textWidth = 250;
const int FlexemuOptionsDialog::stextWidth = 150;
const int FlexemuOptionsDialog::gap = 5;

FlexemuOptionsDialog::FlexemuOptionsDialog(
    struct sGuiOptions *pGuiOptions,
    struct sOptions *pOptions,
    wxWindow *parent,
    wxWindowID id,
    const wxString &title,
    const wxPoint &pos,
    const wxSize &size,
    long style,
    const wxString &name) :
    wxDialog(parent, id, title, pos, size, style, name),
    m_guiOptions(pGuiOptions), m_options(pOptions),
    c_color(nullptr), c_isInverse(nullptr), c_undocumented(nullptr),
    c_geometry(nullptr), c_nColors(nullptr), c_monitor(nullptr),
    c_diskDir(nullptr),
    c_ramExtension(nullptr), c_flexibleMmu(nullptr),
    c_useRtc(nullptr), c_emulatedHardware(nullptr)

{
    size_t i;

    for (i = 0; i < WXSIZEOF(c_drive); ++i)
    {
        c_drive[i] = nullptr;
    }

    for (i = 0; i < WXSIZEOF(c_mdcrDrive); ++i)
    {
        c_mdcrDrive[i] = nullptr;
    }
}

FlexemuOptionsDialog::~FlexemuOptionsDialog()
{
}

bool FlexemuOptionsDialog::TransferDataToWindow()
{
    wxString str;
    int n = 0;

    for (int x = 1; x <= MAX_PIXELSIZEX; x++)
    {
        int y;

        for (y = 1; y <= MAX_PIXELSIZEY; y++)
        {
            str.Printf(wxT("%ix%i"), WINDOWWIDTH * x, WINDOWHEIGHT * y);

            c_geometry->Append(str);

            if (m_guiOptions->pixelSizeX == x &&
                m_guiOptions->pixelSizeY == y)
            {
                c_geometry->SetSelection(n);
            }

            n++;
        }
    }

    n = 0;

    int *pInt = possible_nColors;

    while (*pInt)
    {
        str.Printf(wxT("%d"), *pInt);
        c_nColors->Append(str);

        if (m_guiOptions->nColors == *(pInt++))
        {
            c_nColors->SetSelection(n);
        }

        n++;
    }
    c_nColors->Enable(!m_options->isEurocom2V5);

    size_t i;
    wxString colorName;
    std::string bColorName;

    for (i = 0; i < WXSIZEOF(color_table); i++)
    {
        colorName = wxGetTranslation(color_table[i]);
        c_color->Append(colorName);
        wxString color(color_table[i], *wxConvCurrent);
        std::string sColorName(color.mb_str(*wxConvCurrent));

        if (!stricmp(m_guiOptions->color.c_str(), sColorName.c_str()))
        {
            c_color->SetSelection(i);
        }
    }

    c_isInverse->SetValue(m_guiOptions->isInverse != 0);

    c_undocumented->SetValue(m_options->use_undocumented);

    wxString hex_file(m_options->hex_file.c_str(), *wxConvCurrent);
    c_monitor->SetValue(hex_file);

    wxString disk_dir(m_options->disk_dir.c_str(), *wxConvCurrent);
    c_diskDir->SetValue(disk_dir);

    for (size_t x = 0; x < WXSIZEOF(c_drive); x++)
    {
        wxString driveName(m_options->drive[x].c_str(), *wxConvCurrent);
        c_drive[x]->SetValue(driveName);
        c_drive[x]->Enable(!m_options->isEurocom2V5);
    }

    for (size_t x = 0; x < WXSIZEOF(c_mdcrDrive); x++)
    {
        wxString driveName(m_options->mdcrDrives[x].c_str(), *wxConvCurrent);
        c_mdcrDrive[x]->SetValue(driveName);
        c_mdcrDrive[x]->Enable(m_options->isEurocom2V5);
    }

    int selection = 0;

    if (m_options->isRamExtension)
    {
        selection = 1;

        if (m_options->isHiMem)
        {
            selection = 2;
        }
    }

    c_ramExtension->SetSelection(selection);
    if (selection == 0)
    {
        c_nColors->SetSelection(0);
    }
    c_nColors->Enable(selection > 0);
    c_ramExtension->Enable(!m_options->isEurocom2V5);

    c_flexibleMmu->SetValue(m_options->isHiMem && m_options->isFlexibleMmu);
    c_flexibleMmu->Enable(m_options->isHiMem && !m_options->isEurocom2V5);

    c_useRtc->SetValue(m_options->useRtc);
    c_useRtc->Enable(!m_options->isEurocom2V5);

    c_emulatedHardware->SetSelection(m_options->isEurocom2V5 ? 0 : 1);

    if (m_options->frequency < 0.0f)
    {
        c_frequencyChoices->SetSelection(0);
        c_frequency->Enable(false);
        c_frequency->SetValue(std::to_string(ORIGINAL_FREQUENCY));
    }
    else
    if (m_options->frequency == 0.0f)
    {
        c_frequencyChoices->SetSelection(1);
        c_frequency->Enable(false);
    }
    else
    {
        c_frequencyChoices->SetSelection(2);
        c_frequency->SetValue(std::to_string(m_options->frequency));
    }

    return wxWindow::TransferDataToWindow();
}

wxPanel *FlexemuOptionsDialog::CreateEmulatedHardwareOptionsPage(
        wxBookCtrlBase *parent)
{
    wxPanel *panel = new wxPanel(parent);
    wxBoxSizer *pPanelSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *pFrequencySizer;
    wxStaticText *pStatic;
    wxString hardwareChoices[2];
    wxString frequencyChoices[3];

    hardwareChoices[0] = _("Eurocom II V5 (Philips Mini DCR)");
    hardwareChoices[1] = _("Eurocom II V7 (Floppy Disk)");
    c_emulatedHardware = new wxRadioBox(panel, IDC_EmulatedHardware,
                                    _("Emulated Hardware"), wxDefaultPosition,
                                    wxDefaultSize,
                                    WXSIZEOF(hardwareChoices), hardwareChoices,
                                    1, wxRA_SPECIFY_COLS);
    pPanelSizer->Add(c_emulatedHardware, 0, wxTOP | wxLEFT, gap);
    frequencyChoices[0] = _("Use original frequency");
    frequencyChoices[1] = _("Emulate as fast as possible");
    frequencyChoices[2] = _("Set frequency:");
    c_frequencyChoices = new wxRadioBox(panel, IDC_FrequencyChoices,
                                    _("CPU Frequency"), wxDefaultPosition,
                                    wxDefaultSize,
                                    WXSIZEOF(frequencyChoices),
                                    frequencyChoices,
                                    1, wxRA_SPECIFY_COLS);
    pPanelSizer->Add(c_frequencyChoices, 0, wxTOP | wxLEFT, gap);
    pFrequencySizer = new wxBoxSizer(wxHORIZONTAL);
    pPanelSizer->Add(pFrequencySizer, 0, wxTOP | wxLEFT, gap);
    c_frequency = new wxTextCtrl(panel, IDC_Frequency, wxT(""),
                               wxDefaultPosition, wxSize(textWidth, -1));
    pFrequencySizer->Add(c_frequency, 0, wxTOP | wxLEFT, gap);
    pStatic = new wxStaticText(panel, -1, _("MHz"),
                               wxDefaultPosition, wxSize(stextWidth, -1));
    pFrequencySizer->Add(pStatic, 0, wxTOP | wxLEFT | wxALIGN_CENTER_VERTICAL,
                         gap);

    panel->SetSizer(pPanelSizer);

    return panel;
}

wxPanel *FlexemuOptionsDialog::CreateGuiOptionsPage(wxBookCtrlBase *parent)
{
    wxPanel *panel = new wxPanel(parent);
    wxStaticText *pStatic;
    wxBoxSizer *pPanelSizer = new wxBoxSizer(wxVERTICAL);
    wxGridSizer *pGridSizer = new wxGridSizer(2);

    pStatic = new wxStaticText(panel, -1, _("Screen geometry"),
                               wxDefaultPosition, wxSize(stextWidth, -1));
    pGridSizer->Add(pStatic, 0, wxTOP | wxLEFT, gap);
    c_geometry = new wxComboBox(panel, IDC_Geometry, wxT(""),
                                wxDefaultPosition, wxDefaultSize, 0, nullptr,
                                wxCB_READONLY);
    pGridSizer->Add(c_geometry, 0, wxEXPAND);

    pStatic = new wxStaticText(panel, -1, _("Number of Colors"),
                               wxDefaultPosition, wxSize(stextWidth, -1));
    pGridSizer->Add(pStatic, 0, wxTOP | wxLEFT, gap);
    c_nColors = new wxComboBox(panel, IDC_nColors, wxT(""),
                               wxDefaultPosition, wxDefaultSize, 0, nullptr,
                               wxCB_READONLY);
    pGridSizer->Add(c_nColors, 0, wxEXPAND);

    pStatic = new wxStaticText(panel, -1, _("Color"),
                               wxDefaultPosition, wxSize(stextWidth, -1));
    pGridSizer->Add(pStatic, 0, wxTOP | wxLEFT, gap);
    c_color = new wxComboBox(panel, IDC_Color, wxT(""), wxDefaultPosition,
                             wxDefaultSize, 0, nullptr, wxCB_READONLY);
    pGridSizer->Add(c_color);
    pPanelSizer->Add(pGridSizer, 0, wxTOP, gap);

    c_isInverse = new wxCheckBox(panel, IDC_Inverse,
                                _("Display inverse colors"), wxDefaultPosition,
                                wxDefaultSize, 0);
    pPanelSizer->Add(c_isInverse, 0, wxTOP | wxLEFT, gap);

    panel->SetSizer(pPanelSizer);

    return panel;
}

wxPanel *FlexemuOptionsDialog::CreateCpuOptionsPage(wxBookCtrlBase *parent)
{
    wxPanel *panel = new wxPanel(parent);
    wxBoxSizer *pPanelSizer = new wxBoxSizer(wxVERTICAL);

    c_undocumented = new wxCheckBox(panel, IDC_Inverse,
                                    _("Undocumented MC6809 instructions"),
                                    wxDefaultPosition, wxDefaultSize, 0);
    pPanelSizer->Add(c_undocumented, 0, wxEXPAND | wxTOP | wxLEFT, gap);

    panel->SetSizer(pPanelSizer);

    return panel;
}

wxPanel *FlexemuOptionsDialog::CreateHardwareOptionsPage(wxBookCtrlBase *parent)
{
    wxPanel *panel = new wxPanel(parent);
    wxBoxSizer *pPanelSizer = new wxBoxSizer(wxVERTICAL);

    c_flexibleMmu = new wxCheckBox(panel, IDC_FlexibleMmu,
                                    _("More flexible MMU (Hardware modification)"),
                                    wxDefaultPosition, wxDefaultSize, 0);
    pPanelSizer->Add(c_flexibleMmu, 0, wxTOP | wxLEFT, gap);
    c_useRtc = new wxCheckBox(panel, IDC_UseRtc,
                              _("MC146818 Realtime clock (Hardware extension)"),
                              wxDefaultPosition, wxDefaultSize, 0);
    pPanelSizer->Add(c_useRtc, 0, wxTOP | wxLEFT, gap);

    panel->SetSizer(pPanelSizer);

    return panel;
}

wxPanel *FlexemuOptionsDialog::CreateMemoryOptionsPage(wxBookCtrlBase *parent)
{
    wxPanel *panel = new wxPanel(parent);
    wxBoxSizer *pPanelSizer = new wxBoxSizer(wxVERTICAL);
    wxString choices[3];

    choices[0] = _("None");
    choices[1] = _("2 x 96 KByte");
    choices[2] = _("2 x 288 KByte");
    c_ramExtension = new wxRadioBox(panel, IDC_RamExtension,
                                    _("RAM extension"), wxDefaultPosition,
                                    wxDefaultSize, WXSIZEOF(choices), choices,
                                    1, wxRA_SPECIFY_COLS);
    pPanelSizer->Add(c_ramExtension, 0, wxTOP | wxLEFT, gap);

    panel->SetSizer(pPanelSizer);

    return panel;
}

wxTextCtrl *FlexemuOptionsDialog::CreateFileControls(
        wxPanel *panel, wxBoxSizer *panelSizer, const wxString &text,
        int textId, int buttonId)
{
    wxBoxSizer *pBoxSizer;
    wxStaticText *pStatic;
    wxButton *pButton;
    wxTextCtrl *textCtrl;

    pBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    pStatic = new wxStaticText(panel, -1, text,
                               wxDefaultPosition, wxSize(stextWidth, -1));
    pBoxSizer->Add(pStatic, 0, wxTOP | wxLEFT, gap);
    textCtrl = new wxTextCtrl(panel, textId, wxT(""),
                              wxDefaultPosition, wxSize(textWidth, -1));
    pBoxSizer->Add(textCtrl, 1, wxEXPAND);
    pButton = new wxButton(panel, buttonId, _("..."),
                           wxDefaultPosition, wxSize(40, 25));
    pBoxSizer->Add(pButton, 0, 0);
    panelSizer->Add(pBoxSizer, 0, wxEXPAND);

    return textCtrl;
}

wxPanel *FlexemuOptionsDialog::CreatePathOptionsPage(wxBookCtrlBase *parent)
{
    wxPanel *panel = new wxPanel(parent);
    wxBoxSizer *pPanelSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *pBoxSizer;
    wxStaticText *pStatic;
    wxButton *pButton;
    wxString text;
    uint32_t i;

    pBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    pStatic = new wxStaticText(panel, -1, _("Disk/Monitor directory"),
                               wxDefaultPosition, wxSize(stextWidth, -1));
    pBoxSizer->Add(pStatic, 0, wxLEFT | wxTOP, gap);
    c_diskDir = new wxTextCtrl(panel, IDC_DiskDir, wxT(""),
                               wxDefaultPosition, wxSize(textWidth, -1));
    pBoxSizer->Add(c_diskDir, 1, wxEXPAND);
    pButton = new wxButton(panel, IDC_DiskDirButton, _("..."),
                           wxDefaultPosition, wxSize(40, 25));
    pBoxSizer->Add(pButton, 0, wxTOP);
    pPanelSizer->Add(pBoxSizer, 0, wxTOP | wxEXPAND, gap);

    pBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    pStatic = new wxStaticText(panel, -1, _("Monitor program"),
                               wxDefaultPosition, wxSize(stextWidth, -1));
    pBoxSizer->Add(pStatic, 0, wxTOP | wxLEFT, gap);
    c_monitor = new wxTextCtrl(panel, IDC_Monitor, wxT(""), wxDefaultPosition,
                               wxSize(textWidth, -1));
    pBoxSizer->Add(c_monitor, 1, wxEXPAND);
    pButton = new wxButton(panel, IDC_MonitorButton, _("..."),
                           wxDefaultPosition, wxSize(40, 25));
    pBoxSizer->Add(pButton, 0, 0);
    pPanelSizer->Add(pBoxSizer, 0, wxEXPAND);

    for (i = 0; i < WXSIZEOF(c_drive); ++i)
    {
        text.Printf(_("Disk Drive %d"), i);
        c_drive[i] = CreateFileControls(panel, pPanelSizer, text,
                                        IDC_Drive0 + i, IDC_Drive0Button + i);
    }

    for (i = 0; i < WXSIZEOF(c_mdcrDrive); ++i)
    {
        text.Printf(_("Cassette Drive %d"), i);
        c_mdcrDrive[i] = CreateFileControls(panel, pPanelSizer, text,
                                            IDC_MdcrDrive0 + i,
                                            IDC_MdcrDrive0Button + i);
    }

    panel->SetSizer(pPanelSizer);

    return panel;
}

wxPanel *FlexemuOptionsDialog::CreateExpertOptionsPage(wxBookCtrlBase *parent)
{
    wxPanel *panel = new wxPanel(parent);
    wxBoxSizer *pPanelSizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText *pStatic;

    pStatic = new wxStaticText(panel, -1,
                               _("Expert options have appropriate default "
                                 "values. They should only be changed by "
                                 "experts."),
                               wxDefaultPosition, wxSize(-1, 50),
                               wxALIGN_LEFT);
    pPanelSizer->Add(pStatic, 0, wxEXPAND | wxTOP | wxLEFT, gap);

    panel->SetSizer(pPanelSizer);

    return panel;
}

void FlexemuOptionsDialog::OnInitDialog(wxInitDialogEvent &event)
{
    wxWindow *parent = this;
    wxPanel *panel;
    wxButton *pButton;
    size_t pageId = 0;

    wxBoxSizer *pMainSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *pButtonSizer = new wxBoxSizer(wxVERTICAL);

    c_notebook = new wxTreebook(this, wxID_ANY, wxDefaultPosition,
                                wxDefaultSize, wxBK_LEFT);

    pMainSizer->Add(c_notebook, 1, wxEXPAND | wxALL, gap);

    panel = CreateEmulatedHardwareOptionsPage(c_notebook);
    c_notebook->AddPage(panel, _("Emulated Hardware"), false);
    panel = CreateGuiOptionsPage(c_notebook);
    c_notebook->AddPage(panel, _("User Interface"), false);
    panel = CreateMemoryOptionsPage(c_notebook);
    c_notebook->AddPage(panel, _("Memory"), false);
    pageId++;
    panel = CreatePathOptionsPage(c_notebook);
    c_notebook->AddPage(panel, _("Files and Directories"), true);
    pageId++;
    panel = CreateExpertOptionsPage(c_notebook);
    c_notebook->AddPage(panel, _("Expert Options"), false);
    pageId++;
    panel = CreateCpuOptionsPage(c_notebook);
    c_notebook->AddSubPage(panel, _("CPU"), false);
    panel = CreateHardwareOptionsPage(c_notebook);
    c_notebook->AddSubPage(panel, _("Hardware"), false);

    pButton = new wxButton(parent, wxID_OK, _("&Ok"));
    pButtonSizer->Add(pButton, 0, wxTOP | wxRIGHT, gap);
    pButton = new wxButton(parent, wxID_CANCEL, _("&Cancel"));
    pButtonSizer->Add(pButton, 0, wxTOP | wxRIGHT, gap);
    pButton->SetFocus();

    pMainSizer->Add(pButtonSizer, 0, wxTOP, gap);

//    pMainSizer->Layout();

    SetSizer(pMainSizer);
    SetMinSize(wxSize(640, 260));
    //SetAutoLayout(true);
    Layout();
    Centre(wxBOTH);

    wxDialog::OnInitDialog(event); // must be last command
}

/*
void FlexemuOptionsDialog::OnCloseWindow(wxCloseEvent &event)
{
    if (!event.CanVeto())
        Destroy();
    else
        wxDialog::OnCloseWindow(event);
}
*/
bool FlexemuOptionsDialog::Validate()
{
    // doing some verification of the values
    if (c_monitor->GetValue().IsEmpty())
    {
        c_notebook->SetSelection(3);
        c_monitor->SetFocus();
        c_monitor->SetSelection(-1, -1);
        wxMessageBox(_("Monitor program must not be empty"),
                     _("FSetup Error"), wxOK | wxCENTRE | wxICON_EXCLAMATION);
        return false;
    }

    if (c_frequencyChoices->GetSelection() == 2)
    {
        std::string valueString =
            c_frequency->GetValue().mb_str(*wxConvCurrent).data();
        float value;

        try
        {
            value = std::stof(valueString);
        }
        catch (std::exception &)
        {
            c_notebook->SetSelection(0);
            c_frequency->SetFocus();
            c_frequency->SetSelection(-1, -1);
            wxMessageBox(_(
                       "CPU Frequency is not a valid floating point number."),
                       _("FSetup Error"), wxOK | wxCENTRE | wxICON_EXCLAMATION);
            return false;
        }

        if (value < 0.1)
        {
            c_notebook->SetSelection(0);
            c_frequency->SetFocus();
            c_frequency->SetSelection(-1, -1);
            wxMessageBox(_(
                       "CPU Frequency has a minimum of 0.1 MHz"),
                       _("FSetup Error"), wxOK | wxCENTRE | wxICON_EXCLAMATION);
            return false;
        }
    }

    return wxWindow::Validate();
}

bool FlexemuOptionsDialog::TransferDataFromWindow()
{
    size_t i;

    wxString geometry;
    unsigned long x = 0;
    unsigned long y = 0;

    geometry = c_geometry->GetValue();

    if (geometry.BeforeFirst(wxT('x')).ToULong(&x) &&
        geometry.AfterFirst(wxT('x')).ToULong(&y))
    {
        m_guiOptions->pixelSizeX = x / WINDOWWIDTH;
        m_guiOptions->pixelSizeY = y / WINDOWHEIGHT;
    }

    wxString nrOfColors;
    unsigned long n;

    nrOfColors = c_nColors->GetValue();

    if (nrOfColors.ToULong(&n))
    {
        m_guiOptions->nColors = n;
    }

    m_guiOptions->color =
        c_color->GetValue().mb_str(*wxConvCurrent);
    wxString colorName;

    for (i = 0; i < WXSIZEOF(color_table); i++)
    {
        colorName = wxGetTranslation(color_table[i]);

        if (!colorName.Cmp(c_color->GetValue()))
        {
            wxString color(color_table[i]);
            m_guiOptions->color =
                color.mb_str(*wxConvCurrent);
        }
    }

    m_guiOptions->isInverse = c_isInverse->GetValue();

    m_options->use_undocumented = (c_undocumented->GetValue() != 0);

    m_options->hex_file = c_monitor->GetValue().mb_str(*wxConvCurrent);

    m_options->disk_dir = c_diskDir->GetValue().mb_str(*wxConvCurrent);

    for (i = 0; i < WXSIZEOF(c_drive); i++)
    {
        m_options->drive[i] = c_drive[i]->GetValue().mb_str(*wxConvCurrent);
    }

    for (i = 0; i < WXSIZEOF(c_mdcrDrive); i++)
    {
        m_options->mdcrDrives[i] =
            c_mdcrDrive[i]->GetValue().mb_str(*wxConvCurrent);
    }

    m_options->isRamExtension = c_ramExtension->GetSelection() > 0;
    m_options->isHiMem = c_ramExtension->GetSelection() > 1;

    m_options->isFlexibleMmu =
            m_options->isHiMem & (c_flexibleMmu->GetValue() != 0);

    m_options->useRtc = (c_useRtc->GetValue() != 0);

    m_options->isEurocom2V5 = (c_emulatedHardware->GetSelection() == 0);

    switch (c_frequencyChoices->GetSelection())
    {
        case 0:
            m_options->frequency = -1.0f;
            break;

        case 1:
            m_options->frequency = 0.0f;
            break;

        case 2:
            std::string value =
                c_frequency->GetValue().mb_str(*wxConvCurrent).data();

            try
            {
                m_options->frequency = std::stof(value);
            }
            catch (std::exception &)
            {
                // This case should be prevented by Validate.
                m_options->frequency = -1.0f;
            }
            break;
    }

    if (!m_options->isRamExtension)
    {
        m_guiOptions->nColors = 2;
        m_options->isHiMem = false;
        m_options->isFlexibleMmu = false;
    }

    if (m_options->isRamExtension && !m_options->isHiMem)
    {
        m_options->isFlexibleMmu = false;
    }

    if (m_options->isEurocom2V5)
    {
        m_guiOptions->nColors = 2;
        m_options->useRtc = false;
        m_options->isRamExtension = false;
        m_options->isHiMem = false;
        m_options->isFlexibleMmu = false;
    }

    return wxWindow::TransferDataFromWindow();
}

wxString FlexemuOptionsDialog::OpenFilePrompter(
    const wxString &defautPath,
    const wxString &caption,
    const wxString &defaultExtension,
    const wxString &filter)
{
    wxString drive;

#ifdef _WIN32
    char wd[PATH_MAX];

    getcwd(wd, PATH_MAX);
#endif
    drive = wxFileSelector(
                caption,
                defautPath,
                wxT(""),
                defaultExtension,
                filter,
                wxFD_OPEN | wxFD_FILE_MUST_EXIST,
                this);
#ifdef _WIN32
    chdir(wd);
#endif
    return drive;
}

void FlexemuOptionsDialog::OnSelectDrive(wxTextCtrl &driveX, bool isDisk)
{
    wxString path;
    wxString diskDir;

    diskDir = c_diskDir->GetValue();

    if (isDisk)
    {
        path = OpenFilePrompter(
                diskDir, wxT("Select a Disk file"),wxT("*.dsk"),
            wxT("FLEX file containers (*.dsk;*.flx;*.wta)|*.dsk;*.flx;*.wta|"
                "All files (*.*)|*.*"));
    }
    else
    {
        path = OpenFilePrompter(
                diskDir, wxT("Select a MDCR file"), wxT("*.mdcr"),
            wxT("MDCR containers (*.mdcr)|*.mdcr|"
                "All files (*.*)|*.*"));
    }

    if (!diskDir.IsEmpty() &&
        (path.Find(diskDir) == 0) &&
        (path.Find(PATHSEPARATOR) >= 0))
    {
        path = path.Mid(diskDir.Len(), path.Len() - diskDir.Len());

        if (path.Find(PATHSEPARATOR) == 0)
        {
            path = path.Mid(1, path.length() - 1);
        }
    }

    if (!path.IsEmpty())
    {
        driveX.SetValue(path);
    }
}

void FlexemuOptionsDialog::OnSelectDrive0(wxCommandEvent &WXUNUSED(event))
{
    OnSelectDrive(*c_drive[0], true);
}

void FlexemuOptionsDialog::OnSelectDrive1(wxCommandEvent &WXUNUSED(event))
{
    OnSelectDrive(*c_drive[1], true);
}

void FlexemuOptionsDialog::OnSelectDrive2(wxCommandEvent &WXUNUSED(event))
{
    OnSelectDrive(*c_drive[2], true);
}

void FlexemuOptionsDialog::OnSelectDrive3(wxCommandEvent &WXUNUSED(event))
{
    OnSelectDrive(*c_drive[3], true);
}

void FlexemuOptionsDialog::OnSelectMdcrDrive0(wxCommandEvent &WXUNUSED(event))
{
    OnSelectDrive(*c_mdcrDrive[0], false);
}

void FlexemuOptionsDialog::OnSelectMdcrDrive1(wxCommandEvent &WXUNUSED(event))
{
    OnSelectDrive(*c_mdcrDrive[1], false);
}

void FlexemuOptionsDialog::OnSelectDiskDir(wxCommandEvent &WXUNUSED(event))
{
    std::unique_ptr<wxDirDialog> dialog;

    m_options->disk_dir = c_diskDir->GetValue().mb_str(*wxConvCurrent);

    wxString disk_dir(m_options->disk_dir.c_str(), *wxConvCurrent);

    dialog = std::unique_ptr<wxDirDialog>(
            new wxDirDialog(this, _("Select folder with DSK files"), disk_dir));

    if (dialog->ShowModal() == wxID_OK)
    {
        m_options->disk_dir = dialog->GetPath().mb_str(*wxConvCurrent);

        c_diskDir->SetValue(dialog->GetPath());
    }
}

void FlexemuOptionsDialog::OnSelectMonitor(wxCommandEvent &WXUNUSED(event))
{
    wxString path;
    wxString diskDir;

    diskDir = c_diskDir->GetValue();

    path = OpenFilePrompter(diskDir, _("Select a monitor program"), wxT("*.hex"),
        wxT("Intel HEX files (*.hex)|*.hex|"
            "Motorola S-Record files (*.s19;*.srec;*.mot)|*.s19;*.srec;*.mot|"
            "FLEX binary files (*.cmd;*.bin)|*.cmd;*.bin|"
            "All files (*.*)|*.*"));

    if (!diskDir.IsEmpty() &&
        (path.Find(diskDir) == 0) &&
        (path.Find(PATHSEPARATOR) >= 0))
    {
        path = path.Mid(diskDir.Len(), path.Len() - diskDir.Len());

        if (path.Find(PATHSEPARATOR) == 0)
        {
            path = path.Mid(1, path.Len() - 1);
        }
    }

    if (!path.IsEmpty())
    {
        c_monitor->SetValue(path);
    }
}

void FlexemuOptionsDialog::OnRamExtensionChanged(
        wxCommandEvent &WXUNUSED(event))
{
    c_nColors->Enable(c_ramExtension->GetSelection() > 0);
    c_flexibleMmu->Enable(c_ramExtension->GetSelection() > 1);
}

void FlexemuOptionsDialog::OnEmulatedHardwareChanged(
        wxCommandEvent &WXUNUSED(event))
{
    bool isEurocom2V7 = c_emulatedHardware->GetSelection() != 0;
    size_t x;

    c_nColors->Enable(isEurocom2V7);
    c_ramExtension->Enable(isEurocom2V7);
    c_flexibleMmu->Enable(isEurocom2V7);
    c_useRtc->Enable(isEurocom2V7);

    for (x = 0; x < WXSIZEOF(c_drive); x++)
    {
        c_drive[x]->Enable(isEurocom2V7);
    }

    for (x = 0; x < WXSIZEOF(c_mdcrDrive); x++)
    {
        c_mdcrDrive[x]->Enable(!isEurocom2V7);
    }
}

void FlexemuOptionsDialog::OnFrequencyChoicesChanged(
        wxCommandEvent &WXUNUSED(event))
{
    bool isEnableFrequency = (c_frequencyChoices->GetSelection() == 2);

    c_frequency->Enable(isEnableFrequency);


    if (c_frequencyChoices->GetSelection() == 0)
    {
        c_frequency->SetValue(std::to_string(ORIGINAL_FREQUENCY));
    }
    else if (c_frequencyChoices->GetSelection() == 1)
    {
        c_frequency->SetValue("");
    }
}

