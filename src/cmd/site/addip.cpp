#include <boost/optional/optional.hpp>
#include <sstream>
#include "cmd/site/addip.hpp"
#include "util/error.hpp"
#include "acl/usercache.hpp"
#include "acl/ipmaskcache.hpp"
#include "acl/secureip.hpp"
#include "acl/ipstrength.hpp"
#include "acl/allowsitecmd.hpp"
#include "cmd/error.hpp"
#include "text/factory.hpp"
#include "text/template.hpp"
#include "text/templatesection.hpp"
#include "text/error.hpp"

namespace cmd { namespace site
{

void ADDIPCommand::Execute()
{
  if (args[1] != client.User().Name() &&
     !acl::AllowSiteCmd(client.User(), "addip"))
  {
    throw cmd::PermissionError();
  }
  
  acl::User user;
  try
  {
    user = acl::UserCache::User(args[1]);
  }
  catch (const util::RuntimeError& e)
  {
    control.Reply(ftp::ActionNotOkay, e.Message());
    return;
  }

  boost::optional<text::Template> templ;
  try
  {
    templ.reset(text::Factory::GetTemplate("addip"));
  }
  catch (const text::TemplateError& e)
  {
    control.Reply(ftp::ActionNotOkay, e.Message());
    return;
  }

  std::ostringstream os;

  text::TemplateSection& head = templ->Head();
  text::TemplateSection& body = templ->Body();
  text::TemplateSection& foot = templ->Foot();

  acl::IPStrength strength;
  std::vector<std::string> deleted;
  for (Args::iterator it = args.begin()+2; it != args.end(); ++it)
  {
    if (it != args.begin()+2) os << "\n";
    if (!acl::SecureIP(client.User(), *it, strength))
    {
      os << "Error adding " << *it << ": Must contain " 
         << strength.NumOctets() << " octets, ";
      if (strength.HasIdent()) os << "have an ident, ";
      if (!strength.IsHostname()) os << "not be a hostname.";
    }
    else
    {
      util::Error ok = acl::IpMaskCache::Add(user, *it, deleted);
      if (!ok)
        os << "Error adding " << *it << ": " << ok.Message();
      else
      {
        os << "IP '" << *it << "' successfully added to " << args[1] << ".";

        for (auto& del: deleted)
          os << "Auto-removing unnecessary IP '" << del << "'...";
      }
    }
  }

  body.RegisterValue("msg", os.str());
  
  os.str(std::string());

  os << head.Compile() << body.Compile() << foot.Compile();

  control.Reply(ftp::CommandOkay, os.str());
  return;
} 

// end 
}
}
