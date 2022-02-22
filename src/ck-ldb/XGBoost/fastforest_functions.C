/**
MIT License
Copyright (c) 2020 Jonas Rembser
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "fastforest.h"
#include "common_details.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <stdexcept>

#include <iostream>

using namespace fastforest;

namespace {

    namespace util {

        inline bool isInteger(const std::string& s) {
          if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+')))
            return false;

          char* p;
          strtol(s.c_str(), &p, 10);

          return (*p == 0);
        }

        template <class NumericType>
        struct NumericAfterSubstrOutput {
            explicit NumericAfterSubstrOutput() : value{0}, found{false}, failed{true} {}
            NumericType value;
            bool found;
            bool failed;
            std::string rest;
        };

        template <class NumericType>
        inline NumericAfterSubstrOutput<NumericType> numericAfterSubstr(std::string const& str,
                                                                        std::string const& substr) {
          std::string rest;
          NumericAfterSubstrOutput<NumericType> output;
          output.rest = str;

          auto found = str.find(substr);
          if (found != std::string::npos) {
            output.found = true;
            std::stringstream ss(str.substr(found + substr.size(), str.size() - found + substr.size()));
            ss >> output.value;
            if (!ss.fail()) {
              output.failed = false;
              output.rest = ss.str();
            }
          }
          return output;
        }

        std::vector<std::string> split(std::string const& strToSplit, char delimeter) {
          std::stringstream ss(strToSplit);
          std::string item;
          std::vector<std::string> splittedStrings;
          while (std::getline(ss, item, delimeter)) {
            splittedStrings.push_back(item);
          }
          return splittedStrings;
        }

        bool exists(std::string const& filename) {
          if (FILE* file = fopen(filename.c_str(), "r")) {
            fclose(file);
            return true;
          } else {
            return false;
          }
        }

    }  // namespace util

    void terminateTree(fastforest::FastForest& ff,
                       int& nPreviousNodes,
                       int& nPreviousLeaves,
                       fastforest::detail::IndexMap& nodeIndices,
                       fastforest::detail::IndexMap& leafIndices,
                       int& treesSkipped) {
      using namespace fastforest::detail;
      correctIndices(ff.rightIndices_.begin() + nPreviousNodes, ff.rightIndices_.end(), nodeIndices, leafIndices);
      correctIndices(ff.leftIndices_.begin() + nPreviousNodes, ff.leftIndices_.end(), nodeIndices, leafIndices);

      if (nPreviousNodes != ff.cutValues_.size()) {
        ff.treeNumbers_.push_back(ff.rootIndices_.size() + treesSkipped);
        ff.rootIndices_.push_back(nPreviousNodes);
      } else {
        int treeNumbers = ff.rootIndices_.size() + treesSkipped;
        ++treesSkipped;
        ff.baseResponses_[treeNumbers % ff.baseResponses_.size()] += ff.responses_.back();
        ff.responses_.pop_back();
      }

      nodeIndices.clear();
      leafIndices.clear();
      nPreviousNodes = ff.cutValues_.size();
      nPreviousLeaves = ff.responses_.size();
    }

}  // namespace