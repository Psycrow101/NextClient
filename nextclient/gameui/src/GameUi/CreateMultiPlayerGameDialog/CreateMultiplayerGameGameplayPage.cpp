//========= Copyright ?1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#include <cstdio>
#include <ctime>

#include "CreateMultiplayerGameGameplayPage.h"

#include <KeyValues.h>
#include <vgui/ILocalize.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/TextEntry.h>

#include "FileSystem.h"
#include "PanelListPanel.h"
#include "ScriptObject.h"
//#include <tier0/vcrmode.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//extern void UTIL_StripInvalidCharacters( char *pszInput );

//-----------------------------------------------------------------------------
// Purpose: class for loading/saving server config file
//-----------------------------------------------------------------------------
class CServerDescription : public CDescription
{
public:
    CServerDescription( CPanelListPanel *panel );

    void WriteScriptHeader( FileHandle_t fp );
    void WriteFileHeader( FileHandle_t fp );
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CCreateMultiplayerGameGameplayPage::CCreateMultiplayerGameGameplayPage(vgui2::Panel *parent, const char *name) : PropertyPage(parent, name)
{
    m_pOptionsList = new CPanelListPanel(this, "GameOptions");

    m_pDescription = new CServerDescription(m_pOptionsList);
    m_pDescription->InitFromFile( "settings.scr" );
    //m_pDescription->TransferCurrentValues( NULL );
    m_pList = NULL;

    LoadControlSettings("Resource/CreateMultiplayerGameGameplayPage.res");

    LoadGameOptionsList();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CCreateMultiplayerGameGameplayPage::~CCreateMultiplayerGameGameplayPage()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CCreateMultiplayerGameGameplayPage::GetMaxPlayers()
{
    return atoi(GetValue("maxplayers", "32"));
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CCreateMultiplayerGameGameplayPage::GetPassword()
{
    return GetValue("sv_password", "");
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CCreateMultiplayerGameGameplayPage::GetHostName()
{
    return GetValue("hostname", "Half-Life");
}

void CCreateMultiplayerGameGameplayPage::ApplySettings()
{
    if (m_pDescription)
        m_pDescription->WriteToConfig();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CCreateMultiplayerGameGameplayPage::GetValue(const char *cvarName, const char *defaultValue)
{
    for (mpcontrol_t *mp = m_pList; mp != NULL; mp = mp->next)
    {
        Panel *control = mp->pControl;
        if (control && !stricmp(mp->GetName(), cvarName))
        {
            KeyValues *data = new KeyValues("GetText");
            static char buf[128];
            if (control && control->RequestInfo(data))
            {
                strncpy(buf, data->GetString("text", defaultValue), sizeof(buf) - 1);
            }
            else
            {
                // no value found, copy in default text
                strncpy(buf, defaultValue, sizeof(buf) - 1);
            }

            // ensure null termination of string
            buf[sizeof(buf) - 1] = 0;

            // free
            data->deleteThis();
            return buf;
        }

    }

    return defaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: called to get data from the page
//-----------------------------------------------------------------------------
void CCreateMultiplayerGameGameplayPage::OnApplyChanges()
{
    // Get the values from the controls
    GatherCurrentValues();

    if ( m_pDescription )
    {
        FileHandle_t fp;

        m_pDescription->WriteToConfig();

        // save out in the settings file
        fp = g_pFullFileSystem->Open( "settings.scr", "wb" );
        if ( fp )
        {
            m_pDescription->WriteToScriptFile( fp );
            g_pFullFileSystem->Close(fp);
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: Creates all the controls in the game options list
//-----------------------------------------------------------------------------
void CCreateMultiplayerGameGameplayPage::LoadGameOptionsList()
{
    // destroy any existing controls
    mpcontrol_t *p, *n;

    p = m_pList;
    while ( p )
    {
        n = p->next;
        //
        delete p->pControl;
        delete p->pPrompt;
        delete p;
        p = n;
    }

    m_pList = NULL;


    // Go through desciption creating controls
    CScriptObject *pObj;

    pObj = m_pDescription->pObjList;

    mpcontrol_t	*pCtrl;

    vgui2::CheckButton *pBox;
    vgui2::TextEntry *pEdit;
    vgui2::ComboBox *pCombo;
    CScriptListItem *pListItem;

    Panel *objParent = m_pOptionsList;

    while ( pObj )
    {
        pCtrl = new mpcontrol_t( objParent, pObj->cvarname );
        pCtrl->type = pObj->type;

        switch ( pCtrl->type )
        {
            case O_BOOL:
                pBox = new vgui2::CheckButton( pCtrl, "DescCheckButton", pObj->prompt );
                pBox->SetSelected( pObj->fdefValue != 0.0f ? true : false );

                pCtrl->pControl = (Panel *)pBox;
                break;
            case O_STRING:
            case O_NUMBER:
                pEdit = new vgui2::TextEntry( pCtrl, "DescEdit");
                pEdit->InsertString(pObj->defValue);
                pCtrl->pControl = (Panel *)pEdit;
                break;
            case O_LIST:
                pCombo = new vgui2::ComboBox( pCtrl, "DescEdit", 5, false );

                pListItem = pObj->pListItems;
                while ( pListItem )
                {
                    pCombo->AddItem(pListItem->szItemText, NULL);
                    pListItem = pListItem->pNext;
                }

                pCombo->ActivateItemByRow((int)pObj->fdefValue);

                pCtrl->pControl = (Panel *)pCombo;
                break;
            default:
                break;
        }

        if ( pCtrl->type != O_BOOL )
        {
            pCtrl->pPrompt = new vgui2::Label( pCtrl, "DescLabel", "" );
            pCtrl->pPrompt->SetContentAlignment( vgui2::Label::a_west );
            pCtrl->pPrompt->SetTextInset( 5, 0 );
            pCtrl->pPrompt->SetText( pObj->prompt );
        }

        pCtrl->pScrObj = pObj;
        pCtrl->SetSize( 100, 28 );
        //pCtrl->SetBorder( scheme()->GetBorder(1, "DepressedButtonBorder") );
        m_pOptionsList->AddItem( pCtrl );

        // Link it in
        if ( !m_pList )
        {
            m_pList = pCtrl;
            pCtrl->next = NULL;
        }
        else
        {
            mpcontrol_t *p;
            p = m_pList;
            while ( p )
            {
                if ( !p->next )
                {
                    p->next = pCtrl;
                    pCtrl->next = NULL;
                    break;
                }
                p = p->next;
            }
        }

        pObj = pObj->pNext;
    }
}

//-----------------------------------------------------------------------------
// Purpose: applies all the values in the page
//-----------------------------------------------------------------------------
void CCreateMultiplayerGameGameplayPage::GatherCurrentValues()
{
    if ( !m_pDescription )
        return;

    // OK
    vgui2::CheckButton *pBox;
    vgui2::TextEntry *pEdit;
    vgui2::ComboBox *pCombo;

    mpcontrol_t *pList;

    CScriptObject *pObj;
    CScriptListItem *pItem;

    char szValue[256];
    char strValue[256];
    wchar_t w_szStrValue[256];

    pList = m_pList;
    while ( pList )
    {
        pObj = pList->pScrObj;

        if ( !pList->pControl )
        {
            pObj->SetCurValue( pObj->defValue );
            pList = pList->next;
            continue;
        }

        switch ( pObj->type )
        {
            case O_BOOL:
                pBox = (vgui2::CheckButton *)pList->pControl;
                strcpy( szValue, pBox->IsSelected() ? "1" : "0" );
                break;
            case O_NUMBER:
                pEdit = (vgui2::TextEntry * )pList->pControl;
                pEdit->GetText( strValue, sizeof( strValue ) );
                strcpy( szValue, strValue );
                break;
            case O_STRING:
                pEdit = (vgui2::TextEntry * )pList->pControl;
                pEdit->GetText( strValue, sizeof( strValue ) );
                strcpy( szValue, strValue );
                break;
            case O_LIST:
                pCombo = (vgui2::ComboBox *)pList->pControl;
                pCombo->GetText( strValue, sizeof( strValue ) );

                pItem = pObj->pListItems;

                while ( pItem )
                {
                    wchar_t *wLocalizedString = NULL;
                    wchar_t w_szStrTemp[256];

                    // Localized string?
                    if ( pItem->szItemText[0] == '#' )
                    {
                        wLocalizedString = g_pVGuiLocalize->Find( pItem->szItemText );
                    }

                    if ( wLocalizedString )
                    {
                        // Copy the string we found into our temp array
                        wcsncpy( w_szStrTemp, wLocalizedString, sizeof( w_szStrTemp ) / sizeof( wchar_t ) );
                    }
                    else
                    {
                        // Just convert what we have to Unicode
                        g_pVGuiLocalize->ConvertANSIToUnicode( pItem->szItemText, w_szStrTemp, sizeof( w_szStrTemp ) );
                    }

                    if ( _wcsicmp( w_szStrTemp, w_szStrValue ) == 0 )
                    {
                        break;
                    }
                    pItem = pItem->pNext;
                }

                if ( pItem )
                {
                    strcpy( szValue, pItem->szValue );
                }
                else  //Couln't find index
                {
                    strcpy( szValue, pObj->defValue );
                }
                break;
        }

        // Remove double quotes and % characters
        UTIL_StripInvalidCharacters(szValue, sizeof(szValue));

        strcpy( strValue, szValue );

        pObj->SetCurValue( strValue );

        pList = pList->next;
    }
}


//-----------------------------------------------------------------------------
// Purpose: Constructor, load/save server settings object
//-----------------------------------------------------------------------------
CServerDescription::CServerDescription(CPanelListPanel *panel) : CDescription(panel)
{
    setHint( "// NOTE:  THIS FILE IS AUTOMATICALLY REGENERATED, \r\n"
             "//DO NOT EDIT THIS HEADER, YOUR COMMENTS WILL BE LOST IF YOU DO\r\n"
             "// Multiplayer options script\r\n"
             "//\r\n"
             "// Format:\r\n"
             "//  Version [float]\r\n"
             "//  Options description followed by \r\n"
             "//  Options defaults\r\n"
             "//\r\n"
             "// Option description syntax:\r\n"
             "//\r\n"
             "//  \"cvar\" { \"Prompt\" { type [ type info ] } { default } }\r\n"
             "//\r\n"
             "//  type = \r\n"
             "//   BOOL   (a yes/no toggle)\r\n"
             "//   STRING\r\n"
             "//   NUMBER\r\n"
             "//   LIST\r\n"
             "//\r\n"
             "// type info:\r\n"
             "// BOOL                 no type info\r\n"
             "// NUMBER       min max range, use -1 -1 for no limits\r\n"
             "// STRING       no type info\r\n"
             "// LIST         "" delimited list of options value pairs\r\n"
             "//\r\n"
             "//\r\n"
             "// default depends on type\r\n"
             "// BOOL is \"0\" or \"1\"\r\n"
             "// NUMBER is \"value\"\r\n"
             "// STRING is \"value\"\r\n"
             "// LIST is \"index\", where index \"0\" is the first element of the list\r\n\r\n\r\n" );

    setDescription ( "SERVER_OPTIONS" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CServerDescription::WriteScriptHeader( FileHandle_t fp )
{
    char am_pm[] = "AM";
    tm newtime;
    time_t ltime;
    time( &ltime );
    tm *pTime = localtime( &ltime );
    newtime = *pTime;

    if( newtime.tm_hour > 12 )        /* Set up extension. */
        strcpy( am_pm, "PM" );
    if( newtime.tm_hour > 12 )        /* Convert from 24-hour */
        newtime.tm_hour -= 12;    /*   to 12-hour clock.  */
    if( newtime.tm_hour == 0 )        /*Set hour to 12 if midnight. */
        newtime.tm_hour = 12;

    g_pFullFileSystem->FPrintf(fp, (char *)getHint());

// Write out the comment and Cvar Info:
    g_pFullFileSystem->FPrintf(fp, "// Half-Life Server Configuration Layout Script (stores last settings chosen, too)\r\n");
    g_pFullFileSystem->FPrintf(fp, "// File generated:  %.19s %s\r\n", asctime(&newtime), am_pm);
    g_pFullFileSystem->FPrintf(fp, "//\r\n//\r\n// Cvar\t-\tSetting\r\n\r\n");

    g_pFullFileSystem->FPrintf(fp, "VERSION %.1f\r\n\r\n", SCRIPT_VERSION);

    g_pFullFileSystem->FPrintf(fp, "DESCRIPTION SERVER_OPTIONS\r\n{\r\n");
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CServerDescription::WriteFileHeader( FileHandle_t fp )
{
    char am_pm[] = "AM";
    tm newtime;
    time_t ltime;
    time( &ltime );
    tm *pTime = localtime( &ltime );
    newtime = *pTime;

    if( newtime.tm_hour > 12 )        /* Set up extension. */
        strcpy( am_pm, "PM" );
    if( newtime.tm_hour > 12 )        /* Convert from 24-hour */
        newtime.tm_hour -= 12;    /*   to 12-hour clock.  */
    if( newtime.tm_hour == 0 )        /*Set hour to 12 if midnight. */
        newtime.tm_hour = 12;

    g_pFullFileSystem->FPrintf(fp, "// Half-Life Server Configuration Settings\r\n");
    g_pFullFileSystem->FPrintf(fp, "// DO NOT EDIT, GENERATED BY HALF-LIFE\r\n");
    g_pFullFileSystem->FPrintf(fp, "// File generated:  %.19s %s\r\n", asctime(&newtime), am_pm);
    g_pFullFileSystem->FPrintf(fp, "//\r\n//\r\n// Cvar\t-\tSetting\r\n\r\n");
}
