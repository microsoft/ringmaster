#include <stdexcept>
#include "split.hh"

using namespace std;

vector<string> split(const string & str, const string & separator)
{
  if (separator.empty()) {
    throw runtime_error("empty separator");
  }

  vector<string> ret;

  size_t curr_pos = 0;
  while (curr_pos < str.size()) {
    size_t next_pos = str.find(separator, curr_pos);

    if (next_pos == string::npos) {
      ret.emplace_back(str.substr(curr_pos));
      break;
    } else {
      ret.emplace_back(str.substr(curr_pos, next_pos - curr_pos));
      curr_pos = next_pos + separator.size();
    }
  }

  return ret;
}
