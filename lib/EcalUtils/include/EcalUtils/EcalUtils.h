/* ========================= eCAL LICENSE =================================
 *
 * Copyright (C) 2016 - 2019 Continental Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ========================= eCAL LICENSE =================================
*/

/**
 * eCALSysCore utilities
**/

#pragma once

#include <cctype>
#include <string>
#include <regex>
#include <list>
#include <vector>

#ifdef _WIN32
#if defined(_MSC_VER) && defined(__clang__) && !defined(CINTERFACE)
#define CINTERFACE
#endif
#include <windows.h>
#include <direct.h>
#endif  // _WIN32

#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#endif  // __linux__

namespace EcalUtils
{
  namespace String
  {
    template <class S>
    S replace(const S& str, const S& from, const S& to, typename S::size_type start = 0)
      /// Replace all occurrences of from (which must not be the empty string)
      /// in str with to, starting at position start.
    {
      S result;
      typename S::size_type pos = 0;
      result.append(str, 0, start);
      do
      {
        pos = str.find(from, start);
        if (pos != S::npos)
        {
          result.append(str, start, pos - start);
          result.append(to);
          start = pos + from.length();
        }
        else result.append(str, start, str.size() - start);
      } while (pos != S::npos);

      return result;
    }

    inline bool icharcompare(char a, char b)
    {
      return(toupper(a) == toupper(b));
    }

    inline bool icompare(const std::string& s1, const std::string& s2)
    {
      return((s1.size() == s2.size()) &&
        equal(s1.begin(), s1.end(), s2.begin(), icharcompare));
    }

    inline void split(const std::string& str, const std::string& delim, std::vector<std::string>& parts)
    {
      size_t end = 0;
      while (end < str.size())
      {
        size_t start = end;
        while (start < str.size() && (delim.find(str[start]) != std::string::npos))
        {
          start++;
        }
        end = start;
        while (end < str.size() && (delim.find(str[end]) == std::string::npos))
        {
          end++;
        }
        if (end - start != 0)
        {
          parts.push_back(std::string(str, start, end - start));
        }
      }
    }

    inline std::string trim(const std::string &s)
    {
      std::string sCopy(s);
      sCopy.erase(sCopy.begin(), std::find_if_not(sCopy.begin(), sCopy.end(), [](char c) { return std::isspace(c); }));
      sCopy.erase(std::find_if_not(sCopy.rbegin(), sCopy.rend(), [](char c) { return std::isspace(c); }).base(), sCopy.end());
      return sCopy;
    }

    template<typename Container>
    inline std::string join(const std::string &delim, Container& parts)
    {
      std::stringstream ss;
      for (auto i = parts.begin(); i != parts.end; i++)
      {
        if (i != parts.begin())
        {
          ss << delim;
        }
        ss << *i;
      }
      return ss.str();
    }

    inline bool CenterString(std::string& str, char padding_char, size_t max_size)
    {
      if (str.length() >= max_size) return false;

      size_t empty_space = max_size - str.length();
      size_t left_padding = empty_space / 2;

      str.insert(0, left_padding, padding_char);
      str.insert(str.length(), empty_space - left_padding, padding_char);

      return true;
    }
  }  // namespace String
  
  namespace Directory
  {
    /**
    * @brief  Checks if directory exists
    *
    * @param  path   directory path
    *
    * @return true if exists, false if it does not
    **/
    inline bool Exists(const std::string& path)
    {
#ifdef _WIN32
      DWORD attribs = ::GetFileAttributesA(path.c_str());
      if (attribs == INVALID_FILE_ATTRIBUTES)
        return false;

      return (attribs & FILE_ATTRIBUTE_DIRECTORY) != 0;
#elif __linux__
      struct stat st;
      if (stat(path.c_str(), &st) == 0)
        if ((st.st_mode & S_IFDIR) != 0)
          return true;
      return false;
#endif  //  __linux__
    }
  }  // namespace Directory

  namespace File
  {
    /**
    * @brief  Checks if file exists
    *
    * @param  path   file path
    *
    * @return true if exists, false if it does not
    **/
    inline bool Exists(const std::string& path)
    {
#ifdef _WIN32
      DWORD attribs = ::GetFileAttributesA(path.c_str());
      return (attribs != INVALID_FILE_ATTRIBUTES);
#elif __linux__
      struct stat st;
      if (stat(path.c_str(), &st) == 0)
        if ((st.st_mode & S_IFREG) != 0)
          return true;
      return false;
#endif  //  __linux__
    }
  }  // namespace File

  namespace Path
  {
    /**
    * @brief  Get file extension from path
    *
    * @param  path   file path
    *
    * @return file extension
    **/
    inline std::string GetExtension(const std::string& path)
    {
      size_t idx_ext = path.find_last_of('.');
      if (idx_ext != std::string::npos)
      {
        return path.substr(idx_ext + 1);
      }
      return "";
    }

    /**
    * @brief  Get file name without extension from path
    *
    * @param  path   file path
    *
    * @return file name without extension
    **/
    inline std::string GetBaseName(const std::string& path)
    {
      size_t idx_base = path.find_last_of('\\');
      if (idx_base == std::string::npos)
      {
        idx_base = path.find_last_of('/');
      }

      if (idx_base != std::string::npos)
      {
        std::string base = path.substr(idx_base + 1);
        size_t idx_ext = base.find_last_of('.');
        if (idx_ext != std::string::npos)
        {
          return base.erase(idx_ext);
        }
        else
        {
          return base;
        }
      }
      return path;
    }

    /**
    * @brief  Get file name with extension from path
    *
    * @param  path   file path
    *
    * @return file name with extension
    **/
    inline std::string GetFileName(const std::string& path)
    {
      size_t idx_name = path.find_last_of('\\');
      if (idx_name == std::string::npos)
      {
        idx_name = path.find_last_of('/');
      }

      if (idx_name != std::string::npos)
      {
        return path.substr(idx_name + 1);
      }
      return path;
    }

    /**
    * @brief  Check if file path is relative
    *
    * @param  path   file path
    *
    * @return true if the path is relative, false otherwise
    **/
    inline bool IsRelative(const std::string &path)
    {
      return (path.find("..") != std::string::npos ||
              path.find("./") != std::string::npos);
    }

    /**
    * @brief  Make path absolute using a base path
    *
    * @param  path   file path
    * @param  base   base path
    **/
    inline void MakeAbsolute(std::string& path, const std::string& base)
    {
      std::string tmp = base;
      size_t idx_path = path.find("..");

      if (idx_path == std::string::npos)
      {
        idx_path = path.find("./");
        if (idx_path != std::string::npos)
        {
          path = tmp + path.substr(idx_path + 1);
        }
      }
      else
      {
        while (idx_path != std::string::npos)
        {
          path = path.substr(idx_path + 2);
          size_t idx_base = tmp.find_last_of('\\');
          if (idx_base == std::string::npos)
          {
            idx_base = tmp.find_last_of('/');
          }
          tmp = tmp.substr(0, idx_base);
          path = tmp + path;

          idx_path = path.find("..");
        }
      }
    }

    /**
    * @brief  Make path absolute from a relative path
    *
    * @param  path   the path
    **/
    inline void MakeAbsolute(std::string& path)
    {
 #ifdef _WIN32
      // make the absolute path to the executable
      char abs_path[MAX_PATH];
      _fullpath(abs_path, path.c_str(), MAX_PATH);
#else
      // absolute path is created only by cutting "." and ".." from the path
      char abs_path[PATH_MAX];
      realpath(path.c_str(), abs_path);
#endif

      path = abs_path;
    }

    #ifdef _WIN32                                                                              
    const std::string separator = "\\";
    const std::string last_folder = "..\\";
    #else      
    const std::string separator = "/";
    const std::string last_folder = "../";
    #endif // _WIN32  

    inline std::string GetRelativePath(const std::string& path, const std::string& base)
    {
      std::vector<std::string> path_vector, base_vector;
      String::split(path, separator, path_vector);
      String::split(base, separator, base_vector);

      size_t size = (path_vector.size() < base_vector.size()) ? path_vector.size() : base_vector.size();
      unsigned int same_size(0);
      for (unsigned int i = 0; i < size; ++i)
      {
        if (path_vector[i] != base_vector[i])
        {
          same_size = i;
          break;
        }
      }

      std::string str_relative_path = "";
      if (same_size > 0)
      {
        for (unsigned int i = 0; i < base_vector.size() - same_size; ++i)
        {
          str_relative_path += last_folder;
        }
      }
        
      for (unsigned int i = same_size; i < path_vector.size(); ++i)
      {
        str_relative_path += path_vector[i];
        if (i < path_vector.size() - 1)
        {
          str_relative_path += separator;
        }
      }

      return str_relative_path;
    }

    inline std::string ExpandEnvVars(const std::string& input)
    {
      std::string output;

      size_t m;
      for (size_t n = 0; n < input.length(); n++)
      {
        switch (input[n])
        {
#ifdef _WIN32
        case '%':
#endif
        case '$':
        {
          enum eBracket
          {
            Bracket_None,
            Bracket_Normal = ')',
            Bracket_Curly = '}',
#ifdef _WIN32
            Bracket_Windows = '%',
#endif
            Bracket_Max
          };
          eBracket bracket;

#ifdef _WIN32
          if (input[n] == '%')
          {
            bracket = Bracket_Windows;
          }
          else
#endif
            if (n == input.length() - 1)
            {
              bracket = Bracket_None;
            }
            else
            {
              switch (input[n + 1])
              {
              case '(':
                bracket = Bracket_Normal;
                n++;                   // skip the bracket
                break;

              case '{':
                bracket = Bracket_Curly;
                n++;                   // skip the bracket
                break;

              default:
                bracket = Bracket_None;
              }
            }

          m = n + 1;

          while (m < input.length() && (isalnum(input[m]) || input[m] == '_'))
            m++;

          std::string var_name = input.substr(n + 1, m - n - 1);
          bool expanded = false;
          char* tmp = getenv(var_name.c_str());
          if (tmp != NULL)
          {
            output += tmp;
            expanded = true;
          }
          else
          {
            // variable doesn't exist => don't change anything
#ifdef _WIN32
            if (bracket != Bracket_Windows)
#endif
              if (bracket != Bracket_None)
              {
                output += input[n - 1];
              }
            output += input[n] + var_name;
          }

          // check the closing bracket
          if (bracket != Bracket_None) {
            if (m == input.length() || input[m] != bracket)
            {
              // under MSW it's common to have '%' characters in the registry
              // and it's annoying to have warnings about them each time, so
              // ignore them silently if they are not used for env vars
              //
              // under Unix, OTOH, this warning could be useful for the user to
              // understand why isn't the variable expanded as intended
//#ifndef _WIN32
              //EcalSysLogger::Log(std::string("Environment variables expansion failed: missing '") + static_cast<char>(bracket) + std::string("' at position ") + std::to_string((unsigned int)(m + 1)) + " in '" + input + "'.", spdlog::level::info);
//#endif
            }
            else
            {
              // skip closing bracket unless the variables wasn't expanded
              if (!expanded)
              {
                output += (char)bracket;
              }
              m++;
            }
          }

          n = m - 1;  // skip variable name
        }
        break;

        case '\\':
          // backslash can be used to suppress special meaning of % and $
          if (n != input.length() - 1 && (input[n + 1] == '%' || input[n + 1] == '$'))
          {
            output += input[++n];
            break;
          }
          else
          {
            output += input[n];
          }
          break;

        default:
          output += input[n];
        }
      }
      return output;
    }
  }  // namespace Path

  namespace CommandLine
  {
    /**
     * @brief Searches the string for the start of the next argument by skipping all spaces
     *
     * @param arg_string  The command line as string
     * @param start_at    Where to start looking for the next non-space character
     *
     * @return The start of the next argument or std::string::npos, if there is no further argument.
     */
    inline size_t GetStartOfNextArgument(const std::string& arg_string, size_t start_at)
    {
      size_t pos = start_at;

      // Find beginning of arguments, if there are whitespaces at the front (which probably will never happen anyway)
      while (pos < arg_string.size())
      {
        if (std::isspace(arg_string.at(pos)))
        {
          pos++;
        }
        else
        {
          break;
        }
      }

      // If there are no further arguments, we return std::string::npos (we cannot return -1, as we are using size_t)
      if (pos == arg_string.size())
      {
        return std::string::npos;
      }

      return pos;
    }

    /**
     * @brief Searches the given argument string for the end of argument that starts at the given start point
     *
     * Note: This function is intended for working with Windows and Linux style
     * coomand lines. There are however some differences between those two
     * styles that might hurt compatibility:
     *
     * - On Windows:
     *      - only double quote (") qualifies as quote (to enclose a string)
     *      - quotes can be escaped by a backslash (\" will result in a quote)
     *      - The backslash is otherwise used as path separator (it does not escape a space)
     *
     * - On Linux:
     *      - quotes can be double or single quotes (" or ') to enclose a string
     *      - quotes can be escaped by a backslash (\" will result in a quote)
     *      - The backslash is used as separator only (it can also escape spaces)
     *
     * - Combined rules:
     *      - A backslash will escape whatever comes after it (=> like Linux. A backslash as the last character might happen for paths to diretories on Windows however, which would lead to errors)
     *      - strings can be in double or single quotes (=> like Linux, hoping that nobody will use single quotes on Windows)
     *
     * @param arg_string  the command line string
     * @param start_at    the start position of the current argument
     *
     * @return The end of the next argument
     */
    inline size_t GetEndOfNextArgument(const std::string& arg_string, size_t start_at)
    {
      size_t pos = start_at;
      bool double_quote = false;
      bool single_quote = false;
      // Find the end of the argument
      while (pos < arg_string.size())
      {
        if (arg_string.at(pos) == '\\')
        {
          // Escape / skip the next char
          pos += 2;
          continue;
        }
        else if (arg_string.at(pos) == ' ' && !double_quote && !single_quote)
        {
          // First space outside a string terminates the argument
          // As what we have actually done is finding the next space, we have to decrement the position again in order to get the argument end
          pos--;
          break;
        }
        else if (arg_string.at(pos) == '\"')
        {
          if (!single_quote) {
            // Start or end a double quoted string
            double_quote = !double_quote;
          }
          pos++;
        }
        else if (arg_string.at(pos) == '\'')
        {
          if (!double_quote) {
            // Start or end a single quoted string
            single_quote = !single_quote;
          }
          pos++;
        }
        else
        {
          pos++;
        }
      }

      // If the command line is not well-formed, the backslash might have gotten us outside the string
      if (pos > arg_string.size() - 1)
      {
        pos = arg_string.size() - 1;
      }

      return pos;
    }

    /**
     * @brief Splits the given command line into its arguments.
     *
     * The maximum number of splits can be set with the max_number_of_arguments
     * parameter. If set, the function will stop splitting the command line
     * and return all remaining arguments in the last element of the list.
     *
     * If the command line does not contain as many arguments as the
     * max_number_of_arguments parameter would indicate, the returned argument
     * list will be shorter!
     *
     * For the rules of splitting see @see{GetEndOfNextArgument()}
     *
     * @param input_command_line       The command line to split
     * @param max_number_of_arguments  The maximum desired number of splits. Can be 0, in order to not stop splitting prematurely.
     *
     * @return A list of arguments
     */
    inline std::vector<std::string> splitCommandLine(const std::string& input_command_line, size_t max_number_of_arguments = 0)
    {
      std::vector<std::string> argument_list;

      size_t next_part_start = 0;

      for (size_t i = 0; (max_number_of_arguments == 0) || (i < max_number_of_arguments); i++)
      {
        // Check if we are already outside the string
        if (next_part_start >(input_command_line.size() - 1))
        {
          break;
        }

        // Get the actual start of the next argument
        size_t argument_start = GetStartOfNextArgument(input_command_line, next_part_start);

        if (argument_start != std::string::npos)
        {
          // Get the end of the next argument
          size_t argument_end = GetEndOfNextArgument(input_command_line, argument_start);

          if (argument_end != std::string::npos)
          {
            if ((max_number_of_arguments == 0)
              || (i != max_number_of_arguments - 1))
            {
              // Add the argument to the argument list
              argument_list.push_back(input_command_line.substr(argument_start, argument_end - argument_start + 1));
              next_part_start = argument_end + 1;
            }
            else
            {
              // Add all remaining arguments as one big block
              argument_list.push_back(input_command_line.substr(argument_start));
            }
          }
          else
          {
            break;
          }
        }
        else
        {
          // The Command line might not contain enough arguments, especiall if the user did not set a maximum number of arguments
          break;
        }
      }
      return argument_list;
    }

    std::vector<std::string> ToArgv(const std::string& command_line)
    {
      std::vector<std::string> argv;

      bool quote = false;
      bool squote = false;
      bool inside_arg = false;

      std::string current_arg;

      size_t pos = 0;
      while (pos < command_line.size())
      {
        char current_char = command_line.at(pos);

        if (inside_arg && !quote && !squote && std::isspace(current_char))
        {
          // first space outside a quoted string terminates the argument
          argv.insert(argv.end(), current_arg);
          current_arg.clear();
          inside_arg = false;
        }
        else if (current_char == '\'' && !quote)
        {
          // Only evaluate single-quotes if we are not currently in a double-quoted string
          inside_arg = true;
          squote = !squote;
        }
        else if (current_char == '\"' && !squote)
        {
          // Only evaluate double-quotes if we are not currently in a single-quoted string
          inside_arg = true;
          quote = !quote;
        }
        else if (current_char == '\\')
        {
          // Copy the next character and skip its evaluation
          if ((pos + 1) < command_line.size())
          {
            current_arg += command_line.at(pos + 1);
          }
          inside_arg = true;
          pos++;
        }
        else if (quote || squote)
        {
          // as long as we are in a quoted string we simply copy everything
          current_arg += current_char;
        }
        else if (!std::isspace(current_char))
        {
          // when we are not in a quoted string and get a non-space character we copy it
          current_arg += current_char;
          inside_arg = true;
        }

        pos++;
      }

      // Add the last arg
      if (inside_arg)
      {
        argv.insert(argv.end(), current_arg);
      }
      return argv;
    }
  }
}   // namespace Utility
