#ifndef __DUCK_DOT_H__
#define __DUCK_DOT_H__

#include "connector/extension.h"

class Duck : public Extension {
 public:
  Duck(DbRead& db_read, void *clips_env);
  virtual bool parse();

 private:
  typedef Extension super;
};

#endif //#ifndef __DUCK_DOT_H__
