#ifndef __UTIL_FS_DIRENUMERATOR_HPP
#define __UTIL_FS_DIRENUMERATOR_HPP

#include <string>
#include <vector>
#include "fs/path.hpp"
#include "util/status.hpp"
#include "fs/owner.hpp"

namespace acl
{
class User;
}

namespace fs
{

class DirEntry
{
  fs::Path path;
  util::path::Status status;
  fs::Owner owner;
  
public:  
  explicit DirEntry(const fs::Path& path, const util::path::Status& status,
                    const fs::Owner& owner) :
    path(path), status(status), owner(owner) { }

  const fs::Path& Path() const { return path; }
  const util::path::Status& Status() const { return status; }
  const fs::Owner& Owner() const { return owner; }
};

class DirEnumerator
{
  const acl::User* user;
  fs::RealPath path;
  unsigned long long totalBytes;
  bool loadOwners;
  
  std::vector<DirEntry> entries;
  
  void Readdir();
  
public:
  typedef std::vector<DirEntry>::const_iterator const_iterator;
  typedef std::vector<DirEntry>::iterator iterator;
  typedef std::vector<DirEntry>::size_type size_type;

  explicit DirEnumerator();
  explicit DirEnumerator(const fs::Path& path, bool loadOwners = true);
  explicit DirEnumerator(const acl::User& user, const fs::VirtualPath& path, bool loadOwners = true);
  
  void Readdir(const fs::Path& path, bool loadOwners = true);
  void Readdir(const acl::User& user, const fs::VirtualPath& path, bool loadOwners = true);

  uintmax_t TotalBytes() const { return totalBytes; }
  
  const_iterator begin() const { return entries.begin(); }
  const_iterator end() const { return entries.end(); }
  iterator begin() { return entries.begin(); }
  iterator end() { return entries.end(); }
  size_type size() const { return entries.size(); }
  bool empty() const { return entries.empty(); }
};

struct DirEntryPathLess
{
  bool operator()(const DirEntry& de1, const DirEntry& de2)
  { return de1.Path() < de2.Path(); }  
};

struct DirEntryPathGreater
{
  bool operator()(const DirEntry& de1, const DirEntry& de2)
  { return de1.Path() > de2.Path(); }  
};

struct DirEntrySizeLess
{
  bool operator()(const DirEntry& de1, const DirEntry& de2)
  { return de1.Status().Size() < de2.Status().Size(); }  
};

struct DirEntrySizeGreater
{
  bool operator()(const DirEntry& de1, const DirEntry& de2)
  { return de1.Status().Size() > de2.Status().Size(); }  
};

struct DirEntryModTimeLess
{
  bool operator()(const DirEntry& de1, const DirEntry& de2)
  { return de1.Status().Native().st_mtime < de2.Status().Native().st_mtime; }  
};

struct DirEntryModTimeGreater
{
  bool operator()(const DirEntry& de1, const DirEntry& de2)
  { return de1.Status().Native().st_mtime > de2.Status().Native().st_mtime; }  
};

} /* fs namespace */

#endif
