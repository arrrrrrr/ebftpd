#include "util/string.hpp"
#include <fstream>
#include <memory>
#include "text/parser.hpp"
#include "text/templatesection.hpp"
#include "text/error.hpp"
#include "logs/logs.hpp"

namespace text
{

Template TemplateParser::Create()
{
  std::ifstream io(file.c_str()); 
  if (!io) throw TemplateError("Unable to open template file: " + file);

  std::stringstream ss;

  while (io.good())
  {
    char c;
    if (io >> std::noskipws >> c)
      buf.Stream() << c;
  }

  buf.Parse();

  return buf.GetTemplate();
}

void TemplateBuffer::ParseInclude(const std::string& file)
{
  std::ifstream io(file.c_str());
  if (!io) throw TemplateError("Unable to open include file (" + file + ")");

  state = TemplateState::None;

  while (io.good())
  {
    char c;
    if (io >> std::noskipws >> c)
      ParseState(c);
  }

  state = TemplateState::None;
}


void TemplateBuffer::Parse()
{ 
  while (ss.good())
  {
    char c = ss.get();
    ParseState(c);
  }
}

void TemplateBuffer::ParseState(char c)
{  
  switch (state)
  {
    case TemplateState::Escape:
      buffer << c; 
      state = TemplateState::None;
      return;

    case TemplateState::Skip:
      if (c == '\r' || c == '}' || c == ' ') return;
      state = TemplateState::None;
      if (c == '\n') return;
      break;

    case TemplateState::Close:
      if (c != '}')
        throw TemplateMalform(linePos, charPos);
      state = TemplateState::None;
      return; /* don't need this in the buffer */

    case TemplateState::Open:
    case TemplateState::None:
    case TemplateState::ReadLogic:
    case TemplateState::ReadFilter:
    default:
      break;
  }

  ParseChar(c);
}
        
void TemplateBuffer::ParseChar(char c)
{
  ++charPos;

  switch (c)
  {
    case '\r':
      // ignore carriage return
      return;
      
    case '\n':
      charPos = 0;
      ++linePos;
      break;

    case '\\':
      if (state == TemplateState::None)
      {
        state = TemplateState::Escape;
        return;
      }

    case '{':
      if (state == TemplateState::Open)
      {
        state = TemplateState::ReadFilter;
        return; // we don't need to capture the {
      }
      else
      {
        state = TemplateState::Open;
        return;
      }
      break;

    case '%':
      if (state == TemplateState::ReadLogic)
      {
        ParseLogic();
        state = TemplateState::Skip;
        return; // we don't need to capture this %
      }
      else if (state == TemplateState::Open)
      {
        state = TemplateState::ReadLogic;
        return; // we don't need to capture this %
      }
      break;

    case '}':
      if (state == TemplateState::ReadFilter)
      {
        ParseFilter();
        state = TemplateState::Close;
        return; // we don't need to capture this }
      }
      break;

    default:
      break;
  }

  if (state == TemplateState::ReadLogic ||
      state == TemplateState::ReadFilter)
  {
    var << c;
  }
  else
  {
    buffer << c;
  }
}

void TemplateBuffer::ParseBlock()
{
  if (block == TemplateBlock::Head)
    templ.Head().RegisterBuffer(buffer.str());
  else if (block == TemplateBlock::Body)
    templ.Body().RegisterBuffer(buffer.str());
  else if (block == TemplateBlock::Foot)
    templ.Foot().RegisterBuffer(buffer.str());
  
  /* reset */
  state = TemplateState::None;
  block = TemplateBlock::Head;
  linePos = 1;
  charPos = 1;
  var.str(std::string());
  buffer.str(std::string());
}

void TemplateBuffer::ParseLogic()
{
  std::string logic = var.str();

  /* cleanup */
  var.str(std::string());

  util::Trim(logic);
  util::ToLower(logic);
 
  std::vector<std::string> args; 
  util::Split(args, logic, " ");
  logic = args[0];
  

  if (logic == "endblock")
    ParseBlock();

  else if (logic == "head")
  {
    buffer.str(std::string()); /* clear buffer because the start of a new block */
    block = TemplateBlock::Head; 
  }
  else if (logic == "body")
  {
    buffer.str(std::string());
    block = TemplateBlock::Body; 
  }
  else if (logic == "foot")
  {
    buffer.str(std::string());
    block = TemplateBlock::Foot; 
  }
  else if (logic == "include")
  {
    if (args.size() != 2)
      throw TemplateMalform(linePos, charPos, "(include requires file to import!)");
    std::string file = args[1];
    util::TrimLeftIf(file, "\"");
    util::TrimRightIf(file, "\"");
    ParseInclude(file);
  }
  else
    throw TemplateMalform(linePos, charPos, "(" + logic + ")"
      + " incorrect syntax. Must be {% endblock|head|body|foot %}");
}

void TemplateBuffer::ParseFilter()
{
  std::string filter = var.str();

  /* cleanup */
  var.str(std::string());

  util::Trim(filter);
  util::ToLower(filter);

  buffer << "{{";

  if (block == TemplateBlock::Head)
    buffer << templ.Head().RegisterTag(filter);
  
  else if (block == TemplateBlock::Body)
    buffer << templ.Body().RegisterTag(filter);

  else if (block == TemplateBlock::Foot)
    buffer << templ.Foot().RegisterTag(filter);

   buffer << "}}";

}

}
