#ifndef TAUVIEW_H
#define TAUVIEW_H

#include <cmath>
#include <cstddef>
#include <functional>
#include <vector>

#include "EventRange.h"
#include "LeptonIDEnums.h"
#include "TLorentzVector.h"
#include "ViewColumns.h"

struct TauSoA {
    ColumnView<float> pt;
    ColumnView<float> eta;
    ColumnView<float> phi;
    ColumnView<float> mass;
    ColumnView<float> dxy;
    ColumnView<float> dz;
    ColumnView<short> charge;
    ColumnView<unsigned char> decayMode;
    ColumnView<unsigned char> genPartFlav;
    ColumnView<short> genPartIdx;
    BoolColumnView idDecayModeNewDMs;
    ColumnView<unsigned char> idDeepTau2018v2p5VSe;
    ColumnView<unsigned char> idDeepTau2018v2p5VSjet;
    ColumnView<unsigned char> idDeepTau2018v2p5VSmu;
    ColumnView<float> rawDeepTau2018v2p5VSe;
    ColumnView<float> rawDeepTau2018v2p5VSjet;
    ColumnView<float> rawDeepTau2018v2p5VSmu;

    // The tau energy scale, materialised the same way the muon momentum
    // correction is: the branch columns stay raw and the corrected values live
    // beside them, so Pt() can mean "the momentum this analysis selects on"
    // without anything rewriting NanoAOD.
    //
    // TES is NOT a weight. It moves taus across the offline pt cut, so it
    // changes which events are selected, and it has to be propagated to MET.
    // That is why it is opt-in: see AnalyzerCore::EnableTauEnergyScale. With
    // it off, correctedPt is the raw pt and every accessor behaves as before.
    std::vector<float> correctedPt;
    std::vector<float> correctedMass;
    std::vector<float> tesPtUp;
    std::vector<float> tesPtDown;
    std::vector<float> tesMassUp;
    std::vector<float> tesMassDown;
    std::function<void()> populateMomentum;
    mutable bool momentumReady = false;
    mutable bool momentumComputing = false;

    std::size_t size() const { return pt.size(); }

    void ensureMomentum() const {
        if (momentumReady)
            return;
        if (momentumComputing)
            throw SKNano::LogicError("[TauSoA] recursive momentum computation");
        // No provider means no correction, which is the right answer for a
        // TauSoA assembled directly from columns -- test fixtures, and any
        // caller that does not go through AnalyzerCore::GetAllTauViews. The
        // corrected accessors fall back to the raw columns, so Pt() is the
        // NanoAOD pt and nothing silently reads as zero.
        if (!populateMomentum) {
            momentumReady = true;
            return;
        }
        momentumComputing = true;
        try {
            populateMomentum();
            momentumComputing = false;
        } catch (...) {
            momentumComputing = false;
            throw;
        }
        if (!momentumReady)
            throw SKNano::LogicError(
                "[TauSoA] momentum provider did not publish a lane");
    }
};

class TauView {
public:
    using WorkingPoint = LeptonID::TauWorkingPoint;
    using WorkingPointVsMu = LeptonID::TauWorkingPointVsMu;

    // A tau ID is a point on each of the three DeepTau axes plus the two
    // quality cuts that always travel with them.  Unlike MuonID this cannot be
    // a flat enum: the axes are chosen independently, and the TAU POG scale
    // factors are keyed by the same combination, so the analyzer has to name
    // all three anyway.  NONE on an axis disables it.
    struct TauID {
        WorkingPoint vsJet = WorkingPoint::NONE;
        WorkingPoint vsE = WorkingPoint::NONE;
        WorkingPointVsMu vsMu = WorkingPointVsMu::NONE;
        bool requireNewDM = true;
        // idDecayModeNewDMs admits decay modes 0, 1, 2, 5, 6, 10 and 11. The
        // TAU POG calibrates only {0, 1, 10, 11}: on the rest
        // tau_energy_scale and DeepTau2018v2p5VSe throw, and tau_trigger
        // returns -1.0, which silently flips the sign of the event weight.
        // Defaults to true so an uncalibrated tau has to be asked for.
        bool requireCalibratedDecayMode = true;
        float maxDz = 0.2f;
    };

    // The decay modes every TAU correctionlib input covers.
    static constexpr bool IsCalibratedDecayMode(unsigned char dm) {
        return dm == 0 || dm == 1 || dm == 10 || dm == 11;
    }

    TauView() = default;
    TauView(const TauSoA *storage, std::size_t index)
        : store_(storage), index_(index) {}

    bool valid() const { return store_ && index_ < store_->size(); }

    // The energy-scale-corrected momentum, which is what the selection cuts on
    // and what every observable should be built from. Equal to MiniAODPt()
    // unless AnalyzerCore::EnableTauEnergyScale was called.
    float Pt() const {
        store_->ensureMomentum();
        return index_ < store_->correctedPt.size() ? store_->correctedPt[index_]
                                                   : store_->pt[index_];
    }
    float M() const {
        store_->ensureMomentum();
        return index_ < store_->correctedMass.size()
                   ? store_->correctedMass[index_]
                   : store_->mass[index_];
    }
    // The uncorrected NanoAOD values. Correction inputs are binned in these:
    // the TAU POG measures its scale factors against the reconstructed tau,
    // not against a tau that already carries the scale.
    float MiniAODPt() const { return store_->pt[index_]; }
    float MiniAODM() const { return store_->mass[index_]; }
    float TesPtUp() const {
        store_->ensureMomentum();
        return index_ < store_->tesPtUp.size() ? store_->tesPtUp[index_] : Pt();
    }
    float TesPtDown() const {
        store_->ensureMomentum();
        return index_ < store_->tesPtDown.size() ? store_->tesPtDown[index_]
                                                 : Pt();
    }
    float TesMassUp() const {
        store_->ensureMomentum();
        return index_ < store_->tesMassUp.size() ? store_->tesMassUp[index_]
                                                 : M();
    }
    float TesMassDown() const {
        store_->ensureMomentum();
        return index_ < store_->tesMassDown.size() ? store_->tesMassDown[index_]
                                                   : M();
    }
    float Eta() const { return store_->eta[index_]; }
    float Phi() const { return store_->phi[index_]; }
    float dXY() const { return store_->dxy[index_]; }
    float dZ() const { return store_->dz[index_]; }
    short Charge() const { return store_->charge[index_]; }
    unsigned char DecayMode() const { return store_->decayMode[index_]; }
    unsigned char GenPartFlav() const { return store_->genPartFlav[index_]; }
    short GenPartIdx() const {
        return store_->genPartIdx.available() ? store_->genPartIdx[index_] : -1;
    }
    bool idDecayModeNewDMs() const { return store_->idDecayModeNewDMs[index_]; }

    // Raw DeepTau discriminator outputs. Return -1 when the column is not
    // present, following the GenPartIdx() convention above.
    float RawVsJet() const {
        return store_->rawDeepTau2018v2p5VSjet.available()
                   ? store_->rawDeepTau2018v2p5VSjet[index_]
                   : -1.f;
    }
    float RawVsE() const {
        return store_->rawDeepTau2018v2p5VSe.available()
                   ? store_->rawDeepTau2018v2p5VSe[index_]
                   : -1.f;
    }
    float RawVsMu() const {
        return store_->rawDeepTau2018v2p5VSmu.available()
                   ? store_->rawDeepTau2018v2p5VSmu[index_]
                   : -1.f;
    }

    bool passVVVLIDvJet() const { return jetId() >= 1; }
    bool passVVLIDvJet() const { return jetId() >= 2; }
    bool passVLIDvJet() const { return jetId() >= 3; }
    bool passLIDvJet() const { return jetId() >= 4; }
    bool passMIDvJet() const { return jetId() >= 5; }
    bool passTIDvJet() const { return jetId() >= 6; }
    bool passVTIDvJet() const { return jetId() >= 7; }
    bool passVVTIDvJet() const { return jetId() >= 8; }
    bool passVVVLIDvEl() const { return electronId() >= 1; }
    bool passVVLIDvEl() const { return electronId() >= 2; }
    bool passVLIDvEl() const { return electronId() >= 3; }
    bool passLIDvEl() const { return electronId() >= 4; }
    bool passMIDvEl() const { return electronId() >= 5; }
    bool passTIDvEl() const { return electronId() >= 6; }
    bool passVTIDvEl() const { return electronId() >= 7; }
    bool passVVTIDvEl() const { return electronId() >= 8; }
    bool passVLIDvMu() const { return muonId() >= 1; }
    bool passLIDvMu() const { return muonId() >= 2; }
    bool passMIDvMu() const { return muonId() >= 3; }
    bool passTIDvMu() const { return muonId() >= 4; }

    bool PassID(const TauID &id) const;

    TLorentzVector P4() const {
        TLorentzVector value;
        value.SetPtEtaPhiM(Pt(), Eta(), Phi(), M());
        return value;
    }

private:
    unsigned char jetId() const { return store_->idDeepTau2018v2p5VSjet[index_]; }
    unsigned char electronId() const { return store_->idDeepTau2018v2p5VSe[index_]; }
    unsigned char muonId() const { return store_->idDeepTau2018v2p5VSmu[index_]; }

    const TauSoA *store_ = nullptr;
    std::size_t index_ = 0;
};

using TauViewCollection = EventRange<TauSoA, TauView>;

#endif
