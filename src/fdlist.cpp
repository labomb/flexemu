/*
    fdlist.cpp


    FLEXplorer, An explorer for any FLEX file or disk container
    Copyright (C) 1998-2019  W. Schwotzer

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
#include <wx/clipbrd.h>
#include <wx/strconv.h>

#include "misc1.h"

#include "fdlist.h"
#include "fdirent.h"
#include "filecont.h"
#include "flexerr.h"
#include "fdcframe.h"
#include "ffilebuf.h"
#include "fcinfo.h"
#include "fcopyman.h"
#include "fmenufac.h"
#include "ifilecnt.h"
#include "bprocess.h"
#include "cvtwchar.h"
#include <memory>
#include <algorithm>

#if defined(__WXGTK__) || defined(__WXX11__)
    #include "bitmaps/dnd_copy.xpm"
    #include "bitmaps/dnd_move.xpm"
    #include "bitmaps/dnd_none.xpm"
#endif

// define the columns indices for the wxListCtrl
const int LC_FILENAME   = 0;
const int LC_FILEDESC   = 1;
const int LC_RANDOM = 2;
const int LC_FILESIZE   = 3;
const int LC_FILEDATE   = 4;
const int LC_FILEATTR   = 5;

#ifdef WIN32
    wxString FlexDiskListCtrl::fileViewer = wxT("Notepad.exe");
#endif
#ifdef UNIX
    wxString FlexDiskListCtrl::fileViewer = wxT("xedit");
#endif

static const wxChar *fileDescription[] =
{
    wxT("BIN"), _("Binary file"),
    wxT("TXT"), _("Text file"),
    wxT("CMD"), _("Executable file"),
    wxT("BAS"), _("Basic file"),
    wxT("SYS"), _("System file"),
    wxT("BAK"), _("Backup file"),
    wxT("BAC"), _("Backup file"),
    wxT("DAT"), _("Data file"),
    nullptr
};

int CALLBACK compareFlexListItems(wxIntPtr item1, wxIntPtr item2,
                                  wxIntPtr sortData)
{
    if (!item1 || !item2)
    {
        return 0;
    }

    auto dirEntry1 = *reinterpret_cast<FlexDirEntry *>(item1);
    auto dirEntry2 = *reinterpret_cast<FlexDirEntry *>(item2);

    switch (sortData)
    {
        case LC_RANDOM:
            return (dirEntry1.IsRandom() && !dirEntry2.IsRandom()) ? -1 : 1;

        case LC_FILESIZE:
            return (dirEntry1.GetSize() < dirEntry2.GetSize()) ? -1 : 1;

        case LC_FILEDATE:
            return (dirEntry1.GetDate() < dirEntry2.GetDate()) ? -1 : 1;

        case LC_FILEATTR:
            return (dirEntry1.GetAttributesString() <
                    dirEntry2.GetAttributesString()) ?  -1 : 1;

        case LC_FILEDESC:
            return (FlexDiskListCtrl::GetFileDescription(dirEntry1) <
                    FlexDiskListCtrl::GetFileDescription(dirEntry2)) ? -1 : 1;

        case LC_FILENAME:
        default:
            return (dirEntry1.GetTotalFileName() <
                    dirEntry2.GetTotalFileName()) ? -1 : 1;
    }
}
// Event table
BEGIN_EVENT_TABLE(FlexDiskListCtrl, wxListCtrl)
#ifdef wxUSE_DRAG_AND_DROP
    #ifndef __WXMOTIF__
        EVT_LIST_BEGIN_DRAG(LIST_CTRL, FlexDiskListCtrl::OnBeginDrag)
    #endif
#endif
    EVT_LIST_KEY_DOWN(LIST_CTRL, FlexDiskListCtrl::OnListKeyDown)
    EVT_LIST_COL_CLICK(LIST_CTRL, FlexDiskListCtrl::OnColClick)
    EVT_LIST_ITEM_SELECTED(LIST_CTRL, FlexDiskListCtrl::OnSelected)
    EVT_LIST_ITEM_DESELECTED(LIST_CTRL, FlexDiskListCtrl::OnDeselected)
    EVT_LIST_ITEM_ACTIVATED(LIST_CTRL, FlexDiskListCtrl::OnActivated)
    EVT_LIST_INSERT_ITEM(LIST_CTRL, FlexDiskListCtrl::OnInsertItem)
    EVT_LIST_DELETE_ITEM(LIST_CTRL, FlexDiskListCtrl::OnDeleteItem)
    EVT_RIGHT_DOWN(FlexDiskListCtrl::OnRightMouseDown)
    EVT_LEFT_DCLICK(FlexDiskListCtrl::OnLeftMouseDClick)
    EVT_MENU(MDI_SELECTALL, FlexDiskListCtrl::OnSelectAll)
    EVT_MENU(MDI_DESELECTALL, FlexDiskListCtrl::OnDeselectAll)
    EVT_MENU(MDI_FIND, FlexDiskListCtrl::OnFind)
    EVT_MENU(MDI_COPY, FlexDiskListCtrl::OnCopy)
    EVT_MENU(MDI_PASTE, FlexDiskListCtrl::OnPaste)
    EVT_MENU(MDI_DELETE, FlexDiskListCtrl::OnDelete)
    EVT_MENU(MDI_RENAME, FlexDiskListCtrl::OnRename)
    EVT_MENU(MDI_VIEW, FlexDiskListCtrl::OnView)
    EVT_MENU(MDI_SET_WRITEPROTECT, FlexDiskListCtrl::OnSetWriteProtect)
    EVT_MENU(MDI_CLEAR_WRITEPROTECT, FlexDiskListCtrl::OnClearWriteProtect)
    EVT_MENU(MDI_SET_READPROTECT, FlexDiskListCtrl::OnSetReadProtect)
    EVT_MENU(MDI_CLEAR_READPROTECT, FlexDiskListCtrl::OnClearReadProtect)
    EVT_MENU(MDI_SET_DELETEPROTECT, FlexDiskListCtrl::OnSetDeleteProtect)
    EVT_MENU(MDI_CLEAR_DELETEPROTECT, FlexDiskListCtrl::OnClearDeleteProtect)
    EVT_MENU(MDI_SET_CATALOGPROTECT, FlexDiskListCtrl::OnSetCatalogProtect)
    EVT_MENU(MDI_CLEAR_CATALOGPROTECT, FlexDiskListCtrl::OnClearCatalogProtect)
END_EVENT_TABLE()

FlexDiskListCtrl::FlexDiskListCtrl(wxWindow *parent, wxWindowID id,
                                   const wxPoint &pos, const wxSize &size,
                                   long style, FileContainerIf *container,
                                   const wxValidator &validator,
                                   const wxString &name) :
    wxListCtrl(parent, id, pos, size, style, validator, name),
    m_container(container), m_popupMenu(nullptr), m_totalSize(0)
{
    // create all columns
    InsertColumn(LC_FILENAME, _("Filename"),   wxLIST_FORMAT_LEFT, 120);
    InsertColumn(LC_FILEDESC, _("Filetype"),   wxLIST_FORMAT_LEFT, 120);
    InsertColumn(LC_RANDOM,   _("Random"),     wxLIST_FORMAT_RIGHT, 65);
    InsertColumn(LC_FILESIZE, _("Size"),       wxLIST_FORMAT_RIGHT, 65);
    InsertColumn(LC_FILEDATE, _("Date"),       wxLIST_FORMAT_RIGHT, 90);
    InsertColumn(LC_FILEATTR, _("Attributes"), wxLIST_FORMAT_RIGHT, 60);

    m_popupMenu.reset(FlexMenuFactory::CreateEditMenu());

    UpdateItems();

    if (m_container->IsWriteProtected())
    {
        SetBackgroundColour(*wxLIGHT_GREY);
    }
    else
    {
        SetBackgroundColour(*wxWHITE);
#ifdef wxUSE_DRAG_AND_DROP
#ifndef __WXMOTIF__
        SetDropTarget(new FlexFileDropTarget(this));
#endif
#endif
    }
}

int FlexDiskListCtrl::UpdateItems()
{
    int     index = 0;
    wxListItem  anItem;
    FileContainerIterator iter;

    BeforeDeleteAllItems();
    DeleteAllItems();

    if (!m_container)
    {
        return index;
    }

    // the new FlexDirEntry can be referenced by anItem.m_data !
    try
    {
        for (iter = m_container->begin(); iter != m_container->end(); ++iter)
        {
            auto dirEntry = new FlexDirEntry(*iter);
            wxString m_text(iter->GetTotalFileName().c_str(), *wxConvCurrent);
            anItem.m_text   = m_text;
            anItem.m_itemId = index;
            anItem.m_col    = 0;
            anItem.m_data   = reinterpret_cast<wxUIntPtr>(dirEntry);
            anItem.m_mask   = wxLIST_MASK_TEXT | wxLIST_MASK_DATA;
            InsertItem(anItem);
            UpdateItem(index++, *dirEntry);
        }
    }
    catch (FlexException &ex)
    {
        wxMessageBox(ex.what(), _("FLEXplorer Error"),
                     wxOK | wxCENTRE | wxICON_EXCLAMATION, this);
    }

    return index;
}

FlexDiskListCtrl::~FlexDiskListCtrl()
{
    BeforeDeleteAllItems();
    DeleteAllItems();

    if (m_container != nullptr)
    {
        m_container->Close();
    }
}

void FlexDiskListCtrl::UpdateItem(int item, FlexDirEntry &dirEntry)
{
    wxString filesize;

    filesize.Printf(wxT("%d"), dirEntry.GetSize());
    SetItem(item, LC_RANDOM, dirEntry.IsRandom() ? _("Yes") : wxT(""));
    SetItem(item, LC_FILESIZE, filesize);
    wxString date(dirEntry.GetDate().GetDateString(), *wxConvCurrent);
    SetItem(item, LC_FILEDATE, date);
    wxString attributes(dirEntry.GetAttributesString().c_str(), *wxConvCurrent);
    SetItem(item, LC_FILEATTR, attributes);
    SetItem(item, LC_FILEDESC, GetFileDescription(dirEntry));
}

const wxMenu *FlexDiskListCtrl::GetMenu()
{
    return m_popupMenu.get();
}

/* Get a vector of indices of each selected item  */
std::vector<long> FlexDiskListCtrl::GetSelections() const
{
    std::vector<long> items;
    auto selectionCount = GetSelectedItemCount();

    if (selectionCount > 0)
    {
        long item = -1;

        items.reserve(selectionCount);

        for (;;)
        {
            item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
            if (item == -1)
            {
                break;
            }
            else
            {
                items.push_back(item);
            }
        }
    }

    return items;
}

void FlexDiskListCtrl::DeleteSelectedItems(bool askUser /* = true */)
{
    auto items = GetSelections();

    if (m_container && !items.empty())
    {
        auto dialog = std::unique_ptr<wxMessageDialog>(new wxMessageDialog(
            this, _("Delete all selected files"),
            _("Delete Files"),
            wxYES_NO | wxICON_QUESTION | wxCENTRE));

        if (!askUser || (dialog->ShowModal() == wxID_YES))
        {
            std::reverse(std::begin(items), std::end(items));

            for (auto item : items)
            {
                auto fileName = GetItemText(item);

                try
                {
                    m_container->DeleteFile(
                        fileName.mb_str(*wxConvCurrent));
                }
                catch (FlexException &ex)
                {
                    int r = wxMessageBox(ex.what(),
                                         _("FLEXplorer Error"),
                                         wxOK | wxCANCEL | wxCENTRE,
                                         this);

                    if (r != wxOK)
                    {
                        break;
                    }

                    continue;
                }

                DeleteItem(item);
            } // for
        } // if
    } // if

    m_totalSize = 0;
    Notify();
}

void FlexDiskListCtrl::RenameSelectedItems()
{
    auto items = GetSelections();

    if (m_container && !items.empty())
    {
        wxString fName;
        wxString dialogText;
        bool do_rename = false;

        // edit the last selected item
        auto item = items[items.size() - 1];
        auto itemText = GetItemText(item);
        dialogText = _("Please input the new file name");

        do
        {
            auto dialog = std::unique_ptr<wxTextEntryDialog>(
                    new wxTextEntryDialog(this, dialogText, _("Rename file"),
                                          itemText));
            do_rename = dialog->ShowModal() == wxID_OK;
            fName  = dialog->GetValue();
            dialogText =  _("Wrong file name specified.");
            dialogText += wxT("\n");
            dialogText += _("Please input the new file name");
        }
        while (do_rename &&
               !m_container->CheckFilename(fName.mb_str(*wxConvCurrent)));

        if (do_rename)
        {
            try
            {
                fName.MakeUpper();
                m_container->RenameFile(
                    itemText.mb_str(*wxConvCurrent),
                    fName.mb_str(*wxConvCurrent));
                auto dirEntry =
                    *reinterpret_cast<FlexDirEntry *>(GetItemData(item));
                dirEntry.SetTotalFileName(fName.mb_str(*wxConvCurrent));
                SetItemText(item, fName);
                UpdateItem(item, dirEntry);
            }
            catch (FlexException &ex)
            {
                wxMessageBox(ex.what(), _("FLEXplorer Error"),
                             wxOK | wxCENTRE | wxICON_EXCLAMATION);
            }
        }
    } // if
}

void FlexDiskListCtrl::ViewSelectedItems()
{
    BProcess    process(fileViewer.mb_str(*wxConvCurrent).data(), ".");

    auto items = GetSelections();

    if (m_container && !items.empty())
    {
        // view the last selected item
        auto item = items[items.size() - 1];
        auto fileName = GetItemText(item);

        try
        {
            auto buffer = m_container->ReadToBuffer(
                              fileName.mb_str(*wxConvCurrent));

            if ((m_container->GetContainerType() & TYPE_CONTAINER) &&
                buffer.IsFlexTextFile())
            {
                buffer.ConvertFromFlex();
            }

#ifdef WIN32
            std::string path;
#ifdef UNICODE
            wchar_t wpath[MAX_PATH], tempPath[MAX_PATH];

            if (!GetTempPath(MAX_PATH, tempPath))
            {
                throw FlexException(GetLastError(),
                    std::string("In function GetTempPath"));
            }

            if (!GetTempFileName(tempPath, _("FLX"), 0, wpath))
            {
                throw FlexException(GetLastError(),
                    std::string("In function GetTempFileName"));
            }
            path = ConvertToUtf8String(wpath);
#else
            char cpath[MAX_PATH], tempPath[MAX_PATH];

            if (!GetTempPath(MAX_PATH, tempPath))
            {
                throw FlexException(GetLastError(),
                    std::string("In function GetTempPath"));
            }

            if (!GetTempFileName(tempPath, _("FLX"), 0, cpath))
            {
                throw FlexException(GetLastError(),
                    std::string("In function GetTempFileName"));
            }
            path = cpath;
#endif

            if (buffer.WriteToFile(path.c_str()))
            {
                process.AddArgument(path.c_str());

                if (!process.Start())
                {
                    throw FlexException(GetLastError(),
                        std::string(fileViewer));
                }
            }
            else
            {
                throw FlexException(FERR_CREATE_TEMP_FILE, path);
            }

#else
            int fd;
            char cTemplate[PATH_MAX];

            strcpy(cTemplate, "/tmp");
            strcat(cTemplate, "/FLXXXXXXX");
            fd = mkstemp(cTemplate);

            if (fd != -1 && buffer.WriteToFile(fd))
            {
                close(fd);
                process.AddArgument(cTemplate);

                if (!process.Start())
                {
                    throw FlexException(
                        FERR_CREATE_PROCESS,
                        fileViewer.mb_str(*wxConvCurrent).data(),
                        cTemplate);
                }
            }
            else
            {
                throw FlexException(FERR_CREATE_TEMP_FILE, cTemplate);
            }
#endif
        }
        catch (FlexException &ex)
        {
            wxMessageBox(ex.what(), _("FLEXplorer Error"),
                         wxOK | wxCENTRE | wxICON_EXCLAMATION);
        }
    } // if
}

#ifdef wxUSE_DRAG_AND_DROP
#ifndef __WXMOTIF__
void FlexDiskListCtrl::OnBeginDrag(wxListEvent &event)
{
    if (m_container != nullptr)
    {
        wxDragResult    result;
        int     flags = 0;
        FlexDnDFiles    files;
        int count = 0;

        for (auto fileName : GetSelectedFileNames())
        {
            try
            {
                files.Add(m_container->ReadToBuffer(fileName.c_str()));
            }
            catch (FlexException &ex)
            {
                int r = wxMessageBox(ex.what(),
                                     _("FLEXPlorer Error"),
                                     wxOK | wxCANCEL | wxCENTRE, this);

                if (r == wxCANCEL)
                {
                    return;
                }
            }

            count++;
        }

        auto dragData =
                 std::unique_ptr<FlexFileDataObject>(new FlexFileDataObject);
        dragData->ReadDataFrom(files);
        wxDropSource dragSource(this,
                                wxDROP_ICON(dnd_copy),
                                wxDROP_ICON(dnd_move),
                                wxDROP_ICON(dnd_none));
        dragSource.SetData(*dragData);
        flags = wxDrag_CopyOnly;
#ifdef __WXMSW__
        // seems to be a bug in wxMSW that
        // the move flag has to be set otherwise
        // a Drop (copy or move) is not possible
        flags |= wxDrag_AllowMove;
#endif
        //if (!m_container->IsWriteProtected())
        //  flags |= wxDrag_AllowMove;
        // When allowing to move files it must be
        // save to either copy all of them or
        // abort the Drag&Drop with an error
        result = dragSource.DoDragDrop(flags);

        switch (result)
        {
            case wxDragMove:
                break;

            // unfinished file moved, source should be deleted;
            case wxDragCopy:
                break;

            case wxDragCancel:
                break;

            case wxDragError:
                break;

            case wxDragLink:
                break;

            case wxDragNone:
                break;
        }
    }
    else
    {
        event.Skip();
    }
}
#endif
#endif

void FlexDiskListCtrl::OnSelected(wxListEvent &event)
{
    FlexDirEntry dirEntry;

    if (m_container && m_container->FindFile(
            GetItemText(event.m_itemIndex).mb_str(*wxConvCurrent), dirEntry))
    {
        m_totalSize += dirEntry.GetSize();
    }

    Notify();
}

void FlexDiskListCtrl::OnDeselected(wxListEvent &event)
{
    FlexDirEntry dirEntry;

    if (m_container && m_container->FindFile(
            GetItemText(event.m_itemIndex).mb_str(*wxConvCurrent), dirEntry))
    {
        m_totalSize -= dirEntry.GetSize();
    }

    Notify();
}

void FlexDiskListCtrl::Notify()
{
    if (m_statusbarObserver != nullptr)
    {
        int id = OBSERVE_STATUS_BAR;

        m_statusbarObserver->UpdateFrom(&id);
    }
}

void FlexDiskListCtrl::OnActivated(wxListEvent &WXUNUSED(event))
{
    Notify();
}

void FlexDiskListCtrl::OnColClick(wxListEvent &event)
{
    SortItems(compareFlexListItems, event.m_col);
}


void FlexDiskListCtrl::OnListKeyDown(wxListEvent &event)
{
    int keyCode = event.GetKeyCode();

    switch (keyCode)
    {
        // Delete-key: delete all selected files
        case WXK_DELETE:
            DeleteSelectedItems();
            break;

        case WXK_F5:
            UpdateItems();
            break;

        case wxT('C') - wxT('A') + 1 :
            CopyToClipboard();
            break;

        case wxT('V') - wxT('A') + 1 :
            PasteFromClipboard();
            break;
    } // switch
}

void FlexDiskListCtrl::SetPropertyOnSelectedItems(Byte protection,
        bool isToBeSet)
{
    if (m_container == nullptr)
    {
        return;
    }

    for (auto item : GetSelections())
    {
        FlexDirEntry dirEntry;

        auto fileName = GetItemText(item);

        try
        {
            Byte setMask = isToBeSet ? protection : 0;
            Byte clearMask = isToBeSet ? 0 : protection;
            auto newDirEntry =
                *reinterpret_cast<FlexDirEntry *>(GetItemData(item));
            m_container->SetAttributes(
                fileName.mb_str(*wxConvCurrent),
                setMask, clearMask);

            // read back properties from container
            // (because maybe not all properties are
            // supported)
            if (m_container->FindFile(
                    fileName.mb_str(*wxConvCurrent), dirEntry))
            {
                setMask = dirEntry.GetAttributes();
                clearMask = ~setMask;
            }

            newDirEntry.SetAttributes(setMask, clearMask);
            UpdateItem(item, newDirEntry);
        }
        catch (FlexException &ex)
        {
            int r = wxMessageBox(ex.what(), _("FLEXPlorer Error"),
                                 wxOK | wxCANCEL | wxCENTRE, this);

            if (r != wxOK)
            {
                break;
            }
        }
    } // for
}

void FlexDiskListCtrl::OnViewProperties(wxCommandEvent &)
{
    FlexContainerInfo   info;
    wxString        str, caption;
    int tracks, sectors;

    if (m_container)
    {
        try
        {
            m_container->GetInfo(info);
        }
        catch (FlexException &ex)
        {
            wxMessageBox(ex.what(), _("FLEXPlorer Error"),
                         wxOK | wxCENTRE, this);
            return;
        }

        caption.Printf(_("Container %s"), info.GetName());
        info.GetTrackSector(&tracks, &sectors);
        str.Append(_("Path: "));
        wxString path(info.GetPath().c_str(), *wxConvCurrent);
        str += path;
        str += wxT("\n");
        str.Append(_("Type: "));
        wxString type(info.GetTypeString().c_str(), *wxConvCurrent);
        type = wxGetTranslation(type);
        str += type;
        str += wxT("\n");
        str.Append(_("Date: "));
        wxString date(info.GetDate().GetDateString(), *wxConvCurrent);
        str += date;

        str += wxT("\n");
        str.Append(_("Tracks: "));
        str << tracks;
        str += wxT("\n");
        str.Append(_("Sectors: "));
        str << sectors;

        str += wxT("\n");
        str.Append(_("Size: "));
        str << info.GetTotalSize();
        str.Append(_(" KByte"));
        str += wxT("\n");
        str.Append(_("Free: "));
        str << info.GetFree();
        str.Append(_(" KByte"));
        str += wxT("\n");

        if (info.GetAttributes() & FLX_READONLY)
        {
            str.Append(_("Attributes: read-only"));
            str += wxT("\n");
        }

        wxMessageBox(str, caption, wxOK | wxCENTRE, this);

    }
}

void FlexDiskListCtrl::OnLeftMouseDClick(wxMouseEvent &WXUNUSED(event))
{
    ViewSelectedItems();
}

void FlexDiskListCtrl::OnRightMouseDown(wxMouseEvent &event)
{
    if (m_popupMenu)
    {
        PopupMenu(m_popupMenu.get(), event.GetX(), event.GetY());
    }
}

void FlexDiskListCtrl::OnInsertItem(wxListEvent &)
{
}

void FlexDiskListCtrl::OnDeleteItem(wxListEvent &event)
{
    // delete the FlexDirEntry * object
    // stored in m_data
    auto dirEntry = reinterpret_cast<FlexDirEntry *>(event.GetData());
    delete dirEntry;
}

void FlexDiskListCtrl::BeforeDeleteAllItems()
{
    // delete the FlexDirEntry * object
    // stored in m_data
    int item = -1;

    do
    {
        item = GetNextItem(item);

        if (item >= 0)
        {
            auto dirEntry = reinterpret_cast<FlexDirEntry *>(GetItemData(item));
            delete dirEntry;
            SetItemData(item, 0);
        }
    } while (item >= 0);
}

IMPLEMENT_MODIFY_PROPERTY(OnSetWriteProtect,     WRITE_PROTECT,   TRUE)
IMPLEMENT_MODIFY_PROPERTY(OnClearWriteProtect,   WRITE_PROTECT,   FALSE)
IMPLEMENT_MODIFY_PROPERTY(OnSetReadProtect,      READ_PROTECT,    TRUE)
IMPLEMENT_MODIFY_PROPERTY(OnClearReadProtect,    READ_PROTECT,    FALSE)
IMPLEMENT_MODIFY_PROPERTY(OnSetDeleteProtect,    DELETE_PROTECT,  TRUE)
IMPLEMENT_MODIFY_PROPERTY(OnClearDeleteProtect,  DELETE_PROTECT,  FALSE)
IMPLEMENT_MODIFY_PROPERTY(OnSetCatalogProtect,   CATALOG_PROTECT, TRUE)
IMPLEMENT_MODIFY_PROPERTY(OnClearCatalogProtect, CATALOG_PROTECT, FALSE)

IMPLEMENT_SIMPLE_MENUCOMMAND(OnSelectAll, SelectAllFiles)
IMPLEMENT_SIMPLE_MENUCOMMAND(OnDeselectAll, DeselectAllFiles)
IMPLEMENT_SIMPLE_MENUCOMMAND(OnFind, FindFiles)
IMPLEMENT_SIMPLE_MENUCOMMAND(OnCopy, CopyToClipboard)
IMPLEMENT_SIMPLE_MENUCOMMAND(OnPaste, PasteFromClipboard)
IMPLEMENT_SIMPLE_MENUCOMMAND(OnDelete, DeleteSelectedItems)
IMPLEMENT_SIMPLE_MENUCOMMAND(OnRename, RenameSelectedItems)
IMPLEMENT_SIMPLE_MENUCOMMAND(OnView, ViewSelectedItems)

FileNames FlexDiskListCtrl::GetSelectedFileNames()
{
    FileNames fileList;

    for (auto item : GetSelections())
    {
        std::string fileName(GetItemText(item).mb_str(*wxConvCurrent));
#ifdef UNIX
        std::transform(fileName.begin(), fileName.end(), fileName.begin(),
                       ::tolower);
#endif
        fileList.push_back(fileName);
    }

    return fileList;
}

void FlexDiskListCtrl::SelectAllFiles()
{
    int i;

    for (i = GetItemCount() - 1; i >= 0; i--)
    {
        SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    }
}

void FlexDiskListCtrl::DeselectAllFiles()
{
    int i;

    for (i = GetItemCount() - 1; i >= 0; i--)
    {
        SetItemState(i, 0, wxLIST_STATE_SELECTED);
    }
}

void FlexDiskListCtrl::FindFiles()
{
    static wxString value(_("*.*"));

    wxTextEntryDialog dialog(this,
                             _("Enter a pattern for files to look for."
                               "Wildcards ? and * are allowed"),
                             _("Find Files"));

    dialog.SetValue(value);
    int result = dialog.ShowModal();

    if (result == wxID_OK)
    {
        value = dialog.GetValue();
        std::string filePattern(value.mb_str(*wxConvCurrent));
        FileContainerIterator it(filePattern.c_str());
        std::string sFileName;

        DeselectAllFiles();

        for (it = m_container->begin(); it != m_container->end(); ++it)
        {
            int i;

            sFileName = (*it).GetTotalFileName();
            wxString fileName(sFileName.c_str(), *wxConvCurrent);

            if (multimatches(sFileName.c_str(), filePattern.c_str(),
                             ';', true) &&
                (i = FindItem(-1, fileName)) >= 0)
            {
                SetItemState(i, wxLIST_STATE_SELECTED,
                             wxLIST_STATE_SELECTED);
            }
        }
    }
}

void FlexDiskListCtrl::CopyToClipboard()
{
    FlexDnDFiles files;
    int count = 0;
    FlexFileDataObject *pClipboardData;

    wxClipboardLocker locker; // implicit open/close wxTheClipboard

    if (!locker)
    {
        wxLogError(_("Can't open clipboard."));
        wxBell();
        return;
    }

    for (auto fileName : GetSelectedFileNames())
    {
        try
        {
            files.Add(m_container->ReadToBuffer(fileName.c_str()));
            count++;
        }
        catch (FlexException &ex)
        {
            wxMessageBox(ex.what(), _("FLEXplorer Error"),
                         wxOK | wxCENTRE | wxICON_EXCLAMATION, this);
        }
    }

    pClipboardData = new FlexFileDataObject();
    pClipboardData->ReadDataFrom(files);

    if (!wxTheClipboard->SetData(pClipboardData))
    {
        wxLogError(_("Can't copy data to the clipboard"));
        wxBell();
    }
}

bool FlexDiskListCtrl::PasteFromClipboard()
{
    FlexFileDataObject flexFileData;

    if (m_container->IsWriteProtected())
    {
        wxLogError(_("Container is read-only"));
        return false;
    }

    wxClipboardLocker locker; // implicit open/close wxTheClipboard

    if (!locker)
    {
        wxLogError(_("Can't open clipboard."));
        wxBell();
        return false;
    }

    if (!wxTheClipboard->IsSupported(FlexFileFormatId))
    {
        wxLogWarning(_("No Flex file data on clipboard"));
        wxBell();
        return false;
    }

    if (!wxTheClipboard->GetData(flexFileData))
    {
        wxLogWarning(_("Unable to paste Flex file data from clipboard"));
        wxBell();
        return false;
    }

    FlexDnDFiles files;
    bool     result;

    flexFileData.WriteDataTo(files);

    result = PasteFrom(files);

    return result;
}

bool FlexDiskListCtrl::PasteFrom(FlexDnDFiles &files)
{
    const char *fileName;
    unsigned int index;
    FlexDirEntry *dirEntry;

    for (index = 0; index < files.GetFileCount(); ++index)
    {
        try
        {
            fileName = files.GetBufferAt(index).GetFilename();
            GetContainer()->WriteFromBuffer(files.GetBufferAt(index));
            dirEntry = new FlexDirEntry;

            if (GetContainer()->FindFile(fileName, *dirEntry))
            {
                wxListItem anItem;

                wxString m_text(dirEntry->GetTotalFileName().c_str(),
                                *wxConvCurrent);
                anItem.m_text   = m_text;
                anItem.m_itemId = 0;
                anItem.m_col    = 0;
                anItem.m_data   = reinterpret_cast<wxUIntPtr>(dirEntry);
                anItem.m_mask   = wxLIST_MASK_TEXT | wxLIST_MASK_DATA;
                auto itemIndex = InsertItem(anItem);
                UpdateItem(itemIndex, *dirEntry);
            }
            else
            {
                delete dirEntry;
            }
        }
        catch (FlexException &ex)
        {
            if (ex.GetErrorCode() == FERR_DISK_FULL_WRITING)
            {
                wxMessageBox(ex.what(), _("FLEXplorer Error"),
                             wxOK | wxCENTRE | wxICON_EXCLAMATION, this);
                return false;
            }
            else
            {
                wxMessageBox(ex.what(), _("FLEXplorer Error"),
                             wxOK | wxCENTRE, this);
            }
        }
    }

    return true;
}

wxString FlexDiskListCtrl::GetFileDescription(const FlexDirEntry &dirEntry)
{
    wxString        tmp;
    wxChar          **pFDesc;

    pFDesc = (wxChar **)fileDescription;
    wxString extension(dirEntry.GetFileExt().c_str(), *wxConvCurrent);

    while (*pFDesc != nullptr)
    {
        tmp = *pFDesc;

        if (tmp.CmpNoCase(extension) == 0)
        {
            tmp = wxGetTranslation(*(++pFDesc));
            return tmp;
        }

        pFDesc += 2;
    }

    extension += wxT(" ");
    extension += _("file");
    return extension;
}

