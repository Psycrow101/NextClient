#include "GameUi.h"
#include "GameUINext.h"
#include "BasePanel.h"

#include "vgui/IInputInternal.h"
#include "vgui/ILocalize.h"
#include "vgui/IPanel.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"
#include "vgui/IVGui.h"
#include "FileSystem.h"
#include <tier2/tier2.h>

#include "IGameUIFuncs.h"

#include "vgui_controls/AnimationController.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Menu.h"
#include "vgui_controls/MenuItem.h"
#include "vgui_controls/PHandle.h"
#include "vgui_controls/MessageBox.h"
#include "vgui_controls/QueryBox.h"
#include "vgui_controls/ControllerMap.h"
#include "vgui_controls/KeyRepeat.h"

#include "ModInfo.h"
#include "LoadingDialog.h"
#include "BackgroundMenuButton.h"
#include "CreateMultiplayerGameDialog.h"

#include "ToolBar.h"
#include "GameConsole.h"
#include "PlayerListDialog.h"

#include <keydefs.h>

#include <nitro_utils/string_utils.h>

#include <algorithm>
#include <vector>
#include <format>

// undef windows stuff
#undef PostMessage

static CBasePanel *g_pBasePanel = NULL;
static float g_flAnimationPadding = 0.01f;

CBasePanel *BasePanel(void)
{
    return g_pBasePanel;
}

vgui2::VPANEL GetGameUIBasePanel(void)
{
    return BasePanel()->GetVPanel();
}

CGameMenuItem::CGameMenuItem(vgui2::Menu *parent, const char *name) : BaseClass(parent, name, "GameMenuItem")
{
    m_bRightAligned = false;
}

void CGameMenuItem::ApplySchemeSettings(vgui2::IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);

    auto bwFgColor = GetSchemeColor("InGameDesktop/MenuColor", pScheme);
    auto bwFgDefaultColor = bwFgColor;
    auto bwFgArmedColor = GetSchemeColor("InGameDesktop/ArmedMenuColor", pScheme);
    auto bwDepressedColor = GetSchemeColor("InGameDesktop/DepressedMenuColor", pScheme);

    SetFgColor(GetSchemeColor("MainMenu.TextColor", bwFgColor, pScheme));
    SetBgColor(Color(0, 0, 0, 0));
    SetDefaultColor(GetSchemeColor("MainMenu.TextColor", bwFgDefaultColor, pScheme), Color(255, 255, 255, 255));
    SetArmedColor(GetSchemeColor("MainMenu.ArmedTextColor", bwFgArmedColor, pScheme), Color(200, 200, 200, 200));
    SetDepressedColor(GetSchemeColor("MainMenu.DepressedTextColor", bwDepressedColor, pScheme), Color(75, 75, 75, 200));
    SetContentAlignment(Label::a_west);

    SetBorder(pScheme->GetBorder("BaseBorder"));
    SetDefaultBorder(NULL);
    SetDepressedBorder(NULL);
    SetKeyFocusBorder(NULL);

    vgui2::HFont hMainMenuFont = pScheme->GetFont("MainMenuFont", IsProportional());

    if (hMainMenuFont)
        SetFont(hMainMenuFont);
    else
        SetFont(pScheme->GetFont("MenuLarge", IsProportional()));

    SetTextInset(0, 0);
    SetArmedSound("resource/UI/buttonrollover.wav");
    SetDepressedSound("resource/UI/buttonclick.wav");
    SetReleasedSound("resource/UI/buttonclickrelease.wav");
    SetButtonActivationType(Button::ACTIVATE_ONPRESSED);
    SetPaintBackgroundType(2);

    if (m_bRightAligned)
        SetContentAlignment(Label::a_east);
}

void CGameMenuItem::PaintBackground(void)
{
    BaseClass::PaintBackground();
}

void CGameMenuItem::SetRightAlignedText(bool state)
{
    m_bRightAligned = state;
}

Color CGameMenuItem::GetButtonBgColor()
{
    return GetBgColor();
}

Color CGameMenuItem::GetButtonFgColor()
{
    return BaseClass::GetButtonFgColor();
}

void CGameMenuItem::OnCursorEntered(void)
{
    BaseClass::OnCursorEntered();
    //vgui2::GetAnimationController()->RunAnimationCommand(this, "bgcolor", Color(150, 150, 150, 150), 0.0f, 0.5, vgui2::AnimationController::INTERPOLATOR_ACCEL);
}

void CGameMenuItem::OnCursorExited(void)
{
    BaseClass::OnCursorExited();
    //vgui2::GetAnimationController()->RunAnimationCommand(this, "bgcolor", Color(0, 0, 0, 0), 0.0f, 0.5, vgui2::AnimationController::INTERPOLATOR_DEACCEL);
}

class CGameMenu : public vgui2::Menu
{
    DECLARE_CLASS_SIMPLE(CGameMenu, vgui2::Menu);

public:
    CGameMenu(vgui2::Panel *parent, const char *name) : BaseClass(parent, name)
    {
        m_pConsoleFooter = NULL;
    }

    virtual void ApplySchemeSettings(vgui2::IScheme *pScheme)
    {
        BaseClass::ApplySchemeSettings(pScheme);

        auto strMenuItemHeight = pScheme->GetResourceString("MainMenu.MenuItemHeight");
        if(!Q_strlen(strMenuItemHeight))
            strMenuItemHeight = pScheme->GetResourceString("InGameDesktop/MenuItemHeight");

        if(Q_strlen(strMenuItemHeight))
            SetMenuItemHeight(atoi(strMenuItemHeight));

        SetBgColor(Color(0, 0, 0, 0));
        SetBorder(NULL);
        SetPaintBackgroundEnabled(false);
    }

    virtual void LayoutMenuBorder(void)
    {
    }

    virtual void SetVisible(bool state)
    {
        BaseClass::SetVisible(true);

        if (!state)
            vgui2::ipanel()->MoveToBack(GetVPanel());
    }

    virtual int AddMenuItem(const char *itemName, const char *itemText, const char *command, Panel *target, KeyValues *userData = NULL)
    {
        vgui2::MenuItem *item = new CGameMenuItem(this, itemName);
        item->AddActionSignalTarget(target);
        item->SetCommand(command);
        item->SetText(itemText);
        item->SetUserData(userData);

        return BaseClass::AddMenuItem(item);
    }

    virtual void SetMenuItemBlinkingState(const char *itemName, bool state)
    {
        for (int i = 0; i < GetChildCount(); i++)
        {
            Panel *child = GetChild(i);
            vgui2::MenuItem *menuItem = dynamic_cast<vgui2::MenuItem *>(child);

            if (menuItem)
            {
                if (Q_strcmp(menuItem->GetCommand()->GetString("command", ""), itemName) == 0)
                    menuItem->SetBlink(state);
            }
        }

        InvalidateLayout();
    }

    virtual void OnCommand(const char *command)
    {
        m_KeyRepeat.Reset();

        if (!stricmp(command, "Open"))
        {
            MoveToFront();
            RequestFocus();
        }
        else
            BaseClass::OnCommand(command);
    }

    virtual void OnKeyCodePressed(vgui2::KeyCode code)
    {
        m_KeyRepeat.KeyDown(code);
        BaseClass::OnKeyCodePressed(code);
    }

    void OnKeyCodeReleased(vgui2::KeyCode code)
    {
        m_KeyRepeat.KeyUp(code);
        BaseClass::OnKeyCodeReleased(code);
    }

    void OnThink(void)
    {
        vgui2::KeyCode code = m_KeyRepeat.KeyRepeated();

        if (code)
            OnKeyCodeTyped(code);

        BaseClass::OnThink();
    }

    virtual void OnKillFocus(void)
    {
        BaseClass::OnKillFocus();

        vgui2::surface()->MovePopupToBack(GetVPanel());

        m_KeyRepeat.Reset();
    }

    void ShowFooter(bool bShow)
    {
        if (m_pConsoleFooter)
            m_pConsoleFooter->SetVisible(bShow);
    }

    void UpdateMenuItemState(bool isInGame, bool isMultiplayer)
    {
        for (int i = 0; i < GetChildCount(); i++)
        {
            Panel *child = GetChild(i);
            vgui2::MenuItem *menuItem = dynamic_cast<vgui2::MenuItem *>(child);

            if (menuItem)
            {
                bool shouldBeVisible = true;
                KeyValues *kv = menuItem->GetUserData();

                if (!kv)
                    continue;

                if (!isInGame && kv->GetInt("OnlyInGame"))
                    shouldBeVisible = false;
                else if (isMultiplayer && kv->GetInt("notmulti"))
                    shouldBeVisible = false;
                else if (isInGame && !isMultiplayer && kv->GetInt("notsingle"))
                    shouldBeVisible = false;
                else if (kv->GetInt("ConsoleOnly"))
                    shouldBeVisible = false;

                menuItem->SetVisible(shouldBeVisible);
            }
        }

        if (!isInGame)
        {
            for (int j = 0; j < GetChildCount() - 2; j++)
                MoveMenuItem(j, j + 1);
        }
        else
        {
            for (int i = 0; i < GetChildCount(); i++)
            {
                for (int j = i; j < GetChildCount() - 2; j++)
                {
                    int iID1 = GetMenuID(j);
                    int iID2 = GetMenuID(j + 1);

                    vgui2::MenuItem *menuItem1 = GetMenuItem(iID1);
                    vgui2::MenuItem *menuItem2 = GetMenuItem(iID2);

                    KeyValues *kv1 = menuItem1->GetUserData();
                    KeyValues *kv2 = menuItem2->GetUserData();
                    if (kv1 && kv2)
                    {
                        if (kv1->GetInt("InGameOrder") > kv2->GetInt("InGameOrder"))
                            MoveMenuItem(iID2, iID1);
                    }
                }
            }
        }

        InvalidateLayout();

        if (m_pConsoleFooter)
        {
            const char *pHelpName;

            if (!isInGame)
                pHelpName = "MainMenu";
            else
                pHelpName = "GameMenu";

            if (!m_pConsoleFooter->GetHelpName() || V_stricmp(pHelpName, m_pConsoleFooter->GetHelpName()))
            {
                m_pConsoleFooter->SetHelpNameAndReset(pHelpName);
                m_pConsoleFooter->AddNewButtonLabel("#GameUI_Action", "#GameUI_Icons_A_BUTTON");

                if (isInGame)
                    m_pConsoleFooter->AddNewButtonLabel("#GameUI_Close", "#GameUI_Icons_B_BUTTON");
            }
        }
    }

private:
    CFooterPanel *m_pConsoleFooter;
    vgui2::CKeyRepeatHandler m_KeyRepeat;
};

static CBackgroundMenuButton *CreateMenuButton(CBasePanel *parent, const char *panelName, const wchar_t *panelText)
{
    CBackgroundMenuButton *pButton = new CBackgroundMenuButton(parent, panelName);
    pButton->SetProportional(true);
    pButton->SetCommand("OpenGameMenu");
    pButton->SetText(panelText);

    return pButton;
}

CBasePanel::CBasePanel(void) : vgui2::Panel(NULL, "BaseGameUIPanel")
{
    g_pBasePanel = this;
    m_bLevelLoading = false;
    m_eBackgroundState = BACKGROUND_INITIAL;
    m_flTransitionStartTime = 0.0f;
    m_flTransitionEndTime = 0.0f;
    m_flFrameFadeInTime = 0.5f;
    m_bRenderingBackgroundTransition = false;
    m_bFadingInMenus = false;
    m_bEverActivated = false;
    m_iGameMenuInset = 24;
    m_bHaveDarkenedBackground = false;
    m_bHaveDarkenedTitleText = true;
    m_bForceTitleTextUpdate = true;
    m_BackdropColor = Color(0, 0, 0, 128);
    m_pConsoleAnimationController = NULL;
    m_pConsoleControlSettings = NULL;
    m_iToolBarSize = 40;
    m_bInitialLoading = true;

    m_pGameMenu = NULL;
    m_pGameLogo = NULL;

    CreateGameMenu();
    // CreateGameLogo();
    CreateBackGround();
    CreateToolbar();
    CreateMainMenuBrowser();

    SetMenuAlpha(0);

    m_pFocusParent = NULL;
    m_pFocusPanel = NULL;

    OnWelcomeNewbie();
}

void CBasePanel::OnWelcomeNewbie(void)
{
    char szBuffer[128];
    if (vgui2::system()->GetRegistryString("HKEY_CURRENT_USER\\Software\\Valve\\Steam\\Language", szBuffer, 127) && strlen(szBuffer) > 0)
        return;
}

KeyValues *CBasePanel::GetConsoleControlSettings(void)
{
    return m_pConsoleControlSettings;
}

CBasePanel::~CBasePanel(void)
{
    g_pBasePanel = NULL;
}

void CBasePanel::PaintBackground(void)
{
    if (!m_hOptionsDialog.Get())
    {
        m_hOptionsDialog = new COptionsDialog(this);
        m_hCreateMultiplayerGameDialog = new CCreateMultiplayerGameDialog(this);

        PositionDialog(m_hOptionsDialog);
        PositionDialog(m_hCreateMultiplayerGameDialog);
    }

    if (!GameUI().IsInLevel() || g_hLoadingDialog.Get())
    {
        DrawBackgroundImage();
    }

    if (m_flBackgroundFillAlpha)
    {
        int swide, stall;
        vgui2::surface()->GetScreenSize(swide, stall);
        vgui2::surface()->DrawSetColor(0, 0, 0, m_flBackgroundFillAlpha);
        vgui2::surface()->DrawFilledRect(0, 0, swide, stall);
    }
}

bool CBasePanel::IsMenuFading(void)
{
    return m_bFadingInMenus;
}

bool CBasePanel::IsInitialLoading(void)
{
    return m_bInitialLoading;
}

void CBasePanel::UpdateBackgroundState(void)
{
    const float alpha_trans_duration = 0.2f;

    GameConsole().SetParent(GetVPanel());

    if (GameUI().IsInLevel())
    {
        SetBackgroundRenderState(BACKGROUND_LEVEL);
    }
    else if (!m_bLevelLoading)
    {
        if (IsPC())
            SetBackgroundRenderState(BACKGROUND_MAINMENU);
    }
    else if (m_bLevelLoading && g_hLoadingDialog.Get())
    {
        SetBackgroundRenderState(BACKGROUND_LOADING);
    }
    else if (m_bEverActivated)
    {
        SetBackgroundRenderState(BACKGROUND_DISCONNECTED);
    }

    bool bHaveActiveDialogs = false;
    bool bIsInLevel = GameUI().IsInLevel();

    for (int i = 0; i < GetChildCount(); ++i)
    {
        vgui2::VPANEL child = vgui2::ipanel()->GetChild(GetVPanel(), i);
        const char *name = vgui2::ipanel()->GetName(child);

        if (child && vgui2::ipanel()->IsVisible(child) && vgui2::ipanel()->IsPopup(child) && child != m_pGameMenu->GetVPanel())
        {
            bHaveActiveDialogs = true;
            break;
        }
    }

    if (!bHaveActiveDialogs)
    {
        vgui2::VPANEL parent = GetVParent();

        for (int i = 0; i < vgui2::ipanel()->GetChildCount(parent); ++i)
        {
            vgui2::VPANEL child = vgui2::ipanel()->GetChild(parent, i);

            if (child && vgui2::ipanel()->IsVisible(child) && vgui2::ipanel()->IsPopup(child) && child != GetVPanel())
            {
                bHaveActiveDialogs = true;
                break;
            }
        }
    }

    bool bNeedDarkenedBackground = (bHaveActiveDialogs || bIsInLevel);

    if (m_bHaveDarkenedBackground != bNeedDarkenedBackground)
    {
        float targetAlpha, duration;

        if (bNeedDarkenedBackground || m_eBackgroundState == BACKGROUND_LOADING)
        {
            targetAlpha = m_BackdropColor[3];
            duration = m_flFrameFadeInTime;
        }
        else
        {
            targetAlpha = 0.0f;
            duration = alpha_trans_duration;
        }

        m_bHaveDarkenedBackground = bNeedDarkenedBackground;

        vgui2::GetAnimationController()->RunAnimationCommand(this, "m_flBackgroundFillAlpha", targetAlpha, 0.0f, duration, vgui2::AnimationController::INTERPOLATOR_LINEAR);
    }

    if (m_bLevelLoading)
        return;

    bool bNeedDarkenedTitleText = bHaveActiveDialogs;

    if (m_bHaveDarkenedTitleText != bNeedDarkenedTitleText || m_bForceTitleTextUpdate)
    {
        float targetTitleAlpha, duration;

        if (bHaveActiveDialogs || m_eBackgroundState == BACKGROUND_LOADING)
        {
            duration = m_flFrameFadeInTime;
            targetTitleAlpha = 128.0f;
        }
        else
        {
            duration = alpha_trans_duration;
            targetTitleAlpha = 255.0f;
        }

        if (m_pGameLogo)
            vgui2::GetAnimationController()->RunAnimationCommand(m_pGameLogo, "alpha", targetTitleAlpha, 0.0f, duration, vgui2::AnimationController::INTERPOLATOR_LINEAR);

        if (m_pGameMenu)
            vgui2::GetAnimationController()->RunAnimationCommand(m_pGameMenu, "alpha", targetTitleAlpha, 0.0f, duration, vgui2::AnimationController::INTERPOLATOR_LINEAR);

        for (int i = 0; i < m_pGameMenuButtons.Count(); ++i)
            vgui2::GetAnimationController()->RunAnimationCommand(m_pGameMenuButtons[i], "alpha", targetTitleAlpha, 0.0f, duration, vgui2::AnimationController::INTERPOLATOR_LINEAR);

        m_bFadingInMenus = false;
        m_bHaveDarkenedTitleText = bNeedDarkenedTitleText;
        m_bForceTitleTextUpdate = false;
    }
}

void CBasePanel::SetBackgroundRenderState(EBackgroundState state)
{
    if (state == m_eBackgroundState)
        return;

    float frametime = engine->GetClientTime();

    m_bRenderingBackgroundTransition = false;
    m_bFadingInMenus = false;

    if (state == BACKGROUND_EXITING)
    {
    }
    else if (state == BACKGROUND_DISCONNECTED || state == BACKGROUND_MAINMENU)
    {
        m_bFadingInMenus = true;
        m_flFadeMenuStartTime = frametime;
        m_flFadeMenuEndTime = frametime + 0.2f;

    }
    else if (state == BACKGROUND_LOADING)
    {
        SetMenuAlpha(0);

    }
    else if (state == BACKGROUND_LEVEL)
    {
        SetMenuAlpha(255);

    }

    m_eBackgroundState = state;
}

void CBasePanel::OnSizeChanged(int newWide, int newTall)
{
    // HACK HACK: We don't get any OnScreenSizeChanged updates on any component so we calling 
    // OnScreenSizeChanged in OnSizeChange event of the root panel that matches the actual viewport bounds

    BaseClass::OnScreenSizeChanged(newWide, newTall);
}

void CBasePanel::OnLevelLoadingStarted(const char *levelName)
{
    m_bLevelLoading = true;
    m_pGameMenu->ShowFooter(false);

    static char imageName[MAX_PATH];
    sprintf(imageName, "resource/maploading/loadingbg_%s", levelName);

    if (!g_hLoadingDialog.Get())
        g_hLoadingDialog = new CLoadingDialog(this);

    g_hLoadingDialog->SetBackgroundImage(imageName);
}

void CBasePanel::OnLevelLoadingFinished(void)
{
    m_bLevelLoading = false;
}

void CBasePanel::DrawBackgroundImage(void)
{
    int swide, stall;
    vgui2::surface()->GetScreenSize(swide, stall);

    int wide, tall;
    GetSize(wide, tall);

    float frametime = engine->GetAbsoluteTime();
    int alpha = 255;

    if (m_bRenderingBackgroundTransition)
    {
        alpha = (m_flTransitionEndTime - frametime) / (m_flTransitionEndTime - m_flTransitionStartTime) * 255;
        alpha = clamp(alpha, 0, 255);
    }

    int ypos = 0;

    float xScale, yScale;
    xScale = (float)swide / (float)m_iBaseResX;
    yScale = (float)stall / (float)m_iBaseResY;

    // iterate and draw all the background pieces
    for (int x = 0; x < m_ImageID.Size(); x++)
    {
        bimage_t &bimage = m_ImageID[x];

        int dx = bimage.x;
        int dy = bimage.y;
        int dw = bimage.x + bimage.width;
        int dt = bimage.y + bimage.height;

        if (bimage.scaled)
        {
            dx = (int)ceil(dx * xScale);
            dy = (int)ceil(dy * yScale);
            dw = (int)ceil(dw * xScale);
            dt = (int)ceil(dt * yScale);
        }

        // draw the color image only if the mono image isn't yet fully opaque
        vgui2::surface()->DrawSetColor(255, 255, 255, 255);
        vgui2::surface()->DrawSetTexture(bimage.imageID);
        vgui2::surface()->DrawTexturedRect(dx, dy, dw, dt);
    }


    if (IsPC() && (m_bRenderingBackgroundTransition || m_eBackgroundState == BACKGROUND_LOADING))
    {
        if (m_pGameMenu->GetAlpha() < 255)
        {
            vgui2::surface()->DrawSetColor(255, 255, 255, alpha);
            vgui2::surface()->DrawSetTexture(m_iLoadingImageID);

            int twide, ttall;
            vgui2::surface()->DrawGetTextureSize(m_iLoadingImageID, twide, ttall);
            vgui2::surface()->DrawTexturedRect(wide - twide, tall - ttall, wide, tall);
        }
    }

    if (m_bFadingInMenus)
    {
        alpha = (frametime - m_flFadeMenuStartTime) / (m_flFadeMenuEndTime - m_flFadeMenuStartTime) * 255;
        alpha = clamp(alpha, 0, 255);

        for (int i = 0; i < m_pGameMenuButtons.Count(); ++i)
            m_pGameMenuButtons[i]->SetAlpha(alpha);

        if (alpha == 255)
            m_bFadingInMenus = false;

        m_pGameMenu->SetAlpha(alpha);
    }
}

void CBasePanel::ApplyMultiplayerGameSettings()
{
    if (m_hCreateMultiplayerGameDialog)
        m_hCreateMultiplayerGameDialog->ApplyMultiplayerGameSettings();
}

void CBasePanel::CreateGameMenu(void)
{
    KeyValues *datafile = new KeyValues("GameMenu");
    //datafile->UsesEscapeSequences(true); // TODO not supported by sdk

    if (datafile->LoadFromFile(g_pFullFileSystem, "Resource/GameMenu.res"))
        m_pGameMenu = RecursiveLoadGameMenu(this, datafile);

    if (!m_pGameMenu)
    {
        Error(_T("Could not load file Resource/GameMenu.res"));
    }
    else
    {
        SETUP_PANEL(m_pGameMenu);
        m_pGameMenu->SetAlpha(0);
    }

    datafile->deleteThis();
}

void CBasePanel::CreateGameLogo(void)
{
    m_pGameLogo = new CMainMenuGameLogo(this, "BgMenuButton");
}

void CBasePanel::CreateBackGround(void)
{
}

void CBasePanel::CreateToolbar(void)
{
    int swide, stall;
    vgui2::surface()->GetScreenSize(swide, stall);

    m_pToolBar = new CToolBar(this, "ToolBar");
    m_pToolBar->SetZPos(-20);
    m_pToolBar->SetVisible(false);
    m_pToolBar->SetBounds(0, stall - m_iToolBarSize, swide, m_iToolBarSize);
}

void CBasePanel::CreateMainMenuBrowser() {
    m_pMainMenuBrowser = new CMainMenuBrowser(this);
    m_pMainMenuBrowser->SetFullscreen(true);

    if(m_pMainMenuBrowser->IsVisible())
        m_pGameMenu->SetParent(m_pMainMenuBrowser);

    // m_pMainMenuBrowser->OpenURL("file://C:/Users/Vladislav/Desktop/test.html");
}

void CBasePanel::UpdateGameMenus(void)
{
    // check our current state
    bool isInGame = GameUI().IsInLevel();
    bool isMulti = isInGame && (engine->GetMaxClients() > 1);

    // iterate all the menu items
    m_pGameMenu->UpdateMenuItemState(isInGame, isMulti);

    // position the menu
    InvalidateLayout();
    m_pGameMenu->SetVisible( true );
}

CGameMenu *CBasePanel::RecursiveLoadGameMenu(vgui2::Panel *parent, KeyValues *datafile)
{
    CGameMenu *menu = new CGameMenu(parent, datafile->GetName());

    for (KeyValues *dat = datafile->GetFirstSubKey(); dat != NULL; dat = dat->GetNextKey())
    {
        const char *label = dat->GetString("label", "<unknown>");
        const char *cmd = dat->GetString("command", NULL);
        const char *name = dat->GetString("name", label);

        menu->AddMenuItem(name, label, cmd, this, dat);
    }

    return menu;
}

void CBasePanel::RunFrame(void)
{
    if (!IsVisible())
        return;

    if (vgui2::surface()->GetModalPanel())
        vgui2::surface()->PaintTraverse(GetVPanel());

    vgui2::GetAnimationController()->UpdateAnimations(engine->GetAbsoluteTime());
    UpdateBackgroundState();
}

void CBasePanel::PerformLayout(void)
{
    BaseClass::PerformLayout();

    int wide, tall;
    vgui2::surface()->GetScreenSize(wide, tall);

    int menuWide, menuTall;
    m_pGameMenu->GetSize(menuWide, menuTall);

    int idealMenuY = tall - menuTall - m_iGameMenuInset;
    //int idealMenuY = tall / 2 - menuTall/2 - m_iGameMenuInset;
    int yDiff = idealMenuY - m_iGameMenuPos.y;

    for (int i = 0; i < m_pGameMenuButtons.Count(); ++i)
    {
        m_pGameMenuButtons[i]->SizeToContents();
        m_pGameMenuButtons[i]->SetPos(m_iGameTitlePos[i].x, m_iGameTitlePos[i].y + yDiff);
    }

    if (m_pGameLogo)
        m_pGameLogo->SetPos(m_iGameMenuPos.x + m_pGameLogo->GetOffsetX(), idealMenuY - m_pGameLogo->GetTall() + m_pGameLogo->GetOffsetY());

    m_pGameMenu->SetPos(m_iGameMenuPos.x, idealMenuY);

    if (m_bInitialLoading)
    {
        m_bInitialLoading = false;
        //GameConsole().CheckPending();

        engine->pfnClientCmd("mp3 loop media/gamestartup.mp3\n");
    }

    UpdateGameMenus();
}

void CBasePanel::ApplySchemeSettings(vgui2::IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);

    m_iGameMenuInset = atoi(pScheme->GetResourceString("MainMenu.Inset"));
    m_iGameMenuInset *= 2;

    CUtlVector<Color> buttonColor;

    if (pScheme)
    {
        m_iGameTitlePos.RemoveAll();

        for (int i = 0; i < m_pGameMenuButtons.Count(); ++i)
        {
            m_pGameMenuButtons[i]->SetFont(pScheme->GetFont("TitleFont"));

            m_iGameTitlePos.AddToTail(coord());
            m_iGameTitlePos[i].x = vgui2::scheme()->GetProportionalScaledValue(53 + 50 * i);
            m_iGameTitlePos[i].y = vgui2::scheme()->GetProportionalScaledValue(190 + 17 * i);
            buttonColor.AddToTail(Color(255, 255, 255, 255));
        }

        m_iGameMenuPos.x = vgui2::scheme()->GetProportionalScaledValue(15);
        m_iGameMenuPos.y = vgui2::scheme()->GetProportionalScaledValue(240);
        m_iGameMenuInset = vgui2::scheme()->GetProportionalScaledValue(32);
    }
    else
    {
        for (int i = 0; i < m_pGameMenuButtons.Count(); ++i)
        {
            m_pGameMenuButtons[i]->SetFont(pScheme->GetFont("TitleFont"));

            buttonColor.AddToTail(Color(255, 255, 255, 255));
        }
    }

    for (int i = 0; i < m_pGameMenuButtons.Count(); ++i)
    {
        m_pGameMenuButtons[i]->SetDefaultColor(buttonColor[i], Color(0, 0, 0, 0));
        m_pGameMenuButtons[i]->SetArmedColor(buttonColor[i], Color(0, 0, 0, 0));
        m_pGameMenuButtons[i]->SetDepressedColor(buttonColor[i], Color(0, 0, 0, 0));
    }

    SetBgColor(Color(0, 0, 0, 0));

    m_flFrameFadeInTime = 0.3f;
    m_BackdropColor = Color(0, 0, 0, 128);

    int screenWide, screenTall;
    vgui2::surface()->GetScreenSize(screenWide, screenTall);

    float aspectRatio = (float)screenWide/(float)screenTall;
    bool bIsWidescreen = aspectRatio >= 1.5999f;

//    for (int y = 0; y < BACKGROUND_ROWS; y++)
//    {
//        for (int x = 0; x < BACKGROUND_COLUMNS; x++)
//        {
//            bimage_t &bimage = m_ImageID[y][x];
//            bimage.imageID = surface()->CreateNewTextureID();
//
//            char filename[MAX_PATH];
//            sprintf(filename, "resource/background/1024_%d_%c_BTE", y + 1, 'a' + x);
//            surface()->DrawSetTextureFile(bimage.imageID, filename, false, false);
//            surface()->DrawGetTextureSize(bimage.imageID, bimage.width, bimage.height);
//        }
//    }

//    m_iLoadingImageID = g_pVGuiSurface->CreateNewTextureID();
//    g_pVGuiSurface->DrawSetTextureFile(m_iLoadingImageID, "gfx/vgui/console/startup_loading", false, false);

    FileHandle_t file = g_pFullFileSystem->Open("resource/BackgroundLayout.txt", "rt");
    if (!file)
        return;

    int fileSize = g_pFullFileSystem->Size(file);
    char *buffer = (char *)alloca(fileSize + 1);
    g_pFullFileSystem->Read(buffer, fileSize, file);
    g_pFullFileSystem->Close(file);
    buffer[fileSize] = 0;

    //int vid_level;
    //gameuifuncs->GetCurrentRenderer(NULL, 0, NULL, NULL, NULL, &vid_level);

    char token[512];
    while (buffer && *buffer)
    {
        buffer = g_pFullFileSystem->ParseFile(buffer, token, NULL);
        if (!buffer || !buffer[0])
            break;

        if (!stricmp(token, "resolution"))
        {
            buffer = g_pFullFileSystem->ParseFile(buffer, token, NULL);
            m_iBaseResX = atoi(token);
            buffer = g_pFullFileSystem->ParseFile(buffer, token, NULL);
            m_iBaseResY = atoi(token);
        }
        else
        {
            bimage_t &bimage = m_ImageID[m_ImageID.AddToTail()];
            bimage.imageID = vgui2::surface()->CreateNewTextureID();

            char *ext = strstr(token, ".tga");
            if (ext)
                *ext = 0;

            vgui2::surface()->DrawSetTextureFile(bimage.imageID, token, 1, false);
            vgui2::surface()->DrawGetTextureSize(bimage.imageID, bimage.width, bimage.height);

            buffer = g_pFullFileSystem->ParseFile(buffer, token, NULL);
            bimage.scaled = stricmp(token, "scaled") == 0;
            buffer = g_pFullFileSystem->ParseFile(buffer, token, NULL);
            bimage.x = atoi(token);
            buffer = g_pFullFileSystem->ParseFile(buffer, token, NULL);
            bimage.y = atoi(token);
        }
    }
}

void CBasePanel::OnActivateModule(int moduleIndex)
{
}

void CBasePanel::OnGameUIActivated(void)
{
    if (!m_bEverActivated)
    {
        UpdateGameMenus();
        m_bEverActivated = true;
    }

    if (GameUI().IsInLevel())
    {
        OnCommand("OpenPauseMenu");
        m_pGameMenu->SetVisible(false);
    }
}

constexpr static char kConnectTo[] = "ConnectTo ";

void CBasePanel::RunMenuCommand(const char *command)
{
    if (!Q_stricmp(command, "OpenServerBrowser"))
    {
        OnOpenServerBrowser();
    }
    else if (!Q_stricmp(command, "OpenCreateMultiplayerGameDialog"))
    {
        OnOpenCreateMultiplayerGameDialog();
    }
    else if (!Q_stricmp(command, "OpenOptionsDialog"))
    {
        OnOpenOptionsDialog();
    }
    else if (!Q_stricmp(command, "ResumeGame"))
    {
        engine->pfnClientCmd("cancelselect");
    }
    else if (!Q_stricmp(command, "Disconnect"))
    {
        engine->pfnClientCmd("disconnect\n");
    }
    else if (!Q_stricmp(command, "Quit"))
    {
        OnOpenQuitConfirmationDialog();
    }
    else if (!Q_stricmp(command, "QuitNoConfirm"))
    {
        SetVisible(false);
        vgui2::surface()->RestrictPaintToSinglePanel(GetVPanel());
        engine->pfnClientCmd("quit\n");
    }
    else if (!Q_stricmp(command, "ReleaseModalWindow"))
    {
        vgui2::surface()->RestrictPaintToSinglePanel(NULL);
    }
    else if (!Q_strcmp(command, "ConnectToRandomServer"))
    {
        servernetadr_t server_addr{};
        GameUINext().InvokeDequeueGameMenuServer(&server_addr);

        if (server_addr.GetIP() != 0 && server_addr.GetConnectionPort() != 0)
        {
            GameUINext().SetLastConnectionInfo(server_addr, GuiConnectionSource::GameMenu, "");
            engine->pfnClientCmd(std::format("connect {}\n", server_addr.GetConnectionAddressString()).c_str());
        }
        else
        {
            auto* dlg = new vgui2::MessageBox("Info", "The master server did not return a list of servers for the Random Servers button, or there is no internet connection.");
            dlg->DoModal();
        }
    }
    else if (!Q_stricmp(command, "OpenPlayerListDialog"))
    {
        OnOpenPlayerListDialog();
    }
    else if (!Q_strncmp(command, kConnectTo, sizeof(kConnectTo) - 1))
    {
        std::vector<std::string> args;
        nitro_utils::split_in_args(command, args, 2);

        if (args.size() != 2)
            return;

        auto& address = args[1];

        if (std::find_if(address.cbegin(), address.cend(), [](char ch) { return ch == ' ' || ch == '\n' || ch == '\r' || ch == '"' || ch == '\''; }) != address.cend())
            return;

        engine->pfnClientCmd(std::format("connect {}\n", address).c_str());
    }
    else if (GameUINext().InvokeRunMenuCommand(command))
    {
        // command handled
    }
    else
    {
        BaseClass::OnCommand(command);
    }
}

void CBasePanel::OnCommand(const char *command)
{
    RunMenuCommand(command);
}

void CBasePanel::RunAnimationWithCallback(vgui2::Panel *parent, const char *animName, KeyValues *msgFunc)
{
    if (!m_pConsoleAnimationController)
        return;

    m_pConsoleAnimationController->StartAnimationSequence(animName);

    float sequenceLength = m_pConsoleAnimationController->GetAnimationSequenceLength(animName);

    if (sequenceLength)
        sequenceLength += g_flAnimationPadding;

    if (parent && msgFunc)
        PostMessage(parent, msgFunc, sequenceLength);
}

class CQuitQueryBox : public vgui2::QueryBox
{
    DECLARE_CLASS_SIMPLE(CQuitQueryBox, vgui2::QueryBox);

public:
    CQuitQueryBox(const char *title, const char *info, Panel *parent) : BaseClass(title, info, parent)
    {
    }

    void DoModal(Frame *pFrameOver)
    {
        BaseClass::DoModal(pFrameOver);
        vgui2::surface()->RestrictPaintToSinglePanel(GetVPanel());
    }

    void OnKeyCodePressed(vgui2::KeyCode code)
    {
        if (code == vgui2::KeyCode::KEY_ESCAPE)
        {
            SetAlpha(0);
            Close();
        }
        else
            BaseClass::OnKeyCodePressed(code);
    }

    virtual void OnClose(void)
    {
        BaseClass::OnClose();
        vgui2::surface()->RestrictPaintToSinglePanel(NULL);
    }
};

void CBasePanel::OnOpenQuitConfirmationDialog(void)
{
    if (GameUI().IsInLevel() && engine->GetMaxClients() == 1)
    {
    }
    else
    {
        if (!m_hQuitQueryBox.Get())
        {
            m_hQuitQueryBox = new CQuitQueryBox("#GameUI_QuitConfirmationTitle", "#GameUI_QuitConfirmationText", this);
            m_hQuitQueryBox->SetOKButtonText("#GameUI_Quit");
            m_hQuitQueryBox->SetOKCommand(new KeyValues("Command", "command", "QuitNoConfirm"));
            m_hQuitQueryBox->SetCancelCommand(new KeyValues("Command", "command", "ReleaseModalWindow"));
            m_hQuitQueryBox->AddActionSignalTarget(this);
            m_hQuitQueryBox->DoModal();
        }
    }
}

void CBasePanel::OnOpenServerBrowser(void)
{
    GameUI().ActivateServerBrowser();
}

void CBasePanel::OnOpenOptionsDialog(const char* tabName)
{
    if (!m_hOptionsDialog.Get())
    {
        m_hOptionsDialog = new COptionsDialog(this);
        PositionDialog(m_hOptionsDialog);
    }

    if(!m_hOptionsDialog->IsVisible())
    {
        m_hOptionsDialog->Activate();
    }

    if(tabName != nullptr)
    {
        m_hOptionsDialog->OpenTab(tabName);
    }
}

void CBasePanel::OnOpenPlayerListDialog()
{
    if (!m_hPlayerListDialog.Get())
    {
        m_hPlayerListDialog = new CPlayerListDialog(this);
        PositionDialog(m_hPlayerListDialog);
    }
    m_hPlayerListDialog->Activate();
}

void CBasePanel::OnOpenCreateMultiplayerGameDialog(void)
{
    if (!m_hCreateMultiplayerGameDialog.Get())
    {
        m_hCreateMultiplayerGameDialog = new CCreateMultiplayerGameDialog(this);
        PositionDialog(m_hCreateMultiplayerGameDialog);
    }

    m_hCreateMultiplayerGameDialog->Activate();
}

void CBasePanel::PositionDialog(vgui2::PHandle dlg)
{
    if (!dlg.Get())
        return;

    int x, y, ww, wt, wide, tall;
    vgui2::surface()->GetWorkspaceBounds(x, y, ww, wt);
    dlg->GetSize(wide, tall);
    dlg->SetPos(x + ((ww - wide) / 2), y + ((wt - tall) / 2));
}

void CBasePanel::PositionDialog(vgui2::Panel *pdlg)
{
    int x, y, ww, wt, wide, tall;
    vgui2::surface()->GetWorkspaceBounds(x, y, ww, wt);
    pdlg->GetSize(wide, tall);
    pdlg->SetPos(x + ((ww - wide) / 2), y + ((wt - tall) / 2));
}

void CBasePanel::OnGameUIHidden(void)
{
    if (m_hOptionsDialog.Get())
        PostMessage(m_hOptionsDialog.Get(), new KeyValues("GameUIHidden"));
}

void CBasePanel::SetMenuAlpha(int alpha)
{
    m_pGameMenu->SetAlpha(alpha);

    if (m_pGameLogo)
        m_pGameLogo->SetAlpha(alpha);

    for (int i = 0; i < m_pGameMenuButtons.Count(); ++i)
        m_pGameMenuButtons[i]->SetAlpha(alpha);

    m_bForceTitleTextUpdate = true;
}

void CBasePanel::SetMenuItemBlinkingState(const char *itemName, bool state)
{
    for (int i = 0; i < GetChildCount(); i++)
    {
        vgui2::Panel *child = GetChild(i);
        CGameMenu *pGameMenu = dynamic_cast<CGameMenu *>(child);

        if (pGameMenu)
            pGameMenu->SetMenuItemBlinkingState(itemName, state);
    }
}

void CBasePanel::RunEngineCommand(const char *command)
{
    engine->pfnClientCmd((char *)command);
}

void CBasePanel::RunCloseAnimation(const char *animName)
{
    RunAnimationWithCallback(this, animName, new KeyValues("FinishDialogClose"));
}

void CBasePanel::FinishDialogClose(void)
{
}

CFooterPanel::CFooterPanel(vgui2::Panel *parent, const char *panelName) : BaseClass(parent, panelName)
{
    SetVisible(true);
    SetAlpha(0);

    m_pHelpName = NULL;
    m_pSizingLabel = new vgui2::Label(this, "SizingLabel", "");
    m_pSizingLabel->SetVisible(false);

    m_nButtonGap = 32;
    m_nButtonGapDefault = 32;
    m_ButtonPinRight = 100;
    m_FooterTall = 80;

    int wide, tall;
    vgui2::surface()->GetScreenSize(wide, tall);

    if (tall <= 480)
        m_FooterTall = 60;

    m_ButtonOffsetFromTop = 0;
    m_ButtonSeparator = 4;
    m_TextAdjust = 0;

    m_bPaintBackground = false;
    m_bCenterHorizontal = false;

    m_szButtonFont[0] = '\0';
    m_szTextFont[0] = '\0';
    m_szFGColor[0] = '\0';
    m_szBGColor[0] = '\0';
}

CFooterPanel::~CFooterPanel(void)
{
    SetHelpNameAndReset(NULL);

    delete m_pSizingLabel;
}

void CFooterPanel::ApplySchemeSettings(vgui2::IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);

    m_hButtonFont = pScheme->GetFont((m_szButtonFont[0] != '\0') ? m_szButtonFont : "GameUIButtons");
    m_hTextFont = pScheme->GetFont((m_szTextFont[0] != '\0') ? m_szTextFont : "MenuLarge");

    SetFgColor(pScheme->GetColor(m_szFGColor, Color(255, 255, 255, 255)));
    SetBgColor(pScheme->GetColor(m_szBGColor, Color(0, 0, 0, 255)));

    int x, y, w, h;
    GetParent()->GetBounds(x, y, w, h);
    SetBounds(x, h - m_FooterTall, w, m_FooterTall);
}

void CFooterPanel::ApplySettings(KeyValues *inResourceData)
{
    BaseClass::ApplySettings(inResourceData);

    m_nButtonGap = inResourceData->GetInt("buttongap", 32);
    m_nButtonGapDefault = m_nButtonGap;
    m_ButtonPinRight = inResourceData->GetInt("button_pin_right", 100);
    m_FooterTall = inResourceData->GetInt("tall", 80);
    m_ButtonOffsetFromTop = inResourceData->GetInt("buttonoffsety", 0);
    m_ButtonSeparator = inResourceData->GetInt("button_separator", 4);
    m_TextAdjust = inResourceData->GetInt("textadjust", 0);

    m_bCenterHorizontal = (inResourceData->GetInt("center", 0) == 1);
    m_bPaintBackground = (inResourceData->GetInt("paintbackground", 0) == 1);

    Q_strncpy(m_szTextFont, inResourceData->GetString("fonttext", "MenuLarge"), sizeof(m_szTextFont));
    Q_strncpy(m_szButtonFont, inResourceData->GetString("fontbutton", "GameUIButtons"), sizeof(m_szButtonFont));

    Q_strncpy(m_szFGColor, inResourceData->GetString("fgcolor", "White"), sizeof(m_szFGColor));
    Q_strncpy(m_szBGColor, inResourceData->GetString("bgcolor", "Black"), sizeof(m_szBGColor));

    for (KeyValues *pButton = inResourceData->GetFirstSubKey(); pButton != NULL; pButton = pButton->GetNextKey())
    {
        const char *pName = pButton->GetName();

        if (!Q_stricmp(pName, "button"))
        {
            const char *pText = pButton->GetString("text", "NULL");
            const char *pIcon = pButton->GetString("icon", "NULL");
            AddNewButtonLabel(pText, pIcon);
        }
    }

    InvalidateLayout(false, true);
}

void CFooterPanel::AddButtonsFromMap(vgui2::Frame *pMenu)
{
    CControllerMap *pMap = dynamic_cast<CControllerMap *>(pMenu->FindChildByName("ControllerMap"));

    if (pMap)
    {
        int buttonCt = pMap->NumButtons();

        for (int i = 0; i < buttonCt; ++i)
        {
            const char *pText = pMap->GetBindingText(i);

            if (pText)
                AddNewButtonLabel(pText, pMap->GetBindingIcon(i));
        }
    }

    SetHelpNameAndReset(pMenu->GetName());
}

void CFooterPanel::SetStandardDialogButtons(void)
{
    SetHelpNameAndReset("Dialog");
    AddNewButtonLabel("#GameUI_Action", "#GameUI_Icons_A_BUTTON");
    AddNewButtonLabel("#GameUI_Close", "#GameUI_Icons_B_BUTTON");
}

void CFooterPanel::SetHelpNameAndReset(const char *pName)
{
    if (m_pHelpName)
    {
        free(m_pHelpName);
        m_pHelpName = NULL;
    }

    if (pName)
        m_pHelpName = strdup(pName);

    ClearButtons();
}

const char *CFooterPanel::GetHelpName(void)
{
    return m_pHelpName;
}

void CFooterPanel::ClearButtons(void)
{
    m_ButtonLabels.PurgeAndDeleteElements();
}

void CFooterPanel::AddNewButtonLabel(const char *text, const char *icon)
{
    ButtonLabel_t *button = new ButtonLabel_t;

    Q_strncpy(button->name, text, MAX_PATH);
    button->bVisible = true;

    wchar_t *pIcon = g_pVGuiLocalize->Find(icon);

    if (pIcon)
    {
        button->icon[0] = pIcon[0];
        button->icon[1] = '\0';
    }
    else
        button->icon[0] = '\0';

    wchar_t *pText = g_pVGuiLocalize->Find(text);

    if (pText)
        wcsncpy(button->text, pText, wcslen(pText) + 1);
    else
        button->text[0] = '\0';

    m_ButtonLabels.AddToTail(button);
}

void CFooterPanel::ShowButtonLabel(const char *name, bool show)
{
    for (int i = 0; i < m_ButtonLabels.Count(); ++i)
    {
        if (!Q_stricmp(m_ButtonLabels[i]->name, name))
        {
            m_ButtonLabels[i]->bVisible = show;
            break;
        }
    }
}

void CFooterPanel::SetButtonText(const char *buttonName, const char *text)
{
    for (int i = 0; i < m_ButtonLabels.Count(); ++i)
    {
        if (!Q_stricmp(m_ButtonLabels[i]->name, buttonName))
        {
            wchar_t *wtext = g_pVGuiLocalize->Find(text);

            if (text)
                wcsncpy(m_ButtonLabels[i]->text, wtext, wcslen(wtext) + 1);
            else
                m_ButtonLabels[i]->text[0] = '\0';

            break;
        }
    }
}

void CFooterPanel::PaintBackground(void)
{
    if (!m_bPaintBackground)
        return;

    BaseClass::PaintBackground();
}

void CFooterPanel::Paint(void)
{
    int wide = GetWide();
    int right = wide - m_ButtonPinRight;

    int buttonHeight = vgui2::surface()->GetFontTall(m_hButtonFont);
    int fontHeight = vgui2::surface()->GetFontTall(m_hTextFont);
    int textY = (buttonHeight - fontHeight) / 2 + m_TextAdjust;

    if (textY < 0)
        textY = 0;

    int y = m_ButtonOffsetFromTop;

    if (!m_bCenterHorizontal)
    {
        int x = right;

        for (int i = 0; i < m_ButtonLabels.Count(); ++i)
        {
            ButtonLabel_t *pButton = m_ButtonLabels[i];

            if (!pButton->bVisible)
                continue;

            m_pSizingLabel->SetFont(m_hTextFont);
            m_pSizingLabel->SetText(pButton->text);
            m_pSizingLabel->SizeToContents();

            int iTextWidth = m_pSizingLabel->GetWide();

            if (iTextWidth == 0)
                x += m_nButtonGap;
            else
                x -= iTextWidth;

            vgui2::surface()->DrawSetTextFont(m_hTextFont);
            vgui2::surface()->DrawSetTextColor(GetFgColor());
            vgui2::surface()->DrawSetTextPos(x, y + textY);
            vgui2::surface()->DrawPrintText(pButton->text, wcslen(pButton->text));

            x -= (vgui2::surface()->GetCharacterWidth(m_hButtonFont, pButton->icon[0]) + m_ButtonSeparator);

            vgui2::surface()->DrawSetTextFont(m_hButtonFont);
            vgui2::surface()->DrawSetTextColor(255, 255, 255, 255);
            vgui2::surface()->DrawSetTextPos(x, y);
            vgui2::surface()->DrawPrintText(pButton->icon, 1);

            x -= m_nButtonGap;
        }
    }
    else
    {
        int x = wide / 2;
        int totalWidth = 0;
        int i = 0;
        int nButtonCount = 0;

        for (i = 0; i < m_ButtonLabels.Count(); ++i)
        {
            ButtonLabel_t *pButton = m_ButtonLabels[i];

            if (!pButton->bVisible)
                continue;

            m_pSizingLabel->SetFont(m_hTextFont);
            m_pSizingLabel->SetText(pButton->text);
            m_pSizingLabel->SizeToContents();

            totalWidth += vgui2::surface()->GetCharacterWidth(m_hButtonFont, pButton->icon[0]);
            totalWidth += m_ButtonSeparator;
            totalWidth += m_pSizingLabel->GetWide();

            nButtonCount++;
        }

        totalWidth += (nButtonCount - 1) * m_nButtonGap;
        x -= (totalWidth / 2);

        for (i = 0; i < m_ButtonLabels.Count(); ++i)
        {
            ButtonLabel_t *pButton = m_ButtonLabels[i];

            if (!pButton->bVisible)
                continue;

            m_pSizingLabel->SetFont(m_hTextFont);
            m_pSizingLabel->SetText(pButton->text);
            m_pSizingLabel->SizeToContents();

            int iTextWidth = m_pSizingLabel->GetWide();

            vgui2::surface()->DrawSetTextFont(m_hButtonFont);
            vgui2::surface()->DrawSetTextColor(255, 255, 255, 255);
            vgui2::surface()->DrawSetTextPos(x, y);
            vgui2::surface()->DrawPrintText(pButton->icon, 1);

            x += vgui2::surface()->GetCharacterWidth(m_hButtonFont, pButton->icon[0]) + m_ButtonSeparator;

            vgui2::surface()->DrawSetTextFont(m_hTextFont);
            vgui2::surface()->DrawSetTextColor(GetFgColor());
            vgui2::surface()->DrawSetTextPos(x, y + textY);
            vgui2::surface()->DrawPrintText(pButton->text, wcslen(pButton->text));

            x += iTextWidth + m_nButtonGap;
        }
    }
}

DECLARE_BUILD_FACTORY(CFooterPanel);

CMainMenuGameLogo::CMainMenuGameLogo(vgui2::Panel *parent, const char *name) : vgui2::EditablePanel(parent, name)
{
    m_nOffsetX = 0;
    m_nOffsetY = 0;
}

void CMainMenuGameLogo::ApplySettings(KeyValues *inResourceData)
{
    BaseClass::ApplySettings(inResourceData);

    m_nOffsetX = inResourceData->GetInt("offsetX", 0);
    m_nOffsetY = inResourceData->GetInt("offsetY", 0);
}

void CMainMenuGameLogo::ApplySchemeSettings(vgui2::IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);

    LoadControlSettings("Resource/GameLogo.res");
}

void CBasePanel::CloseBaseDialogs(void)
{
    if (m_hCreateMultiplayerGameDialog.Get())
        m_hCreateMultiplayerGameDialog->Close();
}
