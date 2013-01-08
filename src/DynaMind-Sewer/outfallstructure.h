#ifndef OUTFALLSTRUCTURE_H
#define OUTFALLSTRUCTURE_H

#include <dm.h>

class DM_HELPER_DLL_EXPORT OutfallStructure : public DM::Module
{
    DM_DECLARE_NODE( OutfallStructure )
private:
    DM::View view_outlets;
    DM::View view_outfalls;
    DM::View view_conduits;
public:
    OutfallStructure();
    void run();
};

#endif // OUTFALLSTRUCTURE_H
