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

    /** The URL to go to in order to register for an account */
    std::string registration_url;
 
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
        registration_url = conf->getString("registration_url");
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
  bool checkCredentials(LocalUser *user)
  {
    if (user->password.empty())
		{
      ServerInstance->SNO->WriteToSnoMask('c', "Forbidden connection from %s (No password provided). Please register at %s.",
        user->GetFullRealHost().c_str(),
        registration_url.c_str());
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

    if (401 == http_code) {
        ServerInstance->SNO->WriteToSnoMask('c',
            "Authentication failed for %s. Visit %s if you've forgotten your password.",
            username.c_str(),
            registration_url.c_str());
    } else {
        ServerInstance->SNO->WriteToSnoMask('c',
            "Error during authentication with code (%d). Visit %s for help.",
            (int)http_code, registration_url.c_str());
    }

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
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);

        curl_code = curl_easy_perform(curl);

        if (curl_code == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        } else {
          ServerInstance->Logs->Log("m_basic_http_auth", DEFAULT,
                                    "curl failed with curl_code %d",
                                    (int)curl_code);
        }

        curl_easy_cleanup(curl);

        return http_code;
    }
};

MODULE_INIT(ModuleBasicHttpAuth)
