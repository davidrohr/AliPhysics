///
/// \file AliFemtoAnalysisPionPion.cxx
///

#include "AliFemtoAnalysisPionPion.h"

#include "AliFemtoCutMonitorPionPion.h"

#include "AliESDtrack.h"

// event readers
#include "AliFemtoEventReaderAODMultSelection.h"

// event cuts
#include "AliFemtoBasicEventCut.h"
#include "AliFemtoEventCutCentrality.h"

// track cuts
#include "AliFemtoV0TrackCut.h"
#include "AliFemtoBasicTrackCut.h"
#include "AliFemtoESDTrackCut.h"

// pair cuts
#include "AliFemtoPairCutAntiGamma.h"
#include "AliFemtoPairCutDetaDphi.h"

// correlation functions
#include "AliFemtoAvgSepCorrFctn.h"
#include "AliFemtoCorrFctnDPhiStarDEta.h"
#include "AliFemtoCorrFctnDirectYlm.h"
#include "AliFemtoCorrFctn3DLCMSSym.h"
#include "AliFemtoModelCorrFctnTrueQ3D.h"

#include "AliFemtoAnalysisPionPionObjectConstructor.h"

#include <TROOT.h>
#include <TInterpreter.h>

#include <mutex>
#include <sstream>
#include <utility>
#include <cassert>

static const double PionMass = 0.13956995;
static const int UNKNOWN_CHARGE = -9999;

template<>
struct Configuration<AliFemtoAnalysisPionPion> {
    int foo;

    Configuration()
    : foo(123)
    {}

    Configuration(AliFemtoConfigObject cfg):
      Configuration()
    {
      cfg.pop_and_load("foo", foo);
    }

    operator AliFemtoAnalysisPionPion*() const {
      return nullptr;
    }
};

const struct { unsigned int bin_count; float min; float max; }

VertexBinning = {16, -10.0f, 10.0f},
MultBinning = {30, 0.0f, 10000.0f};

const bool default_verbose = kFALSE
         , default_enable_pair_monitors = kTRUE
         , default_group_output_objects = kTRUE
         , default_output_settings = kTRUE
         , default_is_mc_analysis = kFALSE
         ;

const UInt_t default_num_events_to_mix = 6
           , default_min_coll_size = 100
           ;

#include <initializer_list>

// typedef Float_t RangeF_t[2];
typedef std::pair<Float_t, Float_t> RangeF_t;

/// Structure for storing all configuration information regarding the
/// analysis event-cut. This is hidden as an implementation detail due
/// the build system not playing well with nested structures; the optimal
/// situation would be to define this structure as a component of the
/// AliFemtoAnalysisPionPion::CutParams structure.
///
///


template<>
AliFemtoEventCutCentrality*
AliFemtoConfigObject::Construct() const
{
  Configuration<AliFemtoBasicEventCut> cfg;
  AliFemtoEventCutCentrality *cut = new AliFemtoEventCutCentrality();

  cut->SetCentralityRange(cfg.centrality.first, cfg.centrality.second);
  cut->SetZPosRange(cfg.vertex_z.first, cfg.vertex_z.second);
  cut->SetEPVZERO(cfg.EP_VZero.first, cfg.EP_VZero.second);
  cut->SetTriggerSelection(cfg.trigger_selection);

  return cut;
}

template<>
AliFemtoBasicEventCut*
AliFemtoConfigObject::Construct() const
{
  Configuration<AliFemtoBasicEventCut> cfg;

  // auto x = cfg.Construct<AliFemtoEventCutCentrality>("event");

  // load("multiplicity", cfg.multiplicity);
  // pop_and_load("vertex_z", cfg.vertex_z);
  // pop_and_load("ep_v0", cfg.EP_VZero);
  // pop_and_load("trigger_selection", cfg.trigger_selection);

  AliFemtoBasicEventCut *cut = new AliFemtoBasicEventCut();

  cut->SetEventMult(cfg.multiplicity.first, cfg.multiplicity.second);
  cut->SetVertZPos(cfg.vertex_z.first, cfg.vertex_z.second);
  cut->SetEPVZERO(cfg.EP_VZero.first, cfg.EP_VZero.second);
  cut->SetTriggerSelection(cfg.trigger_selection);
  // cut->SetAcceptBadVertex(cfg.accept_bad_vertex);
  // cut->SetAcceptOnlyPhysics(cfg.accept_only_physics);

  return cut;
}


/// Configuration for creating a particle cut specifically for usage
/// with pion values.
///
struct CutConfig_Pion {
    RangeF_t pt = {0.2, 2.0}
           , eta = {-0.8, 0.8}
           , DCA = {0.5, 4.0}
           , nSigma = {-3.0, 3.0}
           ;

    Int_t charge = 1,
          min_tpc_ncls = 80;

    Float_t max_impact_xy = 2.4
          , max_impact_z = 3.0
          , max_tpc_chi_ndof = 0.032
          , max_its_chi_ndof = 0.032
          ;

    /// default constructor required to use default initialized members
    CutConfig_Pion(){};
};

/// Configuration for the pair
struct CutConfig_Pair {
  Float_t min_delta_eta = { 0.0 },
          min_delta_phi = { 0.0 },
          phi_star_radius = { 1.2 };

  Float_t max_share_fraction = { 0.05 },
          max_share_quality = { 1.0 };

  Bool_t remove_same_label { kFALSE },
         TPCOnly { kTRUE };
};

template<>
AliFemtoPairCutDetaDphi*
AliFemtoConfigObject::Construct<AliFemtoPairCutDetaDphi>() const
{
  CutConfig_Pion cfg;


  auto cut = new AliFemtoPairCutDetaDphi();
  return cut;
}

template<>
AliFemtoESDTrackCut*
AliFemtoConfigObject::Construct() const
{

  CutConfig_Pion cfg;

  AliFemtoESDTrackCut *cut = new AliFemtoESDTrackCut();
  cut->SetCharge(cfg.charge);
  cut->SetMass(PionMass);
  cut->SetPt(cfg.pt.first, cfg.pt.second);
  cut->SetEta(cfg.eta.first, cfg.eta.second);
  cut->SetRapidity(cfg.eta.first, cfg.eta.second);
  cut->SetMostProbablePion();
//   cut->SetStatus(AliESDtrack::kTPCrefit | AliESDtrack::kITSrefit);

  /// Settings for TPC-Inner Runmode
  cut->SetStatus(AliESDtrack::kTPCin);
  cut->SetminTPCncls(cfg.min_tpc_ncls);
  // cut->SetRemoveKinks(cfg.remove_kinks);
  // cut->SetLabel(cfg.set_label);
  cut->SetMaxTPCChiNdof(cfg.max_tpc_chi_ndof);
  cut->SetMaxITSChiNdof(cfg.max_its_chi_ndof);
  cut->SetMaxImpactXY(cfg.max_impact_xy);
  cut->SetMaxImpactZ(cfg.max_impact_z);

  return cut;
}


static const Configuration<AliFemtoBasicEventCut> default_event;
static const CutConfig_Pion default_pion;
static const CutConfig_Pair default_pair;

static const float default_pion_PtMin = 0.2
           , default_pion_PtMax = 2.0

          , default_pion_EtaMin = -0.8
          , default_pion_EtaMax = 0.8

          , default_pion_DCAMin = 0.5
          , default_pion_DCAMax = 4.0

          , default_pion_NSigmaMin = -3.0
          , default_pion_NSigmaMax = 3.0

          , default_pion_max_impact_xy = 0.2
          , default_pion_max_impact_z = 0.15

          , default_pion_max_tpc_chi_ndof = 0.032
          , default_pion_max_its_chi_ndof = 0.032
          ;

const UInt_t default_pion_min_tpc_ncls = 80;
const Bool_t default_pion_remove_kinks = kTRUE,
             default_pion_set_label = kFALSE;



const AliFemtoAnalysisPionPion::PionType
  default_PionType = AliFemtoAnalysisPionPion::kNone;


static const AliFemtoAnalysisPionPion::AnalysisParams
analysis_params_from_pion_types(AliFemtoAnalysisPionPion::PionType one, AliFemtoAnalysisPionPion::PionType two) {
    auto result = AliFemtoAnalysisPionPion::DefaultConfig();
    result.pion_type_1 = one;
    result.pion_type_2 = two;
    return result;
}


AliFemtoAnalysisPionPion::AliFemtoAnalysisPionPion():
  AliFemtoAnalysisPionPion("AliFemtoAnalysisPionPion")
{
}

AliFemtoAnalysisPionPion::AliFemtoAnalysisPionPion(const char *name):
  AliFemtoAnalysisPionPion(name, default_PionType, default_PionType)
{
}


AliFemtoAnalysisPionPion::AliFemtoAnalysisPionPion(const char *name,
                                                   const PionType pion_1,
                                                   const PionType pion_2):
  AliFemtoAnalysisPionPion(name, pion_1, pion_2, DefaultCutConfig())
{
}

AliFemtoAnalysisPionPion
  ::AliFemtoAnalysisPionPion(const char *name,
                             const PionType pion_1,
                             const PionType pion_2,
                             const CutParams &cut_params):
    AliFemtoAnalysisPionPion(name, analysis_params_from_pion_types(pion_1, pion_2), cut_params)
{
}

AliFemtoAnalysisPionPion::AliFemtoAnalysisPionPion(const char *name,
                                                   const AnalysisParams &params,
                                                   const CutParams &cut_params):
  AliFemtoVertexMultAnalysis(params.vertex_bins,
                             params.vertex_min,
                             params.vertex_max,
                             params.mult_bins,
                             params.mult_min,
                             params.mult_max)
  , fAnalysisName(name)
  , fPionType_1(params.pion_type_1)
  , fPionType_2(params.pion_type_2)
  , fGroupOutputObjects(params.group_output_objects)
  , fOutputSettings(params.output_settings)
  , fMCAnalysis(params.is_mc_analysis)
  , fConfiguration()
{
  SetVerboseMode(params.verbose);
  SetEnablePairMonitors(params.enable_pair_monitors);
  SetNumEventsToMix(params.num_events_to_mix);
  SetMinSizePartCollection(params.min_coll_size);

  if (fFirstParticleCut == nullptr) {
    SetFirstParticleCut(BuildPionCut1(cut_params));
  }

  if (fPionType_2 != kNone && fSecondParticleCut == nullptr) {
    SetSecondParticleCut(BuildPionCut2(cut_params));
  } else if (fPionType_2 == kNone) {
    fSecondParticleCut = fFirstParticleCut;
  }

  if (fEventCut == nullptr) {
    SetEventCut(BuildEventCut(cut_params));
  }

  if (fPairCut == nullptr) {
    SetPairCut(BuildPairCut(cut_params));
  }

  fConfiguration = AliFemtoConfigObject::Parse(TString::Format(R"#({
    type: 'AliFemtoAnalysisPionPion',
    is_mc: %d,
    event: {
      multiplicity: %d:%d,
      centrality: %f:%f,
      zVertex: %f:%f,
      trigger: %d,
      accept_bad_vertex: %s,
      accept_only_physics: %s,
    },
    track: {
      pt: %f:%f,
      eta: %f:%f,
      DCA: %f:%f,
      nSigma: %f:%f,
      impact_xy: %f,
      impact_z: %f,
      min_tpc_ncls: %d,
      remove_kinks: %s,
      set_label: %s,
    },
    pair: {
      max_share_quality: %f,
      max_share_fraction: %f,
      delta_eta_min: %f,
      delta_phi_min: %f,
      phi_star_radius: %f,
      tpc_only: %d,
    },
  })#",
    fMCAnalysis,
    cut_params.event_MultMin, cut_params.event_MultMax,
    cut_params.event_CentralityMin, cut_params.event_CentralityMax,
    cut_params.event_VertexZMin, cut_params.event_VertexZMax,
    cut_params.event_TriggerSelection,
    (cut_params.event_AcceptBadVertex ? "true" : "false"),
    (cut_params.event_AcceptOnlyPhysics ? "true" : "false"),

    cut_params.pion_1_PtMin, cut_params.pion_1_PtMax,
    cut_params.pion_1_EtaMin, cut_params.pion_1_EtaMax,
    cut_params.pion_1_DCAMin, cut_params.pion_1_DCAMax,
    cut_params.pion_1_NSigmaMin, cut_params.pion_1_NSigmaMax,
    cut_params.pion_1_max_impact_xy, cut_params.pion_1_max_impact_z,
    cut_params.pion_1_min_tpc_ncls,
    (cut_params.pion_1_remove_kinks ? "true" : "false"),
    (cut_params.pion_1_set_label ? "true" : "false"),

    cut_params.pair_max_share_quality, cut_params.pair_max_share_fraction,
    cut_params.pair_delta_eta_min, cut_params.pair_delta_phi_min,
    cut_params.pair_phi_star_radius, cut_params.pair_TPCOnly
  ).Data());
}


AliFemtoAnalysisPionPion::AnalysisParams
AliFemtoAnalysisPionPion::DefaultConfig()
{
  AliFemtoAnalysisPionPion::AnalysisParams params = {
    VertexBinning.bin_count
  , VertexBinning.min
  , VertexBinning.max

  , MultBinning.bin_count
  , MultBinning.min
  , MultBinning.max

  , default_PionType
  , default_PionType

  , default_num_events_to_mix
  , default_min_coll_size

  , default_verbose
  , default_enable_pair_monitors
  , default_group_output_objects
  , default_output_settings
  , default_is_mc_analysis
  };

  assert(params.min_coll_size == default_min_coll_size);
  assert(params.is_mc_analysis == default_is_mc_analysis);

  return params;
}


AliFemtoAnalysisPionPion::CutParams
AliFemtoAnalysisPionPion::DefaultCutConfig()
{
  AliFemtoAnalysisPionPion::CutParams params = {
    // Event
    std::get<0>(default_event.multiplicity)
  , std::get<1>(default_event.multiplicity)
  , default_event.centrality.first
  , default_event.centrality.second
  , default_event.vertex_z.first
  , default_event.vertex_z.second
  , default_event.EP_VZero.first
  , default_event.EP_VZero.second
  , default_event.trigger_selection
  , default_event.accept_bad_vertex
  , default_event.accept_only_physics

    // Pion 1
  , default_pion.pt.first
  , default_pion.pt.second
  , default_pion_EtaMin
  , default_pion_EtaMax
  , default_pion_DCAMin
  , default_pion_DCAMax

  , default_pion_NSigmaMin
  , default_pion_NSigmaMax

  , default_pion_max_impact_xy
  , default_pion_max_impact_z
  , default_pion_max_tpc_chi_ndof
  , default_pion_max_its_chi_ndof

  , default_pion_min_tpc_ncls
  , default_pion_remove_kinks
  , default_pion_set_label

    // Pion 2
  , default_pion_PtMin
  , default_pion_PtMax
  , default_pion_EtaMin
  , default_pion_EtaMax
  , default_pion_DCAMin
  , default_pion_DCAMax

  , default_pion_NSigmaMin
  , default_pion_NSigmaMax

  , default_pion_max_impact_xy
  , default_pion_max_impact_z
  , default_pion_max_tpc_chi_ndof
  , default_pion_max_its_chi_ndof

  , default_pion_min_tpc_ncls
  , default_pion_remove_kinks
  , default_pion_set_label

    // Pair
  , default_pair.TPCOnly

  , default_pair.min_delta_eta
  , default_pair.min_delta_phi
  , default_pair.phi_star_radius

  , default_pair.max_share_quality
  , default_pair.max_share_fraction
  , default_pair.remove_same_label
  };

  // sanity checks
  assert(params.event_MultMin == default_event.multiplicity.first);
  assert(params.pion_1_PtMin == default_pion_PtMin);
  assert(params.pion_2_PtMin == default_pion_PtMin);
  assert(params.pair_TPCOnly == default_pair_TPCOnly);
  // assert(params.pair_TPCExitSepMin == default_pair_TPCExitSepMin);
  assert(params.pair_delta_eta_min == default_pair_delta_eta_min);
  assert(params.pair_max_share_fraction == default_pair_max_share_fraction);
  assert(params.pair_remove_same_label == default_pair_remove_same_label);

  return params;
}

AliFemtoTrackCut*
AliFemtoAnalysisPionPion::BuildPionCut1(const CutParams &p) const
{
  const int charge = (fPionType_1 == kPiMinus) ? -1 :
                      (fPionType_1 == kPiPlus) ? +1 :
                                                UNKNOWN_CHARGE;
  if (charge == UNKNOWN_CHARGE) {
    std::cerr << "E-AliFemtoAnalysisPionPion::BuildPionCut: Invalid pion type: '" << fPionType_1 << "'\n";
  }

//   AliFemtoBasicTrackCut *cut = new AliFemtoBasicTrackCut();
//   cut->SetNSigmaPion(p.pion_1_NSigmaMin, p.pion_1_NSigmaMax);
//   cut->SetDCA(p.pion_1_DCAMin, p.pion_1_DCAMax);

  AliFemtoESDTrackCut *cut = new AliFemtoESDTrackCut();
  cut->SetCharge(charge);
  cut->SetMass(PionMass);
  cut->SetPt(p.pion_1_PtMin, p.pion_1_PtMax);
  cut->SetEta(p.pion_1_EtaMin, p.pion_1_EtaMax);
  cut->SetRapidity(p.pion_1_EtaMin, p.pion_1_EtaMax);
  cut->SetMostProbablePion();
//   cut->SetStatus(AliESDtrack::kTPCrefit | AliESDtrack::kITSrefit);

  /// Settings for TPC-Inner Runmode
  cut->SetStatus(AliESDtrack::kTPCin);
  cut->SetminTPCncls(p.pion_1_min_tpc_ncls);
  cut->SetRemoveKinks(p.pion_1_remove_kinks);
  cut->SetLabel(p.pion_1_set_label);
  cut->SetMaxTPCChiNdof(p.pion_1_max_tpc_chi_ndof);
  cut->SetMaxITSChiNdof(p.pion_1_max_its_chi_ndof);
  cut->SetMaxImpactXY(p.pion_1_max_impact_xy);
  cut->SetMaxImpactZ(p.pion_1_max_impact_z);

  return cut;
}

AliFemtoTrackCut*
AliFemtoAnalysisPionPion::BuildPionCut2(const CutParams &p) const
{
  const int charge = (fPionType_2 == kPiMinus) ? -1 :
                      (fPionType_2 == kPiPlus) ? +1 :
                                                UNKNOWN_CHARGE;
  if (charge == UNKNOWN_CHARGE) {
    std::cerr << "E-AliFemtoAnalysisPionPion::BuildPionCut: Invalid pion type: '" << fPionType_2 << "'\n";
  }

//   AliFemtoBasicTrackCut *cut = new AliFemtoBasicTrackCut();
//   cut->SetCharge(charge);
//   cut->SetMass(PionMass);
//   cut->SetNSigmaPion(p.pion_2_NSigmaMin, p.pion_2_NSigmaMax);
//   cut->SetPt(p.pion_2_PtMin, p.pion_2_PtMax);
//   cut->SetRapidity(p.pion_2_EtaMin, p.pion_2_EtaMax);
//   cut->SetDCA(p.pion_2_DCAMin, p.pion_2_DCAMax);
  // AliFemtoConfigObject *cfg;
  // auto cut = cfg->Construct<AliFemtoESDTrackCut>();

  AliFemtoESDTrackCut *cut = new AliFemtoESDTrackCut();
  cut->SetCharge(charge);
  cut->SetMass(PionMass);
  cut->SetEta(p.pion_2_EtaMin, p.pion_2_EtaMax);
  cut->SetMostProbablePion();

  /// Settings for TPC-Inner Runmode
  cut->SetStatus(AliESDtrack::kTPCin);

  cut->SetminTPCncls(p.pion_2_min_tpc_ncls);
  cut->SetRemoveKinks(p.pion_2_remove_kinks);
  cut->SetLabel(p.pion_2_set_label);
  cut->SetMaxTPCChiNdof(p.pion_2_max_tpc_chi_ndof);
  cut->SetMaxImpactXY(p.pion_2_max_impact_xy);
  cut->SetMaxImpactZ(p.pion_2_max_impact_z);

  return cut;
}

AliFemtoEventCut*
AliFemtoAnalysisPionPion::BuildEventCut(const AliFemtoAnalysisPionPion::CutParams& p) const
{
/*
  AliFemtoBasicEventCut *cut = new AliFemtoBasicEventCut();

  cut->SetEventMult(p.event_MultMin,
                    p.event_MultMax);
  cut->SetVertZPos(p.event_VertexZMin,
                   p.event_VertexZMax);
  cut->SetEPVZERO(p.event_EP_VZeroMin,
                  p.event_EP_VZeroMax);
  cut->SetTriggerSelection(p.event_TriggerSelection);
  cut->SetAcceptBadVertex(p.event_AcceptBadVertex);
*/
  AliFemtoEventCutCentrality *cut = new AliFemtoEventCutCentrality();
  cut->SetCentralityRange(p.event_CentralityMin, p.event_CentralityMax);
  cut->SetZPosRange(p.event_VertexZMin, p.event_VertexZMax);
  cut->SetEPVZERO(p.event_EP_VZeroMin, p.event_EP_VZeroMax);
  cut->SetTriggerSelection(p.event_TriggerSelection);

//   if (!fMCAnalysis) {
//  cut->SetAcceptOnlyPhysics(p.event_AcceptOnlyPhysics);
//   }

  return cut;
}



AliFemtoPairCut*
AliFemtoAnalysisPionPion::BuildPairCut(const CutParams &p) const
{
  /*
  AliFemtoPairCutAntiGamma *cut = new AliFemtoPairCutAntiGamma();

  // cut->SetTPCOnly(p.pair_TPCOnly);
  // cut->SetTPCExitSepMinimum(p.pair_TPCExitSepMin);
  // cut->SetMinAvgSeparation(0, p.pair_MinAvgSeparationPos);
  // cut->SetMinAvgSeparation(1, p.pair_MinAvgSeparationNeg);
  */


  AliFemtoPairCutDetaDphi *cut = new AliFemtoPairCutDetaDphi(p.pair_delta_eta_min,
                                                             p.pair_delta_phi_min,
                                                             p.pair_phi_star_radius);

  cut->SetCutTechnique(AliFemtoPairCutDetaDphi::Quad);
  cut->SetShareQualityMax(p.pair_max_share_quality);
  cut->SetShareFractionMax(p.pair_max_share_fraction);
  cut->SetRemoveSameLabel(p.pair_remove_same_label);

  return cut;
}

void AliFemtoAnalysisPionPion::AddStanardCutMonitors()
{
  const bool identical = AnalyzeIdenticalParticles();

  if (fEventCut) {
    fEventCut->AddCutMonitor(new AliFemtoCutMonitorPionPion::Event(true, identical, fMCAnalysis, false),
                             new AliFemtoCutMonitorPionPion::Event(false, identical, fMCAnalysis, false));
  } else {
    std::cout << "E-AliFemtoAnalysisPionPion::AddStanardCutMonitors: NO fEventCut!\n";
  }

  const TString p1_type_str = (fPionType_1 == kPiPlus)  ? "Pi+"
                            : (fPionType_1 == kPiMinus) ? "Pi-"
                                                        : "ERROR";

  if (fFirstParticleCut) {
    fFirstParticleCut->AddCutMonitor(new AliFemtoCutMonitorPionPion::Pion(true, p1_type_str, fMCAnalysis),
                                     new AliFemtoCutMonitorPionPion::Pion(false, p1_type_str, fMCAnalysis));
  }

  if (!identical) {
    const TString p2_type_str = (fPionType_2 == kPiPlus)  ? "Pi+"
                              : (fPionType_2 == kPiMinus) ? "Pi-"
                                                          : "ERROR";

    fSecondParticleCut->AddCutMonitor(new AliFemtoCutMonitorPionPion::Pion(true, p2_type_str, fMCAnalysis),
                                      new AliFemtoCutMonitorPionPion::Pion(false, p2_type_str, fMCAnalysis));

    // Non-Identical Pion Pairs
    if (fPairCut) {
      const TString pair_type_str = p1_type_str + "/" + p2_type_str;
      fPairCut->AddCutMonitor(new AliFemtoCutMonitorPionPion::Pair(true, pair_type_str, fMCAnalysis),
                              new AliFemtoCutMonitorPionPion::Pair(false, pair_type_str, fMCAnalysis));
    }
  }
  // Identical Pion Pairs
  else if (fPairCut) {
    const TString pair_type_str = "(Identical) " + p1_type_str;
    fPairCut->AddCutMonitor(new AliFemtoCutMonitorPionPion::Pair(true, pair_type_str, fMCAnalysis),
                            new AliFemtoCutMonitorPionPion::Pair(false, pair_type_str, fMCAnalysis));
  }

}

static TObjArray* GetPassFailOutputList(const TString &name,
                                        AliFemtoCutMonitorHandler *handler)
{
  AliFemtoCutMonitorCollection *passing_monitors = handler->PassMonitorColl(),
                               *failing_monitors = handler->FailMonitorColl();

  TObjArray *res = new TObjArray(),
            *passout = new TObjArray(),
            *failout = new TObjArray();

  res->SetName(name);
  passout->SetName("pass");
  failout->SetName("fail");

  res->Add(passout);
  res->Add(failout);

  for (const auto &cut_monitor : *passing_monitors) {
    TList *output_list = cut_monitor->GetOutputList();
    passout->AddAll(output_list);
    delete output_list;
  }

  for (const auto &cut_monitor : *failing_monitors) {
    TList *output_list = cut_monitor->GetOutputList();
    failout->AddAll(output_list);
    delete output_list;
  }

  return res;
}

TList* AliFemtoAnalysisPionPion::GetOutputList()
{
  TList *outputlist = nullptr;
  TSeqCollection *output = nullptr;

  // get "standard" outputs - that's what we output
  if (!fGroupOutputObjects) {
    outputlist = AliFemtoVertexMultAnalysis::GetOutputList();
    output = outputlist;
  } else {

    output = new TObjArray();
    output->SetName(fAnalysisName);

    if (fEventCut)
    output->Add(GetPassFailOutputList("Event", EventCut()));

    if (AnalyzeIdenticalParticles()) {
    if (fFirstParticleCut)
      output->Add(GetPassFailOutputList("Tracks", fFirstParticleCut));
    } else {
      output->Add(GetPassFailOutputList("Track1", fFirstParticleCut));
      output->Add(GetPassFailOutputList("Track2", fSecondParticleCut));
    }

    output->Add(GetPassFailOutputList("Pair", PairCut()));

    for (auto &corr_fctn : *fCorrFctnCollection) {
      TList *cf_output = corr_fctn->GetOutputList();
      output->AddAll(cf_output);
      delete cf_output;
    }

    outputlist = new TList();
    outputlist->Add(output);
  }

  // get the list of settings from each cut and correlation function
  if (fOutputSettings) {
    TString settings("== settings ==\n");

    TList *setting_list = ListSettings();
    TIter next_setting(setting_list);
    for (TObject *setting = next_setting();
                  setting != nullptr;
                  setting = next_setting()) {
      TObjString *setting_str = (TObjString*)setting;
      settings += setting_str->String() + "\n";
    }
    delete setting_list;
    output->Add(new TObjString(settings));
  }

  output->Add(new AliFemtoConfigObject(fConfiguration));
  return outputlist;
}

TList* AliFemtoAnalysisPionPion::ListSettings()
{
  TList *setting_list = new TList();

  setting_list->AddVector(

    new TObjString(
      TString::Format("AliFemtoAnalysisPionPion.mc_analysis=%d", fMCAnalysis)
    ),

    new TObjString(
      TString::Format("AliFemtoAnalysisPionPion.identical_analysis=%d", AnalyzeIdenticalParticles())
    ),

    new TObjString(
      TString::Format("AliFemtoAnalysisPionPion.pion_1_type=%d", fPionType_1)
    ),

  nullptr);

  if (!AnalyzeIdenticalParticles()) {
    setting_list->Add(
      new TObjString(TString::Format("AliFemtoAnalysisPionPion.pion_2_type=%d", fPionType_2))
    );
  }

  TList *parent_list = AliFemtoVertexMultAnalysis::ListSettings();

  setting_list->AddAll(parent_list);
  delete parent_list;

  setting_list->SetOwner(kTRUE);
  return setting_list;
}

void AliFemtoAnalysisPionPion::EventBegin(const AliFemtoEvent* ev)
{
  fEventCut->EventBegin(ev);
  fFirstParticleCut->EventBegin(ev);
  if (!AnalyzeIdenticalParticles()) {
    fSecondParticleCut->EventBegin(ev);
  }
  fPairCut->EventBegin(ev);
  for (auto &corr_fctn : *fCorrFctnCollection) {
    corr_fctn->EventBegin(ev);
  }
}

void AliFemtoAnalysisPionPion::EventEnd(const AliFemtoEvent* ev)
{
  fEventCut->EventEnd(ev);
  fFirstParticleCut->EventEnd(ev);
  if (!AnalyzeIdenticalParticles()) {
    fSecondParticleCut->EventEnd(ev);
  }
  fPairCut->EventEnd(ev);
  for (auto &corr_fctn : *fCorrFctnCollection) {
    corr_fctn->EventEnd(ev);
  }
}


template <typename T>
T*
ConstructClassOfType(const std::string classname)
{
  static std::mutex m;

  T *result = nullptr;

  {
    auto globals = gROOT->GetListOfGlobals();
    const auto tmpname = TString::Format("___FEMTO_TMP");
    std::lock_guard<std::mutex> guard(m);


    TH1C h(tmpname, "", 1, 0, 1);
    globals->Add(&h);

    auto tmp_buff = h.fArray;

    // const TString cmd = TString::Format("std::cout << '~' << (%s->fArray = reinterpret_cast<char*>(new %s())) << '\\n';", tmpname.Data(), classname.c_str());
    // const TString cmd = TString::Format("dynamic_cast<TH1C*>(gROOT->GetListOfGlobals()->FindObject(\"%s\"))->fArray = reinterpret_cast<char*>(new %s());", tmpname.Data(), classname.c_str());
    const TString cmd = TString::Format(R"( auto a = dynamic_cast<TH1C*>(gROOT->GetListOfGlobals()->FindObject("%s")); a->fArray = reinterpret_cast<char*>(new %s()); )", tmpname.Data(), classname.c_str());

    Int_t err;
    gROOT->ProcessLine(cmd, &err);
    if (err) {
      std::cerr << " --> Error Encountered when processing: " << err << "\n";
    }
    result = reinterpret_cast<T*>(h.fArray);

    h.fArray = tmp_buff;
    globals->Remove(&h);
  }

  return result;
}

// AliFemtoEventReader*
// AliFemtoAnalysisPionPion::ConstructEventReader(AliFemtoConfigObject cfg)
// {

//   std::string classname;
//   cfg.pop_and_load("class", classname);
//   auto result = ConstructClassOfType<AliFemtoEventReader>(classname);

//   std::cout << "\n\n\nCreated event reader at " << (void*)result << "\n\n";
//   std::cout << "Clone.... " << result->Report() << "\n";
//   return result;
// }

AliFemtoEventReader*
AliFemtoAnalysisPionPion::ConstructEventReader(AliFemtoConfigObject cfg)
{
  std::string classname;
  if (!cfg.pop_and_load("class", classname)) {
    TString msg = "Could not load string-property 'class' from object:\n" + cfg.Stringify(true);
    std::cerr << "[AliFemtoAnalysisPionPion::ConstructEventReader] " << msg;
    return nullptr;
  }

  std::cout << "classname=`" << classname << "`\n";
  TClass tclass(classname.c_str(), true);

  // this class is registered with ROOT and is of the correct type
  if (tclass.GetClassInfo() && tclass.InheritsFrom("AliFemtoEventReader")) {

    // can be constructed from ConfigObject
    auto ctor = tclass.GetMethodWithPrototype(classname.c_str(), "AliFemtoConfigObject");
    if (ctor) {
      TString cfgstr = cfg.Stringify();
      auto obj = (AliFemtoEventReader*)gInterpreter->ProcessLine(
        TString::Format("new %s(AliFemtoConfigObject::Parse(\"%s\");", classname.c_str(), cfgstr.Data())
      );
      std::cout << "obj=" << obj << "\n";
      return obj;
    }
  }

  #define TRY_CONSTRUCTING_CLASS(__name) (classname == #__name) ? (AliFemtoEventReader*)(Configuration<__name>(cfg))

  AliFemtoEventReader *result = TRY_CONSTRUCTING_CLASS(AliFemtoEventReaderAOD)
                              : TRY_CONSTRUCTING_CLASS(AliFemtoEventReaderAODChain)
                              : TRY_CONSTRUCTING_CLASS(AliFemtoEventReaderAODMultSelection)
                              : nullptr;

  #undef TRY_CONSTRUCTING_CLASS

  if (result == nullptr) {
    std::cerr << "[AliFemtoAnalysisPionPion::ConstructEventReader] " << "Could not load class '" << classname << "' \n";
  }

  return result;
}

AliFemtoPairCut*
AliFemtoAnalysisPionPion::ConstructPairCut(AliFemtoConfigObject cfg)
{
  std::string classname;
  if (!cfg.pop_and_load("class", classname)) {
    TString msg = "Could not load string-property 'class' from object:\n" + cfg.Stringify(true);
    std::cerr << "[AliFemtoAnalysisPionPion::ConstructPairCut] " << msg;
    return nullptr;
  }
  std::cout << "classname=`" << classname << "`\n";


  #define TRY_CONSTRUCTING_CLASS(__name) (classname == #__name) ? (AliFemtoPairCut*)(Configuration<__name>(cfg))

  auto *result = TRY_CONSTRUCTING_CLASS(AliFemtoPairCutAntiGamma)
               : TRY_CONSTRUCTING_CLASS(AliFemtoPairCutDetaDphi)
               : TRY_CONSTRUCTING_CLASS(AliFemtoShareQualityPairCut)
               : nullptr;

  #undef TRY_CONSTRUCTING_CLASS

  return result;
}

AliFemtoParticleCut*
AliFemtoAnalysisPionPion::ConstructParticleCut(AliFemtoConfigObject cfg)
{
  std::string classname;
  if (!cfg.pop_and_load("class", classname)) {
    TString msg = "Could not load string-property 'class' from object:\n" + cfg.Stringify(true);
    std::cerr << "[AliFemtoAnalysisPionPion::ConstructEventReader] " << msg;
    return nullptr;
  }

  #define TRY_CONSTRUCTING_CLASS(__name) (classname == #__name) ? (AliFemtoParticleCut*)(Configuration<__name>(cfg))

  AliFemtoParticleCut *result = /* TRY_CONSTRUCTING_CLASS(AliFemtoESDTrackCut)
                          //  : TRY_CONSTRUCTING_CLASS(AliFemtoEventReaderAODMultSelection)
                           : */ nullptr;
  #undef TRY_CONSTRUCTING_CLASS

  return result;

}

AliFemtoAnalysis*
AliFemtoAnalysisPionPion::BuildAnalysisFromConfiguration(AliFemtoConfigObject cfg)
{
  std::string classname;
  if (!cfg.pop_and_load("class", classname)) {
    TString msg = "Could not load string-property 'class' from object:\n" + cfg.Stringify(true);
    std::cerr << "[AliFemtoAnalysisPionPion::ConstructEventReader] " << msg;
    return nullptr;
  }

  #define TRY_CONSTRUCTING_CLASS(__name) (classname == #__name) ? (AliFemtoAnalysis*)(Configuration<__name>(cfg))

  AliFemtoAnalysis *result = TRY_CONSTRUCTING_CLASS(AliFemtoAnalysisPionPion)
                          //  : TRY_CONSTRUCTING_CLASS(AliFemtoEventReaderAODMultSelection)
                           : nullptr;
  #undef TRY_CONSTRUCTING_CLASS

  return result;
}

AliFemtoCorrFctn*
AliFemtoAnalysisPionPion::ConstructCorrelationFunction(AliFemtoConfigObject cfg)
{
  std::string classname;
  if (!cfg.pop_and_load("class", classname)) {
    TString msg = "Could not load string-property 'class' from object:\n" + cfg.Stringify(true);
    std::cerr << "[AliFemtoAnalysisPionPion::ConstructEventReader] " << msg;
    return nullptr;
  }

  #define TRY_CONSTRUCTING_CLASS(__name) (classname == #__name) ? (AliFemtoCorrFctn*)(Configuration<__name>(cfg))

  AliFemtoCorrFctn *result = TRY_CONSTRUCTING_CLASS(AliFemtoAvgSepCorrFctn)
                           : TRY_CONSTRUCTING_CLASS(AliFemtoModelCorrFctnTrueQ3D)
                           : TRY_CONSTRUCTING_CLASS(AliFemtoCorrFctnDirectYlm)
                           : TRY_CONSTRUCTING_CLASS(AliFemtoCorrFctn3DLCMSSym)
                           : TRY_CONSTRUCTING_CLASS(AliFemtoCorrFctnDPhiStarDEta)
                           : nullptr;
  #undef TRY_CONSTRUCTING_CLASS

  return result;
}
