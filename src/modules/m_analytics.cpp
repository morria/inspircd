/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 *   Copyright (C) 2009-2010 Daniel De Graaf <danieldg@inspircd.org>
 *
 * This file is part of InspIRCd.  InspIRCd is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "inspircd.h"

/* $ModDesc: Allows for the logging of all messages to a channel */

class ModuleAnalytics : public Module
{
 private:

 public:
	void init()
	{
		Implementation eventlist[] = { I_OnUserMessage, I_OnUserJoin, I_OnUserPart, I_OnRehash };
		ServerInstance->Modules->Attach(eventlist, this, sizeof(eventlist)/sizeof(Implementation));
		OnRehash(NULL);
	}

	void OnRehash(User *user)
	{
		// ConfigTag* tag = ServerInstance->Config->ConfValue("analytics");
	}

	void OnUserMessage(User* user, void* dest, int target_type, const std::string &text, char status, const CUList&)
	{
        std::string target_name;

		if (target_type == TYPE_CHANNEL && status == 0)
		{
			Channel *target_channel = (Channel *)dest;
            target_name = target_channel->name;
        }
        else if (target_type == TYPE_USER) {
            return;
        }

        ServerInstance->Logs->Log("ANALYTICS_MESSAGES",
                                  DEFAULT,
                                  "%s\t%s\t%s",
                                  user->nick.c_str(),
                                  target_name.c_str(),
                                  text.c_str());
	}

    void OnUserJoin(Membership* memb, bool sync, bool created, CUList& except)
    {
        ServerInstance->Logs->Log("ANALYTICS_CHANNEL_USERS",
                                  DEFAULT,
                                  "JOIN\t%s\t%s",
                                  memb->chan->name.c_str(),
                                  memb->user->nick.c_str());
    }

    void OnUserPart(Membership* memb, std::string &partmessage, CUList& except)
    {
        ServerInstance->Logs->Log("ANALYTICS_CHANNEL_USERS",
                                  DEFAULT,
                                  "PART\t%s\t%s",
                                  memb->chan->name.c_str(),
                                  memb->user->nick.c_str());
    }

	Version GetVersion()
	{
		return Version("Provides for logging of all messages", VF_VENDOR);
	}
};

MODULE_INIT(ModuleAnalytics)
