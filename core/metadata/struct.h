#include <string>
#include <vector>

struct NameID {
  uint64_t key;
  string name;
};

vector<NameID> path_resolution(string path) {
  vector<NameID> path_ids;

  int pos = 0;
  while (true) {
    pos = path.find('/', 1);
    if (pos > 0) {
      //

    } else {
      break;
    }
  }

  return path_ids;
}