#include <fstream>
#include <iostream>
#include <map>
#include <omp.h>
#include <regex>
#include <sstream>
#include <string>
#define MAX_CLIENT 100

enum class DataType { String, Number, Variable };
struct Data {
  std::string str;
  DataType type;
  bool operator==(const Data &rhs) {
    return str == rhs.str && type == rhs.type;
  }
  bool operator!=(const Data &rhs) { return !(*this == rhs); }
};
using Tuple = std::vector<Data>;

int main() {
  // std::istream &fin = std::cin;
  std::ifstream fin("testcase/test.txt");

  int threads;
  fin >> threads >> std::ws;
  omp_set_num_threads(threads);

  bool running = true;
  int client_id;
  std::vector<Tuple> tspace;
  // std::string tin[MAX_CLIENT] = {};
  std::map<std::string, Data> mvars[MAX_CLIENT];
  Tuple line_tuple;
  std::string cmd;
#pragma omp parallel shared(running, client_id, line_tuple, cmd, mvars)
  {
    const int tid = omp_get_thread_num();
    std::ofstream fout(tid == 0 ? "server.txt" : std::to_string(tid) + ".txt");
    do {
#pragma omp barrier
      { ; }
#pragma omp master
      {
        std::string line, token;
        if (fin.peek() == 'e') {
          running = false;
        } else {
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
              data.type = DataType::Variable;
            }
            line_tuple.emplace_back(data);
          }
          if (cmd == "out") {
            // remove variables
            auto &mvar = mvars[client_id];
            for (auto &tuple : line_tuple) {
              if (tuple.type == DataType::Variable) {
                const Data &var = mvar[tuple.str];
                tuple.str = var.str;
                tuple.type = var.type;
              }
            }
            // insert into tuple space
            tspace.push_back(line_tuple);
            // output tuple space
            fout << '(';
            const size_t tpsize = tspace.size();
            for (size_t i = 0; i < tpsize; ++i) {
              fout << '(';
              const auto &tuple = tspace[i];
              const size_t tsize = tuple.size();
              for (size_t j = 0; j < tsize; ++j) {
                fout << tuple[j].str;
                if (j + 1 != tsize)
                  fout << ',';
              }
              fout << ')';
              if (i + 1 != tpsize)
                fout << ',';
            }
            fout << ')' << std::endl;
            // cancel client_id
            client_id = 0;
          }
        }
      }
#pragma omp barrier
      { ; }
      if (tid == client_id && tid > 0) {
        // if (cmd == "in" || cmd == "read") {
        const size_t lsize = line_tuple.size();
        for (size_t i = 0; i < tspace.size(); ++i) {
          auto &tuple = tspace[i];
          if (tuple.size() != lsize)
            continue;
          bool equal = true;
          for (size_t j = 0; j < lsize; ++j) {
            if (line_tuple[j].type == DataType::Variable)
              continue;
            if (tuple[j] != line_tuple[j]) {
              equal = false;
              break;
            }
          }
          if (equal) {
            fout << '(';
            for (size_t j = 0; j < lsize; ++j) {
              // assign variable
              if (line_tuple[j].type == DataType::Variable) {
                mvars[tid][line_tuple[j].str] = tuple[j];
              }
              // output tuple
              fout << tuple[j].str;
              if (j + 1 != lsize) {
                fout << ',';
              }
            }
            fout << ')' << std::endl;
            // remove if "in"
            if (cmd == "in") {
              tspace.erase(tspace.begin() + i);
            }
            break;
          }
        }
        //}
      }
    } while (running);
    fout.close();
  }
}
