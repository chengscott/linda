#include <fstream>
#include <iostream>
#include <map>
#include <omp.h>
#include <regex>
#include <sstream>
#include <string>
#define MAX_CLIENT 100
#define MAX_TIME 0xffffffffffffffff

enum class DataType { String, Number, Variable };
struct Data {
  std::string str;
  DataType type;
  bool operator==(const Data &rhs) const {
    if (type == DataType::Variable || rhs.type == DataType::Variable)
      return true;
    return str == rhs.str && type == rhs.type;
  }
};
using Tuple = std::vector<Data>;
template <class T = Data>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &rhs) {
  os << '(';
  auto end = std::prev(std::end(rhs));
  for (auto it = std::begin(rhs); it != end; ++it) {
    os << it->str << ",";
  }
  os << rhs.back().str << ')';
  return os;
}

int main() {
  std::istream &fin = std::cin;
  // std::ifstream fin("testcase/test.txt");

  int threads;
  fin >> threads >> std::ws;
  omp_set_num_threads(threads + 1);

  bool running = true;
  // client to handle current command
  int client_id;
  // global timer
  size_t global_time = 0;
  // global tuple space
  std::vector<Tuple> tspace;
  // global variables
  std::map<std::string, Data> tvars;
  // tuple to wait for each client
  Tuple twaits[MAX_CLIENT] = {};
  size_t ttimes[MAX_CLIENT] = {};
  std::string tcmds[MAX_CLIENT] = {};
  // current input tuple
  Tuple line_tuple;
  // current command: out, in, read
  std::string cmd;
#pragma omp parallel default(shared)
  {
    const int tid = omp_get_thread_num();
    std::ofstream fout(tid == 0 ? "server.txt" : std::to_string(tid) + ".txt");
    do {
#pragma omp master
      {
        std::string line, token;
        if (fin.peek() == 'e') {
          running = false;
        } else {
          ++global_time;
          fin >> client_id >> cmd;
          std::getline(fin, line);
          // parse line into data
          std::stringstream ss(line);
          line_tuple.clear();
          while (ss >> token) {
            Data data{.str = token};
            const char t = token[0];
            if (t == '"') {
              data.type = DataType::String;
            } else if ('0' <= t && t <= '9') {
              data.type = DataType::Number;
            } else if (cmd != "out" && t == '?') {
              data.type = DataType::Variable;
              data.str = token.substr(1);
            } else {
              // lookup variable
              const Data &var = tvars[token];
              data.type = var.type;
              data.str = var.str;
            }
            line_tuple.emplace_back(data);
          }
          bool output;
          if (cmd == "out") {
            output = true;
            // check for waiting
            client_id = 0;
            size_t ttime = MAX_TIME;
            for (size_t i = 1; i <= threads; ++i) {
              const auto &tuple = twaits[i];
              if (tuple == line_tuple && ttimes[i] < ttime) {
                ttime = ttimes[i];
                cmd = tcmds[i];
                client_id = i;
              }
            }
            // if waitings
            if (client_id != 0) {
              // store variables
              const auto &tuple = twaits[client_id];
              const size_t lsize = line_tuple.size();
              for (size_t i = 0; i < lsize; ++i) {
                if (tuple[i].type == DataType::Variable) {
                  tvars[tuple[i].str] = line_tuple[i];
                }
              }
              // remove from waitings
              twaits[client_id] = Tuple();
            }
            // if no waitings, insert into tuple space
            if (cmd != "in") {
              tspace.push_back(line_tuple);
            }
          } else if (cmd == "in" || cmd == "read") {
            output = false;
            if (twaits[client_id].empty()) {
              bool found = false;
              const size_t lsize = line_tuple.size();
              for (size_t i = 0; i < tspace.size(); ++i) {
                auto &tuple = tspace[i];
                if (tuple == line_tuple) {
                  found = true;
                  // store variables
                  for (size_t j = 0; j < lsize; ++j) {
                    if (line_tuple[j].type == DataType::Variable) {
                      tvars[line_tuple[j].str] = tuple[j];
                    }
                  }
                  line_tuple = tuple;
                  // remove from waitings
                  twaits[client_id] = Tuple();
                  // remove if "in"
                  if (cmd == "in") {
                    output = true;
                    tspace.erase(tspace.begin() + i);
                  }
                  break;
                }
              }
              if (!found) {
                twaits[client_id] = line_tuple;
                ttimes[client_id] = global_time;
                tcmds[client_id] = cmd;
                line_tuple = Tuple();
              }
            } else {
              line_tuple = Tuple();
            }
          }
          // output tuple space
          if (output) {
            fout << '(';
            const size_t tpsize = tspace.size();
            for (size_t i = 0; i < tpsize; ++i) {
              fout << tspace[i];
              if (i + 1 != tpsize)
                fout << ',';
            }
            fout << ')' << std::endl;
          }
        }
      }
#pragma omp barrier
      { ; }
      if (!running) {
        break;
      }
      if (tid == client_id && tid > 0) {
        if (!line_tuple.empty()) {
          fout << line_tuple << std::endl;
        }
      }
#pragma omp barrier
      { ; }
    } while (true);
    fout.close();
  }
}
