/******************************************************************************
Copyright (c) 2018 Georgia Instititue of Technology
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
*******************************************************************************/

/*
 * Adapted from MAESTRO options
 */

#ifndef OPTION_H
#define OPTION_H

#include <iostream>
#include <list>
#include <string>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>

namespace Utility {

  namespace po = boost::program_options;

  class Options {
    public:
      std::string logfile = "";

      // Cache options
      unsigned long long int cache_line_data_width = 64; // Bytes
      unsigned long long int cache_num_lines = 1024;
      unsigned long long int cache_num_set_associative_way = 8;
      
      // Scratchpad options
      unsigned long long int scratchpad_line_data_width = 64; // Bytes
      unsigned long long int scratchpad_num_lines = 16384; // lines
      unsigned long long int scratchpad_num_set_associative_way = 8;
      unsigned long long int scratchpad_read_latency = 1;
      unsigned long long int scratchpad_write_latency = 1;
      unsigned long long int scratchpad_num_simultaneous_requests = 4;

      // Simultaion Options
      unsigned long long int num_iter = 10000;
      unsigned long long int num_pipelines = 1;
      unsigned long long int num_dst_readers = 4;
      int shouldInit = 0; // Used for the readGraph
      std::string graph_path = "";
      std::string app = "";
      std::string result = "";

      std::vector<std::string> token;
      

      bool parse(long long int argc, char** argv)
      {
          std::vector<std::string> config_fnames;

          po::options_description desc("General Options");
          desc.add_options()
            ("help", "Display help message")
          ;

          po::options_description io("File IO options");
          io.add_options()
            ("logfile", po::value<std::string>(&logfile) ,"the name of the log file to write to")
          ;

          po::options_description cache("Cache Options");
          cache.add_options()
            ("cache_line_data_width", po::value<unsigned long long int>(&cache_line_data_width), "cache line data width in bytes")
            ("cache_num_lines", po::value<unsigned long long int>(&cache_num_lines), "number of cache lines")
            ("cache_num_set_associative_way", po::value<unsigned long long int>(&cache_num_set_associative_way), "number of cache set associative way")
          ;

          po::options_description scratch("Scratchpad Options");
          scratch.add_options()
            ("scratchpad_line_data_width", po::value<unsigned long long int>(&scratchpad_line_data_width), "scratchpad line data width in bytes")
            ("scratchpad_num_lines", po::value<unsigned long long int>(&scratchpad_num_lines), "number of scratchpad lines")
            ("scratchpad_num_set_associative_way", po::value<unsigned long long int>(&scratchpad_num_set_associative_way), "number of scratchpad set associative way")
            ("scratch_read_latency", po::value<unsigned long long int>(&scratchpad_read_latency), "scratchpad read latency in cycles")
            ("scratch_write_latency", po::value<unsigned long long int>(&scratchpad_write_latency), "scratchpad write latency in cycles")
            ("scratchpad_num_simultaneous_requests", po::value<unsigned long long int>(&scratchpad_num_simultaneous_requests), "number of scratchpad simultaneous requests")
          ;

          po::options_description sim("Simulation Options");
          sim.add_options()
            ("app", po::value<std::string>(&app), "type of app")
            ("num_iter", po::value<unsigned long long int>(&num_iter), "the number of iterations to simulate")
            ("num_pipelines", po::value<unsigned long long int>(&num_pipelines), "the number of pipelines in parallel")
            ("num_dst_readers", po::value<unsigned long long int>(&num_dst_readers), "the number of readVertexProperty  modules")
          ;

          po::options_description graph("ReadGrpah Options");
          graph.add_options()
            ("should_init", po::value<int>(&shouldInit), "graph needs to be initialized")
            ("graph_path", po::value<std::string>(&graph_path), "path to mat market format graph")
            ("vertex_properties", po::value<std::string>(&result), "name for the output file containing vertex values for verification")
          ;

          po::options_description all_options;
          all_options.add(desc);
          all_options.add(io);
          all_options.add(cache);
          all_options.add(scratch);
          all_options.add(sim);
          all_options.add(graph);


          po::variables_map vm;
          po::store(po::parse_command_line(argc, argv, all_options), vm);
          po::notify(vm);

          return true;
      }
  }; //End of class Options
}; //End of namespace Utility
#endif
