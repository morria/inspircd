#include "inspircd.h"
#include "aws4c.h"

/* $ModDesc: Allows for the logging of all messages */

class ModuleSQS: public Module
{
 private:
   /** URL for SQS queue holding messages  */
   std::string messageQueueUrl;

   /** URL for SQS queue holding messages  */
   std::string channelMembersQueueUrl;

   /** AWS Access Key */
   std::string awsAccessKey;

   /** AWS Secret Key */
   std::string awsSecretKey;

 public:
	void init()
	{
		Implementation eventlist[] = { I_OnUserMessage, I_OnUserJoin, I_OnUserPart, I_OnRehash };
		ServerInstance->Modules->Attach(eventlist, this, sizeof(eventlist)/sizeof(Implementation));

    aws_init();
    aws_set_debug(1);

		OnRehash(NULL);
	}

	void OnRehash(User *user)
	{
		ConfigTag* conf = ServerInstance->Config->ConfValue("sqs");

		messageQueueUrl = conf->getString("message_queue_url");
		channelMembersQueueUrl = conf->getString("channel_members_queue_url");

    awsAccessKey = conf->getString("aws_access_key");
    awsSecretKey = conf->getString("aws_secret_key");

    aws_set_keyid(awsAccessKey.c_str());
    aws_set_key(awsSecretKey.c_str());
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

      IOBuf *bf = aws_iobuf_new();

      char message[2048];
      snprintf(message, sizeof(message), "%s\t%s\t%s",
               user->nick.c_str(),
               target_name.c_str(),
               text.c_str());

      int rv = sqs_send_message(bf,
                                (char *)messageQueueUrl.c_str(),
                                (char *)message);

      if ( rv || bf->code != 200 ) {
        ServerInstance->Logs->Log("MODULE", DEFAULT,
                                  "Failed to send SQS message [%d]: %s.",
                                  bf->code, bf->result);
      }

      aws_iobuf_free(bf);
	}

    void OnUserJoin(Membership* memb, bool sync, bool created, CUList& except)
    {
      IOBuf *bf = aws_iobuf_new();

      char message[2048];
      snprintf(message, sizeof(message), "JOIN\t%s\t%s",
              memb->chan->name.c_str(),
              memb->user->nick.c_str());

      int rv = sqs_send_message(bf,
                                (char *)channelMembersQueueUrl.c_str(),
                                (char *)message);
      if ( rv || bf->code != 200 ) {
        ServerInstance->Logs->Log("MODULE", DEFAULT,
                                  "Failed to send SQS message [%d]: %s.",
                                  bf->code, bf->result);
      }

      aws_iobuf_free(bf);
    }

    void OnUserPart(Membership* memb, std::string &partmessage, CUList& except)
    {
      IOBuf *bf = aws_iobuf_new();

      char message[2048];
      snprintf(message, sizeof(message), "PART\t%s\t%s",
              memb->chan->name.c_str(),
              memb->user->nick.c_str());

      int rv = sqs_send_message(bf,
                                (char *)channelMembersQueueUrl.c_str(),
                                (char *)message);
      if ( rv || bf->code != 200 ) {
        ServerInstance->Logs->Log("MODULE", DEFAULT,
                                  "Failed to send SQS message [%d]: %s.",
                                  bf->code, bf->result);
      }

      aws_iobuf_free(bf);
    }

	Version GetVersion()
	{
		return Version("Provides for logging of all messages", VF_VENDOR);
	}
};

MODULE_INIT(ModuleSQS)
