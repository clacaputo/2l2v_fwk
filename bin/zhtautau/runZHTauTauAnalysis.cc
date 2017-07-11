#include <iostream>
#include <boost/shared_ptr.hpp>

#include "DataFormats/FWLite/interface/Handle.h"
#include "DataFormats/FWLite/interface/Event.h"
//#include "DataFormats/FWLite/interface/ChainEvent.h"
#include "DataFormats/Common/interface/MergeableCounter.h"

//Load here all the dataformat that we will need
#include "SimDataFormats/PileupSummaryInfo/interface/PileupSummaryInfo.h"
#include "SimDataFormats/GeneratorProducts/interface/LHEEventProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/PatCandidates/interface/Electron.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/PatCandidates/interface/Photon.h"
#include "DataFormats/PatCandidates/interface/MET.h"
#include "DataFormats/PatCandidates/interface/Tau.h"
#include "DataFormats/PatCandidates/interface/PackedTriggerPrescales.h"
#include "DataFormats/PatCandidates/interface/GenericParticle.h"

#include "CondFormats/JetMETObjects/interface/JetResolution.h"
#include "CondFormats/JetMETObjects/interface/JetCorrectionUncertainty.h"
#include "CondFormats/JetMETObjects/interface/JetCorrectorParameters.h"

#include "FWCore/FWLite/interface/FWLiteEnabler.h"
#include "FWCore/PythonParameterSet/interface/MakeParameterSets.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "PhysicsTools/Utilities/interface/LumiReWeighting.h"
#include "TauAnalysis/SVfitStandalone/interface/SVfitStandaloneAlgorithm.h" //for svfit

#include "EgammaAnalysis/ElectronTools/interface/ElectronEnergyCalibratorRun2.h"
#include "EgammaAnalysis/ElectronTools/interface/PhotonEnergyCalibratorRun2.h"

#include "UserCode/llvv_fwk/interface/MacroUtils.h"
#include "UserCode/llvv_fwk/interface/HiggsUtils.h"
#include "UserCode/llvv_fwk/interface/SmartSelectionMonitor.h"
#include "UserCode/llvv_fwk/interface/TMVAUtils.h"
#include "UserCode/llvv_fwk/interface/LeptonEfficiencySF.h"
#include "UserCode/llvv_fwk/interface/PDFInfo.h"
#include "UserCode/llvv_fwk/interface/rochcor2016.h"
#include "UserCode/llvv_fwk/interface/muresolution_run2.h"
#include "UserCode/llvv_fwk/interface/BTagCalibrationStandalone.h"
#include "UserCode/llvv_fwk/interface/BtagUncertaintyComputer.h"

#include "UserCode/llvv_fwk/interface/PatUtils.h"
#include "UserCode/llvv_fwk/interface/TrigUtils.h"
#include "UserCode/llvv_fwk/interface/EwkCorrections.h"
#include "UserCode/llvv_fwk/interface/ZZatNNLO.h"
#include "UserCode/llvv_fwk/interface/FRWeights.h"

// root includes

#include "TSystem.h"
#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TProfile.h"
#include "TProfile2D.h"
#include "TEventList.h"
#include "TROOT.h"
#include "TNtuple.h"
#include "TLorentzVector.h"
#include <Math/VectorUtil.h>

using namespace std;

// Additional functions

enum CRTypes {
  CR10,
  CR01,
  CR11,
  DEFAULT
};


LorentzVector getSVFit(pat::MET met, std::vector<patUtils::GenericLepton> selLeptons, int higgsCandL1, int higgsCandL2){
  if(higgsCandL1<0 || higgsCandL2<0) return LorentzVector(0,0,0,0);

  TMatrixD covMET(2, 2); // PFMET significance matrix
  // FIXME in MINIAODv2 74X, covariance is always 0000
   covMET[0][0] = met.getSignificanceMatrix()(0,0);
   covMET[0][1] = met.getSignificanceMatrix()(0,1);
   covMET[1][0] = met.getSignificanceMatrix()(1,0);
   covMET[1][1] = met.getSignificanceMatrix()(1,1);
  //  std::cout<<"MET MATRIX: " << covMET[0][0] << " " << covMET[0][1] << " " << covMET[1][0] << " " << covMET[1][1] << "\n";

  int dlid = abs( selLeptons[higgsCandL1].pdgId() * selLeptons[higgsCandL2].pdgId() );

  std::cout<<"\n"<<selLeptons[higgsCandL1].pdgId() << "  " << selLeptons[higgsCandL2].pdgId() << " Di-Tau ID ------------> " << dlid << std::endl;

  if ( dlid == 165 || dlid == 195){
    if (abs(selLeptons[higgsCandL1].pdgId()) == 15) {
      // Switching leptons in semileptonic pairs:
      // e and mu should be passed as first MesuredTauLepton
      int temp = higgsCandL1;
      higgsCandL1  = higgsCandL2;
      higgsCandL2  = temp;
    }
  }

  // covMET[0][0] = 0.95;  covMET[0][1] = 0.05; covMET[1][0] = 0.05; covMET[1][1] = 0.95;

  std::vector<svFitStandalone::MeasuredTauLepton> measuredTauLeptons;

   if ( dlid == 165 ){
        //std::cout<< " ETau Pair --- > "<< selLeptons[higgsCandL1].pdgId() << "  " << selLeptons[higgsCandL2].pdgId() << std::endl;
        measuredTauLeptons.push_back(svFitStandalone::MeasuredTauLepton(svFitStandalone::kTauToElecDecay, selLeptons[higgsCandL1].pt(), selLeptons[higgsCandL1].eta(),
                                   selLeptons[higgsCandL1].phi(), svFitStandalone::electronMass) );
        measuredTauLeptons.push_back(svFitStandalone::MeasuredTauLepton(svFitStandalone::kTauToHadDecay, selLeptons[higgsCandL2].pt(), selLeptons[higgsCandL2].eta(),
                                   selLeptons[higgsCandL2].phi(), selLeptons[higgsCandL2].mass(), selLeptons[higgsCandL2].tau.decayMode()) );
      }
  else if( dlid == 195 ){
      //std::cout<< " MuTau Pair --- > "<< selLeptons[higgsCandL1].pdgId() << "  " << selLeptons[higgsCandL2].pdgId() << std::endl;
      measuredTauLeptons.push_back(svFitStandalone::MeasuredTauLepton(svFitStandalone::kTauToMuDecay, selLeptons[higgsCandL1].pt(), selLeptons[higgsCandL1].eta(),
                                   selLeptons[higgsCandL1].phi(), svFitStandalone::muonMass) );
      measuredTauLeptons.push_back(svFitStandalone::MeasuredTauLepton(svFitStandalone::kTauToHadDecay, selLeptons[higgsCandL2].pt(), selLeptons[higgsCandL2].eta(),
                                   selLeptons[higgsCandL2].phi(), selLeptons[higgsCandL2].mass(), selLeptons[higgsCandL2].tau.decayMode()) );
    }
  else if ( dlid == 225 ){
      //std::cout<< " TauTau Pair --- > "<< selLeptons[higgsCandL1].pdgId() << "  " << selLeptons[higgsCandL2].pdgId() << std::endl;
       measuredTauLeptons.push_back(svFitStandalone::MeasuredTauLepton(svFitStandalone::kTauToHadDecay, selLeptons[higgsCandL1].pt(), selLeptons[higgsCandL1].eta(),
                                 selLeptons[higgsCandL1].phi(), selLeptons[higgsCandL1].mass(), selLeptons[higgsCandL1].tau.decayMode()) );
       measuredTauLeptons.push_back(svFitStandalone::MeasuredTauLepton(svFitStandalone::kTauToHadDecay, selLeptons[higgsCandL2].pt(), selLeptons[higgsCandL2].eta(),
                                 selLeptons[higgsCandL2].phi(), selLeptons[higgsCandL2].mass(), selLeptons[higgsCandL2].tau.decayMode()) );
    }
    else return LorentzVector(selLeptons[higgsCandL1].p4()+selLeptons[higgsCandL2].p4());
  // measuredTauLeptons.push_back(svFitStandalone::MeasuredTauLepton(abs(selLeptons[higgsCandL1].pdgId())==15?svFitStandalone::kTauToHadDecay:abs(selLeptons[higgsCandL1].pdgId())==11?svFitStandalone::kTauToElecDecay:svFitStandalone::kTauToMuDecay,
  //                               selLeptons[higgsCandL1].pt(),
  //                               selLeptons[higgsCandL1].eta(),
  //                               selLeptons[higgsCandL1].phi(),
  //                               selLeptons[higgsCandL1].mass() ));
  // measuredTauLeptons.push_back(svFitStandalone::MeasuredTauLepton(abs(selLeptons[higgsCandL2].pdgId())==15?svFitStandalone::kTauToHadDecay:abs(selLeptons[higgsCandL2].pdgId())==11?svFitStandalone::kTauToElecDecay:svFitStandalone::kTauToMuDecay,
  //                              selLeptons[higgsCandL2].pt(),
  //                              selLeptons[higgsCandL2].eta(),
  //                              selLeptons[higgsCandL2].phi(),
  //                              selLeptons[higgsCandL2].mass() ));

  SVfitStandaloneAlgorithm algo(measuredTauLeptons, met.px(), met.py() , covMET, 2);
  algo.addLogM(false);
  edm::FileInPath inputFileName_visPtResolution("TauAnalysis/SVfitStandalone/data/svFitVisMassAndPtResolutionPDF.root");
  TH1::AddDirectory(false);
  TFile* inputFile_visPtResolution = new TFile(inputFileName_visPtResolution.fullPath().data());
  algo.shiftVisPt(true, inputFile_visPtResolution);
  algo.integrateMarkovChain();

  double mass = algo.getMass(); // Full SVFit mass - return value is in units of GeV
  //double transverse_mass = algo.getTransverseMass(); // Transverse SVFit mass
  if ( algo.isValidSolution() ) {
    std::cout << "found mass = " << mass << std::endl;
  } else {
    std::cout << "sorry -- status of NLL is not valid [" << algo.fitStatus() << "]" << std::endl;
  }
  return LorentzVector(selLeptons[higgsCandL1].p4()+selLeptons[higgsCandL2].p4());
}


//**********************************************************************************************//
CRTypes checkBkgCR(std::vector<patUtils::GenericLepton> selLeptons, int higgsCandL1, int higgsCandL2, float isoElCut, float isoMuCut, const char* isoHaCut, float sumPtCut, reco::VertexCollection vtx){
//**********************************************************************************************//

  using namespace patUtils; //<---- needed for the cut version

  CRTypes theCR = CRTypes::DEFAULT;
  std::vector<patUtils::GenericLepton*> HiggsLegs = {&(selLeptons[higgsCandL1]), &(selLeptons[higgsCandL2])};

  std::vector<bool> passId;
  std::vector<bool> passIso;

  passId.resize(2);
  passIso.resize(2);

  for(auto lepIt=HiggsLegs.begin();lepIt!=HiggsLegs.end();lepIt++){
    patUtils::GenericLepton* lep = (*lepIt);
    if(abs(lep->pdgId())==11){
      passId.push_back(patUtils::passId(lep->el, vtx[0], patUtils::llvvElecId::Loose, CutVersion::CutSet::ICHEP16Cut, true));
      passIso.push_back((lep->userFloat("relIso") <= isoElCut));
    }else if(abs(lep->pdgId())==13){
      passId.push_back(patUtils::passId(lep->mu, vtx[0], patUtils::llvvMuonId::Loose, CutVersion::CutSet::ICHEP16Cut));
      passIso.push_back((lep->userFloat("relIso") <= isoMuCut));
    }else if(abs(lep->pdgId())==15){
      passId.push_back(lep->tau.tauID("againstElectronTightMVA6") && lep->tau.tauID("againstMuonLoose3"));
      passIso.push_back(bool(lep->tau.tauID(isoHaCut)));
    }
  }

  if( (!passId[0] || !passIso[0]) && (passId[1]&&passIso[1]) ) {
    theCR=CRTypes::CR10;
  } else if (  (passId[0] || passIso[0]) && (!passId[1] || !passIso[1] ) ) {
    theCR=CRTypes::CR01;
  } else if (  (!passId[0] || !passIso[0]) && (!passId[1] || !passIso[1])  ) {
    theCR=CRTypes::CR11;
  }

  return theCR;

}

//**********************************************************************************************//
bool passHiggsCuts(std::vector<patUtils::GenericLepton> selLeptons, int higgsCandL1, int higgsCandL2, float isoElCut, float isoMuCut, const char* isoHaCut, float sumPtCut, bool requireId, reco::VertexCollection vtx)
//**********************************************************************************************//
{
  if(higgsCandL1<0 || higgsCandL2<0)return false;
  std::vector<patUtils::GenericLepton*> HiggsLegs = {&(selLeptons[higgsCandL1]), &(selLeptons[higgsCandL2])};

  bool passId=true;
  bool passIso=true;
  float sumpt = 0;
  for(auto lepIt=HiggsLegs.begin();lepIt!=HiggsLegs.end();lepIt++){
    patUtils::GenericLepton* lep = (*lepIt);

    using namespace patUtils; //<----

    if(abs(lep->pdgId())==11){
      //        passId  &= patUtils::passId(lep->el, vtx[0], patUtils::llvvElecId::Loose);
      passId  &= patUtils::passId(lep->el, vtx[0], patUtils::llvvElecId::Loose, CutVersion::CutSet::ICHEP16Cut, true);
      passIso &= (lep->userFloat("relIso") <= isoElCut);
    }else if(abs(lep->pdgId())==13){
      //        passId  &= patUtils::passId(lep->mu, vtx[0], patUtils::llvvMuonId::Loose);
      passId  &= patUtils::passId(lep->mu, vtx[0], patUtils::llvvMuonId::Loose, CutVersion::CutSet::ICHEP16Cut);
      passIso &= (lep->userFloat("relIso") <= isoMuCut);
    }else if(abs(lep->pdgId())==15){
      passId  &= lep->tau.tauID("againstElectronTightMVA6") && lep->tau.tauID("againstMuonLoose3");
      passIso &= bool(lep->tau.tauID(isoHaCut));
    }
    sumpt += lep->pt();
  }
  return sumpt>sumPtCut && passIso && (passId || !requireId);
}



//**********************************************************************************************//
double closestJet(const LorentzVector& obj, pat::JetCollection& selJets, int& closestJetIndex)
//**********************************************************************************************//
{
  double dRMin = 1E100;  closestJetIndex = -1;
  for(int j=0;j<(int)selJets.size();j++){
    double dR = deltaR(selJets[j].p4(), obj);
    if(dR<dRMin){dRMin=dR; closestJetIndex=j;}
  }
  return dRMin;
}

 //**********************************************************************************************//
float getTheFRWeight(std::vector<patUtils::GenericLepton>& selLeptons, pat::JetCollection& selJets, int higgsCandL1, int higgsCandL2, FRWeights theFRWeightTool,float isoElCut, float isoMuCut, const char* isoHaCut, float sumPtCut,std::string CRType)
//**********************************************************************************************//
{
  float theFinalWeight=1;

  std::vector<patUtils::GenericLepton*> HiggsLegs = {&(selLeptons[higgsCandL1]), &(selLeptons[higgsCandL2])};
  std::vector<float> theWeights;

  for(auto lepIt=HiggsLegs.begin();lepIt!=HiggsLegs.end();lepIt++){
    patUtils::GenericLepton* lep = (*lepIt);

    int closestJetIndex=-1; double pTclosestJet=-1; double etaclosestJet=-1;
    std::string etabin = "";
    std::string isobin = "";

    double dRmin = closestJet(lep->p4(), selJets, closestJetIndex);
    if(closestJetIndex>=0 && dRmin<0.5){pTclosestJet=selJets[closestJetIndex].pt(); etaclosestJet=abs(selJets[closestJetIndex].eta());}

    if(sumPtCut<30){
      etabin = etaclosestJet <1.4 ? "_B" : "_E";
    } else {
      etabin = etaclosestJet <1.4 ? "_TMCut_B" : "_TMCut_E";
    }

    if(abs(lep->pdgId())==11){

      if(isoElCut<=0.1) isobin= "_Id_Iso01weight";
      if(isoElCut<=0.2) isobin= "_Id_Iso02weight";
      if(isoElCut<=0.3) isobin= "_Id_Iso03weight";

      theWeights.push_back(theFRWeightTool.getWeight("FR_El",etabin,isobin,"_wrtJetPt",pTclosestJet));

    } else if (abs(lep->pdgId())==13){

      if(isoMuCut<=0.1) isobin= "_Id_Iso01weight";
      if(isoMuCut<=0.2) isobin= "_Id_Iso02weight";
      if(isoMuCut<=0.3) isobin= "_Id_Iso03weight";

      theWeights.push_back(theFRWeightTool.getWeight("FR_Mu",etabin,isobin,"_wrtJetPt",pTclosestJet));

     } else if(abs(lep->pdgId())==15){

      if(strcmp(isoHaCut,"byLooseCombinedIsolationDeltaBetaCorr3Hits")==0)    isobin = "_Id_IsoLoweight";
      if(strcmp(isoHaCut,"byMediumCombinedIsolationDeltaBetaCorr3Hits")==0) isobin = "_Id_IsoMeweight";

      theWeights.push_back(theFRWeightTool.getWeight("FT_Ta",etabin,isobin,"_wrtJetPt",pTclosestJet));

    }
  }

  if(CRType=="CR10"){
    theFinalWeight = theWeights[0] / (1 - theWeights[0]);
  } else if (CRType=="CR01"){
    theFinalWeight = theWeights[1] / (1 - theWeights[1]);
  } else if (CRType=="CR11"){
    theFinalWeight = - (theWeights[0]*theWeights[1]) / (1 + theWeights[0]*theWeights[1] - theWeights[0] -theWeights[1] );
  } else {
    cout << "unrecognized Control Region" << endl;
  }

  return theFinalWeight;

}

//**********************************************************************************************//
std::vector<patUtils::GenericLepton> getLepVariations(  std::vector<patUtils::GenericLepton>& selLeptons, float factor)
//**********************************************************************************************//
{
  std::vector<patUtils::GenericLepton> selLeptonsNew;
  for(size_t ilep=0; ilep<selLeptons.size(); ilep++){
    if(abs(selLeptons[ilep].pdgId())!=15){
      patUtils::GenericLepton selLeptonNew = selLeptons[ilep];
      selLeptonNew.setP4(selLeptons[ilep].p4() * factor);
      selLeptonsNew.push_back(selLeptonNew);
    }else{
      selLeptonsNew.push_back(selLeptons[ilep]);
    }
  }
  std::sort(selLeptonsNew.begin(), selLeptonsNew.end(), utils::sort_CandidatesByPt);
  return selLeptonsNew;
}

//**********************************************************************************************//
std::vector<patUtils::GenericLepton> getTauVariations( std::vector<patUtils::GenericLepton>& selLeptons,float factor)
//**********************************************************************************************//
{
  std::vector<patUtils::GenericLepton> selLeptonsNew;
  for(size_t ilep=0; ilep<selLeptons.size(); ilep++){
    if(abs(selLeptons[ilep].pdgId())==15){
      patUtils::GenericLepton selLeptonNew = selLeptons[ilep];
      selLeptonNew.setP4(selLeptons[ilep].p4() * factor);
      selLeptonsNew.push_back(selLeptonNew);
    }else{
      selLeptonsNew.push_back(selLeptons[ilep]);
    }
  }
  std::sort(selLeptonsNew.begin(), selLeptonsNew.end(), utils::sort_CandidatesByPt);
  return selLeptonsNew;
}

int main(int argc, char* argv[])
{
  //##############################################
  //########    GLOBAL INITIALIZATION     ########
  //##############################################

  // check arguments
  if(argc<2){ std::cout << "Usage : " << argv[0] << " parameters_cfg.py" << std::endl; exit(0); }

  // load framework libraries
  gSystem->Load( "libFWCoreFWLite" );
  FWLiteEnabler::enable();

  // configure the process
  const edm::ParameterSet &runProcess = edm::readPSetsFrom(argv[1])->getParameter<edm::ParameterSet>("runProcess");

  bool isMC = runProcess.getParameter<bool>("isMC");
  double xsec = runProcess.getParameter<double>("xsec");
  int mctruthmode=runProcess.getParameter<int>("mctruthmode");
  TString dtag=runProcess.getParameter<std::string>("dtag");

  TString suffix=runProcess.getParameter<std::string>("suffix");
  std::vector<std::string> urls=runProcess.getUntrackedParameter<std::vector<std::string> >("input");
  TString outUrl = runProcess.getParameter<std::string>("outfile");

  //good lumi MASK
  lumiUtils::GoodLumiFilter goodLumiFilter(runProcess.getUntrackedParameter<std::vector<edm::LuminosityBlockRange> >("lumisToProcess", std::vector<edm::LuminosityBlockRange>()));

  bool filterOnlyEE(false), filterOnlyMUMU(false), filterOnlyEMU(false), filterOnlyPhoton(false), filterOnlyE(false), filterOnlyMU(false);
  if(!isMC){
    if(dtag.Contains("DoubleElectron"))   filterOnlyEE=true;
    if(dtag.Contains("DoubleMuon"))    filterOnlyMUMU=true;
    if(dtag.Contains("MuEG"))        filterOnlyEMU=true;
    if(dtag.Contains("SinglePhoton"))filterOnlyPhoton=true;
    if(dtag.Contains("SingleMuon"))    filterOnlyMU=true;
    if(dtag.Contains("SingleElectron"))filterOnlyE=true;
  }
  bool isV0JetsMC(false);//isMC && (dtag.Contains("DYJetsToLL_50toInf") || dtag.Contains("_WJets")));  #FIXME should be reactivated as soon as we have exclusive jet samples
  bool isWGmc(isMC && dtag.Contains("WG"));
  bool isZGmc(isMC && dtag.Contains("ZG"));
  bool isMC_ZZ  = isMC && ( string(dtag.Data()).find("TeV_ZZ")  != string::npos);
  bool isMC_ZZ2l2nu  = isMC && ( string(dtag.Data()).find("TeV_ZZ2l2nu")  != string::npos);
  bool isMC_WZ  = isMC && ( string(dtag.Data()).find("TeV_WZ")  != string::npos);
  bool isMC_WZ3lnu  = isMC && ( string(dtag.Data()).find("TeV_WZ3lnu")  != string::npos);
  bool isMC_QCD = (isMC && dtag.Contains("QCD"));
  bool isMC_GJet = (isMC && dtag.Contains("GJet"));
  bool is2015data = (!isMC && dtag.Contains("2015"));
  bool is2015MC = (isMC && dtag.Contains("2015"));
  bool is2016data = (!isMC && dtag.Contains("2016"));
  bool is2016MC = (isMC);


  //tree info
  TString dirname = runProcess.getParameter<std::string>("dirName");

  //systematics
  bool runSystematics                        = runProcess.getParameter<bool>("runSystematics");
  std::vector<TString> varNames(1,"");

  std::vector<string> jetVarNames = {"", "_scale_jup","_scale_jdown", "_res_jup", "_res_jdown"};


  if(runSystematics){
    if(true){
      varNames.push_back("_scale_umetup"); varNames.push_back("_scale_umetdown");    //unclustered met
      varNames.push_back("_res_jup");      varNames.push_back("_res_jdown");    //jet energy resolution
      varNames.push_back("_scale_jup");    varNames.push_back("_scale_jdown");  //jet energy scale
      varNames.push_back("_scale_mup");    varNames.push_back("_scale_mdown");  //muon energy scale
      varNames.push_back("_scale_eup");    varNames.push_back("_scale_edown");  //electron energy scale
      varNames.push_back("_puup");         varNames.push_back("_pudown");      //pileup uncertainty
      varNames.push_back("_eff_bup");      varNames.push_back("_eff_bdown");    //btag veto
      varNames.push_back("_lepveto");                                           //3rd lepton veto
      varNames.push_back("_th_factup");    varNames.push_back("_th_factdown"); //factorization and renormalization scales
      varNames.push_back("_th_pdf");                                           //pdf
      varNames.push_back("_th_alphas");                                         //alpha_s (QCD)
    }
    if(isMC_ZZ || isMC_WZ){
      varNames.push_back("_th_ewkup"); varNames.push_back("_th_ewkdown"); //EWK+QCD corrections
    }
  }
  size_t nvarsToInclude=varNames.size();


  //ELECTROWEAK CORRECTION WEIGHTS
  std::vector<std::vector<float>> ewkTable, ZZ_NNLOTable;
  if(isMC_ZZ2l2nu){
        ewkTable = EwkCorrections::readFile_and_loadEwkTable(dtag);
        ZZ_NNLOTable = ZZatNNLO::readFile_and_loadTable(dtag);
  }
  if(isMC_WZ3lnu){
        ewkTable = EwkCorrections::readFile_and_loadEwkTable(dtag);
  }


  //##############################################
  //########    INITIATING HISTOGRAMS     ########
  //##############################################
  SmartSelectionMonitor mon;
  printf("Definition of plots");

  //event selection
  TH1 *h1=mon.addHistogram( new TH1F ("eventflow", ";;Events", 10,0,10) );
  h1->GetXaxis()->SetBinLabel(1,"InitialEv");
  h1->GetXaxis()->SetBinLabel(2,"Nlep#geq2");
  h1->GetXaxis()->SetBinLabel(3,"Zmass");
  h1->GetXaxis()->SetBinLabel(4,"Zkin");
  h1->GetXaxis()->SetBinLabel(5,"Nlep+Ntau#geq4");
  h1->GetXaxis()->SetBinLabel(6,"Lep Veto");
  h1->GetXaxis()->SetBinLabel(7,"Btag Veto");
  h1->GetXaxis()->SetBinLabel(8,"#Delta #phi Z-MET");
  h1->GetXaxis()->SetBinLabel(9,"di-#tau Cand");

  TH1 *h_tr= mon.addHistogram( new TH1F ("trigger", ";;Events", 10,0,10) );
  h_tr->GetXaxis()->SetBinLabel(1,"#mu#mu");
  h_tr->GetXaxis()->SetBinLabel(2,"#mu");
  h_tr->GetXaxis()->SetBinLabel(3,"ee");
  h_tr->GetXaxis()->SetBinLabel(4,"e");
  h_tr->GetXaxis()->SetBinLabel(5,"e#mu");
  h_tr->GetXaxis()->SetBinLabel(6,"#gamma");

  TH1 *h2=mon.addHistogram( new TH1F ("yields", ";;Events", 25,0,25) );
  h2->GetXaxis()->SetBinLabel(1,"OS eeee");
  h2->GetXaxis()->SetBinLabel(2,"OS ee#mu#mu");
  h2->GetXaxis()->SetBinLabel(3,"OS eee#mu");
  h2->GetXaxis()->SetBinLabel(4,"OS eee#tau");
  h2->GetXaxis()->SetBinLabel(5,"OS ee#mu#tau");
  h2->GetXaxis()->SetBinLabel(6,"OS ee#tau#tau");
  h2->GetXaxis()->SetBinLabel(7,"OS #mu#muee");
  h2->GetXaxis()->SetBinLabel(8,"OS #mu#mu#mu#mu");
  h2->GetXaxis()->SetBinLabel(9,"OS #mu#mue#mu");
  h2->GetXaxis()->SetBinLabel(10,"OS #mu#mue#tau");
  h2->GetXaxis()->SetBinLabel(11,"OS #mu#mu#mu#tau");
  h2->GetXaxis()->SetBinLabel(12,"OS #mu#mu#tau#tau");
  h2->GetXaxis()->SetBinLabel(13,"SS eeee");
  h2->GetXaxis()->SetBinLabel(14,"SS ee#mu#mu");
  h2->GetXaxis()->SetBinLabel(15,"SS eee#mu");
  h2->GetXaxis()->SetBinLabel(16,"SS eee#tau");
  h2->GetXaxis()->SetBinLabel(17,"SS ee#mu#tau");
  h2->GetXaxis()->SetBinLabel(18,"SS ee#tau#tau");
  h2->GetXaxis()->SetBinLabel(19,"SS #mu#muee");
  h2->GetXaxis()->SetBinLabel(20,"SS #mu#mu#mu#mu");
  h2->GetXaxis()->SetBinLabel(21,"SS #mu#mue#mu");
  h2->GetXaxis()->SetBinLabel(22,"SS #mu#mue#tau");
  h2->GetXaxis()->SetBinLabel(23,"SS #mu#mu#mu#tau");
  h2->GetXaxis()->SetBinLabel(24,"SS #mu#mu#tau#tau");

  TH1 *h3=mon.addHistogram( new TH1F ("yieldsOS", ";;Events", 12,0,12) );
  h3->GetXaxis()->SetBinLabel(1,"OS eeee");
  h3->GetXaxis()->SetBinLabel(2,"OS ee#mu#mu");
  h3->GetXaxis()->SetBinLabel(3,"OS eee#mu");
  h3->GetXaxis()->SetBinLabel(4,"OS eee#tau");
  h3->GetXaxis()->SetBinLabel(5,"OS ee#mu#tau");
  h3->GetXaxis()->SetBinLabel(6,"OS ee#tau#tau");
  h3->GetXaxis()->SetBinLabel(7,"OS #mu#muee");
  h3->GetXaxis()->SetBinLabel(8,"OS #mu#mu#mu#mu");
  h3->GetXaxis()->SetBinLabel(9,"OS #mu#mue#mu");
  h3->GetXaxis()->SetBinLabel(10,"OS #mu#mue#tau");
  h3->GetXaxis()->SetBinLabel(11,"OS #mu#mu#mu#tau");
  h3->GetXaxis()->SetBinLabel(12,"OS #mu#mu#tau#tau");

  mon.addHistogram( new TH1F( "muiso"     ,  ";I_{#mu};Events", 100,0.,1.) );
  mon.addHistogram( new TH1F( "eleiso"     ,  ";I_{ele};Events", 100,0.,1.) );
  //Z leptons kinematics control
  mon.addHistogram( new TH1F( "leadpt"      ,  ";p_{T}^{lead} (GeV);Events/10 GeV", 50,0,500) );
  mon.addHistogram( new TH1F( "leadeta"     ,  ";#eta_{lead};Events", 50,-2.6,2.6) );
  // mon.addHistogram( new TH1F( "leadiso"     ,  ";#I_{lead};Events", 20,0.,1.) );
  mon.addHistogram( new TH1F( "trailerpt"   ,  ";p_{T}^{trail} (GeV);Events/10 GeV", 50,0,500) );
  mon.addHistogram( new TH1F( "trailereta"  ,  ";#eta_{trail};Events", 50,-2.6,2.6) );
  // mon.addHistogram( new TH1F( "traileriso"     ,  ";#I_{trail};Events", 20,0.,1.) );
  mon.addHistogram( new TH1F( "leppt"       ,  ";p_{T}^{lepton} (GeV);Events/10 GeV", 50,0,500) );
  mon.addHistogram( new TH1F( "lepeta"      ,  ";#eta_{lepton};Events", 50,-2.6,2.6) );

  // zll control
  mon.addHistogram( new TH1F( "zlly",      		";y_{ll};Events", 50,-6,6) );
  mon.addHistogram( new TH1F( "zlleta",    		";#eta_{ll};Events", 50,-10,10) );
  mon.addHistogram( new TH1F( "zllpt",     		";p_{T}^{ll} (GeV) ;Events/10 GeV", 50,0,500) );
  mon.addHistogram( new TH1F( "zllmass",   		";M_{ll} (GeV);Events/2 GeV", 80,20,180) );
  mon.addHistogram( new TH1F( "nlep",      	";Number of Leptons;Events", 10,0,10) );

  mon.addHistogram( new TH1F( "sumpt",            ";L_{T} (GeV);Events/10 GeV", 50,0,500) );
  mon.addHistogram( new TH1F( "dPhi_AZ",          ";#DeltaPhi(#tau#tau,ll);Events",50,-3,3));
  mon.addHistogram( new TH1F( "dPhi_AMet",        ";#Delta#phi(#tau#tau,#slash{E}_{T});Events",50,-3,3));
  mon.addHistogram( new TH1F( "met",             ";#slash{E}_{T} (GeV);Events/10 GeV",50,0,500));

  mon.addHistogram( new TH1F( "Amet",             ";#slash{E}_{T} (GeV);Events/10 GeV",50,0,500));
  mon.addHistogram( new TH1F( "Anjets",           ";Number of Jets;Events",10,-0.5,9.5));
  mon.addHistogram( new TH1F( "Apt",              ";p_{T}^{#tau#tau} (GeV);Events/10 GeV",50,0,500));
  mon.addHistogram( new TH1F( "Hpt",              ";p_{T}^{ll#tau#tau} (GeV);Events/10 GeV",50,0,500));

  double bins[]={5, 30,70,110,190,300,550,1800};
  int nbins=sizeof(bins)/sizeof(double) - 1;
  mon.addHistogram( new TH1F( "Amass",            ";M_{#tau#tau} (GeV);Events",nbins,bins));
  mon.addHistogram( new TH1F( "Hmass",            ";M_{ll#tau#tau} (GeV);Events",nbins,bins));
  mon.addHistogram( new TH1F( "Amasssvfit",       ";SVFit M_{#tau#tau} (GeV);Events",nbins,bins));
  mon.addHistogram( new TH1F( "Hmasssvfit",       ";SVFit M_{ll#tau#tau} (GeV);Events",nbins,bins));

  //pu control
  mon.addHistogram( new TH1F( "nvtx",";Vertices;Events",50,0,50) );
  mon.addHistogram( new TH1F( "nvtxraw",";Vertices;Events",50,0,50) );
  mon.addHistogram( new TH1F( "nvtxpuweight",";Vertices;Events",50,0,50) );
  mon.addHistogram( new TH1F( "rho",";#rho;Events",50,0,25) );

  //tau control
  mon.addHistogram( new TH1F( "ntaus",      	";Number of Taus;Events", 10,0,10) );
  mon.addHistogram( new TH1F( "tauleadpt",  	";p_{T}^{#tau} (GeV);Events/10 GeV", 50,0,500) );
  mon.addHistogram( new TH1F( "tauleadeta", 	";#eta_{#tau};Events", 50,-2.6,2.6) );
  mon.addHistogram( new TH1F( "tautrailerpt",  	";p_{T}^{#tau} (GeV);Events/10 GeV", 50,0,500) );
  mon.addHistogram( new TH1F( "tautrailereta", 	";#eta_{#tau};Events", 50,-2.6,2.6) );
  mon.addHistogram( new TH1F( "taupt",  		";p_{T}^{#tau} (GeV);Events/10 GeV", 50,0,500) );
  mon.addHistogram( new TH1F( "taueta", 	";#eta_{#tau};Events", 50,-2.6,2.6) );

  //extra leptons in the event

  TH1 *hbtags=mon.addHistogram( new TH1F("nbtags",   ";b-tag multiplicity;Events",5,0,5) );
  TH1 *hjets=mon.addHistogram( new TH1F("njets",  ";Jet multiplicity;Events",5,0,5) );
  for(int ibin=1; ibin<=hjets->GetXaxis()->GetNbins(); ibin++){
    TString label("");
    if(ibin==hjets->GetXaxis()->GetNbins()) label +="#geq";
    else                                    label +="=";
    label += (ibin-1);
    hjets->GetXaxis()->SetBinLabel(ibin,label);
    hbtags->GetXaxis()->SetBinLabel(ibin,label);
  }

  //fake rate histograms
  float ptbinsJets[] = {10, 20, 30, 40, 60, 80, 100, 125, 150, 175,250};
  int ptbinsJetsN = sizeof(ptbinsJets)/sizeof(float)-1;
  mon.addHistogram( new TH1F( "wrtJetPt",  ";Jet p_{T} (GeV);Events",sizeof(ptbinsJets)/sizeof(float)-1,ptbinsJets));
  mon.addHistogram( new TH1F( "wrtLepPt",  ";Lep p_{T} (GeV);Events",sizeof(ptbinsJets)/sizeof(float)-1,ptbinsJets));


  //
  // HISTOGRAMS FOR OPTIMIZATION and STATISTICAL ANALYSIS
  //
  //

  std::vector<const char*> tauIDiso = {"byLooseCombinedIsolationDeltaBetaCorr3Hits","byLooseIsolationMVArun2v1DBoldDMwLT",
                                        "byLooseIsolationMVArun2v1DBdR03oldDMwLT"};
  std::vector<float>    optim_Cuts_sumPt;
  std::vector<int>      optim_Cuts_taIso;
  std::vector<float>    optim_Cuts_muIso;
  std::vector<float>    optim_Cuts_elIso;

  for(float elIso=0.30;elIso>=0.30;elIso-=0.1){
    for(float muIso=0.3;muIso>=0.30;muIso-=0.1){
      for(int taIso=0;taIso<tauIDiso.size();taIso++){
        for(float sumPt=0;sumPt<=200;sumPt+=20){
          optim_Cuts_elIso.push_back(elIso);
          optim_Cuts_muIso.push_back(muIso);
          optim_Cuts_taIso.push_back(taIso);
          optim_Cuts_sumPt.push_back(sumPt);
        }
      }
    }
  }

  TH2F* Hoptim_cuts  =(TH2F*)mon.addHistogram(new TProfile2D("optim_cut",      ";cut index;variable",       optim_Cuts_sumPt.size(),0,optim_Cuts_sumPt.size(), 4, 0, 4)) ;
  Hoptim_cuts->GetYaxis()->SetBinLabel(1, "eIso<");
  Hoptim_cuts->GetYaxis()->SetBinLabel(2, "muIso<");
  Hoptim_cuts->GetYaxis()->SetBinLabel(3, "tauIso<");
  Hoptim_cuts->GetYaxis()->SetBinLabel(4, "sumPt>");

  for(unsigned int index=0;index<optim_Cuts_sumPt.size();index++){
    Hoptim_cuts->Fill(index,0.0,optim_Cuts_elIso[index]);
    Hoptim_cuts->Fill(index,1.0,optim_Cuts_muIso[index]);
    Hoptim_cuts->Fill(index,2.0,optim_Cuts_taIso[index]);
    Hoptim_cuts->Fill(index,3.0,optim_Cuts_sumPt[index]);
  }

  TH1F* Hoptim_systs     =  (TH1F*) mon.addHistogram( new TH1F ("optim_systs"    , ";syst;", nvarsToInclude,0,nvarsToInclude) ) ;
  for(size_t ivar=0; ivar<nvarsToInclude; ivar++){
    Hoptim_systs->GetXaxis()->SetBinLabel(ivar+1, varNames[ivar]);
    mon.addHistogram( new TH2F (TString("Hsvfit_shapes")+varNames[ivar],";cut index;M_{ll#tau#tau};Events",optim_Cuts_sumPt.size(),0,optim_Cuts_sumPt.size(),nbins,bins) );
    mon.addHistogram( new TH2F (TString("Hsvfit_shapes_CR01")+varNames[ivar],";cut index;M_{ll#tau#tau};Events",optim_Cuts_sumPt.size(),0,optim_Cuts_sumPt.size(),nbins,bins) );
    mon.addHistogram( new TH2F (TString("Hsvfit_shapes_CR10")+varNames[ivar],";cut index;M_{ll#tau#tau};Events",optim_Cuts_sumPt.size(),0,optim_Cuts_sumPt.size(),nbins,bins) );
    mon.addHistogram( new TH2F (TString("Hsvfit_shapes_CR11")+varNames[ivar],";cut index;M_{ll#tau#tau};Events",optim_Cuts_sumPt.size(),0,optim_Cuts_sumPt.size(),nbins,bins) );
    mon.addHistogram( new TH2F (TString("Asvfit_shapes")+varNames[ivar],";cut index;M_{#tau#tau};Events",optim_Cuts_sumPt.size(),0,optim_Cuts_sumPt.size(),nbins,bins) );
    mon.addHistogram( new TH2F (TString("Asvfit_shapes_CR01")+varNames[ivar],";cut index;M_{#tau#tau};Events",optim_Cuts_sumPt.size(),0,optim_Cuts_sumPt.size(),nbins,bins) );
    mon.addHistogram( new TH2F (TString("Asvfit_shapes_CR10")+varNames[ivar],";cut index;M_{#tau#tau};Events",optim_Cuts_sumPt.size(),0,optim_Cuts_sumPt.size(),nbins,bins) );
    mon.addHistogram( new TH2F (TString("Asvfit_shapes_CR11")+varNames[ivar],";cut index;M_{#tau#tau};Events",optim_Cuts_sumPt.size(),0,optim_Cuts_sumPt.size(),nbins,bins) );
    mon.addHistogram( new TH1F(TString("metsys")+varNames[ivar],                   ";#slash{E}_{T} (GeV);Events/10 GeV",50,0,500));
  }


  //##############################################
  //######## GET READY FOR THE EVENT LOOP ########
  //##############################################
  //MC normalization (to 1/pb)
  double xsecWeight = 1.0;
  if(isMC) xsecWeight=xsec/utils::getTotalNumberOfEvents(urls, false, true);//need to use the slow method in order to take NLO negative events into account

  //MET CORRection level
  pat::MET::METCorrectionLevel metcor = pat::MET::METCorrectionLevel::Type1XY;

  //jet energy scale and uncertainties
  TString jecDir = runProcess.getParameter<std::string>("jecDir");
  gSystem->ExpandPathName(jecDir);
  if (is2016data) {
      if(dtag.Contains("2016H")) jecDir+="Moriond17_80X/PromptReco_DATA/";
      else jecDir+="Moriond17_80X/2016SeptRepro_DATA/";
  } else jecDir+="Moriond17_80X/Summer16_re-digi-reco_MC/";

  FactorizedJetCorrector *jesCor        = utils::cmssw::getJetCorrector(jecDir,isMC);
  TString pf(isMC ? "MC" : "DATA");
  JetCorrectionUncertainty *totalJESUnc = new JetCorrectionUncertainty((jecDir+"/"+pf+"_Uncertainty_AK4PFchs.txt").Data());

  //muon energy scale and uncertainties
  TString muscleDir = runProcess.getParameter<std::string>("muscleDir");
  gSystem->ExpandPathName(muscleDir);
  rochcor2016* muCor2016 = new rochcor2016();  //replace the MuScleFitCorrector we used at run1

  //photon and electron enerhy scale based on https://twiki.cern.ch/twiki/bin/viewauth/CMS/EGMSmearer    (adapted to the miniAOD/FWLite framework)
  // std::vector<double> EGammaSmearings = {0.013654,0.014142,0.020859,0.017120,0.028083,0.027289,0.031793,0.030831,0.028083, 0.027289};
  // std::vector<double> EGammaScales    = {0.99544,0.99882,0.99662,1.0065,0.98633,0.99536,0.97859,0.98567,0.98633, 0.99536};
  // PhotonEnergyCalibratorRun2 PhotonEnCorrector(isMC, false, EGammaSmearings, EGammaScales);
  std::string photonESC = "EgammaAnalysis/ElectronTools/data/ScalesSmearings/Moriond17_74x_pho";//string(std::getenv("CMSSW_BASE"))+"/src/EgammaAnalysis/ElectronTools/data/ScalesSmearings/Moriond17_74x_pho";
  // std::string photonSmearings = "../../../../EgammaAnalysis/ElectronTools/data/ScalesSmearings/Moriond17_74x_pho_smearings.dat";
  // std::string correctionFile = isMC ? photonSmearings : photonScales;

  //--- photonESC initializes an EnergyScaleCorrection_class that provides the full file path
  PhotonEnergyCalibratorRun2 PhotonEnCorrector(isMC, false, photonESC);
  PhotonEnCorrector.initPrivateRng(new TRandom(1234));


  /*
  New energy scale comment out
  */
  // // EpCombinationTool theEpCombinationTool;
  // EpCombinationToolSemi theEpCombinationTool;
  // // EpCombinationToolSemi take as argument a std::vector of GBRForestD [CondFormats/EgammaObjects/interface/GBRForestD.h]
  // //-----
  // // Inspired by https://github.com/cms-analysis/JetMETCorrections-METPUSubtraction/blob/master/src/PFMETAlgorithmMVA.cc#L21
  // std::string inputFileName = "UserCode/llvv_fwk/data/weights/GBRForest_data_25ns.root";//string( std::getenv("CMSSW_BASE") ) + "/src/UserCode/llvv_fwk/data/weights/GBRForest_data_25ns.root";
  // std::string mvaName = "gedelectron_p4combination_25ns";
  // TFile* inputFile = new TFile(edm::FileInPath(inputFileName).fullPath().c_str());
  //
  // //const GBRForest* mva = dynamic_cast<GBRForest*>(inputFile->Get(mvaName.data())); // CV: dynamic_cast<GBRForest*> fails for some reason ?!
  // std::vector<const GBRForestD*> forest;
  // const GBRForestD* mva = (GBRForestD*)inputFile->Get(mvaName.data());
  // if ( !mva )
  //   throw cms::Exception("PFMETAlgorithmMVA::loadMVA")
  //       << " Failed to load MVA = " << mvaName.data() << " from file = " << inputFileName.data() << " !!\n";
  // forest.push_back(mva);
  //
  // //-----
  // // theEpCombinationTool.init((string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/weights/GBRForest_data_25ns.root").c_str(), "gedelectron_p4combination_25ns");  //got confirmation from Matteo Sani that this works for both data and MC
  // theEpCombinationTool.init(forest);
  // std::string electronESC = "EgammaAnalysis/ElectronTools/data/ScalesSmearings/Moriond17_23Jan_ele";//string(std::getenv("CMSSW_BASE"))+"/src/EgammaAnalysis/ElectronTools/data/ScalesSmearings/Moriond17_23Jan_ele";
  // // std::string photonSmearings = "../../../../EgammaAnalysis/ElectronTools/data/ScalesSmearings/Moriond17_74x_pho_smearings.dat";
  // // std::string correctionFile = isMC ? photonSmearings : photonScales;
  // ElectronEnergyCalibratorRun2 ElectronEnCorrector(theEpCombinationTool, isMC, false, electronESC);
  // ElectronEnCorrector.initPrivateRng(new TRandom(1234));

  EnergyScaleCorrection_class eScaler("EgammaAnalysis/ElectronTools/data/ScalesSmearings/Moriond17_23Jan_ele");
  eScaler.doScale=true;
  eScaler.doSmearings=true;


  //lepton efficiencies
  LeptonEfficiencySF lepEff;

  FRWeights theFRWeightTool;
  theFRWeightTool.init((string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/test/zhtautau/FR_Weights.root").c_str());

  //b-tagging: beff and leff must be derived from the MC sample using the discriminator vs flavor
  //the scale factors are taken as average numbers from the pT dependent curves see:
  //https://twiki.cern.ch/twiki/bin/viewauth/CMS/BtagPOG#2012_Data_and_MC_EPS13_prescript
  BTagSFUtil btsfutil;
  float beff(0.68), sfb(0.99), sfbunc(0.015);
  float leff(0.13), sfl(1.05), sflunc(0.12);

  //double btagLoose = 0.605; //https://twiki.cern.ch/twiki/bin/viewauth/CMS/BtagRecommendation74X
  double btagLoose = 0.5426;  //Moriond17 recommendation Loose

  // // setup calibration readers
  // BTagCalibration btagCalib("CSVv2", string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/weights/btagSF_CSVv2.csv");
  // BTagCalibrationReader btagCal   (&btagCalib, BTagEntry::OP_LOOSE, "mujets", "central");  // calibration instance, operating point, measurement type, systematics type
  // BTagCalibrationReader btagCalUp (&btagCalib, BTagEntry::OP_LOOSE, "mujets", "up"     );  // sys up
  // BTagCalibrationReader btagCalDn (&btagCalib, BTagEntry::OP_LOOSE, "mujets", "down"   );  // sys down
  // BTagCalibrationReader btagCalL  (&btagCalib, BTagEntry::OP_LOOSE, "comb", "central");  // calibration instance, operating point, measurement type, systematics type
  // BTagCalibrationReader btagCalLUp(&btagCalib, BTagEntry::OP_LOOSE, "comb", "up"     );  // sys up
  // BTagCalibrationReader btagCalLDn(&btagCalib, BTagEntry::OP_LOOSE, "comb", "down"   );  // sys down

  // setup calibration readers 80X
  BTagCalibration btagCalib("CSVv2", string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/weights/CSVv2_Moriond17_B_H.csv");

  BTagCalibrationReader80X btagCal80X   (BTagEntry::OP_LOOSE, "central", {"up", "down"});
  btagCal80X.load(btagCalib, BTagEntry::FLAV_B, "comb");
  btagCal80X.load(btagCalib, BTagEntry::FLAV_C, "comb");
  btagCal80X.load(btagCalib, BTagEntry::FLAV_UDSG, "incl");


  // from Btag SF and eff from https://indico.cern.ch/event/437675/#preview:1629681
  // beff = 0.747; sfb = 0.899; //for Loose WP  //sfb is not actually used as it's taken from btagCal
  //
  //beff = 0.836; sfb = 0.920; //for Loose WP  //sfb is from page 7 https://indico.cern.ch/event/557018/contributions/2246312/attachments/1310986/1961665/csvSF_rwt_July18th_2016.pdf
  //leff = 0.139;
  beff = 0.827; sfb = 0.980; //for Loose WP  //sfb is from page 7 https://indico.cern.ch/event/557018/contributions/2246312/attachments/1310986/1961665/csvSF_rwt_July18th_2016.pdf
  leff = 0.132;

  //pileup weighting
  edm::LumiReWeighting* LumiWeights = NULL;
  utils::cmssw::PuShifter_t PuShifters;
  double PUNorm[] = {1,1,1};
  if(isMC){
    std::vector<double> dataPileupDistributionDouble = runProcess.getParameter< std::vector<double> >("datapileup");
    std::vector<float> dataPileupDistribution; for(unsigned int i=0;i<dataPileupDistributionDouble.size();i++){dataPileupDistribution.push_back(dataPileupDistributionDouble[i]);}
    std::vector<float> mcPileupDistribution;

    utils::getMCPileupDistributionFromMiniAOD(urls,dataPileupDistribution.size(), mcPileupDistribution);
    while(mcPileupDistribution.size()<dataPileupDistribution.size())  mcPileupDistribution.push_back(0.0);
    while(mcPileupDistribution.size()>dataPileupDistribution.size())dataPileupDistribution.push_back(0.0);
    gROOT->cd();  //THIS LINE IS NEEDED TO MAKE SURE THAT HISTOGRAM INTERNALLY PRODUCED IN LumiReWeighting ARE NOT DESTROYED WHEN CLOSING THE FILE
    LumiWeights = new edm::LumiReWeighting(mcPileupDistribution,dataPileupDistribution);
    PuShifters=utils::cmssw::getPUshifters(dataPileupDistribution,0.05);
    utils::getPileupNormalization(mcPileupDistribution, PUNorm, LumiWeights, PuShifters);
  }

  gROOT->cd();  //THIS LINE IS NEEDED TO MAKE SURE THAT HISTOGRAM INTERNALLY PRODUCED IN LumiReWeighting ARE NOT DESTROYED WHEN CLOSING THE FILE


  higgs::utils::EventCategory eventCategoryInst(higgs::utils::EventCategory::EXCLUSIVE2JETSVBF); //jet(0,>=1)+vbf binning

  patUtils::MetFilter metFilter;
  if(!isMC) {
    if(is2015data){
      metFilter.FillBadEvents(string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/MetFilter/DoubleEG_RunD/DoubleEG_csc2015.txt");
      metFilter.FillBadEvents(string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/MetFilter/DoubleEG_RunD/DoubleEG_ecalscn1043093.txt");
      metFilter.FillBadEvents(string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/MetFilter/DoubleMuon_RunD/DoubleMuon_csc2015.txt");
      metFilter.FillBadEvents(string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/MetFilter/DoubleMuon_RunD/DoubleMuon_ecalscn1043093.txt");
      metFilter.FillBadEvents(string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/MetFilter/MuonEG_RunD/MuonEG_csc2015.txt");
      metFilter.FillBadEvents(string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/MetFilter/MuonEG_RunD/MuonEG_ecalscn1043093.txt");
    }
    if(is2016data){}
  }
  string debugText = "";

  //##############################################
  //########           EVENT LOOP         ########
  //##############################################
  //loop on all the events
  //DuplicatesChecker duplicatesChecker;
  //int nDuplicates(0)

  printf("Progressing Bar           :0%%       20%%       40%%       60%%       80%%       100%%\n");
  for(unsigned int f=0;f<urls.size();f++){
    TFile* file = TFile::Open(urls[f].c_str() );
    fwlite::Event ev(file);
    printf("Scanning the ntuple %2i/%2i :", (int)f+1, (int)urls.size());
    int iev=0;
    int treeStep(ev.size()/50);
    for(ev.toBegin(); !ev.atEnd(); ++ev){ iev++;
      if(iev%treeStep==0){printf(".");fflush(stdout);}
      float weight = xsecWeight;
      float shapeWeight = 1.0;
      double puWeightUp = 1.0;
      double puWeightDown = 1.0;
      float puWeight(1.0);
      float weightNoLepSF(1.0);

      //##############################################   EVENT LOOP STARTS   ##############################################
      //if(!isMC && duplicatesChecker.isDuplicate( ev.run, ev.lumi, ev.event) ) { nDuplicates++; continue; }

      //Skip bad lumi
      if(!isMC && !goodLumiFilter.isGoodLumi(ev.eventAuxiliary().run(),ev.eventAuxiliary().luminosityBlock()))continue;
      // Muon SF 2016 Era dependent
      patUtils::CutVersion::CutSet muonSFs;
      if ( is2016data ) muonSFs = (dtag.Contains("2016H") || dtag.Contains("2016G")) ? patUtils::CutVersion::Moriond17Cut_GH : patUtils::CutVersion::Moriond17Cut_BCDEF;
      if ( !is2016data && is2016MC ) {
         TRandom3 *rgen = new TRandom3(0);
         int uniformValue = (int) rgen->Uniform(0, 100);
         //std::cout<<" Random:  "<<uniformValue<<std::endl;
         if ( uniformValue < 65 ) muonSFs = patUtils::CutVersion::Moriond17Cut_BCDEF;
	 else muonSFs = patUtils::CutVersion::Moriond17Cut_GH;
      }

      reco::GenParticleCollection gen;
      GenEventInfoProduct eventInfo;
      if(isMC){
        fwlite::Handle< reco::GenParticleCollection > genHandle;
        genHandle.getByLabel(ev, "prunedGenParticles");
        if(genHandle.isValid()){ gen = *genHandle;}

        fwlite::Handle< GenEventInfoProduct > genEventInfoHandle;
        genEventInfoHandle.getByLabel(ev, "generator");
        if(genEventInfoHandle.isValid()){ eventInfo = *genEventInfoHandle;}

        //WEIGHT for NLO negative interference
        weight *= eventInfo.weight();


        //WEIGHT for Pileup
        int ngenITpu = 0;
        fwlite::Handle< std::vector<PileupSummaryInfo> > puInfoH;
        puInfoH.getByLabel(ev, "slimmedAddPileupInfo");
        for(std::vector<PileupSummaryInfo>::const_iterator it = puInfoH->begin(); it != puInfoH->end(); it++){
          if(it->getBunchCrossing()==0)      { ngenITpu += it->getTrueNumInteractions(); } //getPU_NumInteractions();
        }

        if (ngenITpu == 0) continue; // It prevents to fill vtxraw with -nan values

        puWeight          = LumiWeights->weight(ngenITpu) * PUNorm[0];
        // if ( puWeight == 0 ){
        //   std::cout<<"  Some Problem in with ngenITpu= " << ngenITpu << "vtx size= "<<vtx.size() <<std::endl;
        // }
        puWeightUp  = PuShifters[utils::cmssw::PUUP  ]->Eval(ngenITpu) * (PUNorm[2]/PUNorm[0]);
        puWeightDown = PuShifters[utils::cmssw::PUDOWN]->Eval(ngenITpu) * (PUNorm[1]/PUNorm[0]);
        weight *= puWeight;

        //GEN LEVEL FILTERING
        if(isMC && (mctruthmode==15 || mctruthmode==1113)){// && (string(dtag.Data()).find("Z#rightarrow")==0 || isMC_ZZ2l2nu))
        int prodId = 1;
        for( unsigned int k=0; k<gen.size(); ++k){
          if( gen[k].isHardProcess() && ( abs( gen[k].pdgId() ) == 11 || abs( gen[k].pdgId() ) == 13 || abs( gen[k].pdgId() )==15 ) ) prodId*=gen[k].pdgId();
        }
        if(mctruthmode==15   && abs(prodId)!=225)continue; //skip not tautau
        if(mctruthmode==1113 && abs(prodId)==225)continue; //skip tautau
      }
    }

    weightNoLepSF = weight;

    //apply trigger and require compatibilitiy of the event with the PD
    edm::TriggerResultsByName tr = ev.triggerResultsByName("HLT");
    if(!tr.isValid())return false;

    bool mumuTrigger(true); bool muTrigger(true);	bool eeTrigger(true); bool eTrigger(true); bool emuTrigger(true);

    int metFilterValue = 0;

	  bool filterbadPFMuon = true;
	  bool filterbadChCandidate = true;
	  bool filterbadMuonHIP = true;
	  bool filterduplicateMuonHIP = true;
	  std::unique_ptr<std::vector<reco::Muon*>> outbadMuon(new std::vector<reco::Muon*>());
	  std::unique_ptr<std::vector<reco::Muon*>> outduplicateMuon(new std::vector<reco::Muon*>());

    if(is2016data || is2016MC){

      mumuTrigger        = utils::passTriggerPatterns(tr, "HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_v*", "HLT_Mu17_TrkIsoVVL_TkMu8_TrkIsoVVL_DZ_v*");
      muTrigger          = utils::passTriggerPatterns(tr, "HLT_IsoMu22_v*","HLT_IsoTkMu22_v*", "HLT_IsoMu24_v*", "HLT_IsoTkMu24_v*");
      eeTrigger          = utils::passTriggerPatterns(tr, "HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v*"); //,"HLT_Ele17_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v*",);
      eTrigger           = utils::passTriggerPatterns(tr, "HLT_Ele27_eta2p1_WPLoose_Gsf_v*","HLT_Ele27_WPTight_Gsf_v*");
      emuTrigger         = utils::passTriggerPatterns(tr, "HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_v*", "HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_v*");

      metFilterValue = metFilter.passMetFilterInt( ev, is2016data );

	    // Apply Bad Charged Hadron and Bad Muon Filters from MiniAOD (for Run II 2016 only )
	    filterbadChCandidate = metFilter.passBadChargedCandidateFilter(ev); if (!filterbadChCandidate) {  metFilterValue=9; }
	    filterbadPFMuon = metFilter.passBadPFMuonFilter(ev); if (!filterbadPFMuon) { metFilterValue=8; }
	    filterbadMuonHIP = metFilter.BadGlobalMuonTaggerFilter(ev,outbadMuon,false); if (!filterbadMuonHIP) { metFilterValue=10; }
	    filterduplicateMuonHIP = metFilter.BadGlobalMuonTaggerFilter(ev,outduplicateMuon,true); if (!filterduplicateMuonHIP) { metFilterValue=11; }
    }

    bool passTrigger        = mumuTrigger||muTrigger||eeTrigger||eTrigger;//||emuTrigger;

    //bool passTrigger        = mumuTrigger||eeTrigger;

    if(  mumuTrigger)mon.fillHisto("trigger", "raw", 0 , weight);
    if(    muTrigger)mon.fillHisto("trigger", "raw", 1 , weight);
    if(    eeTrigger)mon.fillHisto("trigger", "raw", 2 , weight);
    if(     eTrigger)mon.fillHisto("trigger", "raw", 3 , weight);
    if(   emuTrigger)mon.fillHisto("trigger", "raw", 4 , weight);

    if(!isMC && passTrigger){ //avoid double counting of events from different PD
      if(filterOnlyMUMU)     { passTrigger = mumuTrigger;}
      if(filterOnlyMU)       { passTrigger = muTrigger     && !mumuTrigger;}
      if(filterOnlyEE)       { passTrigger = eeTrigger     && !muTrigger  && !mumuTrigger;}
      if(filterOnlyE)        { passTrigger = eTrigger      && !eeTrigger  && !muTrigger && !mumuTrigger; }
      if(filterOnlyEMU)      { passTrigger = emuTrigger    && !eTrigger   && !eeTrigger && !muTrigger && !mumuTrigger; }
    }

    if(passTrigger){
      if(  mumuTrigger)mon.fillHisto("trigger", "cleaned", 0 , weight);
      if(    muTrigger)mon.fillHisto("trigger", "cleaned", 1 , weight);
      if(    eeTrigger)mon.fillHisto("trigger", "cleaned", 2 , weight);
      if(     eTrigger)mon.fillHisto("trigger", "cleaned", 3 , weight);
      if(   emuTrigger)mon.fillHisto("trigger", "cleaned", 4 , weight);
    }

    //ONLY RUN ON THE EVENTS THAT PASS OUR TRIGGERS
    if(!passTrigger)continue;

    //##############################################   EVENT PASSED THE TRIGGER   ######################################
	  if (metFilterValue==10 || metFilterValue==11) { metFilterValue=0; }
          if( metFilterValue!=0 ) continue;	 //Note this must also be applied on MC

	  // Apply Bad Charged Hadron and Bad Muon Filters from MiniAOD (for Run II 2016 only )
	  //	  if (!filterbadPFMuon || !filterbadChCandidate) continue;
    //##############################################   EVENT PASSED MET FILTER   #######################################


    //load all the objects we will need to access
    reco::VertexCollection vtx;
    fwlite::Handle< reco::VertexCollection > vtxHandle;
    vtxHandle.getByLabel(ev, "offlineSlimmedPrimaryVertices");
    if(vtxHandle.isValid()){ vtx = *vtxHandle;}

    double rho = 0;
    fwlite::Handle< double > rhoHandle;
    rhoHandle.getByLabel(ev, "fixedGridRhoFastjetAll");
    if(rhoHandle.isValid()){ rho = *rhoHandle;}

    pat::MuonCollection muons;
    fwlite::Handle< pat::MuonCollection > muonsHandle;
    muonsHandle.getByLabel(ev, "slimmedMuons");
    if(muonsHandle.isValid()){ muons = *muonsHandle;}

    pat::ElectronCollection electrons;
    fwlite::Handle< pat::ElectronCollection > electronsHandle;
    electronsHandle.getByLabel(ev, "slimmedElectrons");
    if(electronsHandle.isValid()){ electrons = *electronsHandle;}




    EcalRecHitCollection recHitCollectionEB;
    EcalRecHitCollection recHitCollectionEE;
    fwlite::Handle<EcalRecHitCollection> recHitCollectionEBHandle;
    fwlite::Handle<EcalRecHitCollection> recHitCollectionEEHandle;
    recHitCollectionEBHandle.getByLabel(ev, "reducedEgamma","reducedEBRecHits" );
    recHitCollectionEEHandle.getByLabel(ev, "reducedEgamma","reducedEERecHits" );
    if(recHitCollectionEBHandle.isValid()){ recHitCollectionEB = *recHitCollectionEBHandle;}
    if(recHitCollectionEEHandle.isValid()){ recHitCollectionEE = *recHitCollectionEEHandle;}

    pat::JetCollection jets;
    fwlite::Handle< pat::JetCollection > jetsHandle;
    jetsHandle.getByLabel(ev, "slimmedJets");
    if(jetsHandle.isValid()){ jets = *jetsHandle;}

    //pat::PhotonCollection photons;
    //fwlite::Handle< pat::PhotonCollection > photonsHandle;
    //photonsHandle.getByLabel(ev, "slimmedPhotons");
    //if(photonsHandle.isValid()){ photons = *photonsHandle;}

    pat::METCollection mets;
    fwlite::Handle< pat::METCollection > metsHandle;
    metsHandle.getByLabel(ev, "slimmedMETs");
    if(metsHandle.isValid()){ mets = *metsHandle;}
    pat::MET met = mets[0];

    pat::METCollection puppimets;
    fwlite::Handle< pat::METCollection > puppimetsHandle;
    puppimetsHandle.getByLabel(ev, "slimmedMETsPuppi");
    if(puppimetsHandle.isValid()){ puppimets = *puppimetsHandle;}
    LorentzVector puppimet = puppimets[0].p4();

    pat::TauCollection taus;
    fwlite::Handle< pat::TauCollection > tausHandle;
    tausHandle.getByLabel(ev, "slimmedTaus");
    if(tausHandle.isValid()){ taus = *tausHandle;}

    if(isV0JetsMC){
      fwlite::Handle< LHEEventProduct > lheEPHandle;
      lheEPHandle.getByLabel(ev, "externalLHEProducer");
      if(lheEPHandle.isValid()){
        mon.fillHisto("nup","",lheEPHandle->hepeup().NUP,1);
        if(lheEPHandle->hepeup().NUP>5) continue;
        mon.fillHisto("nupfilt","",lheEPHandle->hepeup().NUP,1);
      }else{
        printf("Handle to externalLHEProducer is invalid --> Can not ignore V0+Jet events from inclusive samples\n");
      }
    }

    //Electroweak corrections to ZZ and WZ simulations
    double ewkCorrectionsWeight = 1.;
    double ewkCorrections_error = 0.;
    if(isMC_ZZ2l2nu || isMC_WZ3lnu) ewkCorrectionsWeight = EwkCorrections::getEwkCorrections(dtag, gen, ewkTable, eventInfo, ewkCorrections_error);
    double ewkCorrections_up = (ewkCorrectionsWeight + ewkCorrections_error)/ewkCorrectionsWeight;
    double ewkCorrections_down = (ewkCorrectionsWeight - ewkCorrections_error)/ewkCorrectionsWeight;



    //final event weight
    weight *= ewkCorrectionsWeight;

    //NNLO corrections on ZZ2l2nu
    double ZZ_NNLOcorrectionsWeight =1.;
    double mzz = - 404; // will be filled by getNNLOCorrections
    if(isMC_ZZ2l2nu) ZZ_NNLOcorrectionsWeight = ZZatNNLO::getNNLOCorrections(dtag, gen, ZZ_NNLOTable, mzz);
    if(isMC_ZZ2l2nu) mon.fillHisto("mzz", "qqZZ_atNLO", mzz, weight);
    weight *= ZZ_NNLOcorrectionsWeight;
    if(isMC_ZZ2l2nu) mon.fillHisto("mzz", "qqZZ_atNNLO", mzz, weight);


    //
    //
    // BELOW FOLLOWS THE ANALYSIS OF THE MAIN SELECTION WITH N-1 PLOTS
    //
    //

    //
    // PHOTON ANALYSIS
    //
    pat::PhotonCollection selPhotons;
    //int nPho55=0; int nPho100=0;
    //for(size_t ipho=0; ipho<photons.size(); ipho++){
    //   pat::Photon photon = photons[ipho];
    //   mon.fillHisto("phopt", "trg", photon.pt(), weight);
    //   mon.fillHisto("phoeta", "trg", photon.eta(), weight);

    //   //calibrate photon energy
    //   PhotonEnCorrector.calibrate(photon, ev.eventAuxiliary().run(), edm::StreamID::invalidStreamID());

    //   if(photon.pt()<55)continue;
    //   if(fabs(photon.superCluster()->eta())>1.4442 ) continue;
    //   if(!patUtils::passId(photon, rho, patUtils::llvvPhotonId::Tight)) continue;

    //   selPhotons.push_back(photon);
    //   if(photon.pt()>55)nPho55++;
    //   if(photon.pt()>100)nPho100++;
    //}




    //
    // LEPTON ANALYSIS
    //

    //start by merging electrons and muons
    std::vector<patUtils::GenericLepton> leptons;
    for(size_t l=0;l<electrons.size();l++){leptons.push_back(patUtils::GenericLepton(electrons[l]));}
    for(size_t l=0;l<muons    .size();l++){leptons.push_back(patUtils::GenericLepton(muons    [l]));}
    std::sort(leptons.begin(),   leptons.end(), utils::sort_CandidatesByPt);

    std::vector<patUtils::GenericLepton> selLeptons, extraLeptons;
    LorentzVector muDiff(0,0,0,0);
    LorentzVector elDiff(0,0,0,0);
    for(size_t ilep=0; ilep<leptons.size(); ilep++){
      bool passKin(true),passId(true),passIso(true);
      bool passVeryLooseLepton(true), passLooseLepton(true), passSoftMuon(true), passSoftElectron(true), passVetoElectron(true);
      int lid=leptons[ilep].pdgId();

      //no need for charge info any longer
      lid=abs(lid);
      TString lepStr( lid==13 ? "mu" : "e");

      //veto nearby photon (loose electrons are many times photons...)
      double minDRlg(9999.);
      for(size_t ipho=0; ipho<selPhotons.size(); ipho++){
        minDRlg=TMath::Min(minDRlg,deltaR(leptons[ilep].p4(),selPhotons[ipho].p4()));
      }
      if(minDRlg<0.1) continue;

      //veto leptons overlaping with other lep
      bool overlapWithLepton=false;
      for(int l1=0; l1<(int)selLeptons.size();++l1){
        if(deltaR(leptons[ilep].p4(), selLeptons[l1])<0.1){overlapWithLepton=true; break;}
      }if(overlapWithLepton)continue;

      //Cut based identification
      // passId
      passId = lid==11 ? patUtils::passId(leptons[ilep].el, vtx[0], patUtils::llvvElecId::Tight, patUtils::CutVersion::CutSet::ICHEP16Cut, true) :
      patUtils::passId(leptons[ilep].mu, vtx[0], patUtils::llvvMuonId::Tight, patUtils::CutVersion::CutSet::ICHEP16Cut);
      // passLooseLepton
      passLooseLepton &= lid==11 ? patUtils::passId(leptons[ilep].el, vtx[0], patUtils::llvvElecId::Loose, patUtils::CutVersion::CutSet::ICHEP16Cut, true) :
      patUtils::passId(leptons[ilep].mu, vtx[0], patUtils::llvvMuonId::Loose, patUtils::CutVersion::CutSet::ICHEP16Cut);
      passVeryLooseLepton &= passLooseLepton;
    // passSoftMuon
      passSoftMuon &= lid==11 ? false : patUtils::passId(leptons[ilep].mu, vtx[0], patUtils::llvvMuonId::Soft, patUtils::CutVersion::CutSet::ICHEP16Cut);

      //isolation
      //  passIso
      passIso = lid==11 ? patUtils::passIso(leptons[ilep].el,  patUtils::llvvElecIso::Tight, patUtils::CutVersion::CutSet::ICHEP16Cut) :
      patUtils::passIso(leptons[ilep].mu,  patUtils::llvvMuonIso::Tight, patUtils::CutVersion::CutSet::Moriond17Cut);
      // passLooseLepton
      passLooseLepton &= lid==11 ? patUtils::passIso(leptons[ilep].el,  patUtils::llvvElecIso::Loose, patUtils::CutVersion::CutSet::ICHEP16Cut) :
      patUtils::passIso(leptons[ilep].mu,  patUtils::llvvMuonIso::Loose, patUtils::CutVersion::CutSet::Moriond17Cut);

      // passVeryLooseLepton
      passVeryLooseLepton &= lid==11 ?  patUtils::passIso(leptons[ilep].el,  patUtils::llvvElecIso::VeryLoose, patUtils::CutVersion::CutSet::ICHEP16Cut) :
      patUtils::passIso(leptons[ilep].mu,  patUtils::llvvMuonIso::VeryLoose, patUtils::CutVersion::CutSet::ICHEP16Cut);


      //apply muon corrections
      //if(abs(lid)==13 && passIso && passId)
      if(abs(lid)==13 && passVeryLooseLepton){
        passSoftMuon=false;
          if(is2016MC || is2016data){
                   if(muCor2016){
                     float qter =1.0;
                     TLorentzVector p4(leptons[ilep].px(),leptons[ilep].py(),leptons[ilep].pz(),leptons[ilep].energy());
                     int ntrk = leptons[ilep].mu.innerTrack()->hitPattern().trackerLayersWithMeasurement();
                     if(is2016MC){muCor2016->momcor_mc  (p4, lid<0 ? -1 :1, ntrk, qter);
                     }else if (is2016data){   muCor2016->momcor_data(p4, lid<0 ? -1 :1, 0, qter);
                     }


                     muDiff -= leptons[ilep].p4();
                     leptons[ilep].mu.setP4(LorentzVector(p4.Px(),p4.Py(),p4.Pz(),p4.E() ) );
          	     leptons[ilep] = patUtils::GenericLepton(leptons[ilep].mu); //recreate the generic lepton to be sure that the p4 is ok
                     muDiff += leptons[ilep].p4();
                   }
          }
        }


      //apply electron corrections
      //if(abs(lid)==11  && passIso && passId)
      if(abs(lid)==11  && passVeryLooseLepton){
        //std::cout<<"START ---- "<<std::endl;
        elDiff -= leptons[ilep].p4();
        const EcalRecHitCollection* recHits = (leptons[ilep].el.isEB()) ? recHitCollectionEBHandle.product() : recHitCollectionEEHandle.product();
        unsigned int gainSeed = patUtils::GainSeed(leptons[ilep].el,recHits);

        if(!isMC){

          double scale_corr=eScaler.ScaleCorrection(ev.eventAuxiliary().run(),leptons[ilep].el.isEB(),leptons[ilep].el.r9(), leptons[ilep].el.superCluster()->eta(), leptons[ilep].el.et(),gainSeed);
          //At this point, the new data energy will be:
          // E_new=E_old*(scale_corr);
          TLorentzVector p4(leptons[ilep].el.px(),leptons[ilep].el.py(),leptons[ilep].el.pz(),leptons[ilep].el.energy());
          leptons[ilep].el.setP4(LorentzVector(p4.Px()*scale_corr,p4.Py()*scale_corr,p4.Pz()*scale_corr,p4.E()*scale_corr ) );
          leptons[ilep] = patUtils::GenericLepton(leptons[ilep].el); //recreate the generic lepton to be sure that the p4 is ok
        }
        if(isMC){
          //std::cout<<"Before  pt  ---- "<<leptons[ilep].el.p4()<<std::endl;
          double sigma=eScaler.getSmearingSigma(ev.eventAuxiliary().run(),leptons[ilep].el.isEB(),leptons[ilep].el.r9(), leptons[ilep].el.superCluster()->eta(), leptons[ilep].el.et(),gainSeed,0,0);
          //Put the last two inputs at 0,0 for the nominal value of sigma
          //Now smear the MC energy
          TRandom3 *rgen_ = new TRandom3(0);
          double smearValue = rgen_->Gaus(1, sigma) ;
          //std::cout<<"smearing  ---- "<<smearValue<<std::endl;
          TLorentzVector p4(leptons[ilep].el.px(),leptons[ilep].el.py(),leptons[ilep].el.pz(),leptons[ilep].el.energy());
          leptons[ilep].el.setP4(LorentzVector(p4.Px()*smearValue,p4.Py()*smearValue,p4.Pz()*smearValue,p4.E()*smearValue ) );
          //std::cout<<"After  pt  ---- "<<leptons[ilep].el.p4()<<std::endl;
          // std::cout<<"\n";
          leptons[ilep] = patUtils::GenericLepton(leptons[ilep].el); //recreate the generic lepton to be sure that the p4 is ok
        }
         elDiff += leptons[ilep].p4();
      }

      // Compute relIso after corrections
      leptons[ilep].addUserFloat("relIso",  patUtils::relIso(leptons[ilep], rho) ); //compute it once for all

      mon.fillHisto( lid == 11 ? "eleiso" : "muiso" ,  "controlPlots" , leptons[ilep].userFloat("relIso"), 1);
        //kinematics
        float leta = fabs(lid==11 ?  leptons[ilep].el.superCluster()->eta() : leptons[ilep].eta());
        if(leta> (lid==11 ? 2.5 : 2.4) )            passKin=false;
        if(lid==11 && (leta>1.4442 && leta<1.5660)) passKin=false;
	passVeryLooseLepton &= passKin;
        passSoftMuon    &= passKin;
        if(lid==13){
          if(leptons[ilep].pt()<10) passVeryLooseLepton=false;
          if(leptons[ilep].pt()<3)  passSoftMuon=false;
          if(leptons[ilep].pt()<25) passKin=false;
        }else if(lid==11){
          if(leptons[ilep].pt()<10) passVeryLooseLepton=false;
          if(leptons[ilep].pt()<30) passKin=false;
        }
        //if(leptons[ilep].pt()<25) passKin=false;

        //if(passId && passIso && passKin)          selLeptons.push_back(leptons[ilep]);
        if(passVeryLooseLepton && passKin)            selLeptons.push_back(leptons[ilep]); //we need loose lepton for FR
        else if(passVeryLooseLepton || passSoftMuon)  extraLeptons.push_back(leptons[ilep]);
      }
      std::sort(selLeptons.begin(),   selLeptons.end(), utils::sort_CandidatesByPt);
      std::sort(extraLeptons.begin(), extraLeptons.end(), utils::sort_CandidatesByPt);

      //update the met for lepton energy scales
      met.setP4(met.p4() - muDiff - elDiff); //note this also propagates to all MET uncertainties
      met.setUncShift(met.px() - muDiff.px()*0.01, met.py() - muDiff.py()*0.01, met.sumEt() - muDiff.pt()*0.01, pat::MET::METUncertainty::MuonEnUp);   //assume 1% uncertainty on muon rochester
      met.setUncShift(met.px() + muDiff.px()*0.01, met.py() + muDiff.py()*0.01, met.sumEt() + muDiff.pt()*0.01, pat::MET::METUncertainty::MuonEnDown); //assume 1% uncertainty on muon rochester
      met.setUncShift(met.px() - elDiff.px()*0.01, met.py() - elDiff.py()*0.01, met.sumEt() - elDiff.pt()*0.01, pat::MET::METUncertainty::ElectronEnUp);   //assume 1% uncertainty on electron scale correction
      met.setUncShift(met.px() + elDiff.px()*0.01, met.py() + elDiff.py()*0.01, met.sumEt() + elDiff.pt()*0.01, pat::MET::METUncertainty::ElectronEnDown); //assume 1% uncertainty on electron scale correction


      //
      //TAU ANALYSIS
      //

      pat::TauCollection selTaus;
      int ntaus(0);
      for(size_t itau=0; itau<taus.size(); ++itau){
        pat::Tau& tau = taus[itau];
        if(tau.pt()<20. || fabs(tau.eta()) >2.3) continue;

        bool overlapWithLepton(false);
        for(int l1=0; l1<(int)selLeptons.size();++l1){
          if(deltaR(tau, selLeptons[l1])<0.1){overlapWithLepton=true; break;}
        }
        if(overlapWithLepton) continue;

        //	if(!tau.isPFTau()) continue; // Only PFTaus
        //	if(tau.emFraction() >=2.) continue;

        // we need to apply a very loose selection here (Lucia's suggestion)
        if(!tau.tauID("againstElectronLooseMVA6")) continue;
        if(!tau.tauID("againstMuonLoose3")) continue;
        if(!tau.tauID("decayModeFinding")) continue;

        selTaus.push_back(tau);
        selLeptons.push_back(tau);
        ntaus++;
      }
      std::sort(selTaus.begin(), selTaus.end(), utils::sort_CandidatesByPt);
      std::sort(selLeptons.begin(),   selLeptons.end(), utils::sort_CandidatesByPt);


      //
      //JET/MET ANALYSIS
      //

      //add scale/resolution uncertainties and propagate to the MET
      utils::cmssw::updateJEC(jets,jesCor,totalJESUnc,rho,vtx.size(),isMC);

      //select the jets
      std::map<string, pat::JetCollection> selJetsVar;
      std::map<string, int   > njetsVar;
      std::map<string, int   > nbtagsVar;
      std::map<string, double> mindphijmetVar;
      for(unsigned int ivar=0;ivar<jetVarNames.size();ivar++){mindphijmetVar[jetVarNames[ivar]] = 9999.0;}  //initialize

      for(size_t ijet=0; ijet<jets.size(); ijet++){
        pat::Jet jet = jets[ijet]; //copy the jet, such that we can update it

        if(jet.pt()<15 || fabs(jet.eta())>4.7 ) continue;

        //mc truth for this jet
        const reco::GenJet* genJet=jet.genJet();
        TString jetType( genJet && genJet->pt()>0 ? "truejetsid" : "pujetsid" );

        //cross-clean with selected leptons and photons  (DISABLED AS WE NEED THOSE FOR FR STUDY)
        //double minDRlj(9999.); for(size_t ilep=0; ilep<selLeptons.size(); ilep++){if(abs(selLeptons[ilep].pdgId())>13){continue;}  minDRlj = TMath::Min( minDRlj, deltaR(jet,selLeptons[ilep]) );}  //ignore taus for the cross-cleaning
        //double minDRlg(9999.); for(size_t ipho=0; ipho<selPhotons.size(); ipho++)  minDRlg = TMath::Min( minDRlg, deltaR(jet,selPhotons[ipho]) );
        //if(minDRlj<0.4 || minDRlg<0.4) continue;

        //jet id
        bool passPFloose = patUtils::passPFJetID("Loose", jet);
        bool passLooseSimplePuId = patUtils::passPUJetID(jet); //FIXME Broken in miniAOD V2 : waiting for JetMET fix. (Hugo)
        if(jet.pt()>30){
          mon.fillHisto(jetType,"",fabs(jet.eta()),0);
          if(passPFloose)                        mon.fillHisto("jetId", jetType,fabs(jet.eta()),1);
          if(passLooseSimplePuId)                mon.fillHisto("jetId", jetType,fabs(jet.eta()),2);
          if(passPFloose && passLooseSimplePuId) mon.fillHisto("jetId", jetType,fabs(jet.eta()),3);
        }
        if(!passPFloose || !passLooseSimplePuId) continue;


        //check for btagging
        if(jet.pt()>30 && fabs(jet.eta())<2.5){
          bool hasCSVtag = (jet.bDiscriminator("pfCombinedInclusiveSecondaryVertexV2BJetTags")>btagLoose);
          bool hasCSVtagUp = hasCSVtag;
          bool hasCSVtagDown = hasCSVtag;

          //update according to the SF measured by BTV
          if(isMC){
            int flavId=jet.partonFlavour();  double eta=jet.eta();
            btsfutil.SetSeed(ev.eventAuxiliary().event()*10 + ijet*10000);
            if(abs(flavId)==5){
              //  74X recommendation
					                 //     btsfutil.modifyBTagsWithSF(hasCSVtag    , btagCal   .eval(BTagEntry::FLAV_B   , eta, jet.pt()), beff);
              //     btsfutil.modifyBTagsWithSF(hasCSVtagUp  , btagCalUp .eval(BTagEntry::FLAV_B   , eta, jet.pt()), beff);
              //     btsfutil.modifyBTagsWithSF(hasCSVtagDown, btagCalDn .eval(BTagEntry::FLAV_B   , eta, jet.pt()), beff);
              //  80X recommendation
              btsfutil.modifyBTagsWithSF(hasCSVtag    , btagCal80X.eval_auto_bounds("central", BTagEntry::FLAV_B   , eta, jet.pt()), beff);
              btsfutil.modifyBTagsWithSF(hasCSVtagUp  , btagCal80X.eval_auto_bounds("up", BTagEntry::FLAV_B   , eta, jet.pt()), beff);
              btsfutil.modifyBTagsWithSF(hasCSVtagDown, btagCal80X.eval_auto_bounds("down", BTagEntry::FLAV_B   , eta, jet.pt()), beff);
            }else if(abs(flavId)==4){
              //  74X recommendation
					    //     btsfutil.modifyBTagsWithSF(hasCSVtag    , btagCal   .eval(BTagEntry::FLAV_C   , eta, jet.pt()), beff);
					    //     btsfutil.modifyBTagsWithSF(hasCSVtagUp  , btagCalUp .eval(BTagEntry::FLAV_C   , eta, jet.pt()), beff);
					    //     btsfutil.modifyBTagsWithSF(hasCSVtagDown, btagCalDn .eval(BTagEntry::FLAV_C   , eta, jet.pt()), beff);
              //  80X recommendation
			        btsfutil.modifyBTagsWithSF(hasCSVtag    , btagCal80X.eval_auto_bounds("central", BTagEntry::FLAV_C   , eta, jet.pt()), beff);
              btsfutil.modifyBTagsWithSF(hasCSVtagUp  , btagCal80X.eval_auto_bounds("up", BTagEntry::FLAV_C   , eta, jet.pt()), beff);
              btsfutil.modifyBTagsWithSF(hasCSVtagDown, btagCal80X.eval_auto_bounds("down", BTagEntry::FLAV_C   , eta, jet.pt()), beff);
            }else{
              //  74X recommendation
					    //     btsfutil.modifyBTagsWithSF(hasCSVtag    , btagCalL  .eval(BTagEntry::FLAV_UDSG, eta, jet.pt()), leff);
              //     btsfutil.modifyBTagsWithSF(hasCSVtagUp  , btagCalLUp.eval(BTagEntry::FLAV_UDSG, eta, jet.pt()), leff);
              //     btsfutil.modifyBTagsWithSF(hasCSVtagDown, btagCalLDn.eval(BTagEntry::FLAV_UDSG, eta, jet.pt()), leff);
              //  80X recommendation
			        btsfutil.modifyBTagsWithSF(hasCSVtag    , btagCal80X.eval_auto_bounds("central", BTagEntry::FLAV_UDSG   , eta, jet.pt()), leff);
              btsfutil.modifyBTagsWithSF(hasCSVtagUp  , btagCal80X.eval_auto_bounds("up", BTagEntry::FLAV_UDSG   , eta, jet.pt()), leff);
              btsfutil.modifyBTagsWithSF(hasCSVtagDown, btagCal80X.eval_auto_bounds("down", BTagEntry::FLAV_UDSG   , eta, jet.pt()), leff);
            }
          }

          if(hasCSVtag    )nbtagsVar[""          ]++;
          if(hasCSVtagUp  )nbtagsVar["_eff_bup"  ]++;
          if(hasCSVtagDown)nbtagsVar["_eff_bdown"]++;
        }


        for(unsigned int ivar=0;ivar<jetVarNames.size();ivar++){
          if(!isMC && ivar>0) continue;
          pat::Jet varJet = jet;
          if(ivar!=0) varJet.setP4(jet.p4() * jet.userFloat(jetVarNames[ivar]));
          selJetsVar[jetVarNames[ivar]].push_back(varJet);

          if(varJet.pt()>30){
            njetsVar[jetVarNames[ivar]]++;

            float dphijmet=fabs(deltaPhi(met.corP4(metcor).phi(), varJet.phi()));
            if(dphijmet<mindphijmetVar[jetVarNames[ivar]]) mindphijmetVar[jetVarNames[ivar]]=dphijmet;
          }
        }
      }
      //sort all jet collection by pT
      for(auto jetCollIt = selJetsVar.begin(); jetCollIt!=selJetsVar.end(); jetCollIt++){
        std::sort(jetCollIt->second.begin(), jetCollIt->second.end(), utils::sort_CandidatesByPt);
      }


      // LOOP ON SYSTEMATIC VARIATION FOR THE STATISTICAL ANALYSIS
      double initialWeight = weight;           //save weight
      //compute scale uncertainty once and for all
      std::pair<double, double> scaleUncVar = patUtils::scaleVariation(ev);  //compute it only once

      for(size_t ivar=0; ivar<nvarsToInclude; ivar++){
        if(!isMC && ivar>0 ) continue; //loop on variation only for MC samples

        //start from a nominal
        float weight = initialWeight;

        //Theoretical Uncertanties: PDF, Alpha and Scale
        if(varNames[ivar]=="_th_factup")     weight *= std::max(0.9, std::min(scaleUncVar.first , 1.1));
        if(varNames[ivar]=="_th_factdown")   weight *= std::max(0.9, std::min(scaleUncVar.second, 1.1));
        if(varNames[ivar]=="_th_alphas")     weight *= patUtils::alphaVariation(ev);
        if(varNames[ivar]=="_th_pdf")        weight *= patUtils::pdfVariation(ev);

        //EwkCorrections variation
        if ( varNames[ivar]=="_th_ewkup")    weight *= ewkCorrections_up;
        if ( varNames[ivar]=="_th_ewkdown")  weight *= ewkCorrections_down;

        //pileup variations
        if(varNames[ivar]=="_puup")          weight *= puWeightUp;
        if(varNames[ivar]=="_pudown")        weight *= puWeightDown;

        //recompute MET with variation
        LorentzVector imet = met.corP4(metcor);
        if(varNames[ivar]=="_scale_jup")      imet = met.shiftedP4(pat::MET::METUncertainty::JetEnUp           , metcor);
        if(varNames[ivar]=="_scale_jdown")    imet = met.shiftedP4(pat::MET::METUncertainty::JetEnDown         , metcor);
        if(varNames[ivar]=="_res_jup")        imet = met.shiftedP4(pat::MET::METUncertainty::JetResUp          , metcor);
        if(varNames[ivar]=="_res_jdown")      imet = met.shiftedP4(pat::MET::METUncertainty::JetResDown        , metcor);
        if(varNames[ivar]=="_scale_umetup")   imet = met.shiftedP4(pat::MET::METUncertainty::UnclusteredEnUp   , metcor);
        if(varNames[ivar]=="_scale_umetdown") imet = met.shiftedP4(pat::MET::METUncertainty::UnclusteredEnDown , metcor);
        if(varNames[ivar]=="_scale_mup")      imet = met.shiftedP4(pat::MET::METUncertainty::MuonEnUp          , metcor);
        if(varNames[ivar]=="_scale_mdown")    imet = met.shiftedP4(pat::MET::METUncertainty::MuonEnDown        , metcor);
        if(varNames[ivar]=="_scale_eup")      imet = met.shiftedP4(pat::MET::METUncertainty::ElectronEnUp      , metcor);
        if(varNames[ivar]=="_scale_edown")    imet = met.shiftedP4(pat::MET::METUncertainty::ElectronEnDown    , metcor);

        //to be implemented
        //	       if(varNames[ivar]=="_tesup")   selLeptons=getTauVariations(selLeptons,1.03);
        //	       if(varNames[ivar]=="_tesdown") selLeptons=getTauVariations(selLeptons,0.97);


        auto& selJets      = selJetsVar[""];        if(selJetsVar    .find(varNames[ivar].Data())!=selJetsVar    .end())selJets     = selJetsVar    [varNames[ivar].Data()];
        auto& njets        = njetsVar [""];         if(njetsVar      .find(varNames[ivar].Data())!=njetsVar      .end())njets       = njetsVar      [varNames[ivar].Data()];
        auto& nbtags       = nbtagsVar[""];         if(nbtagsVar     .find(varNames[ivar].Data())!=nbtagsVar     .end())nbtags      = nbtagsVar     [varNames[ivar].Data()];
        auto& mindphijmet  = mindphijmetVar[""];    if(mindphijmetVar.find(varNames[ivar].Data())!=mindphijmetVar.end())mindphijmet = mindphijmetVar[varNames[ivar].Data()];

        //
        // ASSIGN CHANNEL
        //

        std::vector<TString> chTags;
        TString evCat;
        int dilId(1);
        int dilLep1, dilLep2;
        double BestMass;
        LorentzVector leadingLep, trailerLep, zll, zlltmp;
        //get the Z candidate
        dilLep1=-1; dilLep2=-1; dilId=-1;
        BestMass=0;
        zll = LorentzVector(0.,0.,0.,0.);

        for(unsigned int l1=0   ;l1<selLeptons.size();l1++){
          if(abs(selLeptons[l1].pdgId())==15)continue;
          if( selLeptons[l1].pt()<20 ) continue;
          if(!( abs(selLeptons[l1].pdgId())==11 ? patUtils::passIso(selLeptons[l1].el,  patUtils::llvvElecIso::Tight, patUtils::CutVersion::CutSet::ICHEP16Cut) :
                                                patUtils::passIso(selLeptons[l1].mu,  patUtils::llvvMuonIso::Tight, patUtils::CutVersion::CutSet::Moriond17Cut)) ||
             !( abs(selLeptons[l1].pdgId())==11 ? patUtils::passId(selLeptons[l1].el, vtx[0], patUtils::llvvElecId::Tight, patUtils::CutVersion::CutSet::ICHEP16Cut, true) :
                                                patUtils::passId(selLeptons[l1].mu, vtx[0], patUtils::llvvMuonId::Tight, patUtils::CutVersion::CutSet::ICHEP16Cut)) ) continue;

          for(unsigned int l2=l1+1;l2<selLeptons.size();l2++){
            if(abs(selLeptons[l2].pdgId())==15)continue;
            if( selLeptons[l2].pt()<20 ) continue;
            if(!( abs(selLeptons[l2].pdgId())==11 ? patUtils::passIso(selLeptons[l2].el,  patUtils::llvvElecIso::Tight, patUtils::CutVersion::CutSet::ICHEP16Cut) :
                                                  patUtils::passIso(selLeptons[l2].mu,  patUtils::llvvMuonIso::Tight, patUtils::CutVersion::CutSet::Moriond17Cut)) ||
               !( abs(selLeptons[l2].pdgId())==11 ? patUtils::passId(selLeptons[l2].el, vtx[0], patUtils::llvvElecId::Tight, patUtils::CutVersion::CutSet::ICHEP16Cut, true) :
                                                  patUtils::passId(selLeptons[l2].mu, vtx[0], patUtils::llvvMuonId::Tight, patUtils::CutVersion::CutSet::ICHEP16Cut)) ) continue;

            if(abs(selLeptons[l1].pdgId())!=abs(selLeptons[l2].pdgId())) continue; 				 //SAME FLAVOUR PAIR
            if(selLeptons[l1].pdgId()*selLeptons[l2].pdgId()>=0) continue;					 //OPPOSITE SIGN

            zlltmp = (selLeptons[l1].p4()+selLeptons[l2].p4());
            if( fabs(zlltmp.mass() - 91.2) < fabs(zll.mass()-91.2) ){    //BEST MASS [76.2,106.2]
              dilLep1 = l1;
              dilLep2 = l2;
              zll=zlltmp;
              leadingLep=selLeptons[l1].p4();
              trailerLep=selLeptons[l2].p4();
              dilId = selLeptons[l1].pdgId() * selLeptons[l2].pdgId();
            }
          }
        }
        //get the Z candiate (end)

        bool isDileptonCandidate = false;
        if(dilId!=-1){
          //check the channel
          if( abs(dilId)==121){ chTags.push_back("ll"); chTags.push_back("ee");   isDileptonCandidate=true; }
          if( abs(dilId)==169){ chTags.push_back("ll"); chTags.push_back("mumu"); isDileptonCandidate=true; }

          if(is2016MC) {
            if(abs(dilId)==121){
              weight *= lepEff.getRecoEfficiency( selLeptons[dilLep1].el.superCluster()->eta(), abs(selLeptons[dilLep1].pdgId())).first; //Reconstruction eff
              weight *= lepEff.getRecoEfficiency( selLeptons[dilLep2].el.superCluster()->eta(), abs(selLeptons[dilLep2].pdgId())).first; //Reconstruction eff
            }
            else if(abs(dilId)==169){
              weight *= lepEff.getTrackingEfficiency( selLeptons[dilLep1].eta(), abs(selLeptons[dilLep1].pdgId())).first; //Tracking eff
              weight *= lepEff.getTrackingEfficiency( selLeptons[dilLep2].eta(), abs(selLeptons[dilLep2].pdgId())).first; //Tracking eff

              weight *= isMC ? lepEff.getLeptonEfficiency( selLeptons[dilLep1].pt(), selLeptons[dilLep1].eta(), abs(selLeptons[dilLep1].pdgId()), "tightiso_tightid",patUtils::CutVersion::Moriond17Cut ).first : 1.0; //ISO w.r.t ID
              weight *= isMC ? lepEff.getLeptonEfficiency( selLeptons[dilLep2].pt(), selLeptons[dilLep2].eta(), abs(selLeptons[dilLep2].pdgId()), "tightiso_tightid",patUtils::CutVersion::Moriond17Cut ).first : 1.0;
            }
              weight *= isMC ? lepEff.getLeptonEfficiency( selLeptons[dilLep1].pt(), selLeptons[dilLep1].eta(), abs(selLeptons[dilLep1].pdgId()),  abs(selLeptons[dilLep1].pdgId()) ==11 ? "tight"    : "tight"   ,patUtils::CutVersion::Moriond17Cut ).first : 1.0; //ID
              weight *= isMC ? lepEff.getLeptonEfficiency( selLeptons[dilLep2].pt(), selLeptons[dilLep2].eta(), abs(selLeptons[dilLep2].pdgId()),  abs(selLeptons[dilLep2].pdgId()) ==11 ? "tight"    : "tight"   ,patUtils::CutVersion::Moriond17Cut ).first : 1.0; //ID

         }

        }

        std::vector<TString> tags(1,"all");
        for(size_t ich=0; ich<chTags.size(); ich++){
          tags.push_back( chTags[ich] );
        }

        if(!isDileptonCandidate) continue;
        mon.fillHisto("zllmass","controlPlots",zll.mass(),weight);

  	bool passZmass = (fabs(zll.mass()-91.2)<15);
        bool passZpt   = (zll.pt()>20);
        bool passMass = passZmass;
        bool passBJetVetoMain = (nbtags ==0);
        bool passLepVetoMain = true;

        int higgsCandL1=-1, higgsCandL2=-1;
        LorentzVector higgsCand;
        int HiggsShortId=-1, higgsCandId;
        std::vector<TString> chTagsMain=chTags;
        //std::vector<TString> chTagsMain=tags;

        int NCleanedJetMain = 0;
        bool passDPhiCut    = 0;
        bool passHiggsLoose = 0;
        bool passHiggsMain  = 0;
        LorentzVector higgsCand_SVFit;
        LorentzVector higgsCandH;
        LorentzVector higgsCandH_SVFit;



        //LEPTON FAKE RATE ANALYSIS Z+1jets  (no systematics taken into account here)
        if(ivar==0 && passZmass && (int)selLeptons.size()==3){  //Request exactly one Z + 1 additional lepton
          bool IdentifiedThirdLepton=false;
          double tmass=-999;
          for(int i=0   ;i<(int)selLeptons.size() && !IdentifiedThirdLepton;i++){
            if((i==dilLep1) || (i==dilLep2)) continue;
            if(deltaR(selLeptons[i],  selLeptons[dilLep1])<0.1 || deltaR(selLeptons[i],  selLeptons[dilLep2])<0.1)continue;
            if(abs(selLeptons[i].pdgId())==11||abs(selLeptons[i].pdgId())==13||abs(selLeptons[i].pdgId())==15){
              tmass = TMath::Sqrt(2*selLeptons[i].pt()*met.pt()*(1-TMath::Cos(deltaPhi(met.phi(), selLeptons[i].phi()))));
            }
            if(abs(selLeptons[i].pdgId())==11 || abs(selLeptons[i].pdgId())==13 || abs(selLeptons[i].pdgId())==15){
              int closestJetIndexL1=-1; double pTL1=-1; double etaL1=-1;
              double dRminL1 = closestJet(selLeptons[i].p4(), selJets, closestJetIndexL1);
              if(closestJetIndexL1>=0 && dRminL1<0.5){pTL1=selJets[closestJetIndexL1].pt(); etaL1=abs(selJets[closestJetIndexL1].eta());}
              else{pTL1=selLeptons[i].pt(); etaL1=abs(selLeptons[i].eta());}


              TString PartName = "FR_"+chTags.at(1)+"_";
              if     (abs(selLeptons[i].pdgId())==11)PartName += "El";
              else if(abs(selLeptons[i].pdgId())==13)PartName += "Mu";
              else if(abs(selLeptons[i].pdgId())==15)PartName += "Ta";
              else PartName+= abs(selLeptons[i].pdgId());


              std::vector<TString> TagsFR;

              if(abs(selLeptons[i].pdgId())==11 || abs(selLeptons[i].pdgId())==13){
                bool passId = false;
                if(abs(selLeptons[i].pdgId())==11) passId = patUtils::passId(selLeptons[i].el, vtx[0], patUtils::llvvElecId::Loose, patUtils::CutVersion::CutSet::ICHEP16Cut);
                if(abs(selLeptons[i].pdgId())==13) passId = patUtils::passId(selLeptons[i].mu, vtx[0], patUtils::llvvMuonId::Loose, patUtils::CutVersion::CutSet::ICHEP16Cut);
                float relIso = patUtils::relIso(selLeptons[i], rho);

                if(true                 )TagsFR.push_back(PartName);
                if(passId && relIso<=0.1)TagsFR.push_back(PartName+("_Id_Iso01"));
                if(passId && relIso<=0.2)TagsFR.push_back(PartName+("_Id_Iso02"));
                if(passId && relIso<=0.3)TagsFR.push_back(PartName+("_Id_Iso03"));

                if(passId && relIso<=0.3)IdentifiedThirdLepton=true;
              }else{
                bool IdL = selLeptons[i].tau.tauID("byLooseCombinedIsolationDeltaBetaCorr3Hits");
                bool IdM = selLeptons[i].tau.tauID("byMediumCombinedIsolationDeltaBetaCorr3Hits");

                if(true                 )TagsFR.push_back(PartName);
                if(IdL                  )TagsFR.push_back(PartName+("_Id_IsoLo"));
                if(IdM                  )TagsFR.push_back(PartName+("_Id_IsoMe"));
              }

              if(tmass<30){
                int NTags = TagsFR.size();
                for(unsigned int iTags=0;iTags<NTags;iTags++){
                  TagsFR.push_back(TagsFR[iTags] + TString("_TMCut"));
                }
              }

              auto TagsFRJet = TagsFR;
              auto TagsFRLep = TagsFR;

              for(unsigned int iTags=0;iTags<TagsFR.size();iTags++){
                TagsFRJet.push_back(TagsFR[iTags] + (etaL1<1.4                   ?TString("_B"):TString("_E")));
                TagsFRLep.push_back(TagsFR[iTags] + (abs(selLeptons[i].eta())<1.4?TString("_B"):TString("_E")));
              }

              mon.fillHisto("wrtJetPt", TagsFRJet, pTL1              , weight);
              mon.fillHisto("wrtLepPt", TagsFRLep, selLeptons[i].pt(), weight);
            }
          }//close loop on leptons

        }//close FR study Zmass


        //SIGNAL ANALYSIS Z+2Leptons  (no systematics taken into account here)
        if(passZmass && passZpt && (int)selLeptons.size()>=4){  //Request at least 4 leptons
          //printf("%30s %2i --> ", "BEFORE", -1); for(int l=0   ;l<(int)selLeptons.size();l++){ printf("%i ", selLeptons[l].pdgId());}printf("\n");

          //Get the Higgs candidate
          higgsCandL1=-1;
          higgsCandL2=-1;
          higgsCand = LorentzVector(0.,0.,0.,0.);
          HiggsShortId=-1;
          higgsCandId=0;

          for(int l=0   ;l<(int)selLeptons.size();l++){
            if(l==dilLep1 || l==dilLep2)continue;
            if(higgsCandL1<0){higgsCandL1=l;continue;}
            if(higgsCandL2<0){higgsCandL2=l;break;}//ordered in pT, so all done
          }

          string ChannelName = "none";   string signName = "";
          if(higgsCandL1>=0 && higgsCandL2>=0){
            higgsCandId=selLeptons[higgsCandL1].pdgId()*selLeptons[higgsCandL2].pdgId();
            higgsCand = LorentzVector(selLeptons[higgsCandL1].p4()+selLeptons[higgsCandL2].p4());
            if(higgsCandId<0){signName="_OS";}else{signName="_SS";}
            if(higgsCandId<0){HiggsShortId = 0;}else{HiggsShortId =12;}
            if(abs(selLeptons[dilLep1].pdgId())==11){HiggsShortId += 0;}else{HiggsShortId += 6;}
            switch(abs(higgsCandId)){
              case 11*11:  ChannelName  = "elel";  HiggsShortId+= 0; break;
              case 13*13:  ChannelName  = "mumu";  HiggsShortId+= 1; break;
              case 11*13:  ChannelName  = "elmu";  HiggsShortId+= 2; break;
              case 11*15:  ChannelName  = "elha";  HiggsShortId+= 3; break;
              case 13*15:  ChannelName  = "muha";  HiggsShortId+= 4; break;
              case 15*15:  ChannelName  = "haha";  HiggsShortId+= 5; break;
              default:     ChannelName  = "none";  HiggsShortId =-1; break;
            }
          }
          chTagsMain.push_back(chTagsMain[chTagsMain.size()-1] + signName + ChannelName);
          //Get the Higgs candidate (end)

          //printf("%30s %2i --> %i %i %i %i\n", (chTagsMain[chTagsMain.size()-1]).Data(), HiggsShortId, selLeptons[dilLep1].pdgId(), selLeptons[dilLep2].pdgId(), selLeptons[higgsCandL1].pdgId(), selLeptons[higgsCandL2].pdgId());

          //reweight the event to account for lept eff.
          if(isMC && higgsCandL1>=0 && abs(selLeptons[higgsCandL1].pdgId())<15){

            int id( abs(selLeptons[higgsCandL1].pdgId()) );

            if(id==11)weight *= lepEff.getRecoEfficiency( selLeptons[higgsCandL1].el.superCluster()->eta(), id).first; //Reconstruction eff
            else if(id==13)weight *= lepEff.getTrackingEfficiency( selLeptons[higgsCandL1].eta(), id).first; //Tracking eff
            weight *= isMC ? lepEff.getLeptonEfficiency( selLeptons[higgsCandL1].pt(), selLeptons[higgsCandL1].eta(), id,  id ==11 ? "loose"    : "loose"   ,patUtils::CutVersion::Moriond17Cut).first : 1.0; //ID
			      if(id==13){ weight *= isMC ? lepEff.getLeptonEfficiency( selLeptons[higgsCandL1].pt(), selLeptons[higgsCandL1].eta(), id, "looseiso_looseid",patUtils::CutVersion::Moriond17Cut ).first : 1.0;} //ISO w.r.t ID
          }

          if(isMC && higgsCandL2>=0 && abs(selLeptons[higgsCandL2].pdgId())<15){

            int id( abs(selLeptons[higgsCandL2].pdgId()) );

            if(id==11)weight *= lepEff.getRecoEfficiency( selLeptons[higgsCandL2].el.superCluster()->eta(), id).first; //Reconstruction eff
            else if(id==13)weight *= lepEff.getTrackingEfficiency( selLeptons[higgsCandL2].eta(), id).first; //Tracking eff
            weight *= isMC ? lepEff.getLeptonEfficiency( selLeptons[higgsCandL2].pt(), selLeptons[higgsCandL2].eta(), id,  id ==11 ? "loose"    : "loose"   ,patUtils::CutVersion::Moriond17Cut ).first : 1.0; //ID
			      if(id==13){ weight *= isMC ? lepEff.getLeptonEfficiency( selLeptons[higgsCandL2].pt(), selLeptons[higgsCandL2].eta(), id, "looseiso_looseid",patUtils::CutVersion::Moriond17Cut ).first : 1.0;} //ISO w.r.t ID
          }


          //check how many additional light jets are present
          NCleanedJetMain = 0;
          for(int j1=0;j1<(int)selJets.size();j1++){
            if(dilLep1    !=-1 && deltaR(selJets[j1]   , selLeptons[dilLep1 ])<0.4) continue;
            if(dilLep2    !=-1 && deltaR(selJets[j1]   , selLeptons[dilLep2 ])<0.4) continue;
            if(higgsCandL1         !=-1 && deltaR(selJets[j1]   , selLeptons[higgsCandL1      ])<0.4) continue;
            if(higgsCandL2         !=-1 && deltaR(selJets[j1]   , selLeptons[higgsCandL2      ])<0.4) continue;
            NCleanedJetMain++;
          }

          passDPhiCut    =  (fabs(deltaPhi(zll.phi(), met.phi()))>1.5);
          passHiggsLoose = passHiggsCuts(selLeptons, higgsCandL1, higgsCandL2, 0.5, 0.5, "decayModeFinding", 0., false, vtx);
          passHiggsMain  = passHiggsCuts(selLeptons, higgsCandL1, higgsCandL2, 0.3, 0.15, "byLooseCombinedIsolationDeltaBetaCorr3Hits", 20., true, vtx);

          //SVFIT MASS
          higgsCand_SVFit = higgsCand;

          //FIXME gives a lot of warning currently
       //  if(passZmass && passZpt && passDPhiCut && passHiggsLoose && passLepjetMain && passBJetVetoMain){
         // std::cout<<"START SVFIT\n";
       //   higgsCand_SVFit = getSVFit(met, selLeptons, higgsCandL1, higgsCandL2);  //compute svfit mass in a smart way
        //  std::cout<<"END SVFIT\n";
       //  }

          //build the higgs candH
          higgsCandH = zll + higgsCand;
          higgsCandH_SVFit = zll + higgsCand_SVFit;
        }



        bool passThirdLeptonVeto( selLeptons.size()==2 && extraLeptons.size()==0 );
        bool passBtags(nbtags==0);
        bool passMinDphijmet( njets==0 || mindphijmet>0.5);
        bool removeDump(false);

        //
        // NOW FOR THE CONTROL PLOTS
        //


        if(ivar==0){//fill plots only for nominal
          mon.fillHisto("eventflow"       , chTagsMain,                 0, weight);
          if(selLeptons.size()>=2){
            mon.fillHisto("nlep"           ,   chTags, selLeptons.size(), weight);
            mon.fillHisto("eventflow"     ,   chTagsMain,                 1, weight);
            mon.fillHisto("zllmass"          ,   chTagsMain, zll.mass(),    weight);
            if(passZmass){
              mon.fillHisto("eventflow"   ,   chTagsMain,                 2, weight);
              //pu control
              mon.fillHisto("nvtx"        ,   chTagsMain, vtx.size(),      weight);
              mon.fillHisto("nvtxraw"     ,   chTagsMain, vtx.size(),      weight/puWeight);
              mon.fillHisto("nvtxpuweight"     ,   chTagsMain, vtx.size(),      weightNoLepSF);
              mon.fillHisto("rho"         ,   chTagsMain, rho,       weight);

              //Z kinematics control
              mon.fillHisto("leadpt"      ,   chTagsMain, leadingLep.pt(), weight);
              mon.fillHisto("leadeta"     ,   chTagsMain, leadingLep.eta(), weight);
              mon.fillHisto("trailerpt"   ,   chTagsMain, trailerLep.pt(), weight);
              mon.fillHisto("trailereta"  ,   chTagsMain, trailerLep.eta(), weight);
              mon.fillHisto("leppt"       ,   chTagsMain, leadingLep.pt(), weight);
              mon.fillHisto("leppt"       ,   chTagsMain, trailerLep.pt(), weight);
              mon.fillHisto("lepeta"      ,   chTagsMain, leadingLep.eta(), weight);
              mon.fillHisto("lepeta"      ,   chTagsMain, trailerLep.eta(), weight);

              //analyze dilepton kinematics
              mon.fillHisto("zllpt"         ,   chTagsMain, zll.pt(),      weight);
              mon.fillHisto("zlleta"        ,   chTagsMain, zll.eta(),     weight);
              mon.fillHisto("zlly"          ,   chTagsMain, zll.Rapidity(),weight);

              if(passZpt){
                mon.fillHisto("eventflow",   chTagsMain,                 3, weight);

                mon.fillHisto("ntaus"           ,  chTags, selTaus.size(), weight);
                mon.fillHisto("tauleadpt"       ,  chTagsMain,   selTaus.size()>0?selTaus[0].pt():-1,  weight);
                mon.fillHisto("tauleadeta"      ,  chTagsMain,   selTaus.size()>0?selTaus[0].eta():-10, weight);
                mon.fillHisto("tautrailerpt"    ,  chTagsMain,   selTaus.size()>1?selTaus[1].pt():-1,  weight);
                mon.fillHisto("tautrailereta"   ,  chTagsMain,   selTaus.size()>1?selTaus[1].eta():-10, weight);
                mon.fillHisto("taupt"           ,  chTags, selTaus.size()>0?selTaus[0].pt():-1, weight);
                mon.fillHisto("taupt"           ,  chTags, selTaus.size()>0?selTaus[1].pt():-1, weight);
                mon.fillHisto("taueta"          ,  chTagsMain,   selTaus.size()>0?selTaus[0].eta():-10, weight);
                mon.fillHisto("taueta"          ,  chTagsMain,   selTaus.size()>0?selTaus[0].eta():-10, weight);

                if(selLeptons.size()>=4){
                  mon.fillHisto("eventflow",   chTagsMain,                 4, weight);
                  if(passLepVetoMain){
                    mon.fillHisto("eventflow", chTagsMain,                 5, weight);
                    mon.fillHisto("nbtags"    , chTags, nbtags,  weight);
                    mon.fillHisto("njets"     , chTags, njets,   weight);

                    if(passBJetVetoMain){
                      mon.fillHisto("eventflow"	,   chTagsMain,                 6, weight);

                      mon.fillHisto("dPhi_AZ"    , chTagsMain, deltaPhi(higgsCand.phi(), zll.phi()),    weight);
                      mon.fillHisto("dPhi_AMet"  , chTagsMain, deltaPhi(higgsCand.phi(), met.phi()),    weight);
                      mon.fillHisto("dPhi_ZMet"  , chTagsMain, deltaPhi(zll.phi(), met.phi()),    weight);
                      mon.fillHisto("met"      	, chTagsMain, met.pt()         , weight);

                      if(passDPhiCut){
                        mon.fillHisto("eventflow",   chTagsMain,                 7, weight);
                        if(passHiggsLoose){
                          mon.fillHisto("sumpt",   chTagsMain, selLeptons[higgsCandL1].pt()+selLeptons[higgsCandL2].pt(), weight);
                          if(passHiggsMain){
                            mon.fillHisto("eventflow"   ,chTagsMain,                 8, weight);
                            mon.fillHisto("yields"          ,chTagsMain,                HiggsShortId, weight);
                            mon.fillHisto("yieldsOS"     ,chTagsMain,                HiggsShortId, weight);

                            mon.fillHisto("Apt"       	, chTagsMain, higgsCand.pt(),    weight);
                            mon.fillHisto("Amass"           , chTagsMain, higgsCand.mass(),  weight);
                            mon.fillHisto("Amasssvfit"      , chTagsMain, higgsCand_SVFit.mass(),  weight);
                            mon.fillHisto("Hmass"           , chTagsMain, higgsCandH.mass(),  weight);
                            mon.fillHisto("Hpt"             , chTagsMain, higgsCandH.pt(),  weight);
                            mon.fillHisto("Hmasssvfit"   , chTagsMain, higgsCandH_SVFit.mass(),  weight);

                            mon.fillHisto("Anjets"    	, chTagsMain, NCleanedJetMain      , weight);
                            mon.fillHisto("Amet"      	, chTagsMain, met.pt()         , weight);
                          } // HiggsCuts
                        }   // Loose HiggsCuts
                      }     // DPhi Cut
                    }
                  }
                }
              }
            }
          }
        } // end filling plots for nominal


        if( isMC && selLeptons.size()>=2 && passZmass && passZpt && selLeptons.size()>=4 && passLepVetoMain && passBJetVetoMain && passDPhiCut && passHiggsLoose){
          for(unsigned int index=0; index<optim_Cuts_sumPt.size();index++){
            bool passHiggs = passHiggsCuts(selLeptons, higgsCandL1, higgsCandL2, optim_Cuts_elIso[index], optim_Cuts_muIso[index], tauIDiso[optim_Cuts_taIso[index]], optim_Cuts_sumPt[index],true,vtx);
            if(passHiggs){
              mon.fillHisto(TString("Hsvfit_shapes")+varNames[ivar],chTagsMain,index,higgsCandH_SVFit.mass(),weight);
              mon.fillHisto(TString("Asvfit_shapes")+varNames[ivar],chTagsMain,index,higgsCand_SVFit.mass(),weight);
            } else {

	             // Control regions

	              CRTypes theCR =  checkBkgCR(selLeptons, higgsCandL1, higgsCandL2, optim_Cuts_elIso[index], optim_Cuts_muIso[index], tauIDiso[optim_Cuts_taIso[index]], optim_Cuts_sumPt[index],vtx);

	              float theFRWeight=1;

	              if(theCR==CRTypes::CR10){
                  // CR10
                  theFRWeight*=getTheFRWeight(selLeptons, selJets, higgsCandL1, higgsCandL2, theFRWeightTool, optim_Cuts_elIso[index], optim_Cuts_muIso[index], tauIDiso[optim_Cuts_taIso[index]], optim_Cuts_sumPt[index],"CR10");

                  mon.fillHisto(TString("Hsvfit_shapes_CR10")+varNames[ivar],chTagsMain,index,higgsCandH_SVFit.mass(),weight*theFRWeight);
                  mon.fillHisto(TString("Asvfit_shapes_CR10")+varNames[ivar],chTagsMain,index,higgsCand_SVFit.mass(),weight*theFRWeight);

                } else if (theCR==CRTypes::CR01) {
                  // CR01
                  theFRWeight*=getTheFRWeight(selLeptons, selJets, higgsCandL1, higgsCandL2, theFRWeightTool, optim_Cuts_elIso[index], optim_Cuts_muIso[index], tauIDiso[optim_Cuts_taIso[index]], optim_Cuts_sumPt[index],"CR01");

                  mon.fillHisto(TString("Hsvfit_shapes_CR01")+varNames[ivar],chTagsMain,index,higgsCandH_SVFit.mass(),weight*theFRWeight);
                  mon.fillHisto(TString("Asvfit_shapes_CR01")+varNames[ivar],chTagsMain,index,higgsCand_SVFit.mass(),weight*theFRWeight);
                } else {
                  // CR11
                  theFRWeight*=getTheFRWeight(selLeptons, selJets, higgsCandL1, higgsCandL2, theFRWeightTool, optim_Cuts_elIso[index], optim_Cuts_muIso[index], tauIDiso[optim_Cuts_taIso[index]], optim_Cuts_sumPt[index],"CR11");
                  mon.fillHisto(TString("Hsvfit_shapes_CR11")+varNames[ivar],chTagsMain,index,higgsCandH_SVFit.mass(),weight*theFRWeight);
                  mon.fillHisto(TString("Asvfit_shapes_CR11")+varNames[ivar],chTagsMain,index,higgsCand_SVFit.mass(),weight*theFRWeight);
                }
	             }

               if(index==0 && selLeptons.size()>=2 && passZmass && passZpt && selLeptons.size()>=4 && passLepVetoMain && passBJetVetoMain ){
                 mon.fillHisto(TString("metsys")+varNames[ivar], chTagsMain, imet.pt(), weight);
               }
             }//end of the loop on cutIndex
           }
         }//END SYSTEMATIC LOOP
       }
    printf("\n");
    delete file;
  }

  //##############################################
  //########     SAVING HISTO TO FILE     ########
  //##############################################
  TString terminationCmd = "";
  //save control plots to file
  printf("Results save in local directory and moved to %s\n", outUrl.Data());

  //save all to the file
  terminationCmd += TString("mv out.root ") + outUrl + ";";
  TFile *ofile=TFile::Open("out.root", "recreate");
  mon.Write();
  ofile->Close();

  if(!isMC && debugText!=""){
    TString outTxtUrl= outUrl + ".txt";
    terminationCmd += TString("mv out.txt ") + outTxtUrl + ";";
    FILE* outTxtFile = fopen("out.txt", "w");
    fprintf(outTxtFile, "%s", debugText.c_str());
    printf("TextFile URL = %s\n",outTxtUrl.Data());
    if(outTxtFile)fclose(outTxtFile);
  }

  //Now that everything is done, dump the list of lumiBlock that we processed in this job
  if(!isMC){
    terminationCmd += TString("mv out.json ") + ((outUrl.ReplaceAll(".root",""))+".json") + ";";
    goodLumiFilter.FindLumiInFiles(urls);
    goodLumiFilter.DumpToJson("out.json");
  }

  system(terminationCmd.Data());

}
