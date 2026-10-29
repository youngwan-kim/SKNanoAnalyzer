#include "TauView.h"

#include <cmath>

bool TauView::PassID(const TauID &id) const {
    if (id.requireNewDM && !idDecayModeNewDMs())
        return false;
    if (id.requireCalibratedDecayMode && !IsCalibratedDecayMode(DecayMode()))
        return false;
    if (std::abs(dZ()) >= id.maxDz)
        return false;
    // The stored column is the tightest point the tau passes, so ">=" is the
    // cumulative test and an axis left at NONE (0) never rejects anything.
    if (jetId() < static_cast<unsigned char>(id.vsJet))
        return false;
    if (electronId() < static_cast<unsigned char>(id.vsE))
        return false;
    if (muonId() < static_cast<unsigned char>(id.vsMu))
        return false;
    return true;
}
