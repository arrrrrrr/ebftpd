#ifndef __FTP_CONTROLIMPL_HPP
#define __FTP_CONTROLIMPL_HPP

#include <string>
#include "ftp/replycodes.hpp"
#include "util/net/tcpsocket.hpp"
#include "util/pipe.hpp"
#include "ftp/format.hpp"

namespace boost
{
namespace posix_time
{
class time_duration;
}
}

namespace util { namespace net
{
class TCPListener;
}
}

namespace ftp
{

class ControlImpl
{
  util::net::TCPSocket socket;
  ReplyCode lastCode;
  std::string commandLine;
  bool singleLineReplies;
  std::vector<std::string> deferred;
  
  long bytesRead;
  long bytesWrite;
  
  void SendReply(ReplyCode code, bool part, const std::string& message);
  void MultiReply(ReplyCode code, bool final, const std::vector<std::string>& messages);
  void MultiReply(ReplyCode code, bool final, const std::string& messages);
  
  size_t Read(char* buffer, size_t size)
  { 
    size_t len = socket.Read(buffer, size);
    if (len > 0) bytesRead += len;
    return len;
  }

public:  
  ControlImpl(util::net::TCPSocket** socket);
  
  void Accept(util::net::TCPListener& listener);
 
  std::string NextCommand(const boost::posix_time::time_duration* timeout = nullptr);
  
  void PartReply(ReplyCode code, const std::string& message);
  void Reply(ReplyCode code, const std::string& message);

  void SetSingleLineReplies(bool singleLineReplies)
  { this->singleLineReplies = singleLineReplies; }
  
  bool SingleLineReplies() const { return singleLineReplies; }
  
  void NegotiateTLS();
  
  void Write(const char* buffer, size_t len)
  {
    socket.Write(buffer, len);
    bytesWrite += len;
  }
  
  const util::net::Endpoint& RemoteEndpoint() const
  { return socket.RemoteEndpoint(); }
  
  const util::net::Endpoint& LocalEndpoint() const
  { return socket.LocalEndpoint(); }
  
  bool IsTLS() const { return socket.IsTLS(); }
  std::string TLSCipher() const { return socket.TLSCipher(); }
  
  void Interrupt() { socket.Shutdown(); }
  
  long long BytesRead() const { return bytesRead; }
  long long BytesWrite() const { return bytesWrite; }
  
  std::string WaitForIdnt();
};

} /* ftp namespace */

#endif
