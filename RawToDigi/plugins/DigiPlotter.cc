// -*- C++ -*-
//
// Package:    HGCal/DigiPlotter
// Class:      DigiPlotter
//
/**\class DigiPlotter DigiPlotter.cc HGCal/DigiPlotter/plugins/DigiPlotter.cc

 Description: [one line class summary]

 Implementation:
     [Notes on implementation]
*/
//
// Original Author:  Rajdeep Mohan Chatterjee
//         Created:  Mon, 15 Feb 2016 09:47:43 GMT
//
//


// system include files
#include <memory>
#include <iostream>
#include "TH2Poly.h"
#include "TH1F.h"
#include "TProfile.h"
// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "HGCal/DataFormats/interface/HGCalTBRecHitCollections.h"
#include "HGCal/DataFormats/interface/HGCalTBDetId.h"
#include "HGCal/DataFormats/interface/HGCalTBRecHit.h"
#include "HGCal/Geometry/interface/HGCalTBCellVertices.h"
#include "HGCal/Geometry/interface/HGCalTBTopology.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "HGCal/CondObjects/interface/HGCalElectronicsMap.h"
#include "HGCal/CondObjects/interface/HGCalCondObjectTextIO.h"
#include "HGCal/DataFormats/interface/HGCalTBElectronicsId.h"
#include "HGCal/DataFormats/interface/HGCalTBDataFrameContainers.h"

//
// class declaration
//

// If the analyzer does not use TFileService, please remove
// the template argument to the base class so the class inherits
// from  edm::one::EDAnalyzer<> and also remove the line from
// constructor "usesResource("TFileService");"
// This will improve performance in multithreaded jobs.

class DigiPlotter : public edm::one::EDAnalyzer<edm::one::SharedResources>
{

public:
	explicit DigiPlotter(const edm::ParameterSet&);
	~DigiPlotter();
	static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
private:
	virtual void beginJob() override;
	void analyze(const edm::Event& , const edm::EventSetup&) override;
	virtual void endJob() override;
	// ----------member data ---------------------------
	bool DEBUG = 0;
	HGCalTBTopology IsCellValid;
	HGCalTBCellVertices TheCell;
	std::string mapfile_ = "HGCal/CondObjects/data/map_FNAL_2.txt";
	struct {
		HGCalElectronicsMap emap_;
	} essource_;
	int sensorsize = 128;// The geometry for a 256 cell sensor hasnt been implemted yet. Need a picture to do this.
	std::vector<std::pair<double, double>> CellXY;
	std::pair<double, double> CellCentreXY;
	std::vector<std::pair<double, double>>::const_iterator it;
	const static int NSAMPLES = 2;
	const static int NLAYERS  = 1;
	TH2Poly *h_digi_layer[NSAMPLES][NLAYERS];
	TH1F    *h_digi_layer_summed[NSAMPLES][NLAYERS];
	TProfile    *h_digi_layer_profile[NSAMPLES][NLAYERS];
	const static int cellx = 15;
	const static int celly = 15;
	int Sensor_Iu = 0;
	int Sensor_Iv = 0;
	TH1F  *h_digi_layer_cell[NSAMPLES][NLAYERS][cellx][celly];
	TH1F  *h_digi_layer_channel[2][64][2];
//        TH1F  *h_digi_layer_cell_event[NSAMPLES][NLAYERS][cellx][celly][512];
	char name[50], title[50];
};

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
DigiPlotter::DigiPlotter(const edm::ParameterSet& iConfig)
{
	//now do what ever initialization is needed
	usesResource("TFileService");
	edm::Service<TFileService> fs;
	consumesMany<SKIROC2DigiCollection>();
	const int HalfHexVertices = 4;
	double HalfHexX[HalfHexVertices] = {0.};
	double HalfHexY[HalfHexVertices] = {0.};
	const int FullHexVertices = 6;
	double FullHexX[FullHexVertices] = {0.};
	double FullHexY[FullHexVertices] = {0.};
	int iii = 0;
	for(int ISkiroc = 1; ISkiroc <= 2; ISkiroc++) {
		for(int Channel = 0; Channel < 64; Channel++) {
			for(int iii = 0; iii < 2; iii++) {
				sprintf(name, "Ski_%i_Channel_%i_ADC%i", ISkiroc, Channel, iii);
				sprintf(title, "Ski %i Channel %i ADC%i", ISkiroc, Channel, iii);
				h_digi_layer_channel[ISkiroc - 1][Channel][iii] = fs->make<TH1F>(name, title, 4096, 0., 4095.);
			}
		}
	}
	for(int nsample = 0; nsample < NSAMPLES; nsample++) {
		for(int nlayers = 0; nlayers < NLAYERS; nlayers++) {
//Booking a "hexagonal" histograms to display the sum of Digis for NSAMPLES, in 1 SKIROC in 1 layer. To include all layers soon. Also the 1D Digis per cell in a sensor is booked here for NSAMPLES.
			sprintf(name, "FullLayer_ADC%i_Layer%i", nsample, nlayers + 1);
			sprintf(title, "Sum of adc counts per cell for ADC%i Layer%i", nsample, nlayers + 1);
			h_digi_layer[nsample][nlayers] = fs->make<TH2Poly>();
			h_digi_layer[nsample][nlayers]->SetName(name);
			h_digi_layer[nsample][nlayers]->SetTitle(title);
			sprintf(name, "FullLayer_ADC%i_Layer%i_summed", nsample, nlayers + 1);
			sprintf(title, "Sum of adc counts for all cells in ADC%i Layer%i", nsample, nlayers + 1);
			h_digi_layer_summed[nsample][nlayers] = fs->make<TH1F>(name, title, 4096, 0., 4095.);
			h_digi_layer_summed[nsample][nlayers]->GetXaxis()->SetTitle("Digis[adc counts]");
			sprintf(name, "FullLayer_ADC%i_Layer%i_profile", nsample, nlayers + 1);
			sprintf(title, "profile of adc counts for all cells in ADC%i Layer%i", nsample, nlayers + 1);
			h_digi_layer_profile[nsample][nlayers] = fs->make<TProfile>(name, title, 128, 0, 127, 0., 4095.);
			h_digi_layer_profile[nsample][nlayers]->GetXaxis()->SetTitle("Channel #");
			h_digi_layer_profile[nsample][nlayers]->GetYaxis()->SetTitle("ADC counts");


			for(int iv = -7; iv < 8; iv++) {
				for(int iu = -7; iu < 8; iu++) {
					if(!IsCellValid.iu_iv_valid(nlayers, Sensor_Iu, Sensor_Iv, iu, iv, sensorsize)) continue;
//Some thought needs to be put in about the binning and limits of this 1D histogram, probably different for beam type Fermilab and cern.
					sprintf(name, "Cell_u_%i_v_%i_ADC%i_Layer%i", iu, iv, nsample, nlayers + 1);
					sprintf(title, "Digis for Cell_u_%i_v_%i ADC%i Layer%i", iu, iv, nsample, nlayers + 1);
					h_digi_layer_cell[nsample][nlayers][iu + 7][iv + 7] = fs->make<TH1F>(name, title, 4096, 0., 4095.);
					h_digi_layer_cell[nsample][nlayers][iu + 7][iv + 7]->GetXaxis()->SetTitle("Digis[adc counts]");
					/*
					                                        for(int eee = 0; eee< 512; eee++){
					                                            sprintf(name, "Cell_u_%i_v_%i_ADC%i_Layer%i_Event%i", iu, iv, nsample, nlayers + 1,eee);
					                                            sprintf(title, "Digis for Cell_u_%i_v_%i ADC%i Layer%i Event %i", iu, iv, nsample, nlayers + 1, eee);
					                                            h_digi_layer_cell_event[nsample][nlayers][iu + 7][iv + 7][eee] = fs->make<TH1F>(name, title, 4096, 0., 4095.);
					                                            h_digi_layer_cell_event[nsample][nlayers][iu + 7][iv + 7][eee]->GetXaxis()->SetTitle("Digis[adc counts]");
					                                           }
					*/
					CellXY = TheCell.GetCellCoordinatesForPlots(nlayers, Sensor_Iu, Sensor_Iv, iu, iv, sensorsize);
					int NumberOfCellVertices = CellXY.size();
					iii = 0;
					if(NumberOfCellVertices == 4) {
						for(it = CellXY.begin(); it != CellXY.end(); it++) {
							HalfHexX[iii] =  it->first;
							HalfHexY[iii++] =  it->second;
						}
//Somehow cloning of the TH2Poly was not working. Need to look at it. Currently physically booked another one.
						h_digi_layer[nsample][nlayers]->AddBin(NumberOfCellVertices, HalfHexX, HalfHexY);
					} else if(NumberOfCellVertices == 6) {
						iii = 0;
						for(it = CellXY.begin(); it != CellXY.end(); it++) {
							FullHexX[iii] =  it->first;
							FullHexY[iii++] =  it->second;
						}
						h_digi_layer[nsample][nlayers]->AddBin(NumberOfCellVertices, FullHexX, FullHexY);
					}

				}//loop over iu
			}//loop over iv
		}//loop over nlayers
	}//loop over nsamples
}//contructor ends here


DigiPlotter::~DigiPlotter()
{

	// do anything here that needs to be done at desctruction time
	// (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

// ------------ method called for each event  ------------
void
DigiPlotter::analyze(const edm::Event& event, const edm::EventSetup& setup)
{
	using namespace edm;
	using namespace std;
	std::vector<edm::Handle<SKIROC2DigiCollection> > ski;
	event.getManyByType(ski);
//        int Event = event.id().event();
	if(!ski.empty()) {

		std::vector<edm::Handle<SKIROC2DigiCollection> >::iterator i;
		int counter1 = 0, counter2 = 0;
		for(i = ski.begin(); i != ski.end(); i++) {

			const SKIROC2DigiCollection& Coll = *(*i);
//			cout << "SKIROC2 Digis: " << i->provenance()->branchName() << endl;
			for(SKIROC2DigiCollection::const_iterator j = Coll.begin(); j != Coll.end(); j++) {
				const SKIROC2DataFrame& SKI = *j ;
				int n_layer = (SKI.detid()).layer();
				int n_sensor_IU = (SKI.detid()).sensorIU();
				int n_sensor_IV = (SKI.detid()).sensorIV();
				int n_cell_iu = (SKI.detid()).iu();
				int n_cell_iv = (SKI.detid()).iv();
				uint32_t EID = essource_.emap_.detId2eid(SKI.detid());
				HGCalTBElectronicsId eid(EID);
				if(DEBUG) cout << endl << " Layer = " << n_layer << " Sensor IU = " << n_sensor_IU << " Sensor IV = " << n_sensor_IV << " Cell iu = " << n_cell_iu << " Cell iu = " << n_cell_iv << endl;
				if(!IsCellValid.iu_iv_valid(n_layer, n_sensor_IU, n_sensor_IV, n_cell_iu, n_cell_iv, sensorsize))  continue;
				CellCentreXY = TheCell.GetCellCentreCoordinatesForPlots(n_layer, n_sensor_IU, n_sensor_IV, n_cell_iu, n_cell_iv, sensorsize);
				double iux = (CellCentreXY.first < 0 ) ? (CellCentreXY.first + 0.0001) : (CellCentreXY.first - 0.0001) ;
				double iyy = (CellCentreXY.second < 0 ) ? (CellCentreXY.second + 0.0001) : (CellCentreXY.second - 0.0001);
				int nsample = 0;
				h_digi_layer[nsample][n_layer - 1]->Fill(iux , iyy, SKI[nsample].adcLow());
				h_digi_layer_profile[nsample][n_layer - 1]->Fill(counter1++, SKI[nsample].adcLow(), 1);
				h_digi_layer_summed[nsample][n_layer - 1]->Fill(SKI[nsample].adcLow());
				h_digi_layer_channel[eid.iskiroc() - 1][eid.ichan()][nsample]->Fill(SKI[nsample].adcLow());
				h_digi_layer_cell[nsample][n_layer - 1][7 + n_cell_iu][7 + n_cell_iv]->Fill(SKI[nsample].adcLow());
//                                        h_digi_layer_cell_event[nsample][n_layer - 1][7 + n_cell_iu][7 + n_cell_iv][event.id().event() - 1]->Fill(SKI[nsample].adcLow());
				nsample = 1;
				h_digi_layer[nsample][n_layer - 1]->Fill(iux , iyy, SKI[nsample - 1].adcHigh());
				h_digi_layer_profile[nsample][n_layer - 1]->Fill(counter2++, SKI[nsample - 1].adcHigh(), 1);
				h_digi_layer_summed[nsample][n_layer - 1]->Fill(SKI[nsample - 1].adcHigh());
				if((SKI.detid()).cellType() != 4) h_digi_layer_cell[nsample][n_layer - 1][7 + n_cell_iu][7 + n_cell_iv]->Fill(SKI[nsample - 1].adcHigh());
//                                        if(((SKI.detid()).cellType() != 4) && (eid.ichan() == 0) ) cout<<endl<<"SKIROC=  "<<eid.iskiroc()<<" Chan= "<<eid.ichan()<<" u= "<<n_cell_iu<<" v = "<<n_cell_iv<<endl;
				h_digi_layer_channel[eid.iskiroc() - 1][eid.ichan()][nsample]->Fill(SKI[nsample - 1].adcHigh());
//                                        h_digi_layer_cell_event[nsample][n_layer - 1][7 + n_cell_iu][7 + n_cell_iv][event.id().event() - 1]->Fill(SKI[nsample-1].adcHigh());
			}
		}
	} else {
		edm::LogWarning("DQM") << "No SKIROC2 Digis";
	}

}//analyze method ends here


// ------------ method called once each job just before starting event loop  ------------
void
DigiPlotter::beginJob()
{
	HGCalCondObjectTextIO io(0);
	edm::FileInPath fip(mapfile_);
	if (!io.load(fip.fullPath(), essource_.emap_)) {
		throw cms::Exception("Unable to load electronics map");
	}
}

// ------------ method called once each job just after ending the event loop  ------------
void
DigiPlotter::endJob()
{
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void
DigiPlotter::fillDescriptions(edm::ConfigurationDescriptions& descriptions)
{
	//The following says we do not know what parameters are allowed so do no validation
	// Please change this to state exactly what you do use, even if it is no parameters
	edm::ParameterSetDescription desc;
	desc.setUnknown();
	descriptions.addDefault(desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(DigiPlotter);
