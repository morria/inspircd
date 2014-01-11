/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 *   Copyright (C) 2014 Andrew Morrison <asm@collapse.io>
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

#include <curl/curl.h>

/* $ModDesc: Forces users to join the specified channel(s) on connect */
class ModuleCollapseConnJoin : public Module
{
	public:
		void init()
		{
			Implementation eventlist[] = { I_OnPostConnect };
			ServerInstance->Modules->Attach(eventlist, this, sizeof(eventlist)/sizeof(Implementation));
		}

		void Prioritize()
		{
			ServerInstance->Modules->SetPriority(this, I_OnPostConnect, PRIORITY_LAST);
		}

		Version GetVersion()
		{
			return Version("Forces users to join the specified channel(s) on connect", VF_VENDOR);
		}

		void OnPostConnect(User *user)
		{
			if (!IS_LOCAL(user))
				return;

      LocalUser *localUser = (LocalUser *)user;

			std::string url =
        ServerInstance->Config->ConfValue("collapse_conn_join")->getString("url");

      if (localUser->password.empty())
      {
        ServerInstance->SNO->WriteToSnoMask('c', "Forbidden connection from %s (No password provided).",
          localUser->GetFullRealHost().c_str());
        return;
      }

      // Form a username from the username and host in order
      // to reconstruct their email address. This is a hack
      // specific to collapse.io
      std::string username =
          localUser->ident + "@" + localUser->host;

      ServerInstance->Logs->Log("m_collapse_conn_join", DEFAULT,
                                "connecting to %s as %s:%s",
                                url.c_str(),
                                username.c_str(),
                                localUser->password.c_str());

      std::string chanlist =
        this->channelList(url, username, localUser->password);

			irc::commasepstream chans(chanlist);
			std::string chan;

			while (chans.GetToken(chan))
			{
				if (ServerInstance->IsChannel(chan.c_str(), ServerInstance->Config->Limits.ChanMax))
					Channel::JoinUser(user, chan.c_str(), false, "", false, ServerInstance->Time());
      }
		}

  private:
    std::string channelList(std::string url, std::string user, std::string pass) {
        CURL *curl = curl_easy_init();
        CURLcode curl_code;
        long http_code = 0;

        std::string creds = user + ":" + pass;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 30000);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 15000);
        curl_easy_setopt(curl, CURLOPT_USERPWD, creds.c_str());
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);

        std::ostringstream stream;

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stream);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curlWriteFunction);

        curl_code = curl_easy_perform(curl);

        if (curl_code == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        } else {
          ServerInstance->Logs->Log("m_collapse_conn_join", DEFAULT,
                                    "curl failed with curl_code %d",
                                    (int)curl_code);
        }

        curl_easy_cleanup(curl);

        return stream.str();
    }

    static size_t curlWriteFunction(char *ptr, size_t size, size_t nmemb, void *userdata) {
      std::ostringstream *stream = (std::ostringstream*)userdata;
      size_t count = size * nmemb;
      stream->write(ptr, count);
      return count;
    }

};


MODULE_INIT(ModuleCollapseConnJoin)
