#ifndef EVAL_SEWER_FLOODING_H
#define EVAL_SEWER_FLOODING_H

#include <dmmodule.h>
#include <dm.h>
using namespace DM;
class DM_HELPER_DLL_EXPORT EvalSewerFlooding  : public  Module {
    DM_DECLARE_NODE ( EvalSewerFlooding )

    private:
        DM::View flooding_area;
        std::string FileName;
        int counter;
        DM::View city;


public:
    EvalSewerFlooding();
    void run();
};

#endif // EVAL_SEWER_FLOODING_H
