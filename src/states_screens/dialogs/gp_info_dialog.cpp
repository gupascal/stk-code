//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2009-2013 Marianne Gagnon
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "states_screens/dialogs/gp_info_dialog.hpp"

#include "audio/sfx_manager.hpp"
#include "challenges/unlock_manager.hpp"
#include "config/player_manager.hpp"
#include "config/saved_grand_prix.hpp"
#include "guiengine/engine.hpp"
#include "guiengine/screen.hpp"
#include "guiengine/widgets/button_widget.hpp"
#include "guiengine/widgets/icon_button_widget.hpp"
#include "guiengine/widgets/label_widget.hpp"
#include "io/file_manager.hpp"
#include "race/grand_prix_manager.hpp"
#include "race/race_manager.hpp"
#include "states_screens/state_manager.hpp"
#include "states_screens/tracks_screen.hpp"
#include "tracks/track.hpp"
#include "tracks/track_manager.hpp"
#include "utils/translation.hpp"

#include <iostream>
#include <IGUIEnvironment.h>
#include <IGUIStaticText.h>

using namespace irr::gui;
using namespace irr::video;
using namespace irr::core;
using namespace GUIEngine;

// ------------------------------------------------------------------------------------------------------

GPInfoDialog::GPInfoDialog(const std::string& gpIdent, const float w, const float h) : ModalDialog(w, h)
{
    doInit();
    m_curr_time = 0.0f;

    const int y1 = m_area.getHeight()/7;
    const int y2 = m_area.getHeight()*6/7;

    m_gp_ident = gpIdent;

    const GrandPrixData* gp = grand_prix_manager->getGrandPrix(gpIdent);
    if (gp == NULL)
    {
        assert(false);
        std::cerr << "ERROR at " << __FILE__ << " : " << __LINE__ << "; trying to continue\n";
        ModalDialog::dismiss();
        return;
    }

    // ---- GP Name
    core::rect< s32 > area_top(0, 0, m_area.getWidth(), y1);
    IGUIStaticText* title = GUIEngine::getGUIEnv()->addStaticText( translations->fribidize(gp->getName()),
                                                               area_top, false, true, // border, word wrap
                                                               m_irrlicht_window);
    title->setTabStop(false);
    title->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);


    // ---- Track listings
    const std::vector<std::string> tracks = gp->getTrackNames();
    const int trackAmount = tracks.size();

    int height_of_one_line = (y2 - y1)/(trackAmount+1);
    const int textHeight = GUIEngine::getFontHeight();
    if (height_of_one_line > (int)(textHeight*1.5f)) height_of_one_line = (int)(textHeight*1.5f);

    bool gp_ok = true;

    for (int t=0; t<trackAmount; t++)
    {
        const int from_y = y1 + height_of_one_line*(t+1);

        Track* track = track_manager->getTrack(tracks[t]);
        stringw lineText;
        if (track == NULL)
        {
            lineText = L"MISSING : ";
            lineText.append( stringw(tracks[t].c_str()) );
            gp_ok = false;
        }
        else
        {
            lineText = track->getName();
        }

        LabelWidget* widget = new LabelWidget();
        widget->setText(translations->fribidize(lineText), false);
        widget->m_x = 20;
        widget->m_y = from_y;
        widget->m_w = m_area.getWidth()/2 - 20;
        widget->m_h = height_of_one_line;
        widget->setParent(m_irrlicht_window);

        m_widgets.push_back(widget);
        widget->add();

        // IGUIStaticText* line = GUIEngine::getGUIEnv()->addStaticText( lineText.c_str(),
        //                                       entry_area, false , true , // border, word wrap
        //                                       m_irrlicht_window);
    }

    // ---- Track screenshot

    m_screenshot_widget = new IconButtonWidget(IconButtonWidget::SCALE_MODE_KEEP_CUSTOM_ASPECT_RATIO,
                                               false /* tab stop */, false /* focusable */,
                                               IconButtonWidget::ICON_PATH_TYPE_ABSOLUTE /* Track gives us absolute paths */);
    // images are saved squared, but must be stretched to 4:3
    m_screenshot_widget->setCustomAspectRatio(4.0f / 3.0f);

    m_screenshot_widget->m_x = m_area.getWidth()/2;
    m_screenshot_widget->m_y = y1;
    m_screenshot_widget->m_w = m_area.getWidth()/2;
    m_screenshot_widget->m_h = y2 - y1 - 10;

    Track* track = (tracks.size() == 0 ? NULL : track_manager->getTrack(tracks[0]));

    m_screenshot_widget->m_properties[PROP_ICON] = (track  != NULL ?
                                                    track->getScreenshotFile().c_str() :
                                                    file_manager->getAsset(FileManager::GUI,"main_help.png"));
    m_screenshot_widget->setParent(m_irrlicht_window);
    m_screenshot_widget->add();
    m_widgets.push_back(m_screenshot_widget);


    // ---- Start button
    ButtonWidget* okBtn = new ButtonWidget();
    ButtonWidget* continueBtn = new ButtonWidget();

    SavedGrandPrix* saved_gp = SavedGrandPrix::getSavedGP( StateManager::get()
                                               ->getActivePlayerProfile(0)
                                               ->getUniqueID(),
                                               gpIdent,
                                               race_manager->getDifficulty(),
                                               race_manager->getNumberOfKarts(),
                                               race_manager->getNumLocalPlayers());

    if (tracks.size() == 0)
    {
        okBtn->m_properties[PROP_ID] = "cannot_start";
        okBtn->setText(_("Sorry, no tracks available"));
    }
    else if (gp_ok)
    {
        okBtn->m_properties[PROP_ID] = "start";
        okBtn->setText(_("Start Grand Prix"));

        continueBtn->m_properties[PROP_ID] = "continue";
        continueBtn->setText(_("Continue"));
    }
    else
    {
        okBtn->m_properties[PROP_ID] = "cannot_start";
        okBtn->setText(_("This Grand Prix is broken!"));
        okBtn->setBadge(BAD_BADGE);
    }

    if (saved_gp && gp_ok)
    {
        continueBtn->m_x = m_area.getWidth()/2 + 110;
        continueBtn->m_y = y2;
        continueBtn->m_w = 200;
        continueBtn->m_h = m_area.getHeight() - y2 - 15;
        continueBtn->setParent(m_irrlicht_window);
        m_widgets.push_back(continueBtn);
        continueBtn->add();
        continueBtn->getIrrlichtElement()->setTabStop(true);
        continueBtn->getIrrlichtElement()->setTabGroup(false);
        
        okBtn->m_x = m_area.getWidth()/2 - 310;
    }
    else
    {
        okBtn->m_x = m_area.getWidth()/2 - 200;
    }
    
    okBtn->m_y = y2;
    okBtn->m_w = 400;
    okBtn->m_h = m_area.getHeight() - y2 - 15;
    okBtn->setParent(m_irrlicht_window);
    m_widgets.push_back(okBtn);
    okBtn->add();
    okBtn->getIrrlichtElement()->setTabStop(true);
    okBtn->getIrrlichtElement()->setTabGroup(false);

    okBtn->setFocusForPlayer( PLAYER_ID_GAME_MASTER );
    
}

// ------------------------------------------------------------------------------------------------------

GPInfoDialog::~GPInfoDialog()
{
    // Place focus back on selected GP, in case the dialog was cancelled and we're back to
    // the track selection screen after
    Screen* curr_screen = GUIEngine::getCurrentScreen();
    if (curr_screen->getName() == "tracks.stkgui")
    {
        ((TracksScreen*)curr_screen)->setFocusOnGP(m_gp_ident);
    }

}

// ------------------------------------------------------------------------------------------------------

void GPInfoDialog::onEnterPressedInternal()
{
    // Save the gp identifier, since dismiss will delete this object.
    std::string gp_id = m_gp_ident;
    ModalDialog::dismiss();
    // Disable accidentally unlocking of a challenge
    PlayerManager::getCurrentPlayer()->setCurrentChallenge("");
    race_manager->startGP(grand_prix_manager->getGrandPrix(gp_id), false, false);
}

// ------------------------------------------------------------------------------------------------------

GUIEngine::EventPropagation GPInfoDialog::processEvent(const std::string& eventSource)
{
    if (eventSource == "start")
    {
        // Save GP identifier, since dismiss will delete this object.
        std::string gp_id = m_gp_ident;
        ModalDialog::dismiss();
        race_manager->startGP(grand_prix_manager->getGrandPrix(gp_id), false, false);
        return GUIEngine::EVENT_BLOCK;
    }
    if (eventSource == "continue")
    {
        // Save GP identifier, since dismiss will delete this object.
        std::string gp_id = m_gp_ident;
        ModalDialog::dismiss();
        race_manager->startGP(grand_prix_manager->getGrandPrix(gp_id), false, true);
        return GUIEngine::EVENT_BLOCK;
    }
    else if (eventSource == "cannot_start")
    {
        sfx_manager->quickSound( "anvil" );
    }

    return GUIEngine::EVENT_LET;
}

// ------------------------------------------------------------------------------------------------------

void GPInfoDialog::onUpdate(float dt)
{
    const int frameBefore = (int)(m_curr_time / 1.5f);
    m_curr_time += dt;
    int frameAfter = (int)(m_curr_time / 1.5f);

    if (frameAfter == frameBefore) return; // if nothing changed, return right now

    const GrandPrixData* gp = grand_prix_manager->getGrandPrix(m_gp_ident);
    assert(gp != NULL);
    const std::vector<std::string> tracks = gp->getTrackNames();
    if (frameAfter >= (int)tracks.size())
    {
        frameAfter = 0;
        m_curr_time = 0;
    }

    Track* track = (tracks.size() == 0 ? NULL :
        track_manager->getTrack(tracks[frameAfter]));
    std::string fn = track ? track->getScreenshotFile()
                           : file_manager->getAsset(FileManager::GUI, "main_help.png");
    m_screenshot_widget->setImage(fn.c_str(), IconButtonWidget::ICON_PATH_TYPE_ABSOLUTE);
}

// ------------------------------------------------------------------------------------------------------
