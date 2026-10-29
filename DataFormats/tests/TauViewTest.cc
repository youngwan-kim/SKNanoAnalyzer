#include <gtest/gtest.h>

#include "TauTestSupport.h"

namespace {

using sknano_test::MakeTaus;

TEST(TauViewWorkingPoints, TightTauPassesLooserJetWorkingPoints) {
    auto f = MakeTaus({100.f}, {0.5f}, {6}, {6}, {4});
    const TauView tau = f->at(0);

    EXPECT_TRUE(tau.passTIDvJet());
    EXPECT_TRUE(tau.passMIDvJet());
    EXPECT_TRUE(tau.passLIDvJet());
    EXPECT_TRUE(tau.passVLIDvJet());
    EXPECT_TRUE(tau.passVVLIDvJet());
    EXPECT_TRUE(tau.passVVVLIDvJet());
}

TEST(TauViewWorkingPoints, TightTauFailsTighterJetWorkingPoints) {
    auto f = MakeTaus({100.f}, {0.5f}, {6}, {6}, {4});
    const TauView tau = f->at(0);

    EXPECT_FALSE(tau.passVTIDvJet());
    EXPECT_FALSE(tau.passVVTIDvJet());
}

TEST(TauViewWorkingPoints, ElectronAxisIsAlsoCumulative) {
    auto f = MakeTaus({100.f}, {0.5f}, {6}, {5}, {4});
    const TauView tau = f->at(0);

    EXPECT_TRUE(tau.passMIDvEl());
    EXPECT_TRUE(tau.passLIDvEl());
    EXPECT_TRUE(tau.passVVVLIDvEl());
    EXPECT_FALSE(tau.passTIDvEl());
}

TEST(TauViewWorkingPoints, MuonAxisUsesFourPointScale) {
    auto f = MakeTaus({100.f}, {0.5f}, {6}, {6}, {3});
    const TauView tau = f->at(0);

    EXPECT_TRUE(tau.passMIDvMu());
    EXPECT_TRUE(tau.passLIDvMu());
    EXPECT_TRUE(tau.passVLIDvMu());
    EXPECT_FALSE(tau.passTIDvMu());
}

TEST(TauViewWorkingPoints, UnidentifiedTauFailsEveryWorkingPoint) {
    auto f = MakeTaus({100.f}, {0.5f}, {0}, {0}, {0});
    const TauView tau = f->at(0);

    EXPECT_FALSE(tau.passVVVLIDvJet());
    EXPECT_FALSE(tau.passVVVLIDvEl());
    EXPECT_FALSE(tau.passVLIDvMu());
}

// test raw discrimator 

TEST(TauViewRawScores, RawDiscriminatorsAreReadable) {
    auto f = MakeTaus({100.f}, {0.5f}, {6}, {6}, {4});
    f->rawVsJet = sknano_test::VectorColumn<float>({0.97f});
    f->rawVsE = sknano_test::VectorColumn<float>({0.88f});
    f->rawVsMu = sknano_test::VectorColumn<float>({0.51f});
    f->soa.rawDeepTau2018v2p5VSjet.bind(&f->rawVsJet);
    f->soa.rawDeepTau2018v2p5VSe.bind(&f->rawVsE);
    f->soa.rawDeepTau2018v2p5VSmu.bind(&f->rawVsMu);

    const TauView tau = f->at(0);
    EXPECT_FLOAT_EQ(tau.RawVsJet(), 0.97f);
    EXPECT_FLOAT_EQ(tau.RawVsE(), 0.88f);
    EXPECT_FLOAT_EQ(tau.RawVsMu(), 0.51f);
}

TEST(TauViewRawScores, UnboundRawDiscriminatorsReturnSentinel) {
    auto f = MakeTaus({100.f}, {0.5f}, {6}, {6}, {4});
    const TauView tau = f->at(0);

    EXPECT_FLOAT_EQ(tau.RawVsJet(), -1.f);
    EXPECT_FLOAT_EQ(tau.RawVsE(), -1.f);
    EXPECT_FLOAT_EQ(tau.RawVsMu(), -1.f);
}

TEST(TauViewID, AcceptsTauMeetingEveryAxis) {
    auto f = MakeTaus({100.f}, {0.5f}, {6}, {6}, {4});
    const TauView::TauID id{LeptonID::TauWorkingPoint::MEDIUM, LeptonID::TauWorkingPoint::VVLOOSE, LeptonID::TauWorkingPointVsMu::LOOSE,
                         true, true, 0.2f};
    EXPECT_TRUE(f->at(0).PassID(id));
}

TEST(TauViewID, RejectsTauFailingTheJetAxis) {
    auto f = MakeTaus({100.f}, {0.5f}, {4}, {6}, {4});
    const TauView::TauID id{LeptonID::TauWorkingPoint::MEDIUM, LeptonID::TauWorkingPoint::VVLOOSE, LeptonID::TauWorkingPointVsMu::LOOSE,
                         true, true, 0.2f};
    EXPECT_FALSE(f->at(0).PassID(id));
}

TEST(TauViewID, RejectsTauFailingTheMuonAxis) {
    auto f = MakeTaus({100.f}, {0.5f}, {6}, {6}, {1});
    const TauView::TauID id{LeptonID::TauWorkingPoint::MEDIUM, LeptonID::TauWorkingPoint::VVLOOSE, LeptonID::TauWorkingPointVsMu::TIGHT,
                         true, true, 0.2f};
    EXPECT_FALSE(f->at(0).PassID(id));
}

TEST(TauViewID, NoneWorkingPointDisablesThatAxis) {
    auto f = MakeTaus({100.f}, {0.5f}, {6}, {0}, {0});
    const TauView::TauID id{LeptonID::TauWorkingPoint::TIGHT, LeptonID::TauWorkingPoint::NONE, LeptonID::TauWorkingPointVsMu::NONE,
                         true, true, 0.2f};
    EXPECT_TRUE(f->at(0).PassID(id));
}

TEST(TauViewID, RejectsTauFailingTheImpactParameterCut) {
    auto f = MakeTaus({100.f}, {0.5f}, {6}, {6}, {4}, {0.5f});
    const TauView::TauID id{LeptonID::TauWorkingPoint::MEDIUM, LeptonID::TauWorkingPoint::VVLOOSE, LeptonID::TauWorkingPointVsMu::LOOSE,
                         true, true, 0.2f};
    EXPECT_FALSE(f->at(0).PassID(id));
}

// Decay modes 2, 5 and 6 pass idDecayModeNewDMs but no TAU correction covers
// them: tau_energy_scale and VSe throw and tau_trigger returns -1.0, which
// flips the sign of the event weight. docs/SYSTEMATICS.md 2.1.
TEST(TauViewID, RejectsDecayModesThePOGDoesNotCalibrate) {
    const TauView::TauID id{LeptonID::TauWorkingPoint::MEDIUM, LeptonID::TauWorkingPoint::VVLOOSE, LeptonID::TauWorkingPointVsMu::LOOSE,
                         true, true, 0.2f};
    for (const unsigned char dm : {0, 1, 10, 11}) {
        auto f = MakeTaus({100.f}, {0.5f}, {6}, {6}, {4}, {0.f}, {dm});
        EXPECT_TRUE(f->at(0).PassID(id)) << "dm " << static_cast<int>(dm);
    }
    for (const unsigned char dm : {2, 5, 6}) {
        auto f = MakeTaus({100.f}, {0.5f}, {6}, {6}, {4}, {0.f}, {dm});
        EXPECT_FALSE(f->at(0).PassID(id)) << "dm " << static_cast<int>(dm);
    }
}

TEST(TauViewID, KeepsUncalibratedDecayModesWhenTheFlagIsOff) {
    TauView::TauID id{LeptonID::TauWorkingPoint::MEDIUM, LeptonID::TauWorkingPoint::VVLOOSE, LeptonID::TauWorkingPointVsMu::LOOSE,
                      true, false, 0.2f};
    for (const unsigned char dm : {2, 5, 6}) {
        auto f = MakeTaus({100.f}, {0.5f}, {6}, {6}, {4}, {0.f}, {dm});
        EXPECT_TRUE(f->at(0).PassID(id)) << "dm " << static_cast<int>(dm);
    }
}

TEST(TauViewID, WorkingPointEnumMapsToPogStrings) {
    EXPECT_STREQ(ToCorrectionString(LeptonID::TauWorkingPoint::MEDIUM), "Medium");
    EXPECT_STREQ(ToCorrectionString(LeptonID::TauWorkingPoint::VVLOOSE), "VVLoose");
    EXPECT_STREQ(ToCorrectionString(LeptonID::TauWorkingPoint::VVTIGHT), "VVTight");
    EXPECT_STREQ(ToCorrectionString(LeptonID::TauWorkingPointVsMu::TIGHT), "Tight");
    EXPECT_STREQ(ToCorrectionString(LeptonID::TauWorkingPointVsMu::VLOOSE), "VLoose");
}

}
