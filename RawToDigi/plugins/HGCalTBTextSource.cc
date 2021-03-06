#include "HGCal/RawToDigi/plugins/HGCalTBTextSource.h"
//#define DEBUG
//#define DEBUGTIME

#ifdef DEBUG
#include <iostream>
#endif

#ifdef DEBUGTIME
#include <iostream>
#endif

bool HGCalTBTextSource::openFile(edm::FromFiles& files, std::ifstream& file)
{
#ifdef DEBUG
	std::cout << "[DEBUG] " << files.fileNames().size()
	          << "\t" << files.fileIndex()
	          << "\t" << files.fileNames()[0] << "\t" << files.fileNames()[files.fileIndex()] << std::endl;
#endif

	if(file.is_open() && !file.eof()) return true; // continue to use the same file if the end is not reached
	if(file.is_open()) { //the end of file is reached
		file.close();
		files.incrementFileIndex();
		if(files.fileIndex() >= files.fileNames().size()) return false;
#ifdef DEBUG
		std::cout << "[DEBUG] INCREMENT " << files.fileNames().size() << "\t" << files.fileNames()[0] << "\t" << files.fileNames()[files.fileIndex()] << std::endl;
#endif

	}

	std::string name = files.fileNames()[files.fileIndex()];
	if(name == "NOFILE" || name == "") {
		return false;
	}
	if (name.find("file:") == 0) name = name.substr(5);
#ifdef DEBUG
	std::cout << "[DEBUG] Opening file: " << name << std::endl;
#endif

	file.open(name);
	if (file.fail()) {
		throw cms::Exception("FileNotFound") << "Unable to open file " << name;
	}
	return true;
}


bool HGCalTBTextSource::readLines()
{
	m_skiwords.clear();
	char buffer[1024];
	unsigned int triggerID_tmp = 0;

	//unsigned int length, bcid;
#ifdef DEBUG
	std::cout << "[DEBUG] Readline" << std::endl;
#endif

	if(_hgcalFile.peek() != 'C') {
		//cms::LogError("InputSource") << "Input file format does not match: reading character #" << _hgcalFile.peek() << "#";
		throw cms::Exception("MismatchInputSource") << "#" << _hgcalFile.peek() << "#";
	}

	for(unsigned int iSkiroc = 0 ; iSkiroc < MAXSKIROCS; ++iSkiroc) {

		// read the first line
		buffer[0] = 0;
		_hgcalFile.getline(buffer, 1000);
		if( sscanf(buffer, "CHIP %u TRIG: %x TIME: %x RUN: %u EV: %u", &m_sourceId_tmp, &triggerID_tmp, &m_time_tmp, &m_run_tmp, &m_event_tmp) != 5) return false;
		if(iSkiroc == 0) {
			m_event = m_event_tmp;
			m_run = m_run_tmp;
			m_time = m_time_tmp;
			m_sourceId = m_sourceId_tmp;
			_triggerID = triggerID_tmp;

			++m_event;
			m_time = m_time >> 8;

		}

#ifdef DEBUGTIME
		std::cout << m_run << "\t" << m_event << "\t" << m_time << "\n";
#endif

		assert( (triggerID_tmp & 0xF0000000) == 0x80000000 ); // check if the skiroc is fine  ///\todo transform to exception

		while ( _hgcalFile.peek() != 'C' && _hgcalFile.good()) {
			buffer[0] = 0;
			_hgcalFile.getline(buffer, 1000);
#ifdef DEBUG
			std::cout << m_sourceId << "\t" << m_event << "\t" << m_time << "\t" << buffer << "\n"; // buffer has a \n
#endif
			parseAddSkiword(m_skiwords, buffer);
		}

	}
	return !m_skiwords.empty();
}

bool HGCalTBTextSource::readTelescopeLines()
{
	if(!_telescope_words.empty()) return true; // data have not been used, don't read more
	if(_telescopeFile.peek() && !_telescopeFile.good()) return false;

	char buffer[1024];
	buffer[0] = 0;
	_telescopeFile.getline(buffer, 1000);

	parseAddTelescopeWords(_telescope_words, buffer);
	//words should be multiple of 8!, adding an empty word
	_telescope_words.push_back(-999);
	return !_telescope_words.empty();
}


bool HGCalTBTextSource::setRunAndEventInfo(edm::EventID& id, edm::TimeValue_t& time, edm::EventAuxiliary::ExperimentType&)
{

	if(openFile(_hgcalFiles, _hgcalFile) == false) return false; // if all the files have been processed, then return false
	else {
		if(!readLines()) return false; // readlines is here because the run and event info are contained in the file
	}


	if(_telescopeFiles.fileNames().size() > 0 && openFile(_telescopeFiles, _telescopeFile)) { // even if all the telescope files have been processed, keep going untile the hgcal files have all completed
		if(!readTelescopeLines()) return false;
	}

	id = edm::EventID(m_run, 1, m_event);

	// time is a hack
	time = (edm::TimeValue_t) m_time;
#ifdef DEBUGTIME
	std::cout << m_time << "\t" << time << std::endl;
#endif
	return true;
}

void HGCalTBTextSource::produce(edm::Event & e)
{
	std::auto_ptr<FEDRawDataCollection> bare_product(new  FEDRawDataCollection());

	FEDRawData& fed = bare_product->FEDData(m_sourceId);
	size_t len = sizeof(uint16_t) * m_skiwords.size();
	fed.resize(len);
	memcpy(fed.data(), &(m_skiwords[0]), len);

	FEDRawData& fed2 = bare_product->FEDData(_TELESCOPE_FED_ID_);

//	if((unsigned int) m_run==t_triggerID){ // empty FED if no data are available for the triggerID
	if((unsigned int) m_event == t_triggerID) { // empty FED if no data are available for the triggerID
		len = sizeof(float) * _telescope_words.size();
		fed2.resize(len);
		memcpy(fed2.data(), &(_telescope_words[0]), len);
		_telescope_words.clear();
	} else {
		fed2.resize(0);
	}

	// words are reset, only if the vectors are empty new lines are going to be read
	m_skiwords.clear();

	e.put(bare_product);
}



void HGCalTBTextSource::parseAddSkiword(std::vector<uint16_t>& skiwords, std::string i)
{
	uint32_t a, b;
	sscanf(i.c_str(), "%x %x", &a, &b);
	if(a == 0) return;

// two words of 16bits for ADC high gain and low gain
	skiwords.push_back(uint16_t(b >> 16));
	skiwords.push_back(uint16_t(b));
}

void HGCalTBTextSource::parseAddTelescopeWords(std::vector<float>& telescope_words, std::string i)
{
	unsigned int time, nTracks;
	float chi2, x0, y0, m_x, m_y, m_x_err, m_y_err;
	sscanf(i.c_str(), "%u,%u,%u,%f,%f,%f,%f,%f,%f,%f", &time, &t_triggerID, &nTracks, &chi2, &x0, &y0, &m_x, &m_y, &m_x_err, &m_y_err);
	telescope_words.push_back(chi2);
	telescope_words.push_back(x0);
	telescope_words.push_back(y0);
	telescope_words.push_back(m_x);
	telescope_words.push_back(m_y);
	telescope_words.push_back(m_x_err);
	telescope_words.push_back(m_y_err);

}

void HGCalTBTextSource::fillDescriptions(edm::ConfigurationDescriptions& descriptions)
{
	edm::ParameterSetDescription desc;
	desc.setComment("TEST");
	desc.addUntracked<std::vector<std::string> >("fileNames");
	//descriptions.add("source", desc);
}


#include "FWCore/Framework/interface/InputSourceMacros.h"

DEFINE_FWK_INPUT_SOURCE(HGCalTBTextSource);
