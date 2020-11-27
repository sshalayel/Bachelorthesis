#ifndef GRB_TO_FILE
#define GRB_TO_FILE

#include "gurobi_c++.h"
#include <sstream>
#include <string>
#include <thread>

/// A callback that is notified by the slave when optimisation is started on another LP.
struct grb_resettable_callback : GRBCallback
{
    virtual void reset(){};
};

///Simple Callback that print Gurobimessages to some stream.
class grb_to_file : public grb_resettable_callback
{
  public:
    grb_to_file(std::ostream& os, std::string prefix = "");
    virtual ~grb_to_file() override;

    std::ostream& os;
    std::string prefix;

    virtual void callback() override;
};

/// Like grb_to_file but does thread.yield every now and then.
class slow_grb_to_file : public grb_to_file
{
  public:
    slow_grb_to_file(std::ostream& os);
    virtual ~slow_grb_to_file() override;

    virtual void callback() override;
};

#endif
