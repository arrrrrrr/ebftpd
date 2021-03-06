#include <iostream>
#include <sstream>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/optional.hpp>
#include "cfg/get.hpp"
#include "cfg/error.hpp"
#include "version.hpp"
#include "text/error.hpp"
#include "text/parser.hpp"
#include "cmd/online.hpp"
#include "ftp/online.hpp"

namespace po = boost::program_options;

void DisplayHelp(char* argv0, po::options_description& desc)
{
  std::cout << "usage: " << argv0 << " [options]" << std::endl;
  std::cout << desc;
}

void DisplayVersion()
{
  std::cout << "ebftpd ranks " + std::string(version) << std::endl;
}

bool ParseOptions(int argc, char** argv, std::string& configPath, 
                  bool& siteop, bool& raw, std::string& templatePath)
{
  po::options_description visible("supported options");
  visible.add_options()
    ("help,h", "display this help message")
    ("version,v", "display version")
    ("config-path,c", po::value<std::string>(&configPath), "specify location of config file")
    ("siteop,s", po::value<bool>(&siteop)->default_value(false), "site who")
    ("raw,r", po::value<bool>(&raw)->default_value(false), "raw formatting")
    ("template,y", po::value<std::string>(&templatePath), "template file path")
  ;
  
  po::variables_map vm;
  try
  {
    po::store(po::command_line_parser(argc, argv).options(visible).run(), vm);

    if (vm.count("help"))
    {
      DisplayHelp(argv[0], visible);
      return false;
    }

    if (vm.count("version"))
    {
      DisplayVersion();
      return false;
    }

    po::notify(vm);
  }
  catch (const po::error& e)
  {
    std::cerr << e.what() << std::endl;
    DisplayHelp(argv[0], visible);
    return false;
  }
  
  return true;
}


int main(int argc, char** argv)
{
  bool siteop;
  bool raw;
  std::string configPath;
  std::string templatePath;

  if (!ParseOptions(argc, argv, configPath, siteop, raw, templatePath))
  {
    return 1;
  }

  try
  {
    cfg::UpdateShared(cfg::Config::Load(configPath, true));
  }
  catch (const cfg::ConfigError& e)
  {
    std::cerr << "Failed to load config: " << e.Message() << std::endl;
    return 1;
  }
  
  if (cfg::Get().Pidfile().empty())
  {
    std::cerr << "There must be a pid file set in your config to use this tool." << std::endl;
    return 1;
  }
  
  std::string id;
  if (!ftp::SharedMemoryID(cfg::Get().Pidfile(), id))
  {
    std::cerr << "Unable to load pid from pid file: " << cfg::Get().Pidfile() << std::endl;
    return 1;
  }
  
  boost::optional<text::Template> templ;
  if (!templatePath.empty())
  {
    try
    {
      text::TemplateParser parser(templatePath);
      templ.reset(parser.Create());
    }
    catch (const text::TemplateError& e)
    {
      std::cerr << "Unable to load template file: " << e.Message() << std::endl;
      return 1;
    }
  }
  else
  {
    std::string tmplPath(cfg::Get().Datapath());
    tmplPath += "/text/";
    if (raw) tmplPath += "raw";
    
    if (siteop) tmplPath += "swho";
    else tmplPath += "who";
    
    tmplPath += ".tmpl";
    
    try
    {
      text::TemplateParser parser(tmplPath);
      templ.reset(parser.Create());
    }
    catch (const text::TemplateError& e)
    {
      std::cerr << "Unable to load template file: " << e.Message() << std::endl;
      return 1;
    }
  }

  std::cout << cmd::CompileWhosOnline(id, *templ);
  
  return 0;
}
