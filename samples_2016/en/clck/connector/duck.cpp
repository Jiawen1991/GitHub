#include "duck.h"

#include <iterator>
#include <regex>
#include <string>

using namespace std;

/* Regular expression to search for either 'honk' or 'quack' and
   capture the sound.  Allow whitespace on either side of the
   sound. */
const regex duck_regex("\\s*((?:honk|quack))\\s*");

Duck::Duck(DbRead& db_read, void *clips_env) : super(db_read, clips_env) {
  set_name("duck");
}

bool Duck::parse() {
  /* Query the database */
  NsDb::Rows rows;
  string query = "SELECT MAX(Timestamp) AS Timestamp, Hostname, Stdout_size, "
    "Stdout, rowid, Encoding FROM clck_1 WHERE Provider IS 'duck' "
    "GROUP BY Hostname";

  if (!db_read.select(query, rows, {{"Timestamp", 0}, {"Hostname", 1},
                                    {"Stdout_size", 2}, {"Stdout", 3},
                                    {"rowid", 4}, {"Encoding", 5}})) {
    cerr << "sql query failed" << endl;
    return false;
  }

  /* Define the knowledge base DUCK class slots to be populated */
  set_header({"node_id", "timestamp", "row-id", "count", "sound"});

  /* Loop over each database row */
  for (unsigned int i = 0; i < rows.size(); i++) {
    int count = 0;
    smatch match;
    string sound = "";
    string stdout(rows[i].stdout.begin(), rows[i].stdout.end());

    /* Apply the regular expression */
    if (regex_search(stdout, match, duck_regex)) {
      sound = match.str(1);
      count = distance(sregex_iterator(stdout.begin(), stdout.end(),
                                       duck_regex),
                       sregex_iterator());
    }

    /* Create a knowledge base DUCK instance */
    route({rows[i].hostname, rows[i].timestamp, rows[i].row_id,
           count, sound});
  }

  return true;
}

extern "C" Extension *create(DbRead& db_read, void *clips_env) {
  return new Duck(db_read, clips_env);
}
