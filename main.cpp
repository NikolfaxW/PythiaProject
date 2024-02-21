// main95.cc is a part of the PYTHIA event generator.
// Copyright (C) 2023 Torbjorn Sjostrand.
// PYTHIA is licenced under the GNU GPL v2 or later, see COPYING for details.
// Please respect the MCnet Guidelines, see GUIDELINES for details.

// Authors: Dag Gillberg <dag.gillberg@cern.ch>

// Keywords: root; jets; event display

// This is a program to use ROOT to visualize different jet algoritms.
// The produced figure is used in the article "50 years of Quantum
// Chromodynamics" in celebration of the 50th anniversary of QCD (EPJC).

#include <string>
#include <iostream>
#include "Pythia8/Pythia.h"
#include "TCanvas.h"
#include "TString.h"
#include "TH2D.h"
#include "TMath.h"
#include "TPave.h"
#include "TMarker.h"
#include "TLatex.h"
#include "TRandom3.h"
#include "TStyle.h"
#include "fastjet/PseudoJet.hh"
#include "fastjet/ClusterSequence.hh"
#include "drawF.h"



//==========================================================================

// Hard-coded settings

// Jet and hadron pT thresholds.
// Will only show particles with pT > pTmin and |y| < yMax.
double pTmin_jet = 25; //ok
double pTmin_hadron = 1;//ok
double yMax = 4;

// Amount of pileup. Average number of inelastic pp collisions per event
// (=bunch-crossing). Set to zero to turn off pileup.
double mu = 60;//ok

// Style format. Colours used by various drawn markers.
int colHS = kBlack, colPos = kRed, colNeg = kBlue; //ok
int colNeut = kGreen + 3, colPU = kGray + 1;
bool doAntik_t = true, dok_t = true, doCambridge_Aachen = false; //switch for algorithms
double R = 0.4; // algorithm jet finding radius


//==========================================================================




// Example main program to vizualize jet algorithms.

int main() {

    // Adjust ROOTs default style.
    gStyle->SetOptTitle(0);
    gStyle->SetOptStat(0);
    gStyle->SetPadTickX(1);
    // Tick marks on top and RHS.
    gStyle->SetPadTickY(1);
    gStyle->SetTickLength(0.02, "x");
    gStyle->SetTickLength(0.015, "y");
    // Good with SetMax higher. 57, 91 and 104 also OK.
    gStyle->SetPalette(55);

    // Define the canvas.
    auto can = new TCanvas();
    double x = 0.06, y = 0.96;
    // Left-right-bottom-top
    can->SetMargin(x, 0.02, 0.08, 0.06);

    // Define the energy-flow histogram.
    int NyBins = 400/2, NphiBins = 314/2;
    double yMax = 4, phiMax = TMath::Pi();
    auto pTflow = new TH2D("",
                           ";Rapidity #it{y};Azimuth #it{#phi};Jet #it{p}_{T} [GeV]",
                           NyBins, -yMax, yMax, NphiBins, -phiMax, phiMax);
    pTflow->GetYaxis()->SetTitleOffset(0.8);
    pTflow->GetZaxis()->SetTitleOffset(1.1);

    // Name of output pdf file + open canvas for printing pages to it.
    TString pdf = "result.pdf";
    can->Print(pdf + "[");

    // Generator. Process selection. LHC initialization.
    Pythia8::Pythia pythia;
    // Description of the process (using ROOT's TLatex notation).
    TString desc = "#it{pp} #rightarrow #it{WH} #rightarrow"
                   " #it{q#bar{q}b#bar{b}},  #sqrt{#it{s}} = 13.6 TeV";

    pythia.readFile("../config1.cmnd");
    pythia.init();
    int nEvent = pythia.mode("Main:numberOfEvents");

    // Pileup particles
    Pythia8::Pythia pythiaPU;
    pythiaPU.readFile("../config1.cmnd");

    if (mu > 0) pythiaPU.init();

    // Setup fasjet. Create map with (key, value) = (descriptive text, jetDef).
    std::map<TString, fastjet::JetDefinition> jetDefs;
    if(doAntik_t)jetDefs["Anti-#it{k_{t}} jets, #it{R} = " + std::to_string(R)] = fastjet::JetDefinition(
            fastjet::antikt_algorithm, R, fastjet::E_scheme, fastjet::Best);
    if(dok_t)jetDefs["#it{k_{t}} jets, #it{R} = " + std::to_string(R)] = fastjet::JetDefinition(
            fastjet::kt_algorithm, R, fastjet::E_scheme, fastjet::Best);
    if(doCambridge_Aachen)jetDefs["Cambridge-Aachen jets,  #it{R} = 0.4"] = fastjet::JetDefinition(
            fastjet::cambridge_algorithm, R, fastjet::E_scheme, fastjet::Best);


    auto &event = pythia.event;
    for (int iEvent = 0; iEvent < nEvent; ++iEvent) {
        if(iEvent%100==0) std::cout << "Working on event iEvent = " << iEvent << std::endl;
        if (!pythia.next()) continue;

        // Identify particles. Jets are built from all stable particles after
        // hadronization (particle-level jets).
        std::vector<Pythia8::Particle> VH, ptcls_hs, ptcls_pu;
        std::vector<fastjet::PseudoJet> stbl_ptcls;
        for (int i = 0; i < event.size(); ++i) {
            auto &p = event[i];
            if (p.isResonance() && p.status() == -62) VH.push_back(p);
            if (not p.isFinal()) continue;
            stbl_ptcls.push_back(fastjet::PseudoJet(p.px(), p.py(), p.pz(), p.e()));
            ptcls_hs.push_back(p);
        }

        // Should not happen!
        if (VH.size()!= 2) continue;
        /*
        // Want to show an example where one of the boson is boosted and hence
        // contained within a R=1.0 jet, and one is not.
        // The diboson system should also be fairly central.
        auto pVH = VH[0].p() + VH[1].p();
        double DR1 = event.RRapPhi(VH[0].daughter1(), VH[0].daughter2());//!TODO
        double DR2 = event.RRapPhi(VH[1].daughter1(), VH[1].daughter2());//!TODO
        // Central system.
        if ( std::abs(pVH.rap())>0.5 || std::abs(VH[0].phi())>2.5 ||
             std::abs(VH[1].phi())>2.5 ) continue;
        // One contained, one resolved.
        if ( (DR1<1.0 && DR2<1.0) || (DR1>1.0 && DR2>1.0) ) continue;*/

        // Add in ghost particles on the grid defined by the pTflow histogram.
        fastjet::PseudoJet ghost;
        double pTghost = 1e-100;
        for (int iy= 1;iy<= NyBins; ++iy) {
            for (int iphi= 1;iphi<= NphiBins; ++iphi) {
                double y   = pTflow->GetXaxis()->GetBinCenter(iy);
                double phi = pTflow->GetYaxis()->GetBinCenter(iphi);
                ghost.reset_momentum_PtYPhiM(pTghost, y, phi, 0);
                stbl_ptcls.push_back(ghost);
            }
        }

        // Add in pileup!
        int n_inel = gRandom->Poisson(mu);
        printf("Overlaying particles from %i pileup interactions!\n", n_inel);
        for (int i_pu= 0; i_pu<n_inel; ++i_pu) {
            if (!pythiaPU.next()) continue;
            for (int i = 0; i < pythiaPU.event.size(); ++i) {
                auto &p = pythiaPU.event[i];
                if (not p.isFinal()) continue;
                stbl_ptcls.push_back(
                        fastjet::PseudoJet(p.px(), p.py(), p.pz(), p.e()));
                ptcls_pu.push_back(p);
            }
        }

        can->SetLogz();
        can->SetRightMargin(0.13);
        bool first = true;
        for (auto jetDef:jetDefs) {
            fastjet::ClusterSequence clustSeq(stbl_ptcls, jetDef.second);
            auto jets = sorted_by_pt( clustSeq.inclusive_jets(pTmin_jet) );
            // Fill the pT flow.
            pTflow->Reset();
            // For each jet:
            for (auto jet:jets) {
                // For each particle:
                for (auto c:jet.constituents()) {
                    if (c.pt() > 1e-50) continue;
                    pTflow->Fill(c.rap(), c.phi_std(), jet.pt());
                }
            }
            pTflow->GetZaxis()->SetRangeUser(pTmin_jet/4,
                                             pTflow->GetBinContent(pTflow->GetMaximumBin())*4);
            // pTflow->GetZaxis()->SetRangeUser(8, 1100);
            // pTflow->GetZaxis()->SetMoreLogLabels();
            pTflow->Draw("colz");

            for (auto &p:ptcls_pu) {
                if ( std::abs(p.y()) < yMax && p.pT() > pTmin_hadron ) {
                    drawParticleMarker(p, p.charge()?24:25, colPU, 0.4);
                }
            }

            // Draw the stable particles.
            for (auto &p:ptcls_hs) {
                if ( std::abs(p.y()) < yMax && p.pT() > pTmin_hadron ) {
                    if (p.charge()>0) {
                        drawParticleMarker(p, 5, colPos, 0.8);
                    } else if (p.charge()<0) {
                        drawParticleMarker(p, 5, colNeg, 0.8);
                    } else {
                        drawParticleMarker(p, 21, colNeut, 0.4);
                        drawParticleMarker(p, 5, colNeut, 0.8);
                    }
                }
            }

            // Draw the W and H.
            for (auto p:VH) {
                auto d1 = pythia.event[p.daughter1()];
                auto d2 = pythia.event[p.daughter2()];
                drawParticleText(p, colHS);
                drawParticleText(d1, colHS);
                drawParticleText(d2, colHS);
            }

            drawText(x, y, desc);
            drawText(0.87, y, jetDef.first +
                              Form(", #it{p}_{T} > %.0f GeV", pTmin_jet), 31);

            // 'Hand-made' legend used to specific plot in the
            // '50 years of Quantum Chromodynamics', EPJC.
            if (first) {
                drawLegendBox(0.66, 0.67, 0.85, 0.925);
                drawText(0.715, 0.90, "Hard scatter", 12);
                drawMarker(0.68, 0.90, 20, colHS, 0.8);
                drawMarker(0.7, 0.90, 29, colHS, 1.2);

                drawText(0.675, 0.85, "Stable particles", 12);
                drawText(0.675, 0.824, "   +    #bf{#minus}    #scale[0.9]{neutral}",
                         12);
                drawMarker(0.683, 0.82, 5, colPos, 0.8);
                drawMarker(0.717, 0.82, 5, colNeg, 0.8);
                drawMarker(0.75, 0.82, 21, colNeut, 0.4);
                drawMarker(0.75, 0.82, 5, colNeut, 0.8);

                drawText(0.675, 0.775, Form("Pileup  #it{#mu} = %.0f", mu), 12);
                drawText(0.675, 0.745, "   #pm    #scale[0.9]{neutral}", 12);
                drawMarker(0.683, 0.74, 24, colPU, 0.4);
                drawMarker(0.717, 0.74, 25, colPU, 0.4);
                drawText(0.70, 0.70, Form("#scale[0.8]{#it{p}_{T}^{ptcl} > %.1f GeV}",
                                          pTmin_hadron), 12);
            }
            first = false;
            can->Print(pdf);
        }
        //break; //!TODO remove this to draw several events!
    }

    // Close the pdf
    can->Print(pdf + "]");
    printf("Produced %s\n\n", pdf.Data());

    // Done.
    return 0;
}
