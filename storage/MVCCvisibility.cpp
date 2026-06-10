#include <climits>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

struct RowVersion {

  string value;
  int beginTimestamp, endTimestamp;
};

class MVCC_Row {

  vector<RowVersion> versions;

public:
  void write(const string &value, int txnTimestamp) {

    // set endTimestamp to the latest stored transaction
    if (!versions.empty())
      versions.back().endTimestamp = txnTimestamp;

    versions.push_back({value, txnTimestamp, INT_MAX});
    // current transaction will always have endTimestamp = INT_MAX

    cout << "TXN at : " << txnTimestamp << " wrote value : " << value << endl;
  }

  // snapshot read using binary search
  // get snapshot value with beginTimestamp <= txnTimestamp < endTimestamp
  string read(int txnTimestamp) const {

    int lo = 0, hi = versions.size() - 1, candidate = -1;

    while (lo <= hi) {

      int mid = lo + (hi - lo) / 2;

      if (versions[mid].beginTimestamp > txnTimestamp)
        hi = mid - 1;
      else {
        // versions[mid].beginTimestamp <= txnTimestamp
        candidate = mid;
        lo = mid + 1;
      }
    }

    if (candidate != -1 && txnTimestamp < versions[candidate].endTimestamp)
      return versions[candidate].value;

    return "NOT VISIBLE!";
  }

  void print_versions() {
    cout << "--------Version Table--------" << endl;

    for (auto &v : versions) {
      cout << "[value=" << v.value << ", beginTimestamp=" << v.beginTimestamp
           << ", endTimestamp=";
      cout << (v.endTimestamp == INT_MAX ? string("INF")
                                         : to_string(v.endTimestamp))
           << "]" << endl;
    }
  }
};

int main() {

  MVCC_Row row;

  row.write("Alice", 1);
  row.write("Bob", 5);
  row.write("Charlie", 10);
  row.write("David", 15);
  row.write("Eric", 20);

  row.print_versions();

  cout << "Value Read @ Timestamp 2 : " << row.read(2) << endl;
  cout << "Value Read @ Timestamp 7 : " << row.read(7) << endl;
  cout << "Value Read @ Timestamp 12 : " << row.read(12) << endl;
  cout << "Value Read @ Timestamp 20 : " << row.read(20) << endl;
  cout << "Value Read @ Timestamp 24 : " << row.read(24) << endl;

  return 0;
}
/*
TXN at : 1 wrote value : Alice
TXN at : 5 wrote value : Bob
TXN at : 10 wrote value : Charlie
TXN at : 15 wrote value : David
TXN at : 20 wrote value : Eric
--------Version Table--------
[value=Alice, beginTimestamp=1, endTimestamp=5]
[value=Bob, beginTimestamp=5, endTimestamp=10]
[value=Charlie, beginTimestamp=10, endTimestamp=15]
[value=David, beginTimestamp=15, endTimestamp=20]
[value=Eric, beginTimestamp=20, endTimestamp=INF]
Value Read @ Timestamp 2 : Alice
Value Read @ Timestamp 7 : Bob
Value Read @ Timestamp 12 : Charlie
Value Read @ Timestamp 20 : Eric
Value Read @ Timestamp 24 : Eric
*/
