#ifndef __CMD_SITE_ABOR_HPP
#define __CMD_SITE_ABOR_HPP

#include "cmd/command.hpp"

namespace cmd { namespace site
{

class EPSVCommand : public Command
{
public:
  EPSVCommand(ftp::Client& client, const std::string& argStr, const Args& args) :
    Command(client, client.Control(), client.Data(), argStr, args) { }

  void Execute();
};

} /* rfc namespace */
} /* cmd namespace */

#endif
