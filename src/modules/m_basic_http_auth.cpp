#include "inspircd.h"

#include <curl/curl.h>

/* $ModDesc: Provides HTTP Basic Authentication support for clients */

/* $LinkerFlags:  -lcurl */

/* $NoPedantic */

class ModuleBasicHttpAuth : public Module
{
	LocalIntExt basicHttpAuthed;

    /** The URL to send HTTP Basic Auth to */
    std::string url;

    /** The message sent to the client upon auth failure */
    std::string killreason;
 
  public:
	ModuleBasicHttpAuth()
		: basicHttpAuthed("basic_http_auth", this)
	{
	}

	void init()
	{
		ServerInstance->Modules->AddService(basicHttpAuthed);
		Implementation eventlist[] = { I_OnCheckReady, I_OnRehash, I_OnUserRegister, I_OnUserConnect };
		ServerInstance->Modules->Attach(eventlist, this, sizeof(eventlist)/sizeof(Implementation));
		OnRehash(NULL);
	}

	void OnRehash(User* user)
	{
		ConfigTag* conf = ServerInstance->Config->ConfValue("basic_http_auth");
		url = conf->getString("url");
		killreason = conf->getString("killreason");
	}

	virtual void OnUserConnect(LocalUser *user)
    {
    }

	ModResult OnUserRegister(LocalUser* user)
	{
        if (true == checkCredentials(user))
        {
            basicHttpAuthed.set(user,1);
            return MOD_RES_PASSTHRU;
        }

        ServerInstance->Users->QuitUser(user, killreason);
        return MOD_RES_DENY;
	}

	ModResult OnCheckReady(LocalUser* user)
	{
		return basicHttpAuthed.get(user) ? MOD_RES_PASSTHRU : MOD_RES_DENY;
	}


	Version GetVersion()
	{
		return Version("Allow/Deny connections based upon HTTP Basic Authentication", VF_VENDOR);
	}

  private: 
    bool checkCredentials(LocalUser *user) {
        if (user->password.empty())
		{
            ServerInstance->SNO->WriteToSnoMask('c', "Forbidden connection from %s (No password provided)", user->GetFullRealHost().c_str());
			return false;
		}

        // Form a username from the username and host in order
        // to reconstruct their email address. This is a hack
        // specific to collapse.io
        std::string username =
            user->ident + "@" + user->host;

        long http_code =
            authenticationStatus(username, user->password);

        if (200 == http_code) {
            return true;
        }

        ServerInstance->SNO->WriteToSnoMask('c',
            "Forbidden connection from %s with username %s and code %L",
            user->GetFullRealHost().c_str(),
            username.c_str(),
            http_code);

        return false;
    }

    long authenticationStatus(std::string user, std::string pass) {
        CURL *curl = curl_easy_init();
        CURLcode curl_code;
        long http_code = 0;

        std::string creds = user + ":" + pass;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 30000);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 15000);
        curl_easy_setopt(curl, CURLOPT_USERPWD, creds.c_str());

        curl_code = curl_easy_perform(curl);

        if (curl_code == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            curl_easy_cleanup(curl);
        }

        return http_code;
    }

};

MODULE_INIT(ModuleBasicHttpAuth)
