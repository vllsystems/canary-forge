#include "app/main.h"
#include "io/otbm/iomap_otbm.h"
#include "game/zones.h"
#include "map/map.h"

bool IOMapOTBM::loadZones(Map &map, const FileName &dir) {
	std::string fn = (const char*)(dir.GetPath(wxPATH_GET_SEPARATOR | wxPATH_GET_VOLUME).mb_str(wxConvUTF8));
	fn += map.zonefile;
	FileName filename(wxstr(fn));
	if (!filename.FileExists()) {
		return false;
	}

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(fn.c_str());
	if (!result) {
		return false;
	}
	return loadZones(map, doc);
}

bool IOMapOTBM::loadZones(Map &map, pugi::xml_document &doc) {
	pugi::xml_node node = doc.child("zones");
	if (!node) {
		warnings.push_back("IOMapOTBM::loadZones: Invalid rootheader.");
		return false;
	}

	pugi::xml_attribute attribute;
	for (pugi::xml_node zoneNode = node.first_child(); zoneNode; zoneNode = zoneNode.next_sibling()) {
		if (as_lower_str(zoneNode.name()) != "zone") {
			continue;
		}

		std::string name = zoneNode.attribute("name").as_string();
		unsigned int id = zoneNode.attribute("zoneid").as_uint();
		map.zones.addZone(name, id);
	}
	return true;
}

bool IOMapOTBM::saveZones(Map &map, const FileName &dir) {
	wxString filepath = dir.GetPath(wxPATH_GET_SEPARATOR | wxPATH_GET_VOLUME);
	filepath += wxString(map.zonefile.c_str(), wxConvUTF8);

	// Create the XML file
	pugi::xml_document doc;
	if (saveZones(map, doc)) {
		return doc.save_file(filepath.wc_str(), "\t", pugi::format_default, pugi::encoding_utf8);
	}
	return false;
}

bool IOMapOTBM::saveZones(Map &map, pugi::xml_document &doc) {
	pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
	if (!decl) {
		return false;
	}

	decl.append_attribute("version") = "1.0";

	pugi::xml_node zoneNodes = doc.append_child("zones");
	for (const auto &[name, id] : map.zones) {
		if (id <= 0) {
			continue;
		}
		pugi::xml_node zoneNode = zoneNodes.append_child("zone");

		zoneNode.append_attribute("name") = name.c_str();
		zoneNode.append_attribute("zoneid") = id;
	}
	return true;
}
