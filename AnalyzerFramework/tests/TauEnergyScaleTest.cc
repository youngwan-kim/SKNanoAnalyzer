#include <gtest/gtest.h>

#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include <AnalyzerFramework/AnalyzerCore.h>

#include "TauTestSupport.h"

namespace {

using sknano_test::MakeTauCollection;
using sknano_test::MakeTaus;
using sknano_test::TauFixture;
using sknano_test::VectorColumn;

const TauView::TauID kMediumTau{LeptonID::TauWorkingPoint::MEDIUM, LeptonID::TauWorkingPoint::VVLOOSE, LeptonID::TauWorkingPointVsMu::LOOSE,
                             true, true, 0.2f};

void UseCorePopulation(AnalyzerCore &core, TauFixture &f) {
    f.soa.populateMomentum = [&core, target = &f.soa] {
        core.PopulateTauMomentum(*target);
    };
}

void UseFixedCorrectedPt(TauFixture &f, std::vector<float> corrected) {
    f.soa.populateMomentum = [target = &f.soa, corrected] {
        target->correctedPt = corrected;
        target->momentumReady = true;
    };
}

TEST(TauEnergyScale, EnableRejectsAnIDKeepingUncalibratedDecayModes) {
    AnalyzerCore core;
    TauView::TauID relaxed = kMediumTau;
    relaxed.requireCalibratedDecayMode = false;

    EXPECT_THROW(core.EnableTauEnergyScale(relaxed), SKNano::ConfigError);
    EXPECT_FALSE(core.TauEnergyScaleEnabled());
}

TEST(TauEnergyScale, EnableMarksCachedTausStale) {
    AnalyzerCore core;
    const auto taus = core.GetAllTauViews();
    ASSERT_TRUE(taus.storage());
    taus.storage()->momentumReady = true;

    core.EnableTauEnergyScale(kMediumTau);

    EXPECT_FALSE(taus.storage()->momentumReady);
}

TEST(TauEnergyScale, VariationChangeMarksCachedTausStale) {
    AnalyzerCore core;
    core.EnableTauEnergyScale(kMediumTau);
    const auto taus = core.GetAllTauViews();
    taus.storage()->momentumReady = true;

    core.SetTauEnergyScaleVariation(MyCorrection::variation::up);

    EXPECT_FALSE(taus.storage()->momentumReady);
}

TEST(TauEnergyScale, TauViewsAreCachedPerEntry) {
    AnalyzerCore core;
    const auto first = core.GetAllTauViews();
    const auto second = core.GetAllTauViews();

    EXPECT_EQ(first.storage().get(), second.storage().get());

    core.currentEntry += 1;
    const auto third = core.GetAllTauViews();

    EXPECT_NE(first.storage().get(), third.storage().get());
}

TEST(TauEnergyScale, PopulationWithoutCorrectionsKeepsRawMomentum) {
    AnalyzerCore core;
    core.EnableTauEnergyScale(kMediumTau);
    auto f = MakeTaus({100.f, 40.f}, {0.5f, -1.2f}, {6, 6}, {6, 6}, {4, 4});
    UseCorePopulation(core, *f);

    for (std::size_t i = 0; i < 2; ++i) {
        const TauView tau = f->at(i);
        EXPECT_FLOAT_EQ(tau.Pt(), tau.MiniAODPt()) << "tau " << i;
        EXPECT_FLOAT_EQ(tau.M(), tau.MiniAODM()) << "tau " << i;
        EXPECT_FLOAT_EQ(tau.TesPtUp(), tau.MiniAODPt()) << "tau " << i;
        EXPECT_FLOAT_EQ(tau.TesPtDown(), tau.MiniAODPt()) << "tau " << i;
        EXPECT_FLOAT_EQ(tau.TesMassUp(), tau.MiniAODM()) << "tau " << i;
        EXPECT_FLOAT_EQ(tau.TesMassDown(), tau.MiniAODM()) << "tau " << i;
    }
    EXPECT_TRUE(f->soa.momentumReady);
    EXPECT_EQ(f->soa.correctedPt.size(), 2u);
}

TEST(TauEnergyScale, PopulationAppliesTheScaleOnMC) {
    if (!std::getenv("JSONPOG_REPO_PATH") || !std::getenv("SKNANO_DATA") ||
        !std::getenv("ROCCOR_PATH"))
        GTEST_SKIP() << "correction environment not set; source setup.sh first";

    AnalyzerCore core;
    try {
        core.myCorr = new MyCorrection("2024", "CDE", "TTLJ_powheg", false);
    } catch (const std::exception &e) {
        GTEST_SKIP() << "tau corrections unavailable: " << e.what();
    }
    core.EnableTauEnergyScale(kMediumTau);

    const std::vector<unsigned char> dms{0, 1, 10, 11};
    const std::size_t n = dms.size();
    auto f = MakeTaus(std::vector<float>(n, 100.f), std::vector<float>(n, 0.5f),
                      std::vector<unsigned char>(n, 6),
                      std::vector<unsigned char>(n, 6),
                      std::vector<unsigned char>(n, 4),
                      std::vector<float>(n, 0.f), dms);
    UseCorePopulation(core, *f);

    for (std::size_t i = 0; i < n; ++i) {
        const TauView tau = f->at(i);
        const float nom = core.myCorr->GetTauES(kMediumTau, tau);
        const float up =
            core.myCorr->GetTauES(kMediumTau, tau, MyCorrection::variation::up);
        const float down = core.myCorr->GetTauES(kMediumTau, tau,
                                                 MyCorrection::variation::down);
        const int dm = dms[i];
        EXPECT_FLOAT_EQ(tau.Pt(), tau.MiniAODPt() * nom) << "dm " << dm;
        EXPECT_FLOAT_EQ(tau.M(), tau.MiniAODM() * nom) << "dm " << dm;
        EXPECT_FLOAT_EQ(tau.TesPtUp(), tau.MiniAODPt() * up) << "dm " << dm;
        EXPECT_FLOAT_EQ(tau.TesPtDown(), tau.MiniAODPt() * down) << "dm " << dm;
        EXPECT_FLOAT_EQ(tau.TesMassUp(), tau.MiniAODM() * up) << "dm " << dm;
        EXPECT_FLOAT_EQ(tau.TesMassDown(), tau.MiniAODM() * down) << "dm " << dm;
    }
}

TEST(TauEnergyScale, PopulationLeavesDataUncorrected) {
    AnalyzerCore core;
    core.IsDATA = true;
    core.EnableTauEnergyScale(kMediumTau);
    auto f = MakeTaus({100.f}, {0.5f}, {6}, {6}, {4});
    UseCorePopulation(core, *f);

    EXPECT_FLOAT_EQ(f->at(0).Pt(), 100.f);
}

TEST(PropagateTauSystToMET, SubtractsTheTauMomentumShift) {
    AnalyzerCore core;
    core.EnableTauEnergyScale(kMediumTau);
    auto f = MakeTaus({100.f, 50.f}, {0.5f, 0.5f}, {6, 6}, {6, 6}, {4, 4});
    f->phi = VectorColumn<float>({0.f, static_cast<float>(M_PI / 2.)});
    f->soa.phi.bind(&f->phi);
    UseFixedCorrectedPt(*f, {110.f, 45.f});
    Particle met;
    met.SetPxPyPzE(30., 20., 0., 0.);

    core.PropagateTauSystToMET(MakeTauCollection(f), met);

    EXPECT_NEAR(met.Px(), 20., 1e-4);
    EXPECT_NEAR(met.Py(), 25., 1e-4);
}

TEST(PropagateTauSystToMET, LeavesMETAloneWhenTheScaleIsDisabled) {
    AnalyzerCore core;
    auto f = MakeTaus({100.f}, {0.5f}, {6}, {6}, {4});
    UseFixedCorrectedPt(*f, {110.f});
    Particle met;
    met.SetPxPyPzE(30., 20., 0., 0.);

    core.PropagateTauSystToMET(MakeTauCollection(f), met);

    EXPECT_DOUBLE_EQ(met.Px(), 30.);
    EXPECT_DOUBLE_EQ(met.Py(), 20.);
}

TEST(PropagateTauSystToMET, LeavesMETAloneOnData) {
    AnalyzerCore core;
    core.IsDATA = true;
    core.EnableTauEnergyScale(kMediumTau);
    auto f = MakeTaus({100.f}, {0.5f}, {6}, {6}, {4});
    UseFixedCorrectedPt(*f, {110.f});
    Particle met;
    met.SetPxPyPzE(30., 20., 0., 0.);

    core.PropagateTauSystToMET(MakeTauCollection(f), met);

    EXPECT_DOUBLE_EQ(met.Px(), 30.);
    EXPECT_DOUBLE_EQ(met.Py(), 20.);
}

}
