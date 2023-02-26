#include "conversion.hh"

using namespace std;

int strict_stoi(const string & str, const int base)
{
  size_t pos;
  int ret = stoi(str, &pos, base);

  if (pos != str.size()) {
    throw runtime_error("strict_stoi");
  }

  return ret;
}

long long strict_stoll(const string & str, const int base)
{
  size_t pos;
  int ret = stoll(str, &pos, base);

  if (pos != str.size()) {
    throw runtime_error("strict_stoll");
  }

  return ret;
}

string double_to_string(const double input, const int precision)
{
  stringstream stream;
  stream << fixed << setprecision(precision) << input;
  return stream.str();
}
